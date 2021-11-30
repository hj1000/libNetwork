#pragma once

#include <list>
using namespace std;

#include <inaddr.h>

class CAllowIpList
{
public:
	CAllowIpList();
	virtual ~CAllowIpList();

	void				Init();
	BOOL				LoadIpList( CString strFileName );
	BOOL				CheckAllowIp( in_addr sin_addr );

	list<string>		m_listIp;

	BOOL				m_bCheck;
};
