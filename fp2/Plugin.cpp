#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "SampleDlg.h"

// Example Plugin cpp_fp2.


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
	return 1;
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int k)
{
	return CFunctionPluginSample2::GetInstance;
}


CFunctionPluginSample2::CFunctionPluginSample2()
  : hwnd(NULL)
{
	_wsetlocale(LC_ALL, L".ACP"); 
}

struct PluginData __stdcall CFunctionPluginSample2::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.name = L"Sample2";
	pluginData.desc = L"Sample function plugin 2";
	pluginData.info = L"Additional Info plugin 2";

	return pluginData;
}

enum REQUEST_TYPE __stdcall CFunctionPluginSample2::start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	hwnd = hwnd;

	// Get preview rect sizes.
	CWnd parent;
	parent.Attach(hwnd);

	vector<picture_data> picture_data_list;
	CSampleDlg SampleDlg(picture_data_list, &parent);
	
	SampleDlg.Create(IDD_DIALOG_SAMPLE2, &parent);
	parent.Detach();

	// Request one picture data set for the preview size.
	request_data_sizes.push_back(request_data_size(SampleDlg.m_PreviewPositionRect.Width(), SampleDlg.m_PreviewPositionRect.Height()));

	return REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA;
}

bool __stdcall CFunctionPluginSample2::process_picture(const picture_data& picture_data) 
{ 
	picture_data_list.push_back(picture_data);

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginSample2::end() 
{ 
	CWnd parent;
	parent.Attach(hwnd);

	CSampleDlg SampleDlg(picture_data_list, &parent);
	SampleDlg.DoModal();

	parent.Detach();

	return update_data_set;
}

