#pragma once

#include <process.h>

unsigned __stdcall AcceptThread( void* lp );
unsigned __stdcall WorkerThread( void* lp );
unsigned __stdcall DBThread( void* lp );
unsigned __stdcall NpcThread( void* lp );
