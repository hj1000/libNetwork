#include "StdAfx.h"
#include "defExt.h"
#include ".\allowiplist.h"

CAllowIpList::CAllowIpList()
{
	Init();
}

CAllowIpList::~CAllowIpList()
{
}

void CAllowIpList::Init()
{
	m_bCheck = FALSE;
}

BOOL CAllowIpList::LoadIpList( CString strFileName )
{
	//ini 파일에서 Ip정보를 읽어 온다...
	TCHAR pszCurPath[MAX_PATH];
	GetCurrentDirectory( MAX_PATH, pszCurPath );

	CString strIniPath;	strIniPath.Format( "%s\\%s", pszCurPath, strFileName );

	int iCount = GetPrivateProfileInt( "AllowIp", "Count", 0, strIniPath );
	for( int i = 0; i < iCount; ++i )
	{
		char szIp[BUFSIZE_1];	memset( szIp, NULL, BUFSIZE_1 );

		CString strKeyName;		strKeyName.Format( "Ip%d", i );
		GetPrivateProfileString( "AllowIp", strKeyName, "127.0.0.1", szIp, BUFSIZE_1, strIniPath );

		m_listIp.push_back( (string)szIp );
	}

	SetCurrentDirectory( pszCurPath );

	m_bCheck = TRUE;

	return TRUE;
}

BOOL CAllowIpList::CheckAllowIp( in_addr sin_addr )
{
	if( !m_bCheck )		return TRUE;

	CString strIp;
	strIp.Format( "%d.%d.%d.%d"
		, sin_addr.S_un.S_un_b.s_b1
		, sin_addr.S_un.S_un_b.s_b2
		, sin_addr.S_un.S_un_b.s_b3
		, sin_addr.S_un.S_un_b.s_b4
		);

	list<string>::iterator iter;
	for( iter = m_listIp.begin(); iter != m_listIp.end(); ++iter )
	{
		if( iter->c_str() == strIp )	return TRUE;
	}

	ErrorLog( "%s Ip: %s", __FUNCTION__, strIp );

	return FALSE;
}