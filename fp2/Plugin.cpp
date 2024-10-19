#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "SampleDlg.h"

// Example Plugin cpp_fp2.
// Display the selected pictures.
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
	return CFunctionPluginSample2::GetInstance;
}


CFunctionPluginSample2::CFunctionPluginSample2()
  : handle_wnd(NULL)
{
	_wsetlocale(LC_ALL, L".ACP"); 
}

struct PluginData __stdcall CFunctionPluginSample2::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);

	return pluginData;
}

enum REQUEST_TYPE __stdcall CFunctionPluginSample2::start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;

	// Get preview rect sizes.
	CWnd parent;
	parent.Attach(handle_wnd);

	vector<picture_data> picture_data_list;
	CSampleDlg SampleDlg(picture_data_list, &parent);
	
	SampleDlg.Create(IDD_DIALOG_SAMPLE2, &parent);
	parent.Detach();

	// Request one picture data set for the dialog preview size.
	// A negative value requests a relative size for the picture data.
	// For example, -100 requests data for the original 100% picture size.
	// To get picture data for the half size, use
	// request_data_sizes.push_back(request_data_size(-50, -50, DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA));
	request_data_sizes.push_back(
		request_data_size(SampleDlg.preview_position_rect.Width(),
			SampleDlg.preview_position_rect.Height(),
			DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA_DISPLAY)
	);

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginSample2::process_picture(const picture_data& picture_data) 
{ 
	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginSample2::end(const vector<picture_data>& picture_data_list)
{ 
	CWnd parent;
	parent.Attach(handle_wnd);

	CSampleDlg SampleDlg(picture_data_list, &parent);
	SampleDlg.DoModal();

	parent.Detach();

	return update_data_list;
}

