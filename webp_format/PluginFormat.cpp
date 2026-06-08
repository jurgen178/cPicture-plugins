#include "stdafx.h"
#include "resource.h"
#include "pluginformat.h"
#include "webp/decode.h"
#include "webp/encode.h"

#include <algorithm>
#include <cstdio>
#include <vector>
using namespace std;

enum filetime_type
{
	date_modified,
	date_created,
};

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
	WIN32_FIND_DATA c_file;
	HANDLE hFile;

	if ((hFile = FindFirstFile(pFile, &c_file)) != INVALID_HANDLE_VALUE)
	{
		const __int64 size((static_cast<__int64>(c_file.nFileSizeHigh) << 32) + c_file.nFileSizeLow);
		FindClose(hFile);

		return size;
	}

	return 0;
}

CString GetLongFileDateTime(const WCHAR* pFile, FILETIME& filetime, filetime_type type)
{
	CString DateTimeFormat;

	WIN32_FIND_DATA findFileData;
	const HANDLE hFind = FindFirstFile(pFile, &findFileData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		if (type == filetime_type::date_created)
			filetime = findFileData.ftCreationTime;
		else
			filetime = findFileData.ftLastWriteTime;

		SYSTEMTIME stUTC = { 0 };
		SYSTEMTIME sysTime = { 0 };
		FileTimeToSystemTime(&filetime, &stUTC);
		SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &sysTime);

		WCHAR DateStr[100] = { 0 };
		WCHAR TimeStr[100] = { 0 };

		GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &sysTime, NULL, DateStr, sizeof(DateStr) / sizeof(WCHAR));
		GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, TimeStr, sizeof(TimeStr) / sizeof(WCHAR));

		DateTimeFormat.Format(IDS_DATE_TIME_FORMAT_STRING, DateStr, TimeStr);
	}

	return DateTimeFormat;
}

CString GetLongFileDateTime(const WCHAR* pFile, filetime_type type)
{
	FILETIME filetime = { 0 };
	return GetLongFileDateTime(pFile, filetime, type);
}

enum PLUGIN_TYPE operator|(const enum PLUGIN_TYPE t1, const enum PLUGIN_TYPE t2)
{
	return static_cast<enum PLUGIN_TYPE>(static_cast<const unsigned int>(t1) | static_cast<const unsigned int>(t2));
}

enum PLUGIN_TYPE operator&(const enum PLUGIN_TYPE t1, const enum PLUGIN_TYPE t2)
{
	return static_cast<enum PLUGIN_TYPE>(static_cast<const unsigned int>(t1) & static_cast<const unsigned int>(t2));
}

enum info_type operator|(const enum info_type t1, const enum info_type t2)
{
	return static_cast<enum info_type>(static_cast<const unsigned int>(t1) | static_cast<const unsigned int>(t2));
}

enum info_type operator&(const enum info_type t1, const enum info_type t2)
{
	return static_cast<enum info_type>(static_cast<const unsigned int>(t1) & static_cast<const unsigned int>(t2));
}

enum info_type operator|=(enum info_type& t1, const enum info_type t2)
{
	return t1 = static_cast<enum info_type>(static_cast<const unsigned int>(t1) | static_cast<const unsigned int>(t2));
}

const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
#ifdef _DEBUG
	return L"1.0-debug";
#else
	return L"1.0";
#endif
}

const PLUGIN_TYPE __stdcall GetPluginType()
{
	return PLUGIN_TYPE_FORMAT;
}

const int __stdcall GetPluginInit()
{
	return 1;
}

lpfnFormatGetInstanceProc __stdcall GetPluginProc(const int k)
{
	return k == 0 ? CWebPFormat::GetInstance : NULL;
}

const CString CWebPFormat::type = L"WebP";

vector<BYTE> ReadFileData(const CString& FileName, CString& errorMsg)
{
	vector<BYTE> fileData;
	const __int64 fileSize = GetFileSize64(FileName);
	if (fileSize <= 0)
	{
		errorMsg.Format(L"Invalid WebP file size: %I64d bytes", fileSize);
		return fileData;
	}

	FILE* fp = NULL;
	const errno_t err(_wfopen_s(&fp, FileName, L"rb"));
	if (err != 0 || !fp)
	{
		errorMsg.Format(L"Error open file '%s' for reading", FileName);
		return fileData;
	}

	fileData.resize(static_cast<size_t>(fileSize));
	const size_t readBytes = fread(fileData.data(), 1, fileData.size(), fp);
	fclose(fp);

	if (readBytes != fileData.size())
	{
		errorMsg.Format(L"Error reading WebP file '%s'", FileName);
		fileData.clear();
	}

	return fileData;
}

CString __stdcall CWebPFormat::get_ext() const
{
	return L"webp";
}

struct plugin_data __stdcall CWebPFormat::get_plugin_data() const
{
	struct plugin_data pluginData;
	pluginData.name.LoadString(IDS_SHORT_DESC);
	pluginData.desc.LoadString(IDS_LONG_DESC);
	return pluginData;
}

unsigned int __stdcall CWebPFormat::get_cap() const
{
	return PICTURE_READ | PICTURE_WRITE | PICTURE_QUALITY;
}

bool __stdcall CWebPFormat::properties_dlg(const HWND hwnd)
{
	CString msg;
	msg.LoadString(IDS_PROPERTY_DLG_TEXT);
	::MessageBox(hwnd, msg, get_plugin_data().desc, MB_ICONINFORMATION);
	return false;
}

BYTE* __stdcall CWebPFormat::FileToRGB(const CString& FileName,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n,
	const enum scaling_type picture_scaling_type,
	const bool b_scan)
{
	const vector<BYTE> fileData = ReadFileData(FileName, m_ErrorMsg);
	if (fileData.empty())
	{
		return NULL;
	}

	int width = 0;
	int height = 0;
	if (!WebPGetInfo(fileData.data(), fileData.size(), &width, &height) || width <= 0 || height <= 0)
	{
		m_ErrorMsg = L"Invalid WebP image header";
		return NULL;
	}

	const __int64 rowBytes = static_cast<__int64>(width) * 3;
	const __int64 size = rowBytes * height;
	BYTE* buffer = static_cast<BYTE*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
	if (!buffer)
	{
		m_ErrorMsg.Format(L"WebP memory request failed: %I64d bytes", size);
		return NULL;
	}

	if (!WebPDecodeRGBInto(fileData.data(), fileData.size(), buffer, static_cast<size_t>(size), static_cast<int>(rowBytes)))
	{
		m_ErrorMsg = L"WebP decode failed";
		VirtualFree(buffer, 0, MEM_RELEASE);
		return NULL;
	}

	m_OriginalPictureWidth = m_PictureWidth = width;
	m_OriginalPictureHeight = m_PictureHeight = height;
	m_color_space = 2;
	m_bIsValid = true;
	return buffer;
}

bool __stdcall CWebPFormat::RGBToFile(const CString& FileName,
	const BYTE* dataBuf,
	const int width,
	const int height,
	const int quality_L,
	const int quality_C,
	const int jpeg_lossless)
{
	if (!dataBuf || width <= 0 || height <= 0)
	{
		m_ErrorMsg = L"Invalid WebP input buffer";
		return false;
	}

	const int quality = quality_L >= 0 ? min(max(quality_L, 0), 100) : 80;
	uint8_t* output = NULL;
	const size_t outputSize = WebPEncodeRGB(dataBuf, width, height, width * 3, static_cast<float>(quality), &output);
	if (outputSize == 0 || !output)
	{
		m_ErrorMsg = L"WebP encode failed";
		return false;
	}

	FILE* fp = NULL;
	const errno_t err(_wfopen_s(&fp, FileName, L"wb"));
	if (err != 0 || !fp)
	{
		m_ErrorMsg.Format(L"Error open file '%s' for writing", FileName);
		WebPFree(output);
		return false;
	}

	const bool ok = fwrite(output, 1, outputSize, fp) == outputSize;
	fclose(fp);

	if (!ok)
	{
		m_ErrorMsg.Format(L"Error writing WebP file '%s'", FileName);
	}

	WebPFree(output);
	return ok;
}

void CWebPFormat::get_size(const CString& FileName)
{
	m_bIsValid = false;
	m_OriginalPictureWidth = m_OriginalPictureHeight = 0;
	m_PictureWidth = m_PictureHeight = 0;

	CString errorMsg;
	const vector<BYTE> fileData = ReadFileData(FileName, errorMsg);
	if (fileData.empty())
	{
		return;
	}

	int width = 0;
	int height = 0;
	if (WebPGetInfo(fileData.data(), fileData.size(), &width, &height) && width > 0 && height > 0)
	{
		m_OriginalPictureWidth = m_PictureWidth = width;
		m_OriginalPictureHeight = m_PictureHeight = height;
		m_color_space = 2;
		m_bIsValid = m_OriginalPictureWidth > 0 && m_OriginalPictureHeight > 0;
	}
}

vector<CString> info_template;

void __stdcall SetPluginInfoTemplates(const vector<CString>& _info_template)
{
	info_template = _info_template;
}

CString __stdcall CWebPFormat::get_info(const CString& FileName, const enum info_type _info_type)
{
	if (_info_type & (info_type_std | info_type_size | info_type_short))
	{
		get_size(FileName);
	}

	if (_info_type & info_type_short)
	{
		CString msg, info;
		const int info_template_c(static_cast<int>(info_template.size()));
		if (info_template_c > 8)
		{
			const WCHAR* t1 = wcsrchr(FileName, L'\\');
			if (t1)
				++t1;
			else
				t1 = FileName;

			info.FormatMessage(info_template[0], t1);
			msg += info;
			msg += L'\n';

			info.FormatMessage(info_template[7], FileName.Left(FileName.ReverseFind(L'\\') + 1));
			msg += info;
			msg += L'\n';

			const float f_mp(static_cast<float>(m_OriginalPictureWidth) * m_OriginalPictureHeight / 1000 / 1000);
			CString mp;
			mp.Format(L"%.1f", (f_mp < 0.1) ? 0.1 : f_mp);

			info.FormatMessage(info_template[3], m_OriginalPictureWidth, m_OriginalPictureHeight, mp);
			msg += info;

			const __int64 file_size(::GetFileSize64(FileName));
			CString size_str;
			size_str.Format(L"%I64d", file_size);
			info.FormatMessage(info_template[1],
				GetSizeStr(file_size),
				size_str,
				::GetLongFileDateTime(FileName, filetime_type::date_created),
				::GetLongFileDateTime(FileName, filetime_type::date_modified));

			msg += L'\n';
			msg += info;
		}

		return msg;
	}

	return L"";
}
