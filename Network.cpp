#include "StdAfx.h"
#include "NodeBuffer.h"
#include "NodeManager.h"
#include "thread.h"
#include "Crypt.h"
#include "Network.h"
#include "Client.h"
#include "NpcManager.h"
#include "AllowIpList.h"

LIBNETWORK_API CNetwork*		g_pNetwork = NULL;

//Must Static...
OVERLAPPED						g_ovlRecv;
OVERLAPPED						g_ovlSend;
OVERLAPPED						g_ovlClose;
OVERLAPPED						g_ovlDB;

CNetwork::CNetwork( CNetworkCallbacks* pCallbacks )
{
	InitializeCriticalSection( &m_csClientIndex );

	g_ovlRecv.Offset	= OVL_RECEIVE;
	g_ovlSend.Offset	= OVL_SEND;
	g_ovlClose.Offset	= OVL_CLOSE;
	g_ovlDB.Offset		= OVL_DB;
	m_bShutDown			= FALSE;

	m_pCallbacks		= pCallbacks;
	m_pCrypt			= new CCrypt;
	m_pAllowIpList		= new CAllowIpList;

	for( int i = 0; i < MAX_PORT; ++i )			m_pNodeManager[i]	= new CNodeManager;
	for( int i = 0; i < MAX_PORT; ++i )			m_iTotalPacket[i]	= 0;
	for( int i = 0; i < MAX_PORT_DB; ++i )		m_iTotalDBIn[i]		= 0;
	for( int i = 0; i < MAX_PORT_DB; ++i )		m_iTotalDBOut[i]	= 0;

	m_pNpcManager = new CNpcManager;

	m_iListenPort = 0;

	if( !AfxSocketInit() )		AfxMessageBox( "AfxSocketInit Error" );
}

CNetwork::~CNetwork()
{
	DeleteAll();
	
	delete m_pCrypt;
	delete m_pAllowIpList;
	for( int i = 0; i < MAX_PORT; ++i )		delete m_pNodeManager[i];

	delete m_pNpcManager;

	m_iListenPort = 0;

	DeleteCriticalSection( &m_csClientIndex );

	g_pNetwork = NULL;
}

BOOL CNetwork::InitNetwork( int iMaxClient, int iListenPort )
{
	//Init Memory...
	if( !InitClientArray(iMaxClient) )			return FALSE;
	if( !InitClientIndexArray(iMaxClient) )		return FALSE;

	//Init Socket...
	if( !InitListenSocket(iListenPort) )		return FALSE;

	//Create Thread...
	CreateAcceptThread();
	CreateWorkerThread();
	CreateDBThread();
	CreateNpcThread();

	return TRUE;
}

CString CNetwork::GetLocalIP()
{
	char localHostName[256];
	::gethostname( localHostName, 256 );

	HOSTENT* pHost = ::gethostbyname( localHostName );
	if( !pHost )	return "";

	IN_ADDR in_addr;
	memcpy( &in_addr, pHost->h_addr, 4 );
	return _T( inet_ntoa( in_addr ) );
}

int CNetwork::GetListenPort()
{
	return m_iListenPort;
}

BOOL CNetwork::InitListenSocket( int iListenPort )
{
	int  opt;
	struct sockaddr_in  addr;
	
	//Open a TCP socket(an Internet stream socket)...
	if( (m_sockListen = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
	{
		ErrorLog( "Err 1: Can't Open Stream Socket" );
		return FALSE;
	}

	//Bind our local address so that the client can send to us...
	memset( (void*)&addr, 0, sizeof(addr) );

	addr.sin_family			= AF_INET;
	addr.sin_addr.s_addr	= htonl( INADDR_ANY );
	addr.sin_port			= htons( iListenPort );
	
	//added in an attempt to allow rebinding to the port...
	opt = 1;
	setsockopt( m_sockListen, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt) );

	opt = 1;
	setsockopt( m_sockListen, SOL_SOCKET, SO_KEEPALIVE, (char*)&opt, sizeof(opt) );

	//Linger off -> close socket immediately regardless of existance of data...
	struct linger lingerOpt;
	lingerOpt.l_onoff	= 1;
	lingerOpt.l_linger	= 0;

	setsockopt( m_sockListen, SOL_SOCKET, SO_LINGER, (char*)&lingerOpt, sizeof(lingerOpt) );
	
	if( bind(m_sockListen, (struct sockaddr*)&addr, sizeof(addr)) < 0 )
	{
		ErrorLog( "Err 2: Can't bind local address" );
		return FALSE;
	}

	int socklen;
	int len;
	int err;

	socklen = 8192 * 5;
	setsockopt( m_sockListen, SOL_SOCKET, SO_RCVBUF, (LPCSTR)&socklen, sizeof(socklen) );
	
	len = sizeof( socklen );
	err = getsockopt( m_sockListen, SOL_SOCKET, SO_RCVBUF, (char*)&socklen, &len );

	if( err != SOCKET_ERROR )
	{
		TRACE( "Set Socket RecvBuf of port[%d] as [%d]\n", iListenPort, socklen );
	}

	socklen = 8192 * 5;
	setsockopt( m_sockListen, SOL_SOCKET, SO_SNDBUF, (LPCSTR)&socklen, sizeof(socklen) );
	
	len = sizeof( socklen );
	err = getsockopt( m_sockListen, SOL_SOCKET, SO_SNDBUF, (char*)&socklen, &len );

	if( err != SOCKET_ERROR )
	{
		TRACE( "Set Socket SendBuf of port[%d] as [%d]\n", iListenPort, socklen );
	}

	m_iListenPort = iListenPort;

	return TRUE;
}

void CNetwork::CreateAcceptThread()
{
	unsigned int id;

	m_hAcceptThread = (HANDLE)_beginthreadex( NULL, 0, AcceptThread, (LPVOID)this, CREATE_SUSPENDED, &id );

	::SetThreadPriority( m_hAcceptThread, THREAD_PRIORITY_ABOVE_NORMAL );
}

void CNetwork::CreateWorkerThread()
{
	// try to get timing more accurate... Avoid context
	// switch that could occur when threads are released
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );

	// Figure out how many processors we have to size the minimum
	// number of worker threads and concurrency
	SYSTEM_INFO		SystemInfo;
	GetSystemInfo( &SystemInfo );
	
	m_dwNumberOfWorkers		= MAX_PORT;
	m_dwConcurrency			= SystemInfo.dwNumberOfProcessors;
	
	for( int j = 0; j < MAX_PORT; ++j )
	{
		m_hCompPort[j] = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
	}
	
	for( int i = 0; i < (int)m_dwNumberOfWorkers; ++i )
	{
		m_hWorkerThread[i] = (HANDLE)_beginthreadex( NULL, 0, WorkerThread, (LPVOID)this, 0, &m_WorkerId[i] );
	}
}

void CNetwork::CreateDBThread()
{
	// try to get timing more accurate... Avoid context
	// switch that could occur when threads are released
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );

	// Figure out how many processors we have to size the minimum
	// number of worker threads and concurrency
	SYSTEM_INFO		SystemInfo;
	GetSystemInfo( &SystemInfo );
	
	m_dwNumberOfDBs			= MAX_PORT_DB;
	m_dwConcurrency			= SystemInfo.dwNumberOfProcessors;
	
	for( int j = 0; j < MAX_PORT_DB; ++j )
	{
		m_hCompPortDB[j] = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
	}

	for( int i = 0; i < (int)m_dwNumberOfDBs; ++i )
	{
		m_hDBThread[i] = (HANDLE)_beginthreadex( NULL, 0, DBThread, (LPVOID)this, 0, &m_DBId[i] );
	}
}

void CNetwork::CreateNpcThread()
{
	// try to get timing more accurate... Avoid context
	// switch that could occur when threads are released
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );

	// Figure out how many processors we have to size the minimum
	// number of worker threads and concurrency
	SYSTEM_INFO		SystemInfo;
	GetSystemInfo( &SystemInfo );
	
	m_dwNumberOfNpcs		= MAX_PORT_NPC;
	m_dwConcurrency			= SystemInfo.dwNumberOfProcessors;
	
	for( int i = 0; i < (int)m_dwNumberOfNpcs; ++i )
	{
		m_hNpcThread[i] = (HANDLE)_beginthreadex( NULL, 0, NpcThread, (LPVOID)this, 0, &m_NpcId[i] );
	}
}

BOOL CNetwork::InitClientArray( int iMaxClient )
{
	m_iMaxClient = iMaxClient;

	for( int i = 0; i < iMaxClient; ++i )
	{
		CClient* pClient = new CClient( i, this );
		m_arClient.Add( pClient );
	}

	return TRUE;
}

BOOL CNetwork::InitClientIndexArray( int iMaxClient )
{
	EnterCriticalSection( &m_csClientIndex );

	m_arClientIndex.RemoveAll();

	for( int i = 0; i < iMaxClient; ++i )		m_arClientIndex.Add( (short)i );

	LeaveCriticalSection( &m_csClientIndex );

	return TRUE;
}

CClient* CNetwork::GetClient( short iClientIndex )
{
	if( !IS_RANGE(iClientIndex, 0, m_arClient.GetSize()) )	return NULL;

	return (CClient*)( m_arClient[iClientIndex] );
}

void CNetwork::DeleteAll()
{
	DeleteAllClientArray();
	DeleteAllClientIndexArray();
}

void CNetwork::DeleteAllClientArray()
{
	for( int i = 0; i < m_arClient.GetSize(); ++i )
	{
		CClient* pClient = (CClient*)m_arClient[i];
		if( pClient )	delete pClient;
	}
	m_arClient.RemoveAll();
}

void CNetwork::DeleteAllClientIndexArray()
{
	EnterCriticalSection( &m_csClientIndex );

	m_arClientIndex.RemoveAll();

	LeaveCriticalSection( &m_csClientIndex );
}

//Accept Thread를 시작해서 사용자의 접속을 받는다...
void CNetwork::StartAccepting()
{
	//m_bShutDown = FALSE;
	::ResumeThread( m_hAcceptThread );
}

void CNetwork::PauseAccepting()
{
	//m_bShutDown = TRUE;
	::SuspendThread( m_hAcceptThread );
}

BOOL CNetwork::Associate( CClient* pClient )
{
	if( !pClient )
	{
		ErrorLog( "Err %s: There is no Client Pointer", __FUNCTION__ );
		return FALSE;
	}

	return pClient->Associate();
}

short CNetwork::GetNewClientIndex()
{
	int iCount = GetClientIndexCount();
	if( iCount <= 0 )
	{
		ErrorLog( "%s: ClientIndex Array Is Empty", __FUNCTION__ );
		return -1;
	}

	short sClientIndex = -1;

	EnterCriticalSection( &m_csClientIndex );

	sClientIndex = m_arClientIndex[0];
	m_arClientIndex.RemoveAt( 0 );

	LeaveCriticalSection( &m_csClientIndex );

	return sClientIndex;
}

int CNetwork::GetClientIndexCount()
{
	int iCount = 0;

	EnterCriticalSection( &m_csClientIndex );

	iCount = (int)m_arClientIndex.GetSize();

	LeaveCriticalSection( &m_csClientIndex );

	return iCount;
}

void CNetwork::PutOldClientIndex( short sClientIndex )
{
	if( !IS_RANGE(sClientIndex, 0, m_arClient.GetSize()) )
	{
		return ErrorLog( "%s: Over range[%d]", __FUNCTION__, sClientIndex );
	}

	int iCount = GetClientIndexCount();
	if( iCount > m_arClient.GetSize() )		return ErrorLog( "%s: Array OverFlow", __FUNCTION__ );

	BOOL bFind = FALSE;

	EnterCriticalSection( &m_csClientIndex );

	for( int i = 0; i < m_arClientIndex.GetSize(); ++i )
	{
		short sTempIndex = m_arClientIndex[i];
		if( sTempIndex != sClientIndex )		continue;

		bFind = TRUE;
		break;
	}
	if( !bFind )	m_arClientIndex.Add( sClientIndex );		//중복이 없는 경우 추가...

	LeaveCriticalSection( &m_csClientIndex );

	if( bFind )		ErrorLog( "%s: Array Aleady Been[%d]", __FUNCTION__, sClientIndex );	//Duplicate...
}

//노드 매니저로부터 노드 버퍼를 가져온다...
CNodeBuffer* CNetwork::GetNode( CClient* pClient )
{
	if( !pClient )		return NULL;

	return GetNode( pClient->GetClientIndex() );
}

CNodeBuffer* CNetwork::GetNode()
{
	return GetNode( MyRand(0, MAX_PORT - 1) );
}

CNodeBuffer* CNetwork::GetNode( int iClientIndex )
{
	if( !IS_RANGE(iClientIndex, 0, m_arClient.GetSize()) )		return NULL;

	int iManagerIndex = iClientIndex % MAX_PORT;
	if( !IS_RANGE(iManagerIndex, 0, MAX_PORT) )					return NULL;
	if( !m_pNodeManager[iManagerIndex] )						return NULL;

	CNodeBuffer* pNode = m_pNodeManager[iManagerIndex]->GetNode();
	if( !pNode )												return NULL;

	pNode->SetIndex( iClientIndex );
	return pNode;
}

//노드 매니저에 사용한 노드 버퍼를 반환한다...
void CNetwork::ReleaseNode( CNodeBuffer* pNode )
{
	if( !pNode )									return;

	int iManagerIndex = pNode->GetIndex() % MAX_PORT;
	if( !IS_RANGE(iManagerIndex, 0, MAX_PORT) )		return;
	if( !m_pNodeManager[iManagerIndex] )			return;

	m_pNodeManager[iManagerIndex]->ReleaseNode( pNode );
}

//DBJob을 추가한다...
void CNetwork::PostDBJob( CNodeBuffer* pNode )
{
	if( !pNode )		return;

	m_iTotalDBIn[GET_DBPORT(pNode->GetIndex())]	+= 1;

	CNodeBuffer* pTemp = GetNode( pNode->GetIndex() );
	if( pTemp )
	{
		pTemp->CopyNodeData( pNode );

		OVERLAPPED* pOvl = &g_ovlDB;
		PostQueuedCompletionStatus( m_hCompPortDB[GET_DBPORT(pTemp->GetIndex())], (DWORD)0, (ULONG_PTR)pTemp, pOvl );
	}
}

//DBThread처리 내용을 WorkerThread로 넘긴다...
void CNetwork::EndDBWork( CNodeBuffer* pNode, BOOL bSendWorker )
{
	if( !pNode )		return;

	m_iTotalDBOut[GET_DBPORT(pNode->GetIndex())]	+= 1;

	//WorkerThread로 결과를 넘기지 않고 끝내는 경우...
	if( !bSendWorker )	return ReleaseNode( pNode );

	OVERLAPPED* pOvl = &g_ovlDB;
	PostQueuedCompletionStatus( m_hCompPort[GET_PORT(pNode->GetIndex())], (DWORD)0, (ULONG_PTR)pNode, pOvl );
}

void CNetwork::SendTo( short sClientIndex, CNodeBuffer* pPacket )
{
	if( pPacket )	SendTo( sClientIndex, pPacket->GetSize(), (char*)pPacket->GetBuf() );
}

void CNetwork::SendTo( short sClientIndex, int iLength, char* pBuf )
{
	if( iLength >= BUFSIZE_8 )		return ErrorLog( "Err %s: Over Send Buffer[%d]\n", __FUNCTION__, iLength );

	CClient* pClient = GetClient( sClientIndex );
	if( !pClient )					return;

	pClient->SendTo( iLength, pBuf );
}

void CNetwork::SendAll( CNodeBuffer* pPacket, BOOL bLoginUser )
{
	if( pPacket )	SendAll( pPacket->GetSize(), (char*)pPacket->GetBuf(), bLoginUser );
}

//로그인 되어 있는 모든 유저에게 패킷을 보낸다...
void CNetwork::SendAll( int iLength, char* pBuf, BOOL bLoginUser )
{
	if( iLength >= BUFSIZE_8 )		return ErrorLog( "Err %s: Over Send Buffer[%d]\n", __FUNCTION__, iLength );

	for( int i = 0; i < m_arClient.GetSize(); ++i )
	{
		CClient* pClient = GetClient( i );
		if( !pClient )											continue;
		if( bLoginUser && !pClient->CheckState(STATE_LOGIN) )	continue;

		pClient->SendTo( iLength, pBuf );
	}
}

void CNetwork::SendGroup( CNodeBuffer* pGroupNode, CNodeBuffer* pPacket )
{
	if( pPacket )	SendGroup( pGroupNode, pPacket->GetSize(), (char*)pPacket->GetBuf() );
}

//해당 그룹 안에 있는 유저에게 패킷을 보낸다...
void CNetwork::SendGroup( CNodeBuffer* pGroupNode, int iLength, char* pBuf )
{
	if( iLength >= BUFSIZE_8 )		return ErrorLog( "Err %s: Over Send Buffer[%d]\n", __FUNCTION__, iLength );

	int iIndex = 0;
	while( TRUE )
	{
		BOOL bContinue = pGroupNode->GetByte( iIndex );
		if( !bContinue )	break;

		short sClientIndex = pGroupNode->GetShort( iIndex );

		CClient* pClient = GetClient( sClientIndex );
		if( !pClient )								continue;
		if( !pClient->CheckState(STATE_LOGIN) )		continue;

		pClient->SendTo( iLength, pBuf );
	}
}

void CNetwork::ShutDown()
{
	SetShutDown( TRUE );

	for( int i = 0; i < m_arClient.GetSize(); ++i )
	{
		CClient* pClient = GetClient( i );
		if( !pClient )								continue;

		pClient->Close();
	}
}

CString CNetwork::GetTextTotalPacket()
{
	CString strText;

	for( int i = 0; i < MAX_PORT; ++i )
	{
		CString strTemp;
		strTemp.Format( " %d.", m_iTotalPacket[i] );
		m_iTotalPacket[i] = 0;

		strText += strTemp;
	}

	return strText;
}

CString CNetwork::GetTextTotalDBIn()
{
	CString strText;

	for( int i = 0; i < MAX_PORT_DB; ++i )
	{
		CString strTemp;
		strTemp.Format( " %d.", m_iTotalDBIn[i] );
		m_iTotalDBIn[i] = 0;

		strText += strTemp;
	}

	return strText;
}

CString CNetwork::GetTextTotalDBOut()
{
	CString strText;

	for( int i = 0; i < MAX_PORT_DB; ++i )
	{
		CString strTemp;
		strTemp.Format( " %d.", m_iTotalDBOut[i] );
		m_iTotalDBOut[i] = 0;

		strText += strTemp;
	}

	return strText;
}

CString CNetwork::GetTextTotalNode()
{
	CString strText;

	for( int i = 0; i < MAX_PORT; ++i )	
	{
		CString strTemp;
		strTemp.Format( " %d(%d).", m_pNodeManager[i]->GetNodeCount(), m_pNodeManager[i]->GetUseNodeCount() );

		strText += strTemp;
	}

	return strText;
}

BOOL CNetwork::SetAllowIpList( CString strFileName )
{
	if( !m_pAllowIpList )		return FALSE;

	return m_pAllowIpList->LoadIpList( strFileName );
}

BOOL CNetwork::CheckAllowIp( in_addr sin_addr )
{
	if( !m_pAllowIpList )		return FALSE;

	return m_pAllowIpList->CheckAllowIp( sin_addr );
}