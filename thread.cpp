#include "stdafx.h"
#include "NodeBuffer.h"
#include "Network.h"
#include "Client.h"
#include "thread.h"
#include "NpcManager.h"

unsigned __stdcall AcceptThread( void* lp )
{
	CNetwork* pNetwork = (CNetwork*)lp;

	int iBackLog = 200;		//Max Value : SOMAXCONN;
	listen( pNetwork->GetListenSocket(), iBackLog );

	HANDLE hListenSock = WSACreateEvent();

	WSAEventSelect( pNetwork->GetListenSocket(), hListenSock, FD_ACCEPT );

	WSANETWORKEVENTS	network_event;
	DWORD				wait_return;
	short				sClientIndex;
	CClient*			pClient = NULL;

	struct sockaddr_in  addr;
	int					len;
	SOCKET				socket;

	while( TRUE )
	{
		wait_return = WaitForSingleObject( hListenSock, INFINITE );

		if( wait_return == WAIT_FAILED )
		{
			TRACE( "Err %s: Wait failed[%d]", __FUNCTION__, GetLastError() );
			return TRUE;
		}

		WSAEnumNetworkEvents( pNetwork->GetListenSocket(), hListenSock, &network_event );

		if( pNetwork->IsShutDown() )							goto loop_pass_accept;

		//Network Check...
		if( !(network_event.lNetworkEvents & FD_ACCEPT) )		goto loop_pass_accept;
		if( network_event.iErrorCode[FD_ACCEPT_BIT] != 0 )		goto loop_pass_accept;

		sClientIndex = pNetwork->GetNewClientIndex();

		pClient = pNetwork->GetClient( sClientIndex );
		if( !pClient )
		{
			ErrorLog( "Fail %s: NewUid, ClientIndex[%d]", __FUNCTION__, sClientIndex );
			goto loop_pass_accept;
		}

		if( !pClient->CheckState(STATE_DISCONNECT) )
		{
			ErrorLog( "Fail %s: User Array has Broken, ClientIndex[%d]", __FUNCTION__, sClientIndex );
			goto loop_pass_accept;
		}

		pClient->InitUser();

		len		= sizeof( addr );
		socket	= accept( pNetwork->GetListenSocket(), (struct sockaddr*)&addr, &len );

		if( socket == INVALID_SOCKET )
		{
			ErrorLog( "Fail %s: Invalid Listen Socket, ClientIndex[%d]", __FUNCTION__, sClientIndex );

			pClient->UpdateState( STATE_DISCONNECT );
			pNetwork->PutOldClientIndex( sClientIndex );
			goto loop_pass_accept;
		}

		if( !pNetwork->CheckAllowIp(addr.sin_addr) )
		{
			ErrorLog( "Fail %s: CheckAllowIp, ClientIndex[%d]", __FUNCTION__, sClientIndex );

			pClient->UpdateState( STATE_DISCONNECT );
			pNetwork->PutOldClientIndex( sClientIndex );
			goto loop_pass_accept;
		}

		pClient->SetSocket( socket );
		pClient->UpdateState( STATE_ACCEPT );

		if( !pNetwork->Associate( pClient ) )
		{
			ErrorLog( "Fail %s: Associate Fail, ClientIndex[%d]", __FUNCTION__, sClientIndex );

			pClient->UpdateState( STATE_DISCONNECT );
			pNetwork->PutOldClientIndex( sClientIndex );
			goto loop_pass_accept;
		}

		struct linger lingerOpt;
		lingerOpt.l_onoff	= 1;
		lingerOpt.l_linger	= 0;

		if( setsockopt(pClient->GetSocket(), SOL_SOCKET, SO_LINGER, (char*)&lingerOpt, sizeof(struct linger)) < 0 )
		{
			ErrorLog( "Fail %s: Associate Fail, ClientIndex[%d]", __FUNCTION__, sClientIndex );

			pClient->UpdateState( STATE_DISCONNECT );
			pNetwork->PutOldClientIndex( sClientIndex );
			goto loop_pass_accept;
		}

		pClient->Receive();

		pClient->AcceptComplete();
		
		TRACE( "%s: Success Accepting, ClientIndex[%d]\n", __FUNCTION__, sClientIndex );

loop_pass_accept:
		continue;
	}
	
	return TRUE;
}

//#define _CHECK_WORKER

int m_iWorkerCount = 0;		//Worker Thread의 수
unsigned __stdcall WorkerThread( void* lp )
{
	CNetwork* pNetwork = (CNetwork*)lp;

	ULONG_PTR		dwIOKey;
	BOOL			b;
	LPOVERLAPPED	pOvl;
	DWORD			nbytes;
	CClient*		pClient = NULL;

	int iWorkerIndex = m_iWorkerCount++;	//Worker Thread의 고유 인덱스

	srand( (unsigned)time(NULL) + iWorkerIndex * (unsigned)time(NULL) );

	while( TRUE )
	{
		b = GetQueuedCompletionStatus( pNetwork->m_hCompPort[iWorkerIndex], &nbytes, &dwIOKey, &pOvl, INFINITE );

		if( pNetwork->IsShutDown() )					goto loop_pass;
		if( !b && !pOvl )								goto loop_pass;		//IOCP Check...
		
		//DBThread에서 넘어온 경우...
		if( pOvl->Offset == OVL_DB )
		{
			CNodeBuffer* pNode = (CNodeBuffer*)dwIOKey;
			if( !pNode )								goto loop_pass;

			pClient = pNetwork->GetClient( pNode->GetIndex() );
			if( !pClient )								goto loop_pass;

			pClient->ParseFromDB( pNode );
			pNetwork->ReleaseNode( pNode );

#ifdef _CHECK_WORKER
			TRACE( "OvlDB: ClientIndex[%d] workerid[%d]\n", pClient->GetUid(), iWorkerIndex );
#endif
			goto loop_pass;
		}

		pClient = pNetwork->GetClient( (int)dwIOKey );
		if( !pClient )									goto loop_pass;
		if( pClient->CheckState(STATE_DISCONNECT) )		goto loop_pass;

		switch( pOvl->Offset )
		{
		case OVL_RECEIVE:
			if( !nbytes )	// 소켓이 끊어질때 OVL_RECV로 OVERLAPPED의 OffSet값이 설정되면서 nbyte값이 0 이 된다
			{
				pClient->OvlClose( CLOSETYPE_RECEIVE );
				break;
			}

			pClient->ReceiveData( (int)nbytes );
			pClient->Receive();

#ifdef _CHECK_WORKER
			TRACE( "OvlRecv: ClientIndex[%d] workerid[%d]\n", pClient->GetUid(), iWorkerIndex );
#endif
			break;

		case OVL_SEND:
#ifdef _CHECK_WORKER
			TRACE( "OvlSend: ClientIndex[%d] workerid[%d]\n", pClient->GetUid(), iWorkerIndex );
#endif
			break;

		case OVL_CLOSE:
			pClient->OvlClose( CLOSETYPE_TRANSERR );

#ifdef _CHECK_WORKER
			TRACE( "OvlClose: ClientIndex[%d] workerid[%d]\n", pClient->GetUid(), iWorkerIndex );
#endif
			break;
		}

loop_pass:
		continue;
	}

	return TRUE;
}

//#define _CHECK_DB

int m_iDBCount = 0;		//DB Thread의 수...
unsigned __stdcall DBThread( void* lp )
{
	CNetwork* pNetwork = (CNetwork*)lp;

	ULONG_PTR		dwIOKey;
	BOOL			b;
	LPOVERLAPPED	pOvl;
	DWORD			nbytes;
	CNodeBuffer*	pNode = NULL;

	int iDBIndex = m_iDBCount++;	//DB Thread의 고유 인덱스...

	while( TRUE )
	{
		b = GetQueuedCompletionStatus( pNetwork->m_hCompPortDB[iDBIndex], &nbytes, &dwIOKey, &pOvl, INFINITE );

		if( !b && !pOvl )				goto loop_pass;		//IOCP Check...
		if( pOvl->Offset != OVL_DB )	goto loop_pass;

		pNode = (CNodeBuffer*)dwIOKey;
		if( pNode )		pNetwork->GetCallBacks()->NotifyDBWorkFunction( pNode );

#ifdef _CHECK_DB
		TRACE( "%s: ClientIndex[%d] DBid[%d]\n", __FUNCTION__, dwIOKey, iDBIndex );
#endif

loop_pass:
		continue;
	}

	return TRUE;
}

//#define _CHECK_NPC

int m_iNpcCount = 0;		//Npc Thread의 수...
unsigned __stdcall NpcThread( void* lp )
{
	CNetwork* pNetwork = (CNetwork*)lp;

	int iNpcIndex = m_iNpcCount++;	//Npc Thread의 고유 인덱스...

	while( TRUE )
	{
		if( !pNetwork )					break;
		if( pNetwork->IsShutDown() )	break;

#ifdef _CHECK_NPC
		TRACE( "%s: Npcid[%d]\n", __FUNCTION__, iNpcIndex );
#endif

		pNetwork->m_pNpcManager->ExecProc();

		Sleep( 1000 );

//loop_pass:
//		continue;
	}

	return TRUE;
}
