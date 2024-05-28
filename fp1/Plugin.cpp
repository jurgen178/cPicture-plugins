#include "stdafx.h"
#include "plugin.h"
#include "locale.h"

// Example Plugin cpp_fp1.
// List the selected pictures.

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
	return CFunctionPluginSample1::GetInstance;
}


CFunctionPluginSample1::CFunctionPluginSample1()
	: handle_wnd(NULL),
	picture_processed(0),
	pictures(0)
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
	// Start processing and print the picture names to process.

	handle_wnd = hwnd;
	pictures = (int)file_list.size();

	CString list;
	list.Format(L"start event, %d pictures\n------------------------\n", pictures);
	int i = 0;
	const int max_pictures(10);

	// Display the first max_pictures picture names.
	vector<const WCHAR*>::const_iterator it;
	for(it = file_list.begin(); i < max_pictures && it != file_list.end(); ++i, ++it)
	{
		list += *it;
		list += L"\n";
	}
	
	if (it != file_list.end())
	{
		list += L"  :\n";
	}

	::MessageBox(handle_wnd, list, get_plugin_data().desc, MB_ICONINFORMATION);
	
	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginSample1::process_picture(const picture_data& picture_data)
{ 
	// Process each picture.

	CString msg;
	msg.Format(L"process picture event (%d/%d):\n", ++picture_processed, pictures);
	const int ret(::MessageBox(handle_wnd, msg + picture_data.file_name, get_plugin_data().desc, pictures > 1 ? MB_OKCANCEL | MB_ICONINFORMATION : MB_ICONINFORMATION));

	// Signal that pictures could be updated, added or deleted (enum UPDATE_TYPE).
	// For example:
	// update_data_list.push_back(update_data(picture_data.file_name, UPDATE_TYPE_UPDATED));

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return ret == IDOK;
}

const vector<update_data>& __stdcall CFunctionPluginSample1::end(const vector<picture_data>& picture_data_list)
{ 
	// Print summary at the end of the processing.

	CString msg;
	msg.Format(L"end event, %d pictures processed", picture_processed);
	::MessageBox(handle_wnd, msg, get_plugin_data().desc, MB_ICONINFORMATION);

	// Return list of pictures that are updated, added or deleted (enum UPDATE_TYPE).
	return update_data_list;
}

