#include "stdafx.h"
#include "resource.h"
#include "pluginformat.h"
#include "PdfPropertiesDlg.h"
#include "include/fpdf_edit.h"
#include "include/fpdf_save.h"

#include <vector>
#include <mutex>
using namespace std;


// Uses pdfium
// https://github.com/bblanchon/pdfium-binaries  pdfium-win-x64.tgz
// https://pdfium.googlesource.com/pdfium/



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

struct FileWriter : public FPDF_FILEWRITE
{
	FILE* file = NULL;

	static int WriteBlock(FPDF_FILEWRITE* pThis, const void* data, unsigned long size)
	{
		return static_cast<FileWriter*>(pThis)->Write(data, size);
	}

	bool Open(const WCHAR* path) noexcept
	{
		const errno_t err(_wfopen_s(&file, path, L"wb"));
		if (err == 0)
		{
			FPDF_FILEWRITE::version = 1;
			FPDF_FILEWRITE::WriteBlock = &FileWriter::WriteBlock;
			return true;
		}

		return false;
	}

	void Close() const noexcept
	{
		if (file)
		{
			fclose(file);
		}
	}

	int Write(const void* data, unsigned long size) const noexcept
	{
		return fwrite(data, 1, size, file) == size;
	}
};

bool FileExist(const WCHAR* pFile) noexcept
{
	const DWORD file_attr = GetFileAttributes(pFile);
	return file_attr != INVALID_FILE_ATTRIBUTES && ~file_attr & FILE_ATTRIBUTE_DIRECTORY;
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


// PDFium lib is not thread safe.
static std::mutex pdf_lib_mutex;


class PDFiumInit
{
public:
	PDFiumInit()
	{
		pdf_lib_mutex.lock();

		FPDF_LIBRARY_CONFIG config = { 0 };
		config.version = 2;

		FPDF_InitLibraryWithConfig(&config);
	};

	~PDFiumInit()
	{
		FPDF_DestroyLibrary();

		pdf_lib_mutex.unlock();
	}
};


const CString CPdfFormat::type = "PDF";

CPdfFormat::CPdfFormat()
	: start_page(0),
	end_page(0),
	page_count(0),
	pdf_page_width(0),
	pdf_page_height(0),
	num_cols(0),
	num_rows(0),
	border_size_pdf(0),
	separator_border_size_pdf(0)
{
}

CPdfFormat::~CPdfFormat()
{
}


// *** cPicture maintains a property string for this Plugin
//     which can be used for your picture format settings.
//     For example use a string format to create a string in get_properties()
//     and to get the values back in set_properties(...)

CString CPdfFormat::m_property_str;

int CPdfFormat::max_picture_x;
int CPdfFormat::max_picture_y;
int CPdfFormat::scaling;
int CPdfFormat::page_range_from;
int CPdfFormat::page_range_to;
int CPdfFormat::border_size;
int CPdfFormat::border_color;
int CPdfFormat::separator_border_color = 0xFFFFFFFF;

CString format_property_string(L"%d,%d,%d,%d,%d,%d,%06X");
constexpr int format_property_string_elements = 7;


void CPdfFormat::reset_properties()
{
	max_picture_x = 8000;
	max_picture_y = 8000;
	scaling = 100;
	page_range_from = 0;
	page_range_to = -1;
	border_size = 25;
	border_color = RGB(255, 216, 0);
}

void CPdfFormat::validate_properties()
{
	max_picture_x = min(65000, max(100, max_picture_x));
	max_picture_y = min(65000, max(100, max_picture_y));

	scaling = min(1000, max(10, scaling));

	page_range_from = max(0, page_range_from);
	page_range_to = max(-1, page_range_to);
	if (page_range_to < page_range_from)
		page_range_to = -1;

	border_size = min(250, max(0, border_size));
}

void __stdcall CPdfFormat::set_properties(const CString& property_str)
{
	m_property_str = property_str;

	const int ret = swscanf_s(m_property_str, format_property_string,
		&max_picture_x,
		&max_picture_y,
		&scaling,
		&page_range_from,
		&page_range_to,
		&border_size,
		&border_color
	);

	if (ret == format_property_string_elements)
		validate_properties();
	else
		reset_properties();
}

CString __stdcall CPdfFormat::get_properties() const
{
	m_property_str.Format(format_property_string,
		max_picture_x,
		max_picture_y,
		scaling,
		page_range_from,
		page_range_to,
		border_size,
		border_color
	);

	return m_property_str;
}

bool __stdcall CPdfFormat::properties_dlg(const HWND hwnd)
{
	CWnd Parent;
	Parent.Attach(hwnd);

	CPdfPropertiesDlg pdfPropertiesDlg(&Parent);

	pdfPropertiesDlg.max_picture_x = max_picture_x;
	pdfPropertiesDlg.max_picture_y = max_picture_y;
	pdfPropertiesDlg.scaling = scaling;
	pdfPropertiesDlg.page_range_from = page_range_from;
	pdfPropertiesDlg.page_range_to = page_range_to;
	pdfPropertiesDlg.border_size = border_size;
	pdfPropertiesDlg.border_color = border_color;

	bool properties_updated = false;

	if (pdfPropertiesDlg.DoModal() == IDOK)
	{
		properties_updated = 
			pdfPropertiesDlg.max_picture_x != max_picture_x ||
			pdfPropertiesDlg.max_picture_y != max_picture_y ||
			pdfPropertiesDlg.scaling != scaling ||
			pdfPropertiesDlg.page_range_from != page_range_from ||
			pdfPropertiesDlg.page_range_to != page_range_to ||
			pdfPropertiesDlg.border_size != border_size ||
			pdfPropertiesDlg.border_color != border_color;

		max_picture_x = pdfPropertiesDlg.max_picture_x;
		max_picture_y = pdfPropertiesDlg.max_picture_y;
		scaling = pdfPropertiesDlg.scaling;
		page_range_from = pdfPropertiesDlg.page_range_from;
		page_range_to = pdfPropertiesDlg.page_range_to;
		border_size = pdfPropertiesDlg.border_size;
		border_color = pdfPropertiesDlg.border_color;

		validate_properties();
	}

	Parent.Detach();

	// true: reload pictures
	return properties_updated;
}

CString __stdcall CPdfFormat::get_ext() const
{
	// *** File extensions associated with the new format.
	// Multiple extensions are separated by ';' ("abc;defg")
	return L"pdf";
}

struct plugin_data __stdcall CPdfFormat::get_plugin_data() const
{
	struct plugin_data pluginData;

	// *** Short description of the new format,
	// will be used for example in the form: 
	// "The selected folder "c:\..." does not contain any pictures in [format1, format2 or format3] format."
	// Example for Tiff: "The selected folder "c:\..." does not contain any pictures in TIFF format."
	pluginData.name.LoadString(IDS_SHORT_DESC);

	// *** Long description of the new format,
	// will be used as an explanation in the options
	pluginData.desc.LoadString(IDS_LONG_DESC);

	return pluginData;
}

void CPdfFormat::get_page_range(FPDF_DOCUMENT document, int& start, int& end)
{
	const int page_count = FPDF_GetPageCount(document);
	
	start = page_range_from < page_count ? page_range_from : page_count - 1;
	
	if(page_range_to == 0)
		end = start;
	else
	if (page_range_to == -1)
		end = page_count - 1;
	else
		end = min(page_range_to, page_count - 1);
}

CStringA CPdfFormat::get_utf8_file_name(const CString& FileName)
{
	// FPDF_LoadDocument requires a UTF-8 encoded file name.

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

void CPdfFormat::update_page_sizes(FPDF_DOCUMENT document,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n)
{
	pdf_page_width = 0;
	pdf_page_height = 0;

	start_page = 0;
	end_page = 0;
	get_page_range(document, start_page, end_page);
	page_count = end_page - start_page + 1;

	if (page_count <= 0)
	{
		return;
	}

	// Calculate the maximum width and height of the pages.
	for (int i = start_page; i <= end_page; ++i)
	{
		FPDF_PAGE page = FPDF_LoadPage(document, i);
		if (page)
		{
			const int width = static_cast<int>(FPDF_GetPageWidth(page));
			const int height = static_cast<int>(FPDF_GetPageHeight(page));

			if (width > pdf_page_width)
			{
				pdf_page_width = width;
			}

			if (height > pdf_page_height)
			{
				pdf_page_height = height;
			}

			FPDF_ClosePage(page);
		}
	}

	pdf_page_width = scaling * pdf_page_width / 100;
	pdf_page_height = scaling * pdf_page_height / 100;

	// Calculate the number of rows and columns for the rectangle layout.
	num_cols = static_cast<int>(sqrt(page_count));
	if (page_count > 1)
		++num_cols;

	num_rows = (page_count + num_cols - 1) / num_cols;

	// Calculate the size of the border around the pages.
	auto set_border_size = [&]()
		{
			const int average_page_size = (pdf_page_width + pdf_page_height) / 2;
			border_size_pdf = page_count > 1 ? border_size * average_page_size / 1000 : 0;
			separator_border_size_pdf = border_size_pdf / 2;

			if (page_count > 1 && border_size)
			{
				if (border_size_pdf == 0)
					border_size_pdf = 1;

				if (separator_border_size_pdf == 0)
					separator_border_size_pdf = 1;
			}
		};

	set_border_size();

	// Nominal size of the image.
	int nominal_width = num_cols * (pdf_page_width + 2 * (border_size_pdf + separator_border_size_pdf));
	int nominal_height = num_rows * (pdf_page_height + 2 * (border_size_pdf + separator_border_size_pdf));

	int scale_z = 1;
	int scale_n = 1;

	// autosize: abs_size_x == 0 && abs_size_y == 0
	// limit to max_picture_x and max_picture_y
	if (abs_size_x == 0 || abs_size_y == 0)
	{
		if (nominal_width > max_picture_x || nominal_height > max_picture_y)
		{
			const unsigned int nhx = nominal_height * max_picture_x;
			const unsigned int nwy = nominal_width * max_picture_y;
			if (nhx < nwy)
			{
				scale_z = max_picture_x;
				scale_n = nominal_width;
			}
			else
			{
				scale_z = max_picture_y;
				scale_n = nominal_height;
			}
		}
	}
	else
	//if (abs_size_x && abs_size_y)	// size to abs_size_x and abs_size_y
	{
		const unsigned int nhx = nominal_height * abs_size_x;
		const unsigned int nwy = nominal_width * abs_size_y;
		if (nhx < nwy)
		{
			scale_z = abs_size_x;
			scale_n = nominal_width;
		}
		else
		{
			scale_z = abs_size_y;
			scale_n = nominal_height;
		}
	}

	// Scale to size
	nominal_width = scale_z * nominal_width / scale_n;
	nominal_height = scale_z * nominal_height / scale_n;

	// Scale relative
	nominal_width = rel_size_z * nominal_width / rel_size_n;
	nominal_height = rel_size_z * nominal_height / rel_size_n;
	
	if (nominal_width == 0)
		nominal_width = 1;

	if (nominal_height == 0)
		nominal_height = 1;

	// Calculate the new page size from the nominal size.
	// Each picture has left and right side (border + separator_border),
	// with separator_border half the size of the border:
	// Border = 3 * border_size
	const int page_bordersize = page_count > 1 ? border_size : 0;
	pdf_page_width = 1000 * nominal_width / (num_cols * (1000 + 3 * page_bordersize));
	pdf_page_height = 1000 * nominal_height / (num_rows * (1000 + 3 * page_bordersize));
	
	if (pdf_page_width == 0)
		pdf_page_width = 1;

	if (pdf_page_height == 0)
		pdf_page_height = 1;

	// Calculate the new size of the border around the pages.
	set_border_size();

	// Calculate the total width and height of the combined image.
	// Because of int rounding, m_PictureWidth is not equal to nominal_width (up to 1%)
	// if double data type would be used, m_PictureWidth would be equal to nominal_width
	m_PictureWidth = num_cols * (pdf_page_width + 2 * (border_size_pdf + separator_border_size_pdf));
	m_PictureHeight = num_rows * (pdf_page_height + 2 * (border_size_pdf + separator_border_size_pdf));

	// Keep the original size values when using relative resizing.
	m_OriginalPictureWidth = rel_size_n * m_PictureWidth / rel_size_z;
	m_OriginalPictureHeight = rel_size_n * m_PictureHeight / rel_size_z;
}

FPDF_BITMAP CPdfFormat::get_pages(FPDF_DOCUMENT document,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n)
{
	update_page_sizes(document, abs_size_x, abs_size_y, rel_size_z, rel_size_n);

	if (page_count <= 0)
	{
		return nullptr;
	}

	const BYTE red = GetRValue(border_color);
	const BYTE green = GetGValue(border_color);
	const BYTE blue = GetBValue(border_color);
	const int border_color_bgr = (red << 16) + (green << 8) + (blue);

	// Create a single bitmap with the combined width and height.
	FPDF_BITMAP combined_bitmap = FPDFBitmap_Create(m_PictureWidth, m_PictureHeight, 0);
	FPDFBitmap_FillRect(combined_bitmap, 0, 0, m_PictureWidth, m_PictureHeight, 0xFFFFFFFF);

	// Render each page directly into the combined bitmap.
	for (int i = start_page; i <= end_page; ++i)
	{
		FPDF_PAGE page = FPDF_LoadPage(document, i);
		if (page)
		{
			//// Experimental API.
			//// Returns the thumbnail of |page| as a FPDF_BITMAP. Returns a nullptr
			//// if unable to access the thumbnail's stream.
			//FPDF_BITMAP bmp = FPDFPage_GetThumbnailAsBitmap(page);

			const int width = pdf_page_width;
			const int height = pdf_page_height;

			// Calculate the position to paste the bordered image in the combined image.
			const int row = (i - start_page) / num_cols;
			const int col = (i - start_page) % num_cols;

			const int x_offset = col * (pdf_page_width + 2 * (border_size_pdf + separator_border_size_pdf)) + border_size_pdf + separator_border_size_pdf;
			const int y_offset = row * (pdf_page_height + 2 * (border_size_pdf + separator_border_size_pdf)) + border_size_pdf + separator_border_size_pdf;

			// Render the page into the combined bitmap.
			FPDF_BITMAP page_bitmap = FPDFBitmap_Create(width, height, 0);
			FPDFBitmap_FillRect(page_bitmap, 0, 0, width, height, 0xFFFFFFFF);
			FPDF_RenderPageBitmap(page_bitmap, page, 0, 0, width, height, 0, 0);

			const BYTE* src_buffer = static_cast<const BYTE*>(FPDFBitmap_GetBuffer(page_bitmap));
			BYTE* dest_buffer = static_cast<BYTE*>(FPDFBitmap_GetBuffer(combined_bitmap)) + 4 * (y_offset * m_PictureWidth + x_offset);

			register int dest_index = 0;
			const int dest_inc = 4 * m_PictureWidth;
			register int src_index = 0;
			const int src_inc = 4 * width;
			const int size = 4 * width;

			for (register int y = 0; y < height; ++y)
			{
				memcpy(dest_buffer + dest_index, src_buffer + src_index, size);
				dest_index += dest_inc;
				src_index += src_inc;
			}

			// Draw the border around the page.
			FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size_pdf, y_offset - border_size_pdf, width + 2 * border_size_pdf, border_size_pdf, border_color_bgr); // Top border
			FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size_pdf, y_offset + height, width + 2 * border_size_pdf, border_size_pdf, border_color_bgr); // Bottom border
			FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size_pdf, y_offset, border_size_pdf, height, border_color_bgr); // Left border
			FPDFBitmap_FillRect(combined_bitmap, x_offset + width, y_offset, border_size_pdf, height, border_color_bgr); // Right border

			// Draw the separator border around the border.
			FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size_pdf - separator_border_size_pdf, y_offset - border_size_pdf - separator_border_size_pdf, width + 2 * border_size_pdf + 2 * separator_border_size_pdf, separator_border_size_pdf, separator_border_color); // Top separator border
			FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size_pdf - separator_border_size_pdf, y_offset + height + border_size_pdf, width + 2 * border_size_pdf + 2 * separator_border_size_pdf, separator_border_size_pdf, separator_border_color); // Bottom separator border
			FPDFBitmap_FillRect(combined_bitmap, x_offset - border_size_pdf - separator_border_size_pdf, y_offset - separator_border_size_pdf, separator_border_size_pdf, height + 2 * border_size_pdf + 2 * separator_border_size_pdf, separator_border_color); // Left separator border
			FPDFBitmap_FillRect(combined_bitmap, x_offset + width + border_size_pdf, y_offset - separator_border_size_pdf, separator_border_size_pdf, height + 2 * border_size_pdf + 2 * separator_border_size_pdf, separator_border_color); // Right separator border

			FPDFBitmap_Destroy(page_bitmap);
			FPDF_ClosePage(page);
		}
	}

	return combined_bitmap;
}

BYTE* CPdfFormat::convert_to_rgb(FPDF_BITMAP rgba_bitmap)
{
	BYTE* pvmem = NULL;

	if (rgba_bitmap && m_PictureWidth && m_PictureHeight)
	{
		const unsigned int size = 3 * m_PictureWidth * m_PictureHeight;
		pvmem = static_cast<BYTE*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));

		if (pvmem)
		{
			const BYTE* pixels = static_cast<const BYTE*>(FPDFBitmap_GetBuffer(rgba_bitmap));
			BYTE* rgb = pvmem;
			
			register int index = 2;		// BGR -> RGB
			//register int index = 3;	// BGRA -> RGB

			for (register unsigned int k = m_PictureWidth * m_PictureHeight; k != 0; --k)
			{
				// BGR -> RGB
				*rgb = *(pixels + index);
				++rgb; --index;
				*rgb = *(pixels + index);
				++rgb; --index;
				*rgb = *(pixels + index);
				++rgb;

				index += 6;

				//// BGRA -> RGB
				//const int alpha = *(pixels + index);
				//--index;
				//*rgb = alpha * *(pixels + index) / 255;
				//++rgb; --index;
				//*rgb = alpha * *(pixels + index) / 255;
				//++rgb; --index;
				//*rgb = alpha * *(pixels + index) / 255;
				//++rgb;

				//index += 7;
			}
		}
	}

	return pvmem;
}

BYTE* __stdcall CPdfFormat::ReadFile(const CString& FileName,
	const int size_x, const int size_y,
	const int rel_size_z, const int rel_size_n)
{
	PDFiumInit pdfiumInit;

	BYTE* pvmem = NULL;

	FPDF_DOCUMENT document = FPDF_LoadDocument(get_utf8_file_name(FileName), nullptr);
	if (document)
	{
		FPDF_BITMAP rgba_bitmap = get_pages(document, size_x, size_y, rel_size_z, rel_size_n);

		// Convert bitmap pixels to the RGB-format.
		pvmem = convert_to_rgb(rgba_bitmap);

		// Clean up.
		FPDFBitmap_Destroy(rgba_bitmap);
		FPDF_CloseDocument(document);
	}

	return pvmem;
}

BYTE* __stdcall CPdfFormat::FileToPreview(const CString& FileName, int& len, int& size_x, int& size_y, const bool bScanAudio, const bool bMaxSize)
{
	BYTE* pvmem = ReadFile(FileName, size_x, size_y);

	len = 0;
	size_x = m_OriginalPictureWidth;
	size_y = m_OriginalPictureHeight;

	return pvmem;
}

BYTE* __stdcall CPdfFormat::FileToRGB(const CString& FileName,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n,
	const enum scaling_type picture_scaling_type,
	const bool b_scan)
{
	BYTE* pvmem = ReadFile(FileName, 0, 0, rel_size_z, rel_size_n);	// autosize: abs_size_x == 0 && abs_size_y == 0
	m_bIsValid = pvmem != NULL;
	
	return pvmem;
}


// Lambda for the rotate transform.
auto rotate_transform = [](FPDF_PAGE page, const int angle) -> void
	{
		// Get the current rotation.
		// FPDFPage_GetRotation returns one of the following indicating the page rotation:
		//   0 - No rotation.
		//   1 - Rotated 90 degrees clockwise.
		//   2 - Rotated 180 degrees clockwise.
		//   3 - Rotated 270 degrees clockwise.
		const int current_rotation = FPDFPage_GetRotation(page) * 90;

		// Calculate the new rotation.
		const int new_rotation = (current_rotation + angle + 360) % 360;

		// Set the new rotation.
		FPDFPage_SetRotation(page, new_rotation / 90);
	};


// Mirror transform renders the pages with black background.
// Lambda for the mirror transform.
auto mirror_transform = [](FPDF_PAGE page, const bool mirror_horizontal) -> void
	{
		const float width = static_cast<float>(FPDF_GetPageWidth(page));
		const float height = static_cast<float>(FPDF_GetPageHeight(page));

		// Iterate through each page object.
		const int object_count = FPDFPage_CountObjects(page);
		for (int j = 0; j < object_count; ++j)
		{
			FPDF_PAGEOBJECT page_object = FPDFPage_GetObject(page, j);
			if (page_object)
			{
				// Create a transformation matrix for horizontal mirroring.
				const FS_MATRIX matrix = { -1, 0, 0, 1, width, 0 };
				FPDFPageObj_Transform(page_object, matrix.a, matrix.b, matrix.c, matrix.d, matrix.e, matrix.f);
			}
		}

		// Regenerate the page content.
		FPDFPage_GenerateContent(page);
	};


bool CPdfFormat::Transform(const CString& inFileName, const function<void(FPDF_PAGE)>& transform_function)
{
	PDFiumInit pdfiumInit;

	const CString outFileName(inFileName + L".trans");

	FPDF_DOCUMENT document = FPDF_LoadDocument(get_utf8_file_name(inFileName), nullptr);
	if (document)
	{
		// Get the number of pages.
		const int page_count = FPDF_GetPageCount(document);

		// Transform each page.
		for (int i = 0; i < page_count; ++i)
		{
			FPDF_PAGE page = FPDF_LoadPage(document, i);
			if (page)
			{
				transform_function(page);

				FPDF_ClosePage(page);
			}
		}

		// Save the modified document.
		FileWriter file_writer;
		if (file_writer.Open(outFileName))
		{
			FPDF_SaveAsCopy(document, &file_writer, 0);
			file_writer.Close();
		}

		// Clean up.
		FPDF_CloseDocument(document);

		if (FileExist(outFileName))
		{
			// delete input file
			if (::DeleteFile(inFileName))
			{
				// rename .trans file to input file
				return ::MoveFile(outFileName, inFileName) != 0;
			}
			else
			{
				::DeleteFile(outFileName);
			}
		}
	}

	return false;
}

bool __stdcall CPdfFormat::RotateLeft(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly)
{
	auto rotate_left = [](FPDF_PAGE page) -> void
		{
			rotate_transform(page, -90);
		};

	return Transform(inFileName, rotate_left);
}

bool __stdcall CPdfFormat::RotateRight(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly)
{
	auto rotate_right = [](FPDF_PAGE page) -> void
		{
			rotate_transform(page, 90);
		};

	return Transform(inFileName, rotate_right);
}

bool __stdcall CPdfFormat::Rotate180(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly)
{
	auto rotate_180 = [](FPDF_PAGE page) -> void
		{
			rotate_transform(page, 180);
		};

	return Transform(inFileName, rotate_180);
}

bool __stdcall CPdfFormat::FlipH(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly)
{
	auto flipH = [](FPDF_PAGE page) -> void
		{
			mirror_transform(page, true);
		};

	return Transform(inFileName, flipH);
}

bool __stdcall CPdfFormat::FlipV(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly)
{
	auto flipV = [](FPDF_PAGE page) -> void
		{
			mirror_transform(page, false);
		};

	return Transform(inFileName, flipV);
}

unsigned int __stdcall CPdfFormat::get_cap() const
{
	// *** Capabilities of the plugin.
	return PICTURE_READ |		// This plugin can read the picture format.
		//PICTURE_WRITE |		// This plugin can write the picture format.
								// At least one must be specified. Otherwise 
								// the plugin will not be loaded.
								// See PictureFormat.h for the complete list.
								// 
								// This plugin supports several transformations.
								// If not implemented in this plugin, a default implementation will be used.
		PICTURE_ROTATELEFT |
		PICTURE_ROTATERIGHT |
		PICTURE_ROTATE180;

		// Mirror transform renders the pages with black background.
		//PICTURE_FLIPH |
		//PICTURE_FLIPV;
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

CString __stdcall CPdfFormat::get_info(const CString& FileName, const enum info_type _info_type)
{
	// *** This function gets the picture data and should be as efficient as possible.
	// For example: do not load and decompress the image just to get the picture dimensions

	int page_count = 0;

	if (_info_type & (info_type_std | info_type_short))
	{
		PDFiumInit pdfiumInit;
		FPDF_DOCUMENT document = FPDF_LoadDocument(get_utf8_file_name(FileName), nullptr);
		if (document)
		{
			page_count = FPDF_GetPageCount(document);

			if (page_count)
			{
				update_page_sizes(document, 0, 0);
				m_bIsValid = m_OriginalPictureWidth && m_OriginalPictureHeight;
			}
		}

		FPDF_CloseDocument(document);
	}

	if(_info_type & info_type_std)
	{
		CString info;
		if (page_count)
		{
			info.Format(IDS_PAGE_COUNT, page_count);
		}
		return info;
	}

	if(_info_type & info_type_short)
	{
		// List of localized templates:
		//info_template[0] = Bildname:\t%1
	    //info_template[1] = Dateigröße:\t%1 (%2 Bytes)\nGeändert am:\t%3\nDatei geändert am:\t%4
		//info_template[2] = Erstellt am:\t%1
		//info_template[3] = Bildgröße:\t%1!d!x%2!d! Bildpunkte (%3!s! MP)
		//info_template[4] = Modell:\t\t%1!s!
		//info_template[5] = Fehler:\t\t%1!s!
		//info_template[6] = Enthält:\t%1!s!
		//info_template[7] = Bildordner:\t%1
		//info_template[8] = Einstellungen:\t


		CString msg, info;
		const size_t info_template_c(info_template.size());

		if(info_template_c > 8)
		{
			const TCHAR* t1 = wcsrchr(FileName, L'\\');
			if(t1)
				++t1;
			else
				t1 = FileName;

			info.FormatMessage(info_template[0], t1);
			msg += info;
			msg += L'\n';

			// Picture folder
			info.FormatMessage(info_template[7], FileName.Left(FileName.ReverseFind(L'\\') + 1));
			msg += info;
			msg += L'\n';

			if (page_count)
			{
				CString n;
				n.Format(IDS_PAGE_COUNT, page_count);
				info.FormatMessage(info_template[6], n);
				msg += info;
				msg += L'\n';
			}

			//Floating-point printf format specifiers — e, E, f, and g — are not supported in FormatMessage. 
			//The workaround is to use the Format function to format the floating-point number 
			//into a temporary buffer, then use that buffer as the insert string. 
			const float f_mp(static_cast<float>(m_OriginalPictureWidth * m_OriginalPictureHeight) / 1000 / 1000);
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
