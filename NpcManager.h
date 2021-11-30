#pragma once

#include "NpcData.h"

#define MAX_NPC				1000

class CNpcManager
{
public:
	CNpcManager();
	virtual ~CNpcManager();

	void				Init();
	void				CreateNpcData();
	void				RemoveNpcData();
	void				ExecProc();

	CPtrArray			m_arNpcData;

private:
	CRITICAL_SECTION	m_csNpcMan;
};
