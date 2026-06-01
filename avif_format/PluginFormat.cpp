#include "stdafx.h"
#include "resource.h"
#include "pluginformat.h"
#include "avif/avif.h"

#include <algorithm>
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
	return k == 0 ? CAvifFormat::GetInstance : NULL;
}

const CString CAvifFormat::type = L"AVIF";

CStringA CAvifFormat::get_utf8_file_name(const CString& FileName) const
{
	CStringA pictureNameUtf8;
	const int bufferSize = WideCharToMultiByte(CP_UTF8, 0, FileName, -1, NULL, 0, NULL, NULL);
	if (bufferSize)
	{
		vector<char> utf8Buffer(bufferSize, 0);
		const int utf8Length = WideCharToMultiByte(CP_UTF8, 0, FileName, -1, utf8Buffer.data(), bufferSize, NULL, NULL);
		if (utf8Length)
		{
			pictureNameUtf8 = utf8Buffer.data();
		}
	}

	return pictureNameUtf8;
}

CString __stdcall CAvifFormat::get_ext() const
{
	return L"avif;avifs";
}

struct plugin_data __stdcall CAvifFormat::get_plugin_data() const
{
	struct plugin_data pluginData;
	pluginData.name.LoadString(IDS_SHORT_DESC);
	pluginData.desc.LoadString(IDS_LONG_DESC);
	return pluginData;
}

unsigned int __stdcall CAvifFormat::get_cap() const
{
	return PICTURE_READ | PICTURE_WRITE | PICTURE_QUALITY;
}

bool __stdcall CAvifFormat::properties_dlg(const HWND hwnd)
{
	CString msg;
	msg.LoadString(IDS_PROPERTY_DLG_TEXT);
	::MessageBox(hwnd, msg, get_plugin_data().desc, MB_ICONINFORMATION);
	return false;
}

BYTE* __stdcall CAvifFormat::FileToRGB(const CString& FileName,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n,
	const enum scaling_type picture_scaling_type,
	const bool b_scan)
{
	avifImage* image = avifImageCreateEmpty();
	if (!image)
	{
		m_ErrorMsg = L"AVIF image allocation failed";
		return NULL;
	}

	avifDecoder* decoder = avifDecoderCreate();
	if (!decoder)
	{
		avifImageDestroy(image);
		m_ErrorMsg = L"AVIF decoder allocation failed";
		return NULL;
	}

	decoder->maxThreads = 0;
	decoder->ignoreExif = AVIF_TRUE;
	decoder->ignoreXMP = AVIF_TRUE;

	const avifResult result = avifDecoderReadFile(decoder, image, get_utf8_file_name(FileName));
	if (result != AVIF_RESULT_OK)
	{
		m_ErrorMsg.Format(L"AVIF decode failed: %S", avifResultToString(result));
		avifDecoderDestroy(decoder);
		avifImageDestroy(image);
		return NULL;
	}

	avifRGBImage rgb;
	avifRGBImageSetDefaults(&rgb, image);
	rgb.format = AVIF_RGB_FORMAT_RGB;
	rgb.depth = 8;
	rgb.rowBytes = image->width * 3;

	const __int64 size = static_cast<__int64>(rgb.rowBytes) * image->height;
	BYTE* buffer = static_cast<BYTE*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
	if (!buffer)
	{
		m_ErrorMsg.Format(L"AVIF memory request failed: %I64d bytes", size);
		avifDecoderDestroy(decoder);
		avifImageDestroy(image);
		return NULL;
	}

	rgb.pixels = buffer;
	const avifResult rgbResult = avifImageYUVToRGB(image, &rgb);
	if (rgbResult != AVIF_RESULT_OK)
	{
		m_ErrorMsg.Format(L"AVIF RGB conversion failed: %S", avifResultToString(rgbResult));
		VirtualFree(buffer, 0, MEM_RELEASE);
		buffer = NULL;
	}
	else
	{
		m_OriginalPictureWidth = m_PictureWidth = static_cast<int>(image->width);
		m_OriginalPictureHeight = m_PictureHeight = static_cast<int>(image->height);
		m_color_space = 2;
		m_bIsValid = true;
	}

	avifDecoderDestroy(decoder);
	avifImageDestroy(image);
	return buffer;
}

bool __stdcall CAvifFormat::RGBToFile(const CString& FileName,
	const BYTE* dataBuf,
	const int width,
	const int height,
	const int quality_L,
	const int quality_C,
	const int jpeg_lossless)
{
	if (!dataBuf || width <= 0 || height <= 0)
	{
		m_ErrorMsg = L"Invalid AVIF input buffer";
		return false;
	}

	avifImage* image = avifImageCreate(width, height, 8, AVIF_PIXEL_FORMAT_YUV420);
	if (!image)
	{
		m_ErrorMsg = L"AVIF image allocation failed";
		return false;
	}

	image->colorPrimaries = AVIF_COLOR_PRIMARIES_SRGB;
	image->transferCharacteristics = AVIF_TRANSFER_CHARACTERISTICS_SRGB;
	image->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_BT709;
	image->yuvRange = AVIF_RANGE_FULL;

	avifRGBImage rgb;
	avifRGBImageSetDefaults(&rgb, image);
	rgb.format = AVIF_RGB_FORMAT_RGB;
	rgb.depth = 8;
	rgb.pixels = const_cast<BYTE*>(dataBuf);
	rgb.rowBytes = width * 3;

	avifResult result = avifImageRGBToYUV(image, &rgb);
	if (result != AVIF_RESULT_OK)
	{
		m_ErrorMsg.Format(L"AVIF YUV conversion failed: %S", avifResultToString(result));
		avifImageDestroy(image);
		return false;
	}

	avifEncoder* encoder = avifEncoderCreate();
	if (!encoder)
	{
		m_ErrorMsg = L"AVIF encoder allocation failed";
		avifImageDestroy(image);
		return false;
	}

	const int quality = quality_L >= 0 ? min(max(quality_L, 0), 100) : 80;
	encoder->quality = quality;
	encoder->qualityAlpha = quality;
	encoder->speed = AVIF_SPEED_DEFAULT;
	encoder->maxThreads = 0;

	avifRWData output = AVIF_DATA_EMPTY;
	result = avifEncoderWrite(encoder, image, &output);
	if (result != AVIF_RESULT_OK)
	{
		m_ErrorMsg.Format(L"AVIF encode failed: %S", avifResultToString(result));
		avifEncoderDestroy(encoder);
		avifImageDestroy(image);
		return false;
	}

	FILE* fp = NULL;
	const errno_t err(_wfopen_s(&fp, FileName, L"wb"));
	if (err != 0 || !fp)
	{
		m_ErrorMsg.Format(L"Error open file '%s' for writing", FileName);
		avifRWDataFree(&output);
		avifEncoderDestroy(encoder);
		avifImageDestroy(image);
		return false;
	}

	const bool ok = fwrite(output.data, 1, output.size, fp) == output.size;
	fclose(fp);

	if (!ok)
	{
		m_ErrorMsg.Format(L"Error writing AVIF file '%s'", FileName);
	}

	avifRWDataFree(&output);
	avifEncoderDestroy(encoder);
	avifImageDestroy(image);
	return ok;
}

void CAvifFormat::get_size(const CString& FileName)
{
	m_bIsValid = false;
	m_OriginalPictureWidth = m_OriginalPictureHeight = 0;
	m_PictureWidth = m_PictureHeight = 0;

	avifDecoder* decoder = avifDecoderCreate();
	if (!decoder)
	{
		return;
	}

	decoder->ignoreExif = AVIF_TRUE;
	decoder->ignoreXMP = AVIF_TRUE;
	avifResult result = avifDecoderSetIOFile(decoder, get_utf8_file_name(FileName));
	if (result == AVIF_RESULT_OK)
	{
		result = avifDecoderParse(decoder);
	}

	if (result == AVIF_RESULT_OK && decoder->image)
	{
		m_OriginalPictureWidth = m_PictureWidth = static_cast<int>(decoder->image->width);
		m_OriginalPictureHeight = m_PictureHeight = static_cast<int>(decoder->image->height);
		m_color_space = decoder->image->yuvFormat == AVIF_PIXEL_FORMAT_YUV400 ? 1 : 2;
		m_bIsValid = m_OriginalPictureWidth > 0 && m_OriginalPictureHeight > 0;
	}

	avifDecoderDestroy(decoder);
}

vector<CString> info_template;

void __stdcall SetPluginInfoTemplates(const vector<CString>& _info_template)
{
	info_template = _info_template;
}

CString __stdcall CAvifFormat::get_info(const CString& FileName, const enum info_type _info_type)
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
