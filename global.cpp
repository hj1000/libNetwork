#include "stdafx.h"
#include "defNetwork.h"

CFile*				g_pfileErrorLog;

int ParseSpace( char* tBuf, char* sBuf )
{
	int i = 0, index = 0;
	BOOL flag = FALSE;
	
	while( sBuf[index] == ' ' || sBuf[index] == '\t' )	index++;
	
	while( sBuf[index] !=' ' && sBuf[index] !='\t' && sBuf[index] != (BYTE)0 )
	{
		tBuf[i++] = sBuf[index++];
		flag = TRUE;
	}
	tBuf[i] = 0;

	while( sBuf[index] == ' ' || sBuf[index] == '\t' )	index++;

	if( !flag ) return 0;

	return index;
}

int ParseSlash( char* tBuf, char* sBuf )
{
	int i = 0, index = 0;
	BOOL flag = FALSE;
	
	while( sBuf[index] == '/' )		index++;

	while( sBuf[index] !='/' && sBuf[index] !=(BYTE)0 )
	{
		tBuf[i++] = sBuf[index++];
		flag = TRUE;
	}
	tBuf[i] = 0;

	while( sBuf[index] == '/' )		index++;

	if( !flag )	return 0;

	return index;
}

CString GetProgPath()
{
	char Buf[256], Path[256];
	char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

	::GetModuleFileName( AfxGetApp()->m_hInstance, Buf, 256 );
	_splitpath( Buf, drive, dir, fname, ext );
	strcpy( Path, drive );
	strcat( Path, dir );		
	CString _Path = Path;

	return _Path;
}

void DoubleQuote( char* pSource, char* pTarget )
{
	int s_index		= 0;
	int s_len		= 0;
	int t_index		= 0;

	s_len = (int)strlen( pSource );

	for( s_index = 0; s_index < s_len; s_index++ )
	{
		if( pSource[s_index] == '\'' )
		{
		}
		else
		{
			pTarget[t_index++] = pSource[s_index];
		}
	}
}

void ErrorLog( const char* strBuf, ... )
{
	char bufTemp[BUFSIZE_4];	memset( bufTemp, NULL, BUFSIZE_4 );

	va_list va;
	va_start( va, strBuf );

	int iLen = vsprintf( bufTemp, strBuf, va );

	va_end( va );

	_SYSTEMTIME time;
	GetLocalTime( &time );

	CString strMsg;
	strMsg.Format( "(%02d/%02d %02d:%02d %02d) %s\r\n",
		time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, bufTemp );

	TRACE( strMsg );

	if( g_pfileErrorLog )
	{
		g_pfileErrorLog->SeekToEnd();
		g_pfileErrorLog->Write( strMsg, strMsg.GetLength() );
	}
}

//start와 end를 포함한 랜덤 숫자 생성...
int MyRand( int iStart, int iEnd )
{
	if( iStart > iEnd )		std::swap( iStart, iEnd );
	if( iStart == iEnd )	return iStart;

	return iStart + rand() % ( iEnd - iStart + 1 );
}