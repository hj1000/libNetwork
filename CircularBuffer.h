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
	int		GetOutData( char* pData );			//HeadPos, ��ȭ
	char&	GetHeadData()						{	return m_pBuffer[m_iHeadPos];	}

	//1 Byte Operation;
	//false : ��絥���� �ٺ���, TRUE: ���������� ������
	BOOL	HeadIncrease( int increasement = 1 );
	void	SetEmpty()							{	m_iHeadPos = 0; m_iTailPos = 0;		}

	int&	GetBufferSize()						{	return m_iBufSize;	}
	int&	GetHeadPos()						{	return m_iHeadPos;	}
	int&	GetTailPos()						{	return m_iTailPos;	}
	int		GetValidCount();

protected:
	//over flow ���� ������ �� IndexOverFlow ����
	BOOL	IsOverFlowCondition( int& len )		{	return (len >= m_iBufSize - GetValidCount()) ? TRUE: FALSE;	}
	BOOL	IsIndexOverFlow( int& len )			{	return (len + m_iTailPos >= m_iBufSize) ? TRUE:FALSE;		}
	void	BufferResize();			//overflow condition �϶� size�� ������ �ι�� �ø�

protected:
	int		m_iBufSize;
	char*	m_pBuffer;

	int		m_iHeadPos;
	int		m_iTailPos;
};
