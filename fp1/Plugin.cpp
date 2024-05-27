#include "stdafx.h"
#include "plugin.h"
#include "locale.h"

// Example Plugin cpp_fp1.


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
	return CFunctionPluginSample1::GetInstance;
}


CFunctionPluginSample1::CFunctionPluginSample1()
{
	_wsetlocale(LC_ALL, L".ACP"); 
}

struct PluginData __stdcall CFunctionPluginSample1::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.name = L"Sample1";
	pluginData.desc = L"Sample function plugin 1";
	pluginData.info = L"Additional Info plugin 1";

	return pluginData;
}

enum REQUEST_TYPE __stdcall CFunctionPluginSample1::start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	CString list(L"start\n------\n");
	for(vector<const WCHAR*>::const_iterator it = file_list.begin(); it != file_list.end(); ++it)
	{
		list += *it;
		list += L"\n";
	}
	
	AfxMessageBox(list, MB_ICONINFORMATION);
	
	return REQUEST_TYPE::REQUEST_TYPE_FILE_NAME_ONLY;
}

bool __stdcall CFunctionPluginSample1::process_picture(const picture_data& picture_data)
{ 
	const CString msg(L"process picture:\n");
	AfxMessageBox(msg + picture_data.file_name, MB_ICONINFORMATION);

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginSample1::end(const vector<picture_data>& picture_data_list)
{ 
	AfxMessageBox(L"end", MB_ICONINFORMATION);

	return update_data_list;
}

