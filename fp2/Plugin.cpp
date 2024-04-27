#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "SampleDlg.h"

// Example Plugin cpp_fp2.

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
	return CFunctionPluginSample2::GetInstance;
}


CFunctionPluginSample2::CFunctionPluginSample2()
  : m_hwnd(NULL)
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

struct request_info __stdcall CFunctionPluginSample2::start(HWND hwnd, const vector<const WCHAR*>& file_list) 
{
	m_hwnd = hwnd;

	vector<picture_data> picture_data_list;

	CWnd parent;
	parent.Attach(m_hwnd);
	CSampleDlg SampleDlg(picture_data_list, &parent);
	SampleDlg.Create(IDD_DIALOG_SAMPLE2, &parent);
	
	parent.Detach();

	return request_info(PICTURE_LAYOUT_BGR_DWORD_ALIGNED, SampleDlg.m_PreviewPositionRect.Width(), SampleDlg.m_PreviewPositionRect.Height());
}

bool __stdcall CFunctionPluginSample2::process_picture(const picture_data& _picture_data) 
{ 
	if(_picture_data.m_len)
	{
		picture_data picture_data_cpy(_picture_data);
		picture_data_cpy.m_buf = new BYTE[_picture_data.m_len];
		if(picture_data_cpy.m_buf)
		{
			memcpy(picture_data_cpy.m_buf, _picture_data.m_buf, _picture_data.m_len);
			picture_data_list.push_back(picture_data_cpy);
		}
	}

	// Return true to continue, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_info>& __stdcall CFunctionPluginSample2::end() 
{ 
	CWnd parent;
	parent.Attach(m_hwnd);

	CSampleDlg SampleDlg(picture_data_list, &parent);
	SampleDlg.DoModal();

	parent.Detach();

	for(vector<picture_data>::iterator it = picture_data_list.begin(); it != picture_data_list.end(); ++it)
	{
		delete it->m_buf;
	}

	return m_update_info;
}

