// CircularBuffer.cpp: implementation of the CCircularBuffer class.
//
#include "stdafx.h"
#include "CircularBuffer.h"

CCircularBuffer::CCircularBuffer( int size )
{
	ASSERT( size > 0 );

	m_iBufSize	= size;
	m_pBuffer	= new char[m_iBufSize];

	m_iHeadPos = 0;
	m_iTailPos = 0;
}

CCircularBuffer::~CCircularBuffer()
{
	ASSERT( m_pBuffer );

	delete[] m_pBuffer;
	m_pBuffer = NULL;
}

int CCircularBuffer::GetValidCount()
{
	int count = m_iTailPos - m_iHeadPos;
	if( count < 0 )		count = m_iBufSize + count;
	return count;
}

void CCircularBuffer::BufferResize()
{
	int prevBufSize = m_iBufSize;
	m_iBufSize <<= 1;

	char* pNewData = new char[m_iBufSize];
	CopyMemory( pNewData, m_pBuffer, prevBufSize );

	if( m_iTailPos < m_iHeadPos )
	{
		CopyMemory( pNewData + prevBufSize, m_pBuffer, m_iTailPos );
		m_iTailPos += prevBufSize;
	}

	delete[] m_pBuffer;
	m_pBuffer = pNewData;
}

void CCircularBuffer::PutData( char& data )
{
	int len = 1;
	while( IsOverFlowCondition( len ) )		BufferResize();

	m_pBuffer[m_iTailPos++] = data;
	if( m_iTailPos == m_iBufSize )		m_iTailPos = 0;
}

void CCircularBuffer::PutData( char* pData, int len )
{
	if( len <= 0 )
	{
		TRACE( "%s: len is <= 0\n", __FUNCTION__ );
		return;
	}

	while( IsOverFlowCondition(len) )		BufferResize();

	if( IsIndexOverFlow(len) )
	{
		int FirstCopyLen	= m_iBufSize - m_iTailPos;
		int SecondCopyLen	= len - FirstCopyLen;
		
		ASSERT( FirstCopyLen );
		
		CopyMemory( m_pBuffer + m_iTailPos, pData, FirstCopyLen );
		if( SecondCopyLen )
		{
			CopyMemory( m_pBuffer, pData + FirstCopyLen, SecondCopyLen );
			m_iTailPos = SecondCopyLen;
		}
		else
			m_iTailPos = 0;
	}
	else
	{
		CopyMemory( m_pBuffer + m_iTailPos, pData, len );
		m_iTailPos += len;
	}
}

int CCircularBuffer::GetOutData( char* pData )
{
	int len	= GetValidCount();
	int fc	= m_iBufSize - m_iHeadPos;
	if( len > fc )
	{
		int sc = len - fc;
		CopyMemory( pData, m_pBuffer + m_iHeadPos, fc );
		CopyMemory( pData + fc, m_pBuffer, sc );
		m_iHeadPos = sc;
		
		ASSERT( m_iHeadPos == m_iTailPos );
	}
	else
	{
		CopyMemory( pData, m_pBuffer + m_iHeadPos, len );
		m_iHeadPos += len;
		if( m_iHeadPos == m_iBufSize )	m_iHeadPos = 0;
	}
	return len;
}

void CCircularBuffer::GetData( char* pData, int len )
{
	ASSERT( len > 0 && len <= GetValidCount() );

	if( len < m_iBufSize - m_iHeadPos )		CopyMemory( pData, m_pBuffer + m_iHeadPos, len );
	else
	{
		int fc	= m_iBufSize - m_iHeadPos;
		int sc	= len - fc;
		CopyMemory( pData, m_pBuffer + m_iHeadPos, fc );
		if( sc )	CopyMemory( pData + fc, m_pBuffer, sc );
	}
}

BOOL CCircularBuffer::HeadIncrease( int increasement )
{
	ASSERT( increasement <= GetValidCount() );

	m_iHeadPos	+= increasement;
	m_iHeadPos	%= m_iBufSize;

	return m_iHeadPos != m_iTailPos;
}
