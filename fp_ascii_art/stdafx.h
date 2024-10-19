// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT _WIN32_WINNT_MAXVER
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxcmn.h>			// MFC support for Windows Common Controls

#define WM_POST_INITDIALOG (WM_USER + 1)