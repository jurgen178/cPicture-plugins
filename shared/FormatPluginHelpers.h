#pragma once

#include <afxwin.h>

enum filetime_type
{
	date_modified,
	date_created,
};

CString GetSizeStr(const __int64 size);
__int64 GetFileSize64(const WCHAR* pFile);
CString GetLongFileDateTime(const WCHAR* pFile, FILETIME& filetime, filetime_type type);
CString GetLongFileDateTime(const WCHAR* pFile, filetime_type type);
void CopyBgrToRgbRow(const BYTE* src, BYTE* dst, int width);
void CopyBgraToRgb(const BYTE* src, BYTE* dst, __int64 pixelCount);
void CopyRgbaToRgb(const BYTE* src, BYTE* dst, __int64 pixelCount);
