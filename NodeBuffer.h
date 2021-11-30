#pragma once

#define MIN_MEM_BLOCK		4096

class CNodeBuffer
{
public:
	CNodeBuffer( int iBufSize = MIN_MEM_BLOCK );
	virtual ~CNodeBuffer();

	void			Init();
	void			CheckBuf( int iSize );
	void			ReSize( int iSize = MIN_MEM_BLOCK );
	void			CopyNodeData( CNodeBuffer* pSourceNode );

	void			SetString( char* pBuf, int len );
	void			SetByte( BYTE bByte );
	void			SetShort( short sShort );
	void			SetInt( int iInt );
	void			SetDWORD( DWORD dwDword );
	void			SetUINT64( UINT64 uUint64 );
	void			SetNodeBuffer( CNodeBuffer* pNode );

	void			GetString( char* tBuf, int len, int& iIndex );
	BYTE			GetByte( int& iIndex );
	short		 	GetShort( int& iIndex );
	int				GetInt( int& iIndex );
	DWORD			GetDWORD( int& iIndex );
	UINT64			GetUINT64( int& iIndex );

	int				GetIndex();
	void			SetIndex( int iIndex );
	BYTE*			GetBuf();
	short			GetSize();
	CNodeBuffer*	GetNextNode();
	void			SetNextNode( CNodeBuffer* pNode );


private:
	int				m_iIndex;
	BYTE*			m_pBuf;
	short			m_sCurSize;
	short			m_sMaxSize;
	CNodeBuffer*	m_pNextNode;
};
