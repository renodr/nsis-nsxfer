#pragma once

#ifndef _DEBUG
	#if DBG || _DEBUG
		#define _DEBUG
	#endif
#endif

#define COBJMACROS

#define _WIN32_WINNT 0x0500
#define _WIN32_IE    0x0600
#include <windows.h>
#include <wininet.h>
#include <Shlwapi.h>				/// for wvnsprintf
#include "nsiswapi/pluginapi.h"
#include <commctrl.h>
#include <Shobjidl.h>				/// ITaskbarList

extern HINSTANCE g_hInst;			/// Defined in main.c
