#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "resource.h"

// Example Plugin cpp_fp4.
// Combine max two pictures.
// 
// This example uses the REQUEST_TYPE::REQUEST_TYPE_DATA,
// request_data_size type to get resized picture data,
// DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA for picture modification using device contexts.
// and the update_data type to create a picture.


const int picture_width(300);
const int picture_height(200);
const int headline_height(50);
const int border(10);


const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
	return L"1.6";
}

const PLUGIN_TYPE __stdcall GetPluginType()
{
	return PLUGIN_TYPE_FUNCTION;
}

const int __stdcall GetPluginInit()
{
	// Implement one function plugin.
	return 1;
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int k)
{
	// Plugin-Fabric: return the one function plugin.
	return CFunctionPluginSample5::GetInstance;
}


CFunctionPluginSample5::CFunctionPluginSample5()
	: handle_wnd(NULL),
	picture_processed(0),
	Dib(NULL)
{
}

CFunctionPluginSample5::~CFunctionPluginSample5() 
{
	::DeleteObject(Dib);
}

struct PluginData __stdcall CFunctionPluginSample5::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.name = L"Sample5 (combine pictures)";
	pluginData.desc = L"Sample function plugin 5";
	pluginData.info = L"Combine the selected pictures";

	return pluginData;
}

enum REQUEST_TYPE __stdcall CFunctionPluginSample5::start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	// Start event.

	handle_wnd = hwnd;

	// Requires 2 pictures.
	if (file_list.size() != 2)
	{
		CString msg;
		msg.Format(IDS_MIN_SELECTION);
		::MessageBox(hwnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);

		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	// Request picture size data in BGR DWORD aligned layout.
	request_data_sizes.push_back(request_data_size(picture_width, picture_height, DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA));

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginSample5::process_picture(const picture_data& picture_data)
{ 
	// Process picture event.

	picture_processed++;

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	
	// Load max 2 pictures.
	return picture_processed <= 2;
}

const vector<update_data>& __stdcall CFunctionPluginSample5::end(const vector<picture_data>& picture_data_list)
{ 
	// End event.

	// This example code is a set of steps to combine pictures using
	// the Windows device context and DIB section to illustrate the usage.
	// For simplicity, none of the steps are using sub-functions or any other optimizations.
	// For example, you could keep the main device context the same size as the first picture,
	// and add a smaller second picture as a signature or watermark.

	// Setup the bitmap for max two pictures with 300x200 with a headline and border.
	const int bitmap_width(2 * picture_width + 4 * border);
	const int bitmap_height(picture_height + headline_height + 2 * border);
	BITMAPINFO BitmapInfo = { 0 };
	BitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 24;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	BitmapInfo.bmiHeader.biWidth = bitmap_width;
	BitmapInfo.bmiHeader.biHeight = bitmap_height;

	// Create DIB section to add the pictures.
	void* ppvBits;
	Dib = ::CreateDIBSection(
		NULL,
		&BitmapInfo,
		DIB_RGB_COLORS,
		&ppvBits,
		NULL,
		0
	);

	// Create the DeviceContext to draw the Frame, Text and Bitmaps.
	CBitmap* pDib = CBitmap::FromHandle(Dib);
	CDC memDC;
	memDC.CreateCompatibleDC(NULL);

	CBitmap* pOldBitmap = memDC.SelectObject(pDib);

	// Setup the Pen to draw the Frame.
	CPen pen;
	pen.CreatePen(PS_SOLID, 4, RGB(255, 255, 0));
	CPen* pPen = memDC.SelectObject(&pen);
	HPEN hOldPen = (HPEN)pPen->GetSafeHandle();

	// Setup the Font to write the Text.
	CFont headline_font;
	headline_font.CreateFont(headline_height, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEVICE_PRECIS,
		CLIP_CHARACTER_PRECIS, PROOF_QUALITY, VARIABLE_PITCH | FF_SWISS, L"Consolas");
	CFont* pOldFont = memDC.SelectObject(&headline_font);
	HFONT hOldFont = (HFONT)pOldFont->GetSafeHandle();

	// Get the requested picture data.
	picture_data picture_data1 = picture_data_list[0];
	requested_data requested_data1 = picture_data1.requested_data_list[0];
	BYTE* data1 = requested_data1.data;

	picture_data picture_data2 = picture_data_list[1];
	requested_data requested_data2 = picture_data2.requested_data_list[0];
	BYTE* data2 = requested_data2.data;

	// Add the Frame.
	memDC.Rectangle(0, 0, bitmap_width, bitmap_height);

	// Add the Text.
	CString text(L"Sample5 Headline");
	CRect textRect;
	textRect.top = border;
	memDC.DrawText(text, textRect, DT_CALCRECT);
	const int w(textRect.Width());
	textRect.left = max(0, bitmap_width / 2 - w / 2);
	textRect.right = textRect.left + w;
	memDC.DrawText(text, textRect, DT_CENTER);

	// Add the first picture to the DIB section.
	HDRAWDIB hdd = DrawDibOpen();

	BITMAPINFOHEADER bmiHeader = { 0 };
	bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader.biPlanes = 1;
	bmiHeader.biBitCount = 24;
	bmiHeader.biCompression = BI_RGB;
	bmiHeader.biWidth = requested_data1.picture_width;
	bmiHeader.biHeight = requested_data1.picture_height;

	BOOL bDrawOK = DrawDibDraw(
		hdd,
		memDC.m_hDC,
		border + (picture_width - requested_data1.picture_width) / 2,
		border + headline_height + (picture_height - requested_data1.picture_height) / 2,
		requested_data1.picture_width,
		requested_data1.picture_height,
		&bmiHeader,
		data1,
		0,
		0,
		requested_data1.picture_width,
		requested_data1.picture_height,
		0
	);
	DrawDibClose(hdd);

	// Add the second picture to the DIB section.
	hdd = DrawDibOpen();

	bmiHeader.biWidth = requested_data2.picture_width;
	bmiHeader.biHeight = requested_data2.picture_height;

	bDrawOK = DrawDibDraw(
		hdd,
		memDC.m_hDC,
		border + bitmap_width / 2 + (picture_width - requested_data2.picture_width) / 2,
		border + headline_height + (picture_height - requested_data2.picture_height) / 2,
		requested_data2.picture_width,
		requested_data2.picture_height,
		&bmiHeader,
		data2,
		0,
		0,
		requested_data2.picture_width,
		requested_data2.picture_height,
		0
	);
	DrawDibClose(hdd);

	// Create a new file with the combined pictures.
	CString filename1(picture_data1.file_name.Mid(picture_data1.file_name.ReverseFind(L'\\') + 1));
	filename1 = filename1.Left(filename1.ReverseFind(L'.'));
	CString filename2(picture_data2.file_name.Mid(picture_data2.file_name.ReverseFind(L'\\') + 1));
	filename2 = filename2.Left(filename2.ReverseFind(L'.'));

	// Use the file type of the first file.
	const CString filename1ext(picture_data1.file_name.Mid(picture_data1.file_name.ReverseFind(L'.')));
	CString filename(filename1 + L"-" + filename2 + filename1ext);

	// Signal that the picture is added (enum UPDATE_TYPE).
	update_data_list.push_back(update_data(filename,
		UPDATE_TYPE::UPDATE_TYPE_ADDED,
		// Picture will be updated with this data:
		bitmap_width,
		bitmap_height,
		(BYTE*)ppvBits,
		// The Dib is using this format:
		DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA));

	// Clean-up
	if (hOldFont)
	{
		CFont* pFont = CFont::FromHandle(hOldFont);
		memDC.SelectObject(pFont);
	}

	if (hOldPen)
	{
		CPen* pPen = CPen::FromHandle(hOldPen);
		memDC.SelectObject(pPen);
	}
	memDC.SelectObject(pOldBitmap);

	// Return list of pictures that are updated, added or deleted (enum UPDATE_TYPE).
	return update_data_list;
}

