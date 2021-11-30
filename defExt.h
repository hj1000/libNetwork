#pragma once

#include "defNetwork.h"

#ifdef _DEBUG
	#define CHECK_RUNTIME					100
#else
	#define CHECK_RUNTIME					1000
#endif

#define SAFE_DELETE( p )			if( p )	{	delete p;	p = NULL	}
#define IS_RANGE( x, min, max )		( (x >= min) ? ((x < max) ? TRUE : FALSE) : FALSE )
