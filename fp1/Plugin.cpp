#include "stdafx.h"
#include "plugin.h"
#include "locale.h"

// Example Plugin cpp_fp1.

enum PLUGIN_TYPE operator|(const enum PLUGIN_TYPE t1, const enum PLUGIN_TYPE t2)
{
	return (enum PLUGIN_TYPE)((const unsigned int)t1 | (const unsigned int)t2);
}

enum PLUGIN_TYPE operator&(const enum PLUGIN_TYPE t1, const enum PLUGIN_TYPE t2)
{
	return (enum PLUGIN_TYPE)((const unsigned int)t1 & (const unsigned int)t2);
}


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

struct request_info __stdcall CFunctionPluginSample1::start(HWND hwnd, const vector<const WCHAR*>& file_list) 
{
	CString list(L"start\n------\n");
	for(vector<const WCHAR*>::const_iterator it = file_list.begin(); it != file_list.end(); ++it)
	{
		list += *it;
		list += L"\n";
	}
	
	AfxMessageBox(list, MB_ICONINFORMATION);
	
	return request_info();
}

bool __stdcall CFunctionPluginSample1::process_picture(const picture_data& _picture_data) 
{ 
	const CString msg(L"process picture:\n");
	AfxMessageBox(msg + _picture_data.m_name, MB_ICONINFORMATION);

	return true;
}

const vector<update_info>& __stdcall CFunctionPluginSample1::end() 
{ 
	AfxMessageBox(L"end", MB_ICONINFORMATION);

	return m_update_info;
}

