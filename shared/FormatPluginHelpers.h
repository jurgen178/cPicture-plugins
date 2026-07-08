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
void AppendAnimatedFrameCountInfo(CString& msg, const CString& frameCountTemplate, int frameCount);
void AppendAnimatedDurationInfo(CString& msg, const CString& durationTemplate, int durationMs);
void AppendAnimatedLoopCountInfo(CString& msg, const CString& loopCountTemplate, const CString& infiniteLoopText, int loopCount);
void AppendAnimatedFrameRateInfo(CString& msg, const CString& frameRateTemplate, int frameCount, int durationMs);
void AppendAnimatedFrameDurationInfo(CString& msg, const CString& frameDurationTemplate, int minFrameDurationMs, int maxFrameDurationMs);
