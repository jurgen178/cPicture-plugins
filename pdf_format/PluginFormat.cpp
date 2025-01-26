#include "stdafx.h"
#include "resource.h"
#include "pluginformat.h"
//#include "PdfPropertiesDlg.h"
#include <sys/stat.h>
#include "include/fpdfview.h"
#include "include/fpdf_formfill.h"

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

__int64 GetFileSize64(const TCHAR* pFile)
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

CString GetLongFileDateTime(const TCHAR* pFile, FILETIME& filetime, filetime_type type)
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

		TCHAR DateStr[100] = { 0 };
		TCHAR TimeStr[100] = { 0 };

		GetDateFormat(
			LOCALE_USER_DEFAULT,	// locale
			DATE_LONGDATE,			// options
			&sysTime,				// date
			NULL,                   // date format
			DateStr,				// formatted string buffer
			sizeof(DateStr) / sizeof(TCHAR) // size of buffer
		);

		GetTimeFormat(
			LOCALE_USER_DEFAULT,    // locale
			0,						// options
			&sysTime,				// date
			NULL,                   // date format
			TimeStr,				// formatted string buffer
			sizeof(TimeStr) / sizeof(TCHAR)      // size of buffer
		);

		DateTimeFormat.Format(IDS_DATE_TIME_FORMAT_STRING, DateStr, TimeStr);
	}

	return DateTimeFormat;
}

CString GetLongFileDateTime(const TCHAR* pFile, filetime_type type)
{
	FILETIME filetime = { 0 };

	return GetLongFileDateTime(pFile, filetime, type);
}


enum PLUGIN_TYPE
{
	PLUGIN_TYPE_NONE = 0x0000,
	PLUGIN_TYPE_FORMAT = 0x0001,
	PLUGIN_TYPE_FUNCTION = 0x0002,
};

struct PluginData
{
public:
	PluginData() : type(PLUGIN_TYPE::PLUGIN_TYPE_NONE) { };

	CString name;
	CString desc;
	CString file_name;
	CString file_name2;
	CString info;
	CString version;
	enum PLUGIN_TYPE type;

	// Zum Sortieren der Einträge.
	bool operator < (PluginData& rhs)
	{
		return file_name2 < rhs.file_name2;
	}
};

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


const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
	return L"4.4";
}

const PLUGIN_TYPE __stdcall GetPluginType()
{
	return PLUGIN_TYPE_FORMAT;
}

const int __stdcall GetPluginInit()
{
	// Implement one format plugin.
	return 1;
}

lpfnFormatGetInstanceProc __stdcall GetPluginProc(const int k)
{
	// Plugin-Fabric: return the one format plugin.
	return CPdfFormat::GetInstance;
}


// To support multiple formats, setup a format array in GetPluginInit()
// and return the instance from the format array in GetPluginProc:

//enum PICTURE_FORMAT
//{
//	ABC_FORMAT = 0,
//	DEF_FORMAT,
//
//	FORMAT_SIZE,
//};
//
//static lpfnFormatGetInstanceProc PluginProcArray[FORMAT_SIZE];
//
//const int __stdcall GetPluginInit()
//{
//	PluginProcArray[ABC_FORMAT] = CAbcFormat::GetInstance;
//	PluginProcArray[DEF_FORMAT] = CDefFormat::GetInstance;
//
//	return FORMAT_SIZE;
//}
//
//lpfnFormatGetInstanceProc __stdcall GetPluginProc(const int k)
//{
//	if (k >= 0 && k < FORMAT_SIZE)
//		return PluginProcArray[k];
//	else
//		return NULL;
//}



CString CPdfFormat::m_property_str;

// *** cPicture maintains a property string for this PlugIn
//     which can be used for your picture format settings.

CString __stdcall CPdfFormat::get_properties()
{
	return m_property_str;
}


CString __stdcall CPdfFormat::get_ext()
{
	return L".pdf";
}

bool __stdcall CPdfFormat::properties_dlg(const HWND hwnd)
{
	//CString msg;
	//msg.LoadString(IDS_PROPERTY_DLG_PDF_TEXT);
	//::MessageBox(hwnd, msg, get_plugin_data().desc, MB_ICONINFORMATION);

	//CWnd Parent;
	//Parent.Attach(hwnd);

	//CPdfPropertiesDlg pdfPropertiesDlg(&Parent);

	//swscanf_s(m_property_str, L"%d", &pdfPropertiesDlg.m_compression_type);

	//if (pdfPropertiesDlg.DoModal() == IDOK)
	//	m_property_str.Format(L"%d", pdfPropertiesDlg.m_compression_type);

	//Parent.Detach();

	return false;	// false: no reload of the current picture

	return false;	// false: no reload of the current picture
}

struct PluginData __stdcall CPdfFormat::get_plugin_data()
{
	struct PluginData pluginData;

	// *** Short description of the new format,
	// will be used for example in the form: 
	// "The selected folder "c:\..." does not contain any pictures in [format1, format2 or format3] format."
	// Example for Tiff: "The selected folder "c:\..." does not contain any pictures in Jpeg or TIFF format."
	pluginData.name.LoadString(IDS_SHORT_DESC);

	// *** Long description of the new format,
	// will be used as an explanation in the options
	pluginData.desc.LoadString(IDS_LONG_DESC);

	return pluginData;
}

//void __stdcall CPdfFormat::get_size(const CString& FileName)
//{
//	// *** This function sets m_OriginalPictureWidth and m_OriginalPictureHeight with the picture dimensions
//	// and should be as efficient as possible.
//
//	m_bIsValid = false;
//	FPDF_InitLibrary();
//
//	FPDF_DOCUMENT document = FPDF_LoadDocument(CW2A(FileName), nullptr);
//	if (document)
//	{
//		FPDF_PAGE page = FPDF_LoadPage(document, 0);
//		if (page)
//		{
//			m_OriginalPictureWidth = static_cast<int>(FPDF_GetPageWidth(page));
//			m_OriginalPictureHeight = static_cast<int>(FPDF_GetPageHeight(page));
//
//			m_bIsValid = true;
//		}
//	}
//
//	FPDF_CloseDocument(document);
//}

void __stdcall CPdfFormat::get_size(const CString& FileName)
{
	// *** This function sets m_OriginalPictureWidth and m_OriginalPictureHeight with the picture dimensions
	// and should be as efficient as possible.

	m_bIsValid = false;
	FPDF_InitLibrary();

	FPDF_DOCUMENT document = FPDF_LoadDocument(CW2A(FileName), nullptr);
	FPDF_InitLibrary();

	if (document)
	{
		FPDF_DOCUMENT document = FPDF_LoadDocument(CW2A(FileName), nullptr);

		const int page_count = FPDF_GetPageCount(document);
		int max_width = 0;
		int max_height = 0;

		// Calculate the maximum width and height of the pages
		for (int i = 0; i < page_count; ++i)
		{
			FPDF_PAGE page = FPDF_LoadPage(document, i);
			if (!page)
			{
				continue;
			}

			const int width = static_cast<int>(FPDF_GetPageWidth(page));
			const int height = static_cast<int>(FPDF_GetPageHeight(page));
			if (width > max_width)
			{
				max_width = width;
			}
			if (height > max_height)
			{
				max_height = height;
			}

			FPDF_ClosePage(page);
		}


		border_size = min(max_width, max_height) / 40;
		separator_border_size = border_size / 2;

		// Calculate the number of rows and columns for the rectangle layout
		int num_cols = static_cast<int>(sqrt(page_count));
		if (page_count > 1)
			num_cols++;

		const int num_rows = (page_count + num_cols - 1) / num_cols;

		// Calculate the total width and height of the combined image
		const int total_width = num_cols * (max_width + 2 * (border_size + separator_border_size));
		const int total_height = num_rows * (max_height + 2 * (border_size + separator_border_size));
		m_OriginalPictureWidth = m_PictureWidth = total_width;
		m_OriginalPictureHeight = m_PictureHeight = total_height;

		m_bIsValid = true;
	}

	FPDF_CloseDocument(document);
}

//BYTE* __stdcall CPdfFormat::FileToRGB(const CString& FileName,
//	const int abs_size_x, const int abs_size_y,
//	const int rel_size_z, const int rel_size_n,
//	const enum scaling_type picture_scaling_type,
//	const bool b_scan)
//{
//	// *** This function loads the picture file and return an RGB array of the (decompressed) picture.
//
//	// https://github.com/bblanchon/pdfium-binaries/blob/master/example/example.c
//
//	FPDF_InitLibrary();
//
//	FPDF_DOCUMENT document = FPDF_LoadDocument(CW2A(FileName), nullptr);
//	if (!document)
//	{
//		return NULL;
//	}
//
//	FPDF_FORMFILLINFO form_callbacks = { 0 };
//	form_callbacks.version = 2;
//	FPDF_FORMHANDLE form = FPDFDOC_InitFormFillEnvironment(document, &form_callbacks);
//
//	FPDF_PAGE page = FPDF_LoadPage(document, 0);
//	if (!page)
//	{
//		FPDFDOC_ExitFormFillEnvironment(form);
//		FPDF_CloseDocument(document);
//
//		return NULL;
//	}
//
//	// Scale the PDF to the requested screen size.
//	const int pdf_width = static_cast<int>(FPDF_GetPageWidth(page));
//	const int pdf_height = static_cast<int>(FPDF_GetPageHeight(page));
//
//	int z = 1, n = 1;
//	if (abs_size_x && abs_size_y)
//	{
//		if (pdf_height * abs_size_x < pdf_width * abs_size_y)
//		{
//			z = abs_size_x;
//			n = pdf_width;
//		}
//		else
//		{
//			z = abs_size_y;
//			n = pdf_height;
//		}
//	}
//
//	m_OriginalPictureWidth = m_PictureWidth = z * pdf_width / n;
//	m_OriginalPictureHeight = m_PictureHeight = z * pdf_height / n;
//
//	// Setup the bitmap.
//	FPDF_BITMAP bitmap = FPDFBitmap_Create(m_OriginalPictureWidth, m_OriginalPictureHeight, 0);
//	FPDFBitmap_FillRect(bitmap, 0, 0, m_OriginalPictureWidth, m_OriginalPictureHeight, 0xFFFFFFFF);
//
//	// Render the PDF to the bitmap.
//	FPDF_RenderPageBitmap(bitmap, page, 0, 0, m_OriginalPictureWidth, m_OriginalPictureHeight, 0, FPDF_ANNOT);
//	FPDF_FFLDraw(form, bitmap, page, 0, 0, m_OriginalPictureWidth, m_OriginalPictureHeight, 0, FPDF_ANNOT);
//
//	// Convert bitmap pixels to the RGB-format.
//	const int size = 3 * m_OriginalPictureWidth * m_OriginalPictureHeight;
//	BYTE* pvmem = static_cast<BYTE*>(VirtualAlloc(reinterpret_cast<LPVOID>(NULL), size, MEM_COMMIT, PAGE_READWRITE));
//
//	if(pvmem)
//	{
//		const BYTE* pixels = static_cast<BYTE*>(FPDFBitmap_GetBuffer(bitmap));
//		BYTE* rgb = pvmem;
//		register int index = 0;
//
//		for (register int k = m_OriginalPictureWidth * m_OriginalPictureHeight; k != 0; k--)
//		{
//			*rgb++ = pixels[index + 2];
//			*rgb++ = pixels[index + 1];
//			*rgb++ = pixels[index];
//
//			// PDF is RGBA-Layout
//			index += 4;
//		}
//	}
//
//	// Clean up.
//	FPDFDOC_ExitFormFillEnvironment(form);
//	FPDFBitmap_Destroy(bitmap);
//	FPDF_ClosePage(page);
//	FPDF_CloseDocument(document);
//
//	m_bIsValid = size && pvmem != NULL;
//
//	return pvmem;
//}

BYTE* __stdcall CPdfFormat::FileToRGB(const CString& FileName,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n,
	const enum scaling_type picture_scaling_type,
	const bool b_scan)
{
	FPDF_InitLibrary();

	FPDF_DOCUMENT document = FPDF_LoadDocument(CW2A(FileName), nullptr);
	if (!document)
	{
		return NULL;
	}

	const int page_count = FPDF_GetPageCount(document);
	int max_width = 0;
	int max_height = 0;
	const unsigned int border_color = 0xFFFFD800;
	const unsigned int separator_border_color = 0xFFFFFFFF;

	// Calculate the maximum width and height of the pages
	for (int i = 0; i < page_count; ++i)
	{
		FPDF_PAGE page = FPDF_LoadPage(document, i);
		if (!page)
		{
			continue;
		}

		const int width = static_cast<int>(FPDF_GetPageWidth(page));
		const int height = static_cast<int>(FPDF_GetPageHeight(page));
		if (width > max_width)
		{
			max_width = width;
		}
		if (height > max_height)
		{
			max_height = height;
		}

		FPDF_ClosePage(page);
	}

	border_size = min(max_width, max_height) / 40;
	separator_border_size = border_size / 2;

	// Calculate the number of rows and columns for the rectangle layout
	int num_cols = static_cast<int>(sqrt(page_count));
	if (page_count > 1)
		num_cols++;

	const int num_rows = (page_count + num_cols - 1) / num_cols;

	// Calculate the total width and height of the combined image
	const int total_width = num_cols * (max_width + 2 * (border_size + separator_border_size));
	const int total_height = num_rows * (max_height + 2 * (border_size + separator_border_size));
	m_OriginalPictureWidth = m_PictureWidth = total_width;
	m_OriginalPictureHeight = m_PictureHeight = total_height;

	// Create a single bitmap with the combined width and height
	FPDF_BITMAP combined_bitmap = FPDFBitmap_Create(total_width, total_height, 0);
	FPDFBitmap_FillRect(combined_bitmap, 0, 0, total_width, total_height, 0xFFFFFFFF);

	FPDF_FORMFILLINFO form_callbacks = { 0 };
	form_callbacks.version = 2;
	FPDF_FORMHANDLE form = FPDFDOC_InitFormFillEnvironment(document, &form_callbacks);

	// Render each page directly into the combined bitmap
	for (int i = 0; i < page_count; ++i)
	{
		FPDF_PAGE page = FPDF_LoadPage(document, i);
		if (!page)
		{
			continue;
		}

		const int width = static_cast<int>(FPDF_GetPageWidth(page));
		const int height = static_cast<int>(FPDF_GetPageHeight(page));

		// Calculate the position to paste the bordered image in the combined image
		const int row = i / num_cols;
		const int col = i % num_cols;
		//int x_offset = col * (max_width + 2 * border_size) + border_size;
		//int y_offset = row * (max_height + 2 * border_size) + border_size;
		const int x_offset = col * (max_width + 2 * (border_size + separator_border_size)) + border_size + separator_border_size;
		const int y_offset = row * (max_height + 2 * (border_size + separator_border_size)) + border_size + separator_border_size;

		// Render the page into the combined bitmap
		FPDF_BITMAP page_bitmap = FPDFBitmap_Create(width, height, 0);
		FPDFBitmap_FillRect(page_bitmap, 0, 0, width, height, 0xFFFFFFFF);
		FPDF_RenderPageBitmap(page_bitmap, page, 0, 0, width, height, 0, 0);
		FPDF_FFLDraw(form, page_bitmap, page, 0, 0, m_OriginalPictureWidth, m_OriginalPictureHeight, 0, FPDF_ANNOT);

		BYTE* src_buffer = static_cast<BYTE*>(FPDFBitmap_GetBuffer(page_bitmap));
		BYTE* dest_buffer = static_cast<BYTE*>(FPDFBitmap_GetBuffer(combined_bitmap)) + y_offset * total_width * 4 + x_offset * 4;

		for (int y = 0; y < height; ++y)
		{
			memcpy(dest_buffer + y * total_width * 4, src_buffer + y * width * 4, width * 4);
		}

		// Draw the border around the page
		FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size, y_offset - border_size, width + 2 * border_size, border_size, border_color); // Top border
		FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size, y_offset + height, width + 2 * border_size, border_size, border_color); // Bottom border
		FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size, y_offset, border_size, height, border_color); // Left border
		FPDFBitmap_FillRect(combined_bitmap, x_offset + width, y_offset, border_size, height, border_color); // Right border

		// Draw the separator border around the border
		FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size - separator_border_size, y_offset - border_size - separator_border_size, width + 2 * border_size + 2 * separator_border_size, separator_border_size, separator_border_color); // Top separator border
		FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size - separator_border_size, y_offset + height + border_size, width + 2 * border_size + 2 * separator_border_size, separator_border_size, separator_border_color); // Bottom separator border
		FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size - separator_border_size, y_offset - separator_border_size, separator_border_size, height + 2 * border_size + 2 * separator_border_size, separator_border_color); // Left separator border
		FPDFBitmap_FillRect(combined_bitmap, x_offset + width + border_size, y_offset - separator_border_size, separator_border_size, height + 2 * border_size + 2 * separator_border_size, separator_border_color); // Right separator border

		FPDFBitmap_Destroy(page_bitmap);
		FPDF_ClosePage(page);
	}

	// Convert bitmap pixels to the RGB-format.
	const int size = 3 * m_OriginalPictureWidth * m_OriginalPictureHeight;
	BYTE* pvmem = static_cast<BYTE*>(VirtualAlloc(reinterpret_cast<LPVOID>(NULL), size, MEM_COMMIT, PAGE_READWRITE));

	if (pvmem)
	{
		const BYTE* pixels = static_cast<BYTE*>(FPDFBitmap_GetBuffer(combined_bitmap));
		BYTE* rgb = pvmem;
		register int index = 0;

		for (register int k = m_OriginalPictureWidth * m_OriginalPictureHeight; k != 0; k--)
		{
			*rgb++ = pixels[index + 2];
			*rgb++ = pixels[index + 1];
			*rgb++ = pixels[index];

			// PDF is RGBA-Layout
			index += 4;
		}
	}

	// Clean up.
	FPDFDOC_ExitFormFillEnvironment(form);
	FPDFBitmap_Destroy(combined_bitmap);
	FPDF_CloseDocument(document);

	m_bIsValid = size && pvmem != NULL;

	return pvmem;
}

unsigned int __stdcall CPdfFormat::get_cap() const
{
	// *** Capabilities of the plugin.
	return	PICTURE_READ;		// This plugin can read the picture format.
			//PICTURE_WRITE |	// This plugin can write the picture format.
								// At least one must be specified. Otherwise 
								// the plugin will not be loaded.
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
}

CString __stdcall CPdfFormat::get_info(const CString& FileName, const enum info_type _info_type)
{
	if(_info_type & (info_type_std | info_type_size))
	{
		get_size(FileName);
	}

	if(_info_type & info_type_short)
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


		get_size(FileName);

		CString msg, info;

		const int info_template_c((int)info_template.size());
		if(info_template_c > 8)
		{
			const TCHAR* t1 = wcsrchr(FileName, L'\\');
			if(t1)
				t1++;
			else
				t1 = FileName;

			//_stprintf(msg, info_template[0], t1);
			info.FormatMessage(info_template[0], t1);
			msg += info;
			msg += L'\n';

			// Picture folder
			info.FormatMessage(info_template[7], FileName.Left(FileName.ReverseFind(L'\\') + 1));
			msg += info;
			msg += L'\n';
			
			//_stprintf(msg, info_template[3], m_OriginalPictureWidth, m_OriginalPictureHeight);

			//Floating-point printf format specifiers — e, E, f, and g — are not supported. 
			//The workaround is to use the sprintf function to format the floating-point number 
			//into a temporary buffer, then use that buffer as the insert string. 
			const float f_mp((float)m_OriginalPictureWidth * m_OriginalPictureHeight / 1000 / 1000);
			CString mp;
			mp.Format(L"%.1f", (f_mp<0.1)?0.1:f_mp);

			info.FormatMessage(info_template[3], m_OriginalPictureWidth, m_OriginalPictureHeight, mp);
			msg += info;

			//_stprintf(msg, info_template[1], file_size?((file_size >> 10)?(file_size >> 10):1):0,
			//								 file_size, LongFileDateTime);
	
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
