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
