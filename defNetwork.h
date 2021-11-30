#pragma once

#ifdef LIBNETWORK_EXPORTS
	#define LIBNETWORK_API			__declspec(dllexport)
#else
	#define LIBNETWORK_API			__declspec(dllimport)
#endif

//Default Value...
#define DEFAULT_PORT_LISTEN			10000
#define DEFAULT_MAX_CLIENT			2000

#define OVL_RECEIVE					0X01
#define OVL_SEND					0X02
#define OVL_CLOSE					0X03
#define OVL_DB						0X04

//Close Kind...
#define CLOSETYPE_RECEIVE			1
#define CLOSETYPE_TRANSERR			2

//Packet Info...
#define PACKET_START1				0XA1
#define PACKET_START2				0XB1
#define PACKET_END1					0X1A
#define PACKET_END2					0X1B

//Buffer Size...
#define RECV_BUF_LENGTH				10240

//Send Kind...
#define SEND_USER					0
#define SEND_INSIGHT				1
#define SEND_ZONE					2
#define SEND_ALL					3
#define SEND_RANGE					4

//Defines About Max Value
#define MAX_PORT					4
#define MAX_PORT_DB					MAX_PORT
#define MAX_PORT_NPC				10
#define GET_PORT(x)					( x % MAX_PORT )
#define GET_DBPORT(x)				( x % MAX_PORT_DB )
#define GET_NPCPORT(x)				( x % MAX_PORT_NPC )

//DB Type
#define DBTYPE_GAME					0
#define DBTYPE_LOGIN				( DBTYPE_GAME + 1 )

//Buffer Size...
#ifndef _BUFSIZE
	#define _BUFSIZE
	#define BUFSIZE_1					1024
	#define BUFSIZE_2					( BUFSIZE_1 * 2 )
	#define BUFSIZE_4					( BUFSIZE_1 * 4 )
	#define BUFSIZE_8					( BUFSIZE_1 * 8 )
	#define BUFSIZE_10					( BUFSIZE_1 * 10 )
	#define BUFSIZE_MAX					BUFSIZE_10
#endif

//State Value
#define STATE_DISCONNECT			0X01
#define STATE_ACCEPT				0X02
#define STATE_LOGIN					0X03
#define STATE_LOGOUT				0X04

//Result...
#define RESULT_SUCCESS				TRUE
#define RESULT_FAIL					FALSE

int		ParseSpace( char* tBuf, char* sBuf );
int		ParseSlash( char* tBuf, char* sBuf );

void	DoubleQuote( char* pSource, char* pTarget );
void	ErrorLog( const char* strBuf, ... );
int		MyRand( int iStart, int iEnd );

CString GetProgPath();

typedef union{
	short int	i;
	BYTE		b[2];
} MYSHORT;

typedef unsigned char	T_OCTET;
typedef LONGLONG 		T_KEY;

typedef CArray <int, short>		CUidArray;
typedef CArray <int, short>		CClientIndexArray;

extern CFile*				g_pfileErrorLog;
