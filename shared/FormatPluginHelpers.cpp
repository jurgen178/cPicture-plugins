#include "stdafx.h"
#include "FormatPluginHelpers.h"

CString GetSizeStr(const __int64 size)
{
	CString s;
	__int64 d(1024);
	__int64 z(1024);

	if (size < d)
	{
		s.Format(L"%lld Byte", size);
		return s;
	}

	d <<= 10;
	if (size < d)
	{
		s.Format(L"%lld KB", size / z);
		return s;
	}

	d <<= 10;
	z <<= 10;
	if (size < d)
	{
		s.Format(L"%.1lf MB", static_cast<double>(size) / z);
		return s;
	}

	d <<= 10;
	z <<= 10;
	if (size < d)
	{
		s.Format(L"%.2lf GB", static_cast<double>(size) / z);
		return s;
	}

	z <<= 10;
	s.Format(L"%.2lf TB", static_cast<double>(size) / z);
	return s;
}

__int64 GetFileSize64(const WCHAR* pFile)
{
	WIN32_FIND_DATA findFileData;
	const HANDLE hFile = FindFirstFile(pFile, &findFileData);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		const __int64 size((static_cast<__int64>(findFileData.nFileSizeHigh) << 32) + findFileData.nFileSizeLow);
		FindClose(hFile);
		return size;
	}

	return 0;
}

CString GetLongFileDateTime(const WCHAR* pFile, FILETIME& filetime, filetime_type type)
{
	CString dateTime;
	WIN32_FIND_DATA findFileData;
	const HANDLE hFind = FindFirstFile(pFile, &findFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		if (type == filetime_type::date_created)
		{
			filetime = findFileData.ftCreationTime;
		}
		else
		{
			filetime = findFileData.ftLastWriteTime;
		}

		SYSTEMTIME stUTC = { 0 };
		SYSTEMTIME sysTime = { 0 };
		FileTimeToSystemTime(&filetime, &stUTC);
		SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &sysTime);

		WCHAR dateStr[100] = { 0 };
		WCHAR timeStr[100] = { 0 };

		GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &sysTime, NULL, dateStr, sizeof(dateStr) / sizeof(WCHAR));
		GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, timeStr, sizeof(timeStr) / sizeof(WCHAR));

		dateTime.Format(L"%s %s", dateStr, timeStr);
	}

	return dateTime;
}

CString GetLongFileDateTime(const WCHAR* pFile, filetime_type type)
{
	FILETIME filetime = { 0 };
	return GetLongFileDateTime(pFile, filetime, type);
}

void CopyBgrToRgbRow(const BYTE* src, BYTE* dst, const int width)
{
	const BYTE* dstEnd = dst + static_cast<size_t>(width) * 3;
	while (dst < dstEnd)
	{
		const BYTE blue = *src++;
		const BYTE green = *src++;
		const BYTE red = *src++;

		*dst++ = red;
		*dst++ = green;
		*dst++ = blue;
	}
}

void CopyBgraToRgb(const BYTE* src, BYTE* dst, const __int64 pixelCount)
{
	const BYTE* dstEnd = dst + pixelCount * 3;
	while (dst < dstEnd)
	{
		const BYTE blue = *src++;
		const BYTE green = *src++;
		const BYTE red = *src++;
		++src;

		*dst++ = red;
		*dst++ = green;
		*dst++ = blue;
	}
}

void CopyRgbaToRgb(const BYTE* src, BYTE* dst, const __int64 pixelCount)
{
	const BYTE* dstEnd = dst + pixelCount * 3;
	while (dst < dstEnd)
	{
		*dst++ = *src++;
		*dst++ = *src++;
		*dst++ = *src++;
		++src;
	}
}

void AppendAnimatedFrameCountInfo(CString& msg, const CString& frameCountTemplate, const int frameCount)
{
	if (frameCount <= 1 || frameCountTemplate.IsEmpty())
	{
		return;
	}

	CString info;
	info.FormatMessage(frameCountTemplate, frameCount);
	msg += L'\n';
	msg += info;
}

void AppendAnimatedDurationInfo(CString& msg, const CString& durationTemplate, const int durationMs)
{
	if (durationMs <= 0 || durationTemplate.IsEmpty())
	{
		return;
	}

	CString duration;
	if (durationMs < 1000)
	{
		duration.Format(L"%d ms", durationMs);
	}
	else
	{
		duration.Format(L"%.1fs", static_cast<double>(durationMs) / 1000.0);
	}

	CString info;
	info.FormatMessage(durationTemplate, duration);
	msg += L'\n';
	msg += info;
}

void AppendAnimatedLoopCountInfo(CString& msg, const CString& loopCountTemplate, const CString& infiniteLoopText, const int loopCount)
{
	if (loopCount < 0 || loopCountTemplate.IsEmpty())
	{
		return;
	}

	CString loopText;
	if (loopCount == 0)
	{
		loopText = infiniteLoopText;
	}
	else
	{
		loopText.Format(L"%d", loopCount);
	}

	CString info;
	info.FormatMessage(loopCountTemplate, loopText);
	msg += L'\n';
	msg += info;
}

void AppendAnimatedFrameRateInfo(CString& msg, const CString& frameRateTemplate, const int frameCount, const int durationMs)
{
	if (frameCount <= 1 || durationMs <= 0 || frameRateTemplate.IsEmpty())
	{
		return;
	}

	CString frameRate;
	frameRate.Format(L"%.1f fps", static_cast<double>(frameCount) * 1000.0 / static_cast<double>(durationMs));

	CString info;
	info.FormatMessage(frameRateTemplate, frameRate);
	msg += L'\n';
	msg += info;
}

void AppendAnimatedFrameDurationInfo(CString& msg, const CString& frameDurationTemplate, const int minFrameDurationMs, const int maxFrameDurationMs)
{
	if (minFrameDurationMs <= 0 || maxFrameDurationMs <= 0 || frameDurationTemplate.IsEmpty())
	{
		return;
	}

	CString frameDuration;
	if (minFrameDurationMs == maxFrameDurationMs)
	{
		frameDuration.Format(L"%d ms", minFrameDurationMs);
	}
	else
	{
		frameDuration.Format(L"%d-%d ms", minFrameDurationMs, maxFrameDurationMs);
	}

	CString info;
	info.FormatMessage(frameDurationTemplate, frameDuration);
	msg += L'\n';
	msg += info;
}
