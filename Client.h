#pragma once

#include "Network.h"
#include "defExt.h"

class CCircularBuffer;
class LIBNETWORK_API CClient
{
public:
	CClient( int iClientIndex, CNetwork* pNetwork );
	virtual ~CClient();

	BOOL				InitUser( BOOL bAll = TRUE );
	void				SetPrivateKey( LONGLONG key );

	short				GetClientIndex();

	void				SetSocket( SOCKET socket );
	SOCKET				GetSocket();

	BOOL				UpdateState( BYTE bState );
	BOOL				CheckState( BYTE bState );

	BOOL				Associate();
	void				ReceiveData( int length );
	BOOL				PullOutCore( CNodeBuffer* pNode );
	void				Receive();

	BOOL				CheckMatchVersion( CNodeBuffer* pNode, int& iIndex );
	void				ParseFromSocket( CNodeBuffer* pNode );
	void				ParseFromDB( CNodeBuffer* pNode );

	void				MakePacket( char* pTarBuf, int& iTarLen, char* pSrcBuf, int iSrcLen );
	void				SendLast( char* pBuf, int iLength, int iTempCode = 0 );

	void				SendTo( int iLength, char* pBuf );
	void				SendTo( CNodeBuffer* pPacket );
	
	void				Close();
	void				OvlClose( int iType );

	void				AcceptComplete();

	CString				GetIp();


private:
	CNetwork*			m_pNetwork;
	short				m_sClientIndex;

	CRITICAL_SECTION	m_csSock;
	CRITICAL_SECTION	m_csState;
	SOCKET				m_socket;
	BYTE				m_bState;

	char				m_bufRecv[RECV_BUF_LENGTH];
	CCircularBuffer*	m_pCBuffer;

	BOOL				m_bMatchVersion;
	T_KEY				m_keyPrivate;
};
