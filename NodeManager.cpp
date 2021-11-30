#include "stdafx.h"
#include "NodeBuffer.h"
#include ".\nodemanager.h"

CNodeManager::CNodeManager( int iNodeBalance )
{
	InitializeCriticalSection( &m_csNodeMan );

	m_iNodeBalance	= iNodeBalance;
	m_iNodeLen		= 0;
	m_iUseNodeCount	= 0;
	m_pNodeHead		= NULL;
	m_pNodeTail		= NULL;

	Init();
}

CNodeManager::~CNodeManager()
{
	Init();

	DeleteCriticalSection( &m_csNodeMan );
}

void CNodeManager::Init()
{
	EnterCriticalSection( &m_csNodeMan );

	//delete All Node...
	CNodeBuffer* pNodeCur = m_pNodeHead;
	while( pNodeCur )
	{
		CNodeBuffer* pNodeOld = pNodeCur;
		pNodeCur = pNodeCur->GetNextNode();
		delete pNodeOld;
	}

	//Init Value...
	m_iNodeLen		= 0;
	m_iUseNodeCount	= 0;
	m_pNodeHead		= NULL;
	m_pNodeTail		= NULL;

	LeaveCriticalSection( &m_csNodeMan );
}

//머리와 꼬리가 같은지 체크한다...
BOOL CNodeManager::IsSameHeadTail()
{
	return (m_pNodeHead == m_pNodeTail)	? TRUE : FALSE;
}

//유지할 노드 리스트 크기를 초과 하는지 체크한다...
BOOL CNodeManager::IsBalance()
{
	return (m_iNodeLen <= m_iNodeBalance) ? TRUE : FALSE;
}

//사용할 노드를 가져온다...
CNodeBuffer* CNodeManager::GetNode()
{
	CNodeBuffer* pNodeRet = NULL;

	EnterCriticalSection( &m_csNodeMan );

	if( !IsSameHeadTail() )								//머리와 꼬리가 다른 경우 미리 생성된 노드를 반환한다...
	{
		pNodeRet		= m_pNodeHead;					//헤드 노드를 반환한다...
		m_pNodeHead		= m_pNodeHead->GetNextNode();	//헤드 노드의 위치를 옮긴다...
		m_iNodeLen		-= 1;							//노드 리스트 사이즈를 줄인다...
	}

	m_iUseNodeCount += 1;

	LeaveCriticalSection( &m_csNodeMan );

	if( !pNodeRet )		pNodeRet = new CNodeBuffer;		//사용할 노드를 생성한다...
	else				pNodeRet->Init();				//사용할 노드를 초기화 한다...

	return pNodeRet;
}

//사용한 노드를 반납한다...
void CNodeManager::ReleaseNode( CNodeBuffer* pNode )
{
	BOOL bDelete = FALSE;

	EnterCriticalSection( &m_csNodeMan );

	if( !m_pNodeTail )		//꼬리가 없는 경우(처음 반납하는 경우)...
	{
		//처음 반납된 노드에 머리와 꼬리를 가리키게 한다...
		m_pNodeTail		= pNode;
		m_pNodeHead		= pNode;
		m_iNodeLen		= 1;
	}
	else
	{
		//유지할 노드 리스트 사이즈를 초과하는 경우 리스트에 넣지 않는다...
		if( !IsBalance() )		bDelete = TRUE;
		else
		{
			m_pNodeTail->SetNextNode( pNode );		//기존 꼬리 노드의 다음 링크를 새 노드를 가리키게 한다...
			m_pNodeTail			= pNode;			//꼬리 노드를 새 노드로 바꾼다...
			m_iNodeLen			+= 1;				//전체 노드 리스트 크기를 증가시킨다...
		}
	}

	m_iUseNodeCount -= 1;

	LeaveCriticalSection( &m_csNodeMan );

	if( bDelete )	delete pNode;
}

int CNodeManager::GetNodeCount()
{
	int iCount = 0;

	EnterCriticalSection( &m_csNodeMan );

	iCount = m_iNodeLen;

	LeaveCriticalSection( &m_csNodeMan );

	return iCount;
}

int CNodeManager::GetUseNodeCount()
{
	int iCount = 0;

	EnterCriticalSection( &m_csNodeMan );

	iCount = m_iUseNodeCount;

	LeaveCriticalSection( &m_csNodeMan );

	return iCount;
}