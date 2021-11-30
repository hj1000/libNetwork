#include "stdafx.h"
#include "defExt.h"
#include "CircularBuffer.h"
#include "NodeBuffer.h"
#include "thread.h"
#include "Crypt.h"
#include "Network.h"
#include "Client.h"

extern OVERLAPPED			g_ovlRecv;
extern OVERLAPPED			g_ovlSend;
extern OVERLAPPED			g_ovlClose;
extern OVERLAPPED			g_ovlDB;

CClient::CClient( int iClientIndex, CNetwork* pNetwork )
{
	InitializeCriticalSection( &m_csSock );
	InitializeCriticalSection( &m_csState );

	srand( (unsigned)time(NULL) + iClientIndex * (unsigned)time(NULL) );

	m_pNetwork			= pNetwork;
	m_sClientIndex		= iClientIndex;
	m_bMatchVersion		= FALSE;
	m_pCBuffer			= new CCircularBuffer;		//Socket Recv에서 Data를 받아서 쌓아두는 버퍼...

	memset( m_bufRecv, NULL, RECV_BUF_LENGTH );

	UpdateState( STATE_DISCONNECT );
	SetPrivateKey( (LONGLONG)rand() );
}

CClient::~CClient()
{
	delete m_pCBuffer;

	DeleteCriticalSection( &m_csState );
	DeleteCriticalSection( &m_csSock );
}

//유저 데이터 초기화...
BOOL CClient::InitUser( BOOL bAll )
{
	return TRUE;
}

//비공개키를 생성한다...
void CClient::SetPrivateKey( LONGLONG key )
{
	m_keyPrivate	= key;
	m_keyPrivate	*= (LONGLONG)rand();
	m_keyPrivate	*= (LONGLONG)rand();
	m_keyPrivate	*= (LONGLONG)rand();
}

void CClient::SetSocket( SOCKET socket )
{
	EnterCriticalSection( &m_csSock );

	m_socket = socket;

	LeaveCriticalSection( &m_csSock );
}

SOCKET	CClient::GetSocket()		{	return m_socket;			}
short	CClient::GetClientIndex()	{	return m_sClientIndex;		}

BOOL CClient::UpdateState( BYTE bState )
{
	BOOL bResult = FALSE;

	EnterCriticalSection( &m_csState );

	if( m_bState != bState )
	{
		m_bState	= bState;
		bResult		= TRUE;
	}

	LeaveCriticalSection( &m_csState );

	return bResult;
}

BOOL CClient::CheckState( BYTE bState )
{
	BOOL bResult = FALSE;

	EnterCriticalSection( &m_csState );

	bResult = ( m_bState == bState );
	
	LeaveCriticalSection( &m_csState );

	return bResult;
}

BOOL CClient::Associate()
{
	int iModSid = (int)GET_PORT( m_sClientIndex );
	if( !m_pNetwork->m_hCompPort[iModSid] )
	{
		ErrorLog( "Err %s: There is no Completion Port", __FUNCTION__ );
		return FALSE;
	}

	HANDLE hTemp;

	EnterCriticalSection( &m_csSock );

	hTemp = CreateIoCompletionPort( (HANDLE)m_socket, m_pNetwork->m_hCompPort[iModSid], (DWORD)m_sClientIndex, m_pNetwork->GetConcurrency() );

	LeaveCriticalSection( &m_csSock );
	
	return( hTemp == m_pNetwork->m_hCompPort[iModSid] );
}

void CClient::Close()
{
	OVERLAPPED* pOvl = &g_ovlClose;
	PostQueuedCompletionStatus( m_pNetwork->m_hCompPort[GET_PORT(m_sClientIndex)], (DWORD)0, (DWORD)m_sClientIndex, pOvl );

	TRACE( "%s: CUser uid[%d]\n", __FUNCTION__, m_sClientIndex );
}

void CClient::ReceiveData( int length )
{
	if( length <= 0 )	return;

	m_pCBuffer->PutData( m_bufRecv, length );		// 받은 Data를 버퍼에 넣는다

	CNodeBuffer* pNode = m_pNetwork->GetNode( this );
	if( pNode )
	{
		while( PullOutCore(pNode) )
		{
			int len	= pNode->GetSize();

			ASSERT( len );
			if( len <= 0 )				continue;

			if( !m_bMatchVersion )		ParseFromSocket( pNode );
			else
			{
				char* pDecData = new char[len+1];		memset( pDecData, NULL, len + 1 );
				m_pNetwork->GetCrypt()->JvDecryption( m_keyPrivate, len, (unsigned char*)pNode->GetBuf(), (unsigned char*)pDecData );
				pDecData[len] = NULL;

				CNodeBuffer* pDecNode = m_pNetwork->GetNode( this );
				if( pDecNode )
				{
					pDecNode->SetString( pDecData, len );

					ParseFromSocket( pDecNode );

					m_pNetwork->ReleaseNode( pDecNode );
				}

				delete[] pDecData;
				pDecData = NULL;
			}
		}

		m_pNetwork->ReleaseNode( pNode );
	}
}

BOOL CClient::PullOutCore( CNodeBuffer* pNode )
{
	if( !pNode )		return FALSE;
	
	pNode->Init();

	int len = m_pCBuffer->GetValidCount();
	if( len <= 0 )		return FALSE;

	BYTE* pTmp = new BYTE[len];

	m_pCBuffer->GetData( (char*)pTmp, len );

	BOOL foundCore	= FALSE;
	int length		= 0;

	for( int i = 0; i < len && !foundCore; i++ )
	{
		if( i + 2 >= len )							break;
		if( pTmp[i] != PACKET_START1 )				continue;
		if( pTmp[i+1] != PACKET_START2 )			continue;

		int sPos = i + 2;

		MYSHORT slen;
		slen.b[0] = pTmp[sPos];
		slen.b[1] = pTmp[sPos+1];

		length = slen.i;
		if( length < 0 )							break;
		if( length > len )							break;

		int ePos = sPos + length + 2;

		if( (ePos + 2) > len )						break;

		if		( pTmp[ePos] != PACKET_END1 )		m_pCBuffer->HeadIncrease( 3 );
		else if	( pTmp[ePos+1] != PACKET_END2 )		m_pCBuffer->HeadIncrease( 3 );
		else
		{
			pNode->SetString( (char*)(pTmp + sPos + 2), length );
			foundCore = TRUE;
		}
		break;
	}
	if( foundCore )		m_pCBuffer->HeadIncrease( 6 + length );	//6: header 2+ end 2+ length 2

	delete[] pTmp;

	return foundCore;
}

void CClient::Receive()
{
	int				err;
	int				last_err;
	WSABUF			in;
	DWORD			insize;
	DWORD			dwFlag=0;
	OVERLAPPED*		pOvl;

	memset( m_bufRecv, NULL, RECV_BUF_LENGTH );

	in.len = RECV_BUF_LENGTH;
	in.buf = m_bufRecv;

	pOvl = &g_ovlRecv;
	
	EnterCriticalSection( &m_csSock );

	err = WSARecv( m_socket, &in, 1, &insize, &dwFlag, pOvl, NULL );	// Completion Port에 Associate된 Socket에 버퍼를 건다

	LeaveCriticalSection( &m_csSock );

 	if( err == SOCKET_ERROR )
	{
		last_err = WSAGetLastError();

		if( last_err != WSA_IO_PENDING )
		{
			TRACE( "%s: Error In Receiving[%d]\n", __FUNCTION__, last_err );

			pOvl = &g_ovlClose;
			PostQueuedCompletionStatus( m_pNetwork->m_hCompPort[GET_PORT(m_sClientIndex)], (DWORD)0, (DWORD)m_sClientIndex, pOvl );
		}
//		else	TRACE( "%s: ERROR_IO_PENDING\n", __FUNCTION__ );
	}
}

void CClient::OvlClose( int iType )
{
	if( !m_pNetwork )					return;
	if( !m_pNetwork->GetCallBacks() )	return;

	m_pNetwork->GetCallBacks()->NotifyClientLeave( this );

	EnterCriticalSection( &m_csSock );

	shutdown( m_socket, SD_BOTH );
	closesocket( m_socket );

	LeaveCriticalSection( &m_csSock );

	if( !UpdateState(STATE_DISCONNECT) )	return;

	m_pNetwork->PutOldClientIndex( m_sClientIndex );

	switch( iType )
	{
	case CLOSETYPE_RECEIVE:		TRACE( "%s: By receive[%d]\n", __FUNCTION__, m_sClientIndex );		break;
	case CLOSETYPE_TRANSERR:	TRACE( "%s: By transerr[%d]\n", __FUNCTION__, m_sClientIndex );		break;
	}
}

//패킷을 만든다...
void CClient::MakePacket( char* pTarBuf, int& iTarLen, char* pSrcBuf, int iSrcLen )
{
	iTarLen = 0;

	pTarBuf[iTarLen++] = (char)PACKET_START1;		// 패킷 시작을 알리는 2 Byte
	pTarBuf[iTarLen++] = (char)PACKET_START2;

	MYSHORT slen;
	slen.i = iSrcLen;

	pTarBuf[iTarLen++] = (char)(slen.b[0]);			// 패킷 길이를 Short(2Byte)로 실어 보낸다
	pTarBuf[iTarLen++] = (char)(slen.b[1]);			// 2Byte라고 하여 1Byte * 256 + 2Byte가 아님

	if( m_bMatchVersion )
		m_pNetwork->GetCrypt()->JvEncryption( m_keyPrivate, iSrcLen, (unsigned char*)pSrcBuf, (unsigned char*)pTarBuf + iTarLen );
	else
		memcpy( pTarBuf + iTarLen, pSrcBuf, iSrcLen );

	iTarLen += iSrcLen;

	pTarBuf[iTarLen++] = (char)PACKET_END1;			// 패킷 끝을 알리는 2 Byte
	pTarBuf[iTarLen++] = (char)PACKET_END2;
}

void CClient::SendLast( char* pBuf, int iLength, int iTempCode )
{
	int					err;
	int					last_err;
	WSABUF				out;
	DWORD				sent;
	OVERLAPPED*			pOvl;

	out.buf				= pBuf;
	out.len				= iLength;

	pOvl				= &g_ovlSend;
	pOvl->OffsetHigh	= out.len;

	EnterCriticalSection( &m_csSock );

	err = WSASend( m_socket, &out, 1, &sent, 0, pOvl, NULL );

	LeaveCriticalSection( &m_csSock );
	
	if( err == SOCKET_ERROR )
	{
		last_err = WSAGetLastError();

		if( last_err != WSA_IO_PENDING )
		{
			TRACE( "%s: No ERROR_IO_PENDING[%d][%d]\n", __FUNCTION__, last_err, iTempCode );
		}
//		else	TRACE( "%s: ERROR_IO_PENDING\n", __FUNCTION__ );

		pOvl = &g_ovlClose;
		PostQueuedCompletionStatus( m_pNetwork->m_hCompPort[GET_PORT(m_sClientIndex)], (DWORD)0, (DWORD)m_sClientIndex, pOvl );
	}
}

void CClient::SendTo( CNodeBuffer* pPacket )
{
	if( pPacket )	SendTo( pPacket->GetSize(), (char*)pPacket->GetBuf() );
}

//패킷을 보낸다...
void CClient::SendTo( int iLength, char* pBuf )
{
	if( iLength >= BUFSIZE_8 )		return ErrorLog( "Err %s: Over Send Buffer[%d]\n", __FUNCTION__, iLength );

	MYSHORT sCommand;
	sCommand.b[0] = pBuf[0];
	sCommand.b[1] = pBuf[1];
//	TRACE( "%s: uid[%d] Command[%d]\n", __FUNCTION__, m_sUid, sCommand.i );

	int iSendIndex = 0;
	char bufSend[BUFSIZE_10];		memset( bufSend, NULL, BUFSIZE_10 );
	MakePacket( bufSend, iSendIndex, pBuf, iLength );

	SendLast( bufSend, iSendIndex, sCommand.i );
}

//소켓에서 받은 패킷을 처리한다...
void CClient::ParseFromSocket( CNodeBuffer* pNode )
{
	if( !pNode )						return;
	if( !m_pNetwork )					return;
	if( !m_pNetwork->GetCallBacks() )	return;

	m_pNetwork->m_iTotalPacket[GET_PORT(m_sClientIndex)] += 1;

	m_pNetwork->GetCallBacks()->NotifyParseFromSocket( this, pNode );
}

//DBThread에서 넘어온 결과를 처리한다...
void CClient::ParseFromDB( CNodeBuffer* pNode )
{
	if( !pNode )						return;
	if( !m_pNetwork )					return;
	if( !m_pNetwork->GetCallBacks() )	return;

	m_pNetwork->GetCallBacks()->NotifyParseFromDB( this, pNode );
}

//암호화 및 패킷 인덱스를 체크한다...
BOOL CClient::CheckMatchVersion( CNodeBuffer* pNode, int& iIndex )
{
	if( !m_bMatchVersion )		return TRUE;
	if( !pNode )				return FALSE;

	DWORD dClientIndex	= pNode->GetDWORD( iIndex );
	BYTE bCheckCode		= pNode->GetByte( iIndex );
	if( bCheckCode == 0XDF )	return TRUE;

	ErrorLog( "%s: ClientIndex[%d] Index[%d] CheckCode[%d]", __FUNCTION__, m_sClientIndex, dClientIndex, bCheckCode );

	return FALSE;
}

void CClient::AcceptComplete()
{
	if( !m_pNetwork )					return;
	if( !m_pNetwork->GetCallBacks() )	return;

	m_pNetwork->GetCallBacks()->NotifyClientJoin( this );
}

//클라이언트의 아이피를 구한다...
CString CClient::GetIp()
{
	CString strIp;

	struct sockaddr_in  addr;
	int					iLen = sizeof( addr );

	EnterCriticalSection( &m_csSock );

	getpeername( m_socket, (struct sockaddr*)&addr, &iLen );

	strIp = inet_ntoa( addr.sin_addr );

	LeaveCriticalSection( &m_csSock );

	return strIp;
}