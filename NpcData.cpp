#include "StdAfx.h"
#include "Network.h"
#include "NpcData.h"

CNpcData::CNpcData( short sNid )
{
	InitializeCriticalSection( &m_csNpc );

	m_sNid			= sNid;

	Init();
}

CNpcData::~CNpcData()
{
	DeleteCriticalSection( &m_csNpc );
}

void CNpcData::Init()
{
	EnterCriticalSection( &m_csNpc );
	
	m_sImageNum		= MyRand( 0, 10 );
	m_sRoomNum		= 0;

	LeaveCriticalSection( &m_csNpc );
}
