#pragma once

class CNpcData
{
public:
	CNpcData( short sNid );
	virtual ~CNpcData();

	void				Init();
	void				RandMove();


private:
	CRITICAL_SECTION	m_csNpc;
	short				m_sNid;
	short				m_sImageNum;
	
	short				m_sRoomNum;
};
