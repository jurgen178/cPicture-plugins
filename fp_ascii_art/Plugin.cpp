#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "AsciiArtDlg.h"

// Ascii Art Plugin cpp_ascii_art.
// 
// This example uses the REQUEST_TYPE::REQUEST_TYPE_DATA,
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

	// Get preview rect sizes.
	CWnd parent;
	parent.Attach(handle_wnd);

	vector<picture_data> picture_data_list;
	CAsciiArtDlg AsciiDlg(picture_data_list, &parent);
	
	AsciiDlg.Create(IDD_DIALOG_ASCII_ART, &parent);
	parent.Detach();

	// Request one picture data set for the dialog preview size.
	// A negative value requests a relative size for the picture data.
	// For example, -100 requests data for the original 100% picture size.
	// To get picture data for the half size, use
	// request_data_sizes.push_back(request_data_size(-50, -50));
	request_data_sizes.push_back(
		request_data_size(AsciiDlg.preview_position_rect.Width(),
			AsciiDlg.preview_position_rect.Height(),
			DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA_DISPLAY)
	);

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginAsciiArt::process_picture(const picture_data& picture_data) 
{ 
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

