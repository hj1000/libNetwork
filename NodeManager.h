#pragma once

#define MIN_NODE_BALANCE		100

class CNodeBuffer;
class CNodeManager
{
public:
	CNodeManager( int iNodeBalance = MIN_NODE_BALANCE );
	virtual ~CNodeManager();

	CNodeBuffer*		GetNode();
	void				ReleaseNode( CNodeBuffer* pNode );
	int					GetNodeCount();
	int					GetUseNodeCount();


private:
	void				Init();
	BOOL				IsSameHeadTail();
	BOOL				IsBalance();


	CRITICAL_SECTION	m_csNodeMan;
	int					m_iNodeBalance;
	int					m_iNodeLen;
	int					m_iUseNodeCount;
	CNodeBuffer*		m_pNodeHead;
	CNodeBuffer*		m_pNodeTail;
};
