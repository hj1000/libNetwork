#pragma once

#include "defNetwork.h"

// Socket
#include <afxsock.h>		// MFC 소켓 확장입니다.
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

class CNodeBuffer;
class CNodeManager;
class CClient;
class CCrypt;
class CNpcManager;
class CAllowIpList;

class CNetworkCallbacks
{
public:
	CNetworkCallbacks()			{}
	~CNetworkCallbacks()		{}

	virtual void NotifyClientJoin( CClient* pClient )									{}
	virtual void NotifyClientLeave( CClient* pClient )									{}
	virtual void NotifyParseFromSocket( CClient* pClient, CNodeBuffer* pNode )			{}
	virtual void NotifyParseFromDB( CClient* pClient, CNodeBuffer* pNode )				{}
	virtual void NotifyDBWorkFunction( CNodeBuffer* pNode )								{}
};

class LIBNETWORK_API CNetwork
{
public:
	CNetwork( CNetworkCallbacks* pCallbacks );
	virtual ~CNetwork();

	BOOL				InitNetwork( int iMaxClient = DEFAULT_MAX_CLIENT, int iListenPort = DEFAULT_PORT_LISTEN );
	CString				GetLocalIP();
	int					GetListenPort();
	void				StartAccepting();
	void				PauseAccepting();

	BOOL				Associate( CClient* pClient );
	CClient*			GetClient( short sClientIndex );
	DWORD				GetConcurrency()			{	return m_dwConcurrency;		}
	int					GetMaxClient()				{	return m_iMaxClient;		}
	CNetworkCallbacks*	GetCallBacks()				{	return m_pCallbacks;		}
	SOCKET				GetListenSocket()			{	return m_sockListen;		}
	CCrypt*				GetCrypt()					{	return m_pCrypt;			}
	BOOL				IsShutDown()				{	return m_bShutDown;			}
	void				SetShutDown( BOOL bData )	{	m_bShutDown = bData;		}

	void				ShutDown();

	short				GetNewClientIndex();
	void				PutOldClientIndex( short sClientIndex );
	int					GetClientIndexCount();

	CNodeBuffer*		GetNode( int iClientIndex );
	CNodeBuffer*		GetNode( CClient* pClient );
	CNodeBuffer*		GetNode();
	void				ReleaseNode( CNodeBuffer* pNode );

	void				PostDBJob( CNodeBuffer* pNode );
	void				EndDBWork( CNodeBuffer* pNode, BOOL bReturnWorker = TRUE );

	void				SendTo( short sClientIndex, CNodeBuffer* pPacket );
	void				SendTo( short sClientIndex, int iLength, char* pBuf );
	void				SendAll( CNodeBuffer* pPacket, BOOL bLoginUser = TRUE );
	void				SendAll( int iLength, char* pBuf, BOOL bLoginUser = TRUE );
	void				SendGroup( CNodeBuffer* pGroupNode, CNodeBuffer* pPacket );
	void				SendGroup( CNodeBuffer* pGroupNode, int iLength, char* pBuf );

	CString				GetTextTotalPacket();
	CString				GetTextTotalDBIn();
	CString				GetTextTotalDBOut();
	CString				GetTextTotalNode();

	BOOL				SetAllowIpList( CString strFileName );
	BOOL				CheckAllowIp( in_addr sin_addr );

	HANDLE				m_hCompPort[MAX_PORT];
	HANDLE				m_hCompPortDB[MAX_PORT_DB];
	CNodeManager*		m_pNodeManager[MAX_PORT];
	CNpcManager*		m_pNpcManager;

	int					m_iTotalPacket[MAX_PORT];
	int					m_iTotalDBIn[MAX_PORT_DB];
	int					m_iTotalDBOut[MAX_PORT_DB];

private:
	BOOL				InitListenSocket( int iListenPort );
	void				CreateAcceptThread();
	void				CreateWorkerThread();
	void				CreateDBThread();
	void				CreateNpcThread();

	BOOL				InitClientArray( int iMaxClient );
	BOOL				InitClientIndexArray( int iMaxClient );
	void				DeleteAll();
	void				DeleteAllClientArray();
	void				DeleteAllClientIndexArray();

	CRITICAL_SECTION	m_csClientIndex;

	CNetworkCallbacks*	m_pCallbacks;
	SOCKET				m_sockListen;

	int					m_iListenPort;

	BOOL				m_bShutDown;
	int					m_iMaxClient;
	CPtrArray			m_arClient;
	CClientIndexArray	m_arClientIndex;

	HANDLE				m_hAcceptThread;

	HANDLE				m_hWorkerThread[MAX_PORT];
	unsigned int		m_WorkerId[MAX_PORT];
	DWORD				m_dwNumberOfWorkers;

	HANDLE				m_hDBThread[MAX_PORT_DB];
	unsigned int		m_DBId[MAX_PORT_DB];
	DWORD				m_dwNumberOfDBs;

	HANDLE				m_hNpcThread[MAX_PORT_NPC];
	unsigned int		m_NpcId[MAX_PORT_NPC];
	DWORD				m_dwNumberOfNpcs;

	DWORD				m_dwConcurrency;

	CCrypt*				m_pCrypt;
	CAllowIpList*		m_pAllowIpList;
};

extern LIBNETWORK_API CNetwork*		g_pNetwork;
