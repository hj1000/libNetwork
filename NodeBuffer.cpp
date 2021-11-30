#include "stdafx.h"
#include ".\nodebuffer.h"

//#define _ALWAYS_MEMSET

CNodeBuffer::CNodeBuffer( int iBufSize )
{
	m_pBuf			= new BYTE[iBufSize];
	m_sMaxSize		= iBufSize;
	m_iIndex		= -1;

	memset( m_pBuf, NULL, m_sMaxSize );

	Init();
}

CNodeBuffer::~CNodeBuffer()
{
	delete[] m_pBuf;
}

void CNodeBuffer::Init()
{
#ifdef _ALWAYS_MEMSET
	memset( m_pBuf, NULL, m_sMaxSize );
#endif

	m_sCurSize		= 0;
	m_pNextNode		= NULL;
}

//Check Buffer Size...
void CNodeBuffer::CheckBuf( int iSize )
{
	while( iSize >= m_sMaxSize )	ReSize();
}

void CNodeBuffer::ReSize( int iIncSize )
{
	BYTE* pOldBuf = m_pBuf;

	int iNewSize	= m_sMaxSize + iIncSize;
	BYTE* pNewBuf	= new BYTE[iNewSize];
	memset( pNewBuf, NULL, iNewSize );

	memcpy( pNewBuf, m_pBuf, m_sCurSize );
	m_pBuf		= pNewBuf;
	m_sMaxSize	= iNewSize;

	delete[] pOldBuf;
}

//Copy Node...
void CNodeBuffer::CopyNodeData( CNodeBuffer* pSourceNode )
{
	if( !pSourceNode )		return;

	CheckBuf( pSourceNode->m_sMaxSize );		

//	m_iIndex		= pSourceNode->m_iIndex;		//데이터만 옮기는 것이므로 필요 없음...
	memcpy( m_pBuf, pSourceNode->m_pBuf, pSourceNode->m_sMaxSize );
	m_sMaxSize		= pSourceNode->m_sMaxSize;
	m_sCurSize		= pSourceNode->m_sCurSize;
	m_pNextNode		= pSourceNode->m_pNextNode;
}

//Write to Buffer...
void CNodeBuffer::SetString( char* pBuf, int len )
{
	CheckBuf( m_sCurSize + len );

	CopyMemory( m_pBuf + m_sCurSize, pBuf, len );
	m_sCurSize += len;
}

void CNodeBuffer::SetByte( BYTE bByte )
{
	CheckBuf( m_sCurSize + sizeof(BYTE) );

	CopyMemory( m_pBuf + m_sCurSize, &bByte, sizeof(BYTE) );
	m_sCurSize += sizeof(BYTE);
}

void CNodeBuffer::SetShort( short sShort )
{
	CheckBuf( m_sCurSize + sizeof(short) );

	CopyMemory( m_pBuf + m_sCurSize, &sShort, sizeof(short) );
	m_sCurSize += sizeof(short);
}

void CNodeBuffer::SetInt( int iInt )
{
	CheckBuf( m_sCurSize + sizeof(int) );

	CopyMemory( m_pBuf + m_sCurSize, &iInt, sizeof(int) );
	m_sCurSize += sizeof(int);
}

void CNodeBuffer::SetDWORD( DWORD dwDword )
{
	CheckBuf( m_sCurSize + sizeof(DWORD) );

	CopyMemory( m_pBuf + m_sCurSize, &dwDword, sizeof(DWORD) );
	m_sCurSize += sizeof(DWORD);
}

void CNodeBuffer::SetUINT64( UINT64 uUint64 )
{
	CheckBuf( m_sCurSize + sizeof(UINT64) );

	CopyMemory( m_pBuf + m_sCurSize, &uUint64, sizeof(UINT64) );
	m_sCurSize += sizeof(UINT64);
}

void CNodeBuffer::SetNodeBuffer( CNodeBuffer* pNode )
{
	SetString( (char*)pNode->GetBuf(), pNode->GetSize() );
}

//Read to Buffer...
void CNodeBuffer::GetString( char* tBuf, int len, int& iIndex )
{
	CopyMemory( tBuf, m_pBuf + iIndex, len );
	iIndex += len;
}

BYTE CNodeBuffer::GetByte( int& iIndex )
{
	iIndex += sizeof(BYTE);
	return *(BYTE*)( m_pBuf + iIndex - sizeof(BYTE) );
}

short CNodeBuffer::GetShort( int& iIndex )
{
	iIndex += sizeof(short);
	return *(short*)( m_pBuf + iIndex - sizeof(short) );
}

int CNodeBuffer::GetInt( int& iIndex )
{
	iIndex += sizeof(int);
	return *(int*)( m_pBuf + iIndex - sizeof(int) );
}

DWORD CNodeBuffer::GetDWORD( int& iIndex )
{
	iIndex += sizeof(DWORD);
	return *(DWORD*)( m_pBuf + iIndex - sizeof(DWORD) );
}

UINT64 CNodeBuffer::GetUINT64( int& iIndex )
{
	iIndex += sizeof(UINT64);
	return *(UINT64*)( m_pBuf + iIndex - sizeof(UINT64) );
}

int CNodeBuffer::GetIndex()
{
	return m_iIndex;
}

void CNodeBuffer::SetIndex( int iIndex )
{
	m_iIndex = iIndex;
}

BYTE* CNodeBuffer::GetBuf()
{
	return m_pBuf;
}

short CNodeBuffer::GetSize()
{
	return m_sCurSize;
}

CNodeBuffer* CNodeBuffer::GetNextNode()
{
	return m_pNextNode;
}

void CNodeBuffer::SetNextNode( CNodeBuffer* pNode )
{
	m_pNextNode = pNode;
}
