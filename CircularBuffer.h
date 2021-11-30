// CircularBuffer.h: interface for the CCircularBuffer class.
//
#pragma once

#define BUFFER_SIZE			4096

class CCircularBuffer  
{
public:
	CCircularBuffer( int size = BUFFER_SIZE );
	virtual ~CCircularBuffer();

	void	PutData( char& data );
	void	PutData( char* pData, int len );
	void	GetData( char* pData, int len );
	int		GetOutData( char* pData );			//HeadPos, 변화
	char&	GetHeadData()						{	return m_pBuffer[m_iHeadPos];	}

	//1 Byte Operation;
	//false : 모든데이터 다빠짐, TRUE: 정상적으로 진행중
	BOOL	HeadIncrease( int increasement = 1 );
	void	SetEmpty()							{	m_iHeadPos = 0; m_iTailPos = 0;		}

	int&	GetBufferSize()						{	return m_iBufSize;	}
	int&	GetHeadPos()						{	return m_iHeadPos;	}
	int&	GetTailPos()						{	return m_iTailPos;	}
	int		GetValidCount();

protected:
	//over flow 먼저 점검한 후 IndexOverFlow 점검
	BOOL	IsOverFlowCondition( int& len )		{	return (len >= m_iBufSize - GetValidCount()) ? TRUE: FALSE;	}
	BOOL	IsIndexOverFlow( int& len )			{	return (len + m_iTailPos >= m_iBufSize) ? TRUE:FALSE;		}
	void	BufferResize();			//overflow condition 일때 size를 현재의 두배로 늘림

protected:
	int		m_iBufSize;
	char*	m_pBuffer;

	int		m_iHeadPos;
	int		m_iTailPos;
};
