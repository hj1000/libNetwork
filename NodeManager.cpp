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

//�Ӹ��� ������ ������ üũ�Ѵ�...
BOOL CNodeManager::IsSameHeadTail()
{
	return (m_pNodeHead == m_pNodeTail)	? TRUE : FALSE;
}

//������ ��� ����Ʈ ũ�⸦ �ʰ� �ϴ��� üũ�Ѵ�...
BOOL CNodeManager::IsBalance()
{
	return (m_iNodeLen <= m_iNodeBalance) ? TRUE : FALSE;
}

//����� ��带 �����´�...
CNodeBuffer* CNodeManager::GetNode()
{
	CNodeBuffer* pNodeRet = NULL;

	EnterCriticalSection( &m_csNodeMan );

	if( !IsSameHeadTail() )								//�Ӹ��� ������ �ٸ� ��� �̸� ������ ��带 ��ȯ�Ѵ�...
	{
		pNodeRet		= m_pNodeHead;					//��� ��带 ��ȯ�Ѵ�...
		m_pNodeHead		= m_pNodeHead->GetNextNode();	//��� ����� ��ġ�� �ű��...
		m_iNodeLen		-= 1;							//��� ����Ʈ ����� ���δ�...
	}

	m_iUseNodeCount += 1;

	LeaveCriticalSection( &m_csNodeMan );

	if( !pNodeRet )		pNodeRet = new CNodeBuffer;		//����� ��带 �����Ѵ�...
	else				pNodeRet->Init();				//����� ��带 �ʱ�ȭ �Ѵ�...

	return pNodeRet;
}

//����� ��带 �ݳ��Ѵ�...
void CNodeManager::ReleaseNode( CNodeBuffer* pNode )
{
	BOOL bDelete = FALSE;

	EnterCriticalSection( &m_csNodeMan );

	if( !m_pNodeTail )		//������ ���� ���(ó�� �ݳ��ϴ� ���)...
	{
		//ó�� �ݳ��� ��忡 �Ӹ��� ������ ����Ű�� �Ѵ�...
		m_pNodeTail		= pNode;
		m_pNodeHead		= pNode;
		m_iNodeLen		= 1;
	}
	else
	{
		//������ ��� ����Ʈ ����� �ʰ��ϴ� ��� ����Ʈ�� ���� �ʴ´�...
		if( !IsBalance() )		bDelete = TRUE;
		else
		{
			m_pNodeTail->SetNextNode( pNode );		//���� ���� ����� ���� ��ũ�� �� ��带 ����Ű�� �Ѵ�...
			m_pNodeTail			= pNode;			//���� ��带 �� ���� �ٲ۴�...
			m_iNodeLen			+= 1;				//��ü ��� ����Ʈ ũ�⸦ ������Ų��...
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