#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "AsciiArtDlg.h"

// Ascii Art Plugin cpp_ascii_art.
// 
// This plugin uses the REQUEST_TYPE::REQUEST_TYPE_DATA,
// request_data_size type to get resized picture data,
// DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA_DISPLAY for Windows dialog usage
// and the update_data type.


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
	return CFunctionPluginAsciiArt::GetInstance;
}


CFunctionPluginAsciiArt::CFunctionPluginAsciiArt()
  : handle_wnd(NULL)
{
	// Set the locale to the system's ANSI code page.
	_wsetlocale(LC_ALL, L".ACP"); 
}

struct PluginData __stdcall CFunctionPluginAsciiArt::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);

	return pluginData;
}

enum REQUEST_TYPE __stdcall CFunctionPluginAsciiArt::start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;

	// Allow only one picture.
	if (file_list.size() != 1)
	{
		CString msg;
		msg.Format(IDS_STRING_SINGLE_FILE_ONLY);
		::MessageBox(hwnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);

		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	// Get preview rect sizes.
	CWnd parent;
	parent.Attach(handle_wnd);

	vector<picture_data> picture_data_list;
	CAsciiArtDlg AsciiDlg(picture_data_list, &parent);
	
	AsciiDlg.Create(IDD_DIALOG_ASCII_ART, &parent);
	parent.Detach();

	// Request one picture data set for the dialog preview size and one unresized picture data set for the conversion.
	// A negative value requests a relative size for the picture data.
	// For example, -100 requests data for the original 100% picture size.
	// To get picture data for the half size, use
	// request_data_sizes.push_back(request_data_size(-50, -50, DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA));
	request_data_sizes.push_back(
		request_data_size(AsciiDlg.preview_position_rect.Width(),
			AsciiDlg.preview_position_rect.Height(),
			DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA_DISPLAY)
	);
	request_data_sizes.push_back(request_data_size(-100, -100, DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA));

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginAsciiArt::process_picture(const picture_data& picture_data) 
{ 
	// Get the second requested data set (unresized picture, 100%).
	requested_data requested_data2 = picture_data.requested_data_list.back();

	BYTE* data = requested_data2.data;

	// Modify the picture data.
	for (register unsigned int y = requested_data2.picture_height; y != 0; y--)
	{
		for (register unsigned int x = requested_data2.picture_width; x != 0; x--)
		{
			// Make a grey scale picture.
			const BYTE grey((306 * *(data) + 601 * *(data + 1) + 117 * *(data + 2)) >> 10);
			*data++ = grey;	// red
			*data++ = grey;	// green
			*data++ = grey;	// blue
		}
	}

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginAsciiArt::end(const vector<picture_data>& picture_data_list)
{ 
	CWnd parent;
	parent.Attach(handle_wnd);

	CAsciiArtDlg AsciiArtDlg(picture_data_list, &parent);
	AsciiArtDlg.DoModal();

	parent.Detach();

	return update_data_list;
}
