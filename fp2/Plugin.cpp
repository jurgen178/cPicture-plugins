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

	// Request one picture data set for the preview size.
	vector<request_info_size> request_info_sizes = vector<request_info_size>();
	request_info_sizes.push_back(request_info_size(SampleDlg.m_PreviewPositionRect.Width(), SampleDlg.m_PreviewPositionRect.Height()));
	
	return request_info(PICTURE_REQUEST_INFO_BGR_DWORD_ALIGNED_DATA, request_info_sizes);
}

bool __stdcall CFunctionPluginSample2::process_picture(const picture_data& _picture_data) 
{ 
	picture_data_list.push_back(_picture_data);

	if(_picture_data.requested_data_set.size() > 0)
	{
		//picture_data picture_data_copy(_picture_data);
		//picture_data_list.push_back(picture_data_copy);


		//picture_data_copy.m_buf1 = new BYTE[_picture_data.m_len1];
		//if (picture_data_cpy.m_buf1)
		//{
		//	memcpy(picture_data_cpy.m_buf1, _picture_data.m_buf1, _picture_data.m_len1);
		//	picture_data_list.push_back(picture_data_cpy);
		//}

		//requested_data requested_data_copy(picture_data.requested_data_set[0]);
		//requested_data_copy.buf = new BYTE[requested_data_copy.len];
		//if(requested_data_copy.buf)
		//{
		//	memcpy(requested_data_copy.buf, picture_data.requested_data_set[0].buf, picture_data.requested_data_set[0].len);
		//	picture_data_list.push_back(requested_data_copy);
		//}
	}

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_info>& __stdcall CFunctionPluginSample2::end() 
{ 
	CWnd parent;
	parent.Attach(m_hwnd);

	CSampleDlg SampleDlg(picture_data_list, &parent);
	SampleDlg.DoModal();

	parent.Detach();

	return m_update_info;
}

