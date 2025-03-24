#include "stdafx.h"
#include "resource.h"
#include "pluginformat.h"
#include "libpng/png.h"
#include "libtiff/tiffio.h"
#include <sys/stat.h>

#include <vector>
using namespace std;

enum filetime_type
{
	date_modified,	//FILETIME ftLastWriteTime;
	date_created,	//FILETIME ftCreationTime;
};


CString GetSizeStr(const __int64 size)
{
	CString s;
	__int64 d(1024);
	__int64 z(1024);

	if (size < d)
	{
		s.Format(L"%d Byte", static_cast<int>(size));
		return s;
	}

	d <<= 10;
	if (size < d)
	{
		s.Format(L"%d KB", static_cast<int>(size / z));
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

	if(hFind != INVALID_HANDLE_VALUE) 
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

		GetDateFormat(
			LOCALE_USER_DEFAULT,	// locale
			DATE_LONGDATE,			// options
			&sysTime,				// date
			NULL,                   // date format
			DateStr,				// formatted string buffer
			sizeof(DateStr) / sizeof(WCHAR) // size of buffer
		);

		GetTimeFormat(
			LOCALE_USER_DEFAULT,    // locale
			0,						// options
			&sysTime,				// date
			NULL,                   // date format
			TimeStr,				// formatted string buffer
			sizeof(TimeStr) / sizeof(WCHAR)      // size of buffer
		);

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


// Note: Review all sections with "// ***" when implementing a new format
//		 Copy always PictureFormat.h from the plugin package to your project.
//       The BMP format (.bmp) is already internally registered with cPicture as write only (24bit color depth).
//       The Jpeg format (.jpg;.jpeg) is the standard format of cPicture and therefore also internally registered for reading and writing.


enum PICTURE_FORMAT
{
	TIFF_FORMAT = 0,
	PNG_FORMAT,

	FORMAT_SIZE,
};

static lpfnFormatGetInstanceProc PluginProcArray[FORMAT_SIZE];

const int __stdcall GetPluginInit()
{
	PluginProcArray[TIFF_FORMAT] = CTiffFormat::GetInstance;
	PluginProcArray[PNG_FORMAT] = CPngFormat::GetInstance;

	return FORMAT_SIZE;
}

const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
	return L"4.6";
}

const PLUGIN_TYPE __stdcall GetPluginType()
{
	return PLUGIN_TYPE_FORMAT;
}


lpfnFormatGetInstanceProc __stdcall GetPluginProc(const int k)
{
	if(k >= 0 && k < FORMAT_SIZE)
		return PluginProcArray[k];
	else
		return NULL;
}


CPluginFormat::CPluginFormat()
{
}

CStringA CPluginFormat::get_utf8_file_name(const CString& FileName)
{
	CStringA pictureNameUtf8;

	// Determine the required buffer size for the UTF-8 string.
	const int bufferSize = WideCharToMultiByte(CP_UTF8, 0, FileName, -1, NULL, 0, NULL, NULL);
	if (bufferSize)
	{
		// Allocate the buffer for the UTF-8 string.
		unsigned char* utf8Buffer = new unsigned char[bufferSize];
		memset(utf8Buffer, 0, bufferSize);

		// Convert the CString to a UTF-8 encoded buffer.
		const int utf8Length = WideCharToMultiByte(CP_UTF8, 0, FileName, -1, reinterpret_cast<char*>(utf8Buffer), bufferSize, NULL, NULL);
		if (utf8Length)
		{
			pictureNameUtf8 = utf8Buffer;
		}

		delete[] utf8Buffer;
	}

	return pictureNameUtf8;
}

unsigned int __stdcall CPluginFormat::get_cap() const
{
	// *** Capabilities of the plugin.
	return PICTURE_READ |		// This plugin can read the picture format.
		PICTURE_WRITE |			// This plugin can write the picture format.
								// At least one must be specified. Otherwise 
								// the plugin will not be loaded.
								// See PictureFormat.h for the complete list.
								// 
								// This plugin supports several transformations.
								// If not implemented in this plugin, a default implementation will be used if PICTURE_WRITE is specified.
		PICTURE_ROTATELEFT |
		PICTURE_ROTATERIGHT |
		PICTURE_FLIPH |
		PICTURE_FLIPV |
		PICTURE_TRANSPOSE |
		PICTURE_TRANSVERSE |
		PICTURE_GREYSCALE | 
		PICTURE_CROP;
}

vector<CString> info_template;

void __stdcall SetPluginInfoTemplates(const vector<CString>& _info_template)
{
	info_template = _info_template;

	// List of localized templates:
	//info_template[0] = Picture name:\t%1
	//info_template[1] = File size:\t%1 KB (%2 Bytes)\nCreated on:\t%s\nChanged on:\t%4
	//info_template[2] = Created on:\t%s
	//info_template[3] = Picture size:\t%1!d!x%2!d! pixel (%3!s!MP)
	//info_template[4] = Model:\t\t%1!s!
	//info_template[5] = Error:\t\t%1!s!
	//info_template[6] = Contains:\t%1!s!
	//info_template[7] = Picture folder:\t%1
	//info_template[8] = Settings:\t
}


CString __stdcall CPluginFormat::get_info(const CString& FileName, const enum info_type _info_type)
{
	// *** This function gets the picture data and should be as efficient as possible.
	// For example: do not load and decompress the image just to get the picture dimensions

	if (_info_type & (info_type_std | info_type_size | info_type_short))
	{
		get_size(FileName);
	}

	if (_info_type & info_type_short)
	{
		// List of localized templates:
		//info_template[0] = Bildname:\t%1
		//info_template[1] = Dateigröße:\t%1 (%2 Bytes)\nGeändert am:\t%3\nDatei geändert am:\t%4
		//info_template[2] = Erstellt am:\t%1
		//info_template[3] = Bildgröße:\t%1!d!x%2!d! Bildpunkte (%3!s! MP)
		//info_template[4] = Model:\t\t%1!s!
		//info_template[5] = Fehler:\t\t%1!s!
		//info_template[6] = Enthält:\t%1!s!
		//info_template[7] = Bildordner:\t%1
		//info_template[8] = Einstellungen:\t

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

			// Bildordner
			info.FormatMessage(info_template[7], FileName.Left(FileName.ReverseFind(L'\\') + 1));
			msg += info;
			msg += L'\n';

			//Floating-point printf format specifiers — e, E, f, and g — are not supported. 
			//The workaround is to use the sprintf function to format the floating-point number 
			//into a temporary buffer, then use that buffer as the insert string. 
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


//------------------------------------------------------------


CString __stdcall CTiffFormat::get_ext() const
{
	// *** File extensions associated with the new format.
	// Multiple extensions are separated by ';' ("tif;tiff")
	return L"tif;tiff";
}

struct plugin_data __stdcall CTiffFormat::get_plugin_data() const
{
	struct plugin_data pluginData;

	// *** Short description of the new format,
	// will be used for example in the form: 
	// "The selected folder "c:\..." does not contain any pictures in [format1, format2 or format3] format."
	// Example for Tiff: "The selected folder "c:\..." does not contain any pictures in Jpeg or TIFF format."
	pluginData.name.LoadString(IDS_SHORT_DESC_TIFF);

	// *** Long description of the new format,
	// will be used as an explanation in the options
	pluginData.desc.LoadString(IDS_LONG_DESC_TIFF);

	return pluginData;
}

const CString CTiffFormat::type = "TIFF";

// *** cPicture maintains a property string for this Plugin
//     which can be used for your picture format settings.
//     For example use a string format to create a string in get_properties()
//     and to get the values back in set_properties(...)

CString CTiffFormat::m_property_str;


bool __stdcall CTiffFormat::properties_dlg(const HWND hwnd)
{
	CString msg;
	msg.LoadString(IDS_PROPERTY_DLG_TIFF_TEXT);
	::MessageBox(hwnd, msg, get_plugin_data().desc, MB_ICONINFORMATION);

	return false;	// false: no reload of the current picture
}

bool __stdcall CTiffFormat::RGBToFile(const CString& FileName,
	const BYTE* dataBuf,
	const int width,
	const int height,
	const int quality_L,
	const int quality_C,
	const int jpeg_lossless)
{
	// *** This function saves the RGB array to a file.

	// If the picture format supports quality levels, add PICTURE_QUALITY to get_cap()
	// Otherwise ignore the parameter "quality", which is from 0 to 100

	TIFF* tif = TIFFOpenW(FileName, "w");
	if (!tif)
	{
		return false;
	}

	TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
	TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
	TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
	TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

	//uint32_t rowsPerStrip = TIFFDefaultStripSize(tif, width * 3);
	//TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rowsPerStrip);

	BYTE* rgbArray = const_cast<BYTE*>(dataBuf);

	//for (uint32_t row = 0; row < height; row += rowsPerStrip) {
	//	uint32_t numRows = (row + rowsPerStrip > height) ? height - row : rowsPerStrip;
	//	if (TIFFWriteEncodedStrip(tif, row / rowsPerStrip, rgbArray + row * width * 3, numRows * width * 3) < 0) {
	//		//std::cerr << "Could not write encoded strip!" << std::endl;
	//		TIFFClose(tif);
	//		return false;
	//	}
	//}

	const uint32_t index = 3 * width;

	for (int row = 0; row < height; ++row)
	{
		if (TIFFWriteScanline(tif, rgbArray, row, 0) < 0)
		{
			TIFFClose(tif);
			return false;
		}

		rgbArray += index;
	}

	TIFFClose(tif);

	return true;
}

BYTE* __stdcall CTiffFormat::FileToRGB(const CString& FileName,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n,
	const enum scaling_type picture_scaling_type,
	const bool b_scan)
{
	TIFF* tif = TIFFOpenW(FileName, "r");
	if (!tif)
	{
		return NULL;
	}

	uint32_t width, height;
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

	m_OriginalPictureWidth = m_PictureWidth = width;
	m_OriginalPictureHeight = m_PictureHeight = height;

	uint16_t samplesPerPixel, bitsPerSample, photometric, compression;
	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
	TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
	TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric);
	TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);

	const __int64 size = static_cast<__int64>(3) * width * height; // 3 bytes per pixel (RGB)
	BYTE* image = static_cast<BYTE*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
	tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));

	if (compression == COMPRESSION_CCITTFAX3 || compression == COMPRESSION_CCITTFAX4)
	{
		// Fax images
		for (uint32_t row = 0; row < height; ++row)
		{
			if (TIFFReadScanline(tif, buf, row) == 1)
			{
				uint8_t* data = static_cast<uint8_t*>(buf);
				for (uint32_t col = 0; col < width; ++col)
				{
					__int64 index = static_cast<__int64>(3) * (row * width + col);
					uint8_t value = (data[col / 8] & (1 << (7 - (col % 8)))) ? 255 : 0;
					if (photometric == PHOTOMETRIC_MINISWHITE)
					{
						value = ~value; // Invert if white is zero
					}
					image[index] = value;       // Red
					image[index + 1] = value;   // Green
					image[index + 2] = value;   // Blue
				}
			}
		}
	}
	else if (bitsPerSample == 1)
	{
		// 1-bit (Bilevel)
		uint16_t* redColormap;
		uint16_t* greenColormap;
		uint16_t* blueColormap;
		if (TIFFGetField(tif, TIFFTAG_COLORMAP, &redColormap, &greenColormap, &blueColormap))
		{
			for (uint32_t row = 0; row < height; ++row)
			{
				if (TIFFReadScanline(tif, buf, row) == 1)
				{
					uint8_t* data = static_cast<uint8_t*>(buf);
					for (uint32_t col = 0; col < width; ++col)
					{
						__int64 index = static_cast<__int64>(3) * (row * width + col);
						uint8_t value = (data[col / 8] & (1 << (7 - (col % 8)))) ? 1 : 0;
						image[index] = redColormap[value] >> 8;       // Red
						image[index + 1] = greenColormap[value] >> 8; // Green
						image[index + 2] = blueColormap[value] >> 8;  // Blue
					}
				}
			}
		}
	}
	else if (bitsPerSample == 4)
	{
		// 4-bit (Palette-based)
		uint16_t* redColormap;
		uint16_t* greenColormap;
		uint16_t* blueColormap;
		if (TIFFGetField(tif, TIFFTAG_COLORMAP, &redColormap, &greenColormap, &blueColormap))
		{
			for (uint32_t row = 0; row < height; ++row)
			{
				if (TIFFReadScanline(tif, buf, row) == 1)
				{
					uint8_t* data = static_cast<uint8_t*>(buf);
					for (uint32_t col = 0; col < width; ++col)
					{
						__int64 index = static_cast<__int64>(3) * (row * width + col);
						uint8_t value = (col % 2 == 0) ? (data[col / 2] >> 4) : (data[col / 2] & 0x0F);
						image[index] = redColormap[value] >> 8;       // Red
						image[index + 1] = greenColormap[value] >> 8; // Green
						image[index + 2] = blueColormap[value] >> 8;  // Blue
					}
				}
			}
		}
	}
	else if (samplesPerPixel == 1 && photometric == PHOTOMETRIC_PALETTE)
	{
		// 8-bit (Palette-based)
		uint16_t* redColormap;
		uint16_t* greenColormap;
		uint16_t* blueColormap;
		if (TIFFGetField(tif, TIFFTAG_COLORMAP, &redColormap, &greenColormap, &blueColormap))
		{
			for (uint32_t row = 0; row < height; ++row)
			{
				if (TIFFReadScanline(tif, buf, row) == 1)
				{
					uint8_t* data = static_cast<uint8_t*>(buf);
					for (uint32_t col = 0; col < width; ++col)
					{
						__int64 index = static_cast<__int64>(3) * (row * width + col);
						uint8_t value = data[col];
						image[index] = redColormap[value] >> 8;       // Red
						image[index + 1] = greenColormap[value] >> 8; // Green
						image[index + 2] = blueColormap[value] >> 8;  // Blue
					}
				}
			}
		}
	}
	else if (samplesPerPixel == 1)
	{
		// Greyscale
		for (uint32_t row = 0; row < height; ++row)
		{
			if (TIFFReadScanline(tif, buf, row) == 1)
			{
				uint8_t* data = static_cast<uint8_t*>(buf);
				for (uint32_t col = 0; col < width; ++col)
				{
					__int64 index = static_cast<__int64>(3) * (row * width + col);
					uint8_t value = data[col];
					if (photometric == PHOTOMETRIC_MINISWHITE)
					{
						value = ~value; // Invert if white is zero
					}
					image[index] = value;       // Red
					image[index + 1] = value;   // Green
					image[index + 2] = value;   // Blue
				}
			}
		}
	}
	else if (samplesPerPixel == 3)
	{ 
		// RGB
		for (uint32_t row = 0; row < height; ++row)
		{
			if (TIFFReadScanline(tif, buf, row) == 1)
			{
				uint8_t* data = static_cast<uint8_t*>(buf);
				for (uint32_t col = 0; col < width; ++col)
				{
					__int64 index = static_cast<__int64>(3) * (row * width + col);
					image[index] = data[col * 3];       // Red
					image[index + 1] = data[col * 3 + 1]; // Green
					image[index + 2] = data[col * 3 + 2]; // Blue
				}
			}
		}
	}
	else if (samplesPerPixel == 4)
	{
		// RGBA
		for (uint32_t row = 0; row < height; ++row)
		{
			if (TIFFReadScanline(tif, buf, row) == 1)
			{
				uint8_t* data = static_cast<uint8_t*>(buf);
				for (uint32_t col = 0; col < width; ++col)
				{
					__int64 index = static_cast<__int64>(3) * (row * width + col);
					image[index] = data[col * 4];       // Red
					image[index + 1] = data[col * 4 + 1]; // Green
					image[index + 2] = data[col * 4 + 2]; // Blue
				}
			}
		}
	}

	_TIFFfree(buf);
	TIFFClose(tif);

	return image;
}

void CTiffFormat::get_size(const CString& FileName)
{
	m_bIsValid = false;

	TIFF* tif = TIFFOpenW(FileName, "r");
	if (tif)
	{
		uint32_t width, height;
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

		m_bIsValid = width && height;

		if (m_bIsValid)
		{
			m_OriginalPictureWidth = m_PictureWidth = width;
			m_OriginalPictureHeight = m_PictureHeight = height;

			//typedef enum {
			//	JCS_UNKNOWN,	/* error/unspecified */
			//	JCS_GRAYSCALE,	/* monochrome */
			//	JCS_RGB,		/* red/green/blue, standard RGB (sRGB) */
			//	JCS_YCbCr,		/* Y/Cb/Cr (also known as YUV), standard YCC */
			//	JCS_CMYK,		/* C/M/Y/K */
			//	JCS_YCCK,		/* Y/Cb/Cr/K */
			//	JCS_BG_RGB,		/* big gamut red/green/blue, bg-sRGB */
			//	JCS_BG_YCC		/* big gamut Y/Cb/Cr, bg-sYCC */
			//} J_COLOR_SPACE;
									
			//The TIFFTAG_IT8COLORSEQUENCE tag in the TIFF image format 
			//is of type TIFF_ASCII.This means it stores its value as a
			//string.The values for this tag are typically sequences of 
			//characters that represent the order of color components,
			//such as:
			//"RGB": Red, Green, Blue
			//"CMYK" : Cyan, Magenta, Yellow, Black
			//"YCbCr" : Luminance, Blue - difference chroma, Red - difference chroma
			//"CMY" : Cyan, Magenta, Yellow
			//"YCC" : Luminance, Chrominance, Chrominance

			//m_color_space = 0;
			//char* color_space_text = nullptr;
			//if (TIFFGetField(tif, TIFFTAG_IT8COLORSEQUENCE, &color_space_text)) 
			//{
			//	if (color_space_text != nullptr) 
			//	{
			//		if (strcmp(color_space_text, "RGB") == 0)
			//			m_color_space = 2;
			//		else
			//		if (strcmp(color_space_text, "CMYK") == 0)
			//			m_color_space = 4;
			//		else
			//		if (strcmp(color_space_text, "YCbCr") == 0)
			//			m_color_space = 3;
			//		else
			//		if (strcmp(color_space_text, "YCC") == 0)
			//			m_color_space = 4;
			//	}
			//}
		}

		TIFFClose(tif);
	}
}


//------------------------------------------------------------


const CString CPngFormat::type = "PNG";

CString CPngFormat::m_property_str;

CString __stdcall CPngFormat::get_ext() const
{
	return L"png";
}

struct plugin_data __stdcall CPngFormat::get_plugin_data() const
{
	struct plugin_data pluginData;

	// *** Short description of the new format,
	// will be used for example in the form: 
	// "The selected folder "c:\..." does not contain any pictures in [format1, format2 or format3] format."
	// Example for Png: "The selected folder "c:\..." does not contain any pictures in Jpeg or Png format."
	pluginData.name.LoadString(IDS_SHORT_DESC_PNG);

	// *** Long description of the new format,
	// will be used as an explanation in the options
	pluginData.desc.LoadString(IDS_LONG_DESC_PNG);

	return pluginData;
}

bool __stdcall CPngFormat::RGBToFile(const CString& FileName,
	const BYTE* dataBuf,
	const int width,
	const int height,
	const int quality_L,
	const int quality_C,
	const int jpeg_lossless)
{
	// *** This function saves the RGB array to a file.

	// If the picture format supports quality levels, add PICTURE_QUALITY to get_cap()
	// Otherwise ignore the parameter "quality", which is from 0 to 100

	FILE* fp = NULL;
	const errno_t err(_wfopen_s(&fp, FileName, L"wb"));
	if (err != 0)
	{
		m_ErrorMsg.Format(L"Error open file '%s' for writing", FileName);
		return false;
	}

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png)
	{
		m_ErrorMsg = L"Error in png_create_write_struct";
		fclose(fp);

		return false;
	}

	png_infop info = png_create_info_struct(png);
	if (!info)
	{
		m_ErrorMsg = L"Error in png_create_info_struct";
		png_destroy_write_struct(&png, nullptr);
		fclose(fp);

		return false;
	}

	if (setjmp(png_jmpbuf(png)))
	{
		m_ErrorMsg = L"Error in PNG RGBToFile";
		png_destroy_write_struct(&png, &info);
		fclose(fp);

		return false;
	}

	png_init_io(png, fp);

	png_set_IHDR(
		png,
		info,
		width, height,
		8, // bit depth
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);

	// Add metadata
	CStringA filename = get_utf8_file_name(FileName.Mid(FileName.ReverseFind('\\') + 1));

	png_text text[2];
	text[0].key = "Title";
	text[0].text = filename.GetBuffer();
	text[0].compression = PNG_TEXT_COMPRESSION_NONE;
	text[1].key = "Author";
	text[1].text = "cPicture4 png plugin";
	text[1].compression = PNG_TEXT_COMPRESSION_NONE;
	png_set_text(png, info, text, sizeof(text) / sizeof(png_text));
	
	filename.ReleaseBuffer();

	png_write_info(png, info);

	png_bytep* row_pointers = new png_bytep[height];
	png_bytep* row_pointers2 = row_pointers;
	BYTE* dataBufRow = const_cast<BYTE*>(dataBuf);
	register __int64 index = 0;
	register __int64 row_stride = 3 * width;
	for (unsigned int y = height; y != 0U; --y)
	{
		*row_pointers2 = dataBufRow + index;
		++row_pointers2;
		index += row_stride;
	}

	png_write_image(png, row_pointers);
	png_write_end(png, nullptr);

	png_destroy_write_struct(&png, &info);
	fclose(fp);

	delete[] row_pointers;

	return true;
}

BYTE* __stdcall CPngFormat::FileToRGB(const CString& FileName,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n,
	const enum scaling_type picture_scaling_type,
	const bool b_scan)
{
	png_image image; /* The control structure used by libpng */

	/* Initialize the 'png_image' structure. */
	memset(&image, 0, (sizeof image));
	image.version = PNG_IMAGE_VERSION;

	png_bytep buffer = NULL;

	/* The first argument is the file to read: */
	if (png_image_begin_read_from_file(&image, FileName) == 0)
	{
		m_ErrorMsg = L"Error in png_image_begin_read_from_file";
	}
	else
	{
		/* Set the format in which to read the PNG file.
			*/
		image.format = PNG_FORMAT_RGB;

		/* Now allocate enough memory to hold the image in this format; the
			* PNG_IMAGE_SIZE macro uses the information about the image (width,
			* height and format) stored in 'image'.
			*/
		const __int64 size = PNG_IMAGE_SIZE(image);
		buffer = static_cast<BYTE*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
		
		if (buffer)
		{
			// PNG_FORMAT_RGB, Hintergrund auf weiß setzen
			memset(buffer, 255, size);

			/* If enough memory was available, read the image in the desired
				* format, then write the result out to the new file.  'background' is
				* not necessary when reading the image, because the alpha channel is
				* preserved; if it were to be removed, for example if we requested
				* PNG_FORMAT_RGB, then either a solid background color would have to
				* be supplied, or the output buffer would have to be initialized to
				* the actual background of the image.
				*
				* The fourth argument to png_image_finish_read is the 'row_stride' -
				* this is the number of components allocated for the image in each
				* row.  It has to be at least as big as the value returned by
				* PNG_IMAGE_ROW_STRIDE, but if you just allocate space for the
				* default, minimum size, using PNG_IMAGE_SIZE as above, you can pass
				* zero.
				*
				* The final argument is a pointer to a buffer for the colormap;
				* colormaps have exactly the same format as a row of image pixels
				* (so you choose what format to make the colormap by setting
				* image.format).  A colormap is only returned if
				* PNG_FORMAT_FLAG_COLORMAP is also set in image.format, so in this
				* case NULL is passed as the final argument.  If you do want to force
				* all images into an index/color-mapped format, then you can use:
				*
				*    PNG_IMAGE_COLORMAP_SIZE(image)
				*
				* to find the maximum size of the colormap in bytes.
				*/
			if (png_image_finish_read(&image, NULL/*background*/, buffer,
				0/*row_stride*/, NULL/*colormap*/) != 0)
			{
				m_OriginalPictureWidth = m_PictureWidth = image.width;
				m_OriginalPictureHeight = m_PictureHeight = image.height;
			}
			else
			{
				/* Calling png_image_free is optional unless the simplified API was
					* not run to completion.  In this case, if there wasn't enough
					* memory for 'buffer', we didn't complete the read, so we must
					* free the image:
					*/
				if (buffer == NULL)
					png_image_free(&image);
				else
					VirtualFree(buffer, 0, MEM_RELEASE);

				m_ErrorMsg = L"PNG png_image_finish_read failed";
				buffer = NULL;
			}
		}
		else
		{
			m_ErrorMsg.Format(L"PNG memory request failed: %I64d bytes", size);
		}
	}

	return buffer;
}

void __stdcall CPngFormat::get_size(const CString& FileName)
{
	m_bIsValid = false;
	m_OriginalPictureWidth = m_OriginalPictureHeight = 0;

	FILE* fp = NULL;
	const errno_t err(_wfopen_s(&fp, FileName, L"rb"));
	if (err == 0)
	{
		png_byte header[8];
		fread(header, 1, 8, fp);
		if (png_sig_cmp(header, 0, 8))
		{
			fclose(fp);

			return;
		}

		png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png)
		{
			fclose(fp);

			return;
		}

		png_infop info = png_create_info_struct(png);
		if (!info)
		{
			png_destroy_read_struct(&png, nullptr, nullptr);
			fclose(fp);

			return;
		}

		if (setjmp(png_jmpbuf(png)))
		{
			png_destroy_read_struct(&png, &info, nullptr);
			fclose(fp);

			return;
		}

		png_init_io(png, fp);
		png_set_sig_bytes(png, 8);
		png_read_info(png, info);

		m_OriginalPictureWidth = png_get_image_width(png, info);
		m_OriginalPictureHeight = png_get_image_height(png, info);

		m_bIsValid = true;

		//typedef enum {
		//	JCS_UNKNOWN,	/* error/unspecified */
		//	JCS_GRAYSCALE,	/* monochrome */
		//	JCS_RGB,		/* red/green/blue, standard RGB (sRGB) */
		//	JCS_YCbCr,		/* Y/Cb/Cr (also known as YUV), standard YCC */
		//	JCS_CMYK,		/* C/M/Y/K */
		//	JCS_YCCK,		/* Y/Cb/Cr/K */
		//	JCS_BG_RGB,		/* big gamut red/green/blue, bg-sRGB */
		//	JCS_BG_YCC		/* big gamut Y/Cb/Cr, bg-sYCC */
		//} J_COLOR_SPACE;

		//PNG_COLOR_TYPE_GRAY
		//PNG_COLOR_TYPE_PALETTE
		//PNG_COLOR_TYPE_RGB
		//PNG_COLOR_TYPE_RGB_ALPHA
		//PNG_COLOR_TYPE_GRAY_ALPHA

		const int color_type = png_get_color_type(png, info);
		if (color_type == PNG_COLOR_TYPE_GRAY)
			m_color_space = 1;
		else
		if (color_type == PNG_COLOR_TYPE_RGB)
			m_color_space = 2;
		else
			m_color_space = 0;

		png_destroy_read_struct(&png, &info, nullptr);
		fclose(fp);
	}
}

bool __stdcall CPngFormat::properties_dlg(const HWND hwnd)
{
	CString msg;
	msg.LoadString(IDS_PROPERTY_DLG_PNG_TEXT);
	::MessageBox(hwnd, msg, get_plugin_data().desc, MB_ICONINFORMATION);

	return false;	// false: no reload of the current picture
}
