#include "StdAfx.h"
#include "NpcManager.h"

CNpcManager::CNpcManager()
{
	Init();
}

CNpcManager::~CNpcManager()
{
	RemoveNpcData();
}

void CNpcManager::Init()
{
	for( int i = 0; i < MAX_NPC; ++i )
	{
		CNpcData* pNpcData = new CNpcData( i );
		m_arNpcData.Add( pNpcData );
	}
}

void CNpcManager::RemoveNpcData()
{
	for( int i = 0; i < m_arNpcData.GetSize(); ++i )
	{
		CNpcData* pNpcData = (CNpcData*)m_arNpcData[i];
		if( pNpcData )		delete pNpcData;
	}
	m_arNpcData.RemoveAll();
}

void CNpcManager::ExecProc()
{
}
