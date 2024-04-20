#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "SampleDlg.h"

// Plugin cpp_fp3

enum PLUGIN_TYPE operator|(const enum PLUGIN_TYPE t1, const enum PLUGIN_TYPE t2)
{
	return (enum PLUGIN_TYPE)((const unsigned int)t1 | (const unsigned int)t2);
}

enum PLUGIN_TYPE operator&(const enum PLUGIN_TYPE t1, const enum PLUGIN_TYPE t2)
{
	return (enum PLUGIN_TYPE)((const unsigned int)t1 & (const unsigned int)t2);
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
	return CFunctionPluginSample3::GetInstance;
}


CFunctionPluginSample3::CFunctionPluginSample3()
  : m_hwnd(NULL)
{
	_wsetlocale(LC_ALL, L".ACP"); 
}

struct PluginData __stdcall CFunctionPluginSample3::get_plugin_data()
{
	// pluginData.file_name_dll will be set when loaded into cPicture
	// pluginData.file_name will be set to pluginData.file_name_dll when pluginData.file_name is empty

	struct PluginData pluginData;

	pluginData.name = L"Sample3";
	pluginData.desc = L"Sample function plugin 3";
	pluginData.info = L"Additional Info plugin 3";

	return pluginData;
}

struct request_info __stdcall CFunctionPluginSample3::start(HWND hwnd, const vector<const WCHAR*>& file_list) 
{
	m_hwnd = hwnd;

	return request_info(PICTURE_LAYOUT_BGR_DWORD_ALIGNED, 80, 80);
}

bool __stdcall CFunctionPluginSample3::process_picture(const picture_data& _picture_data) 
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

	return true;
}

const vector<update_info>& __stdcall CFunctionPluginSample3::end() 
{ 
	CWnd parent;
	parent.Attach(m_hwnd);

	CSampleDlg SampleDlg(picture_data_list, &parent);
	if (SampleDlg.DoModal() == IDOK)
	{
		int numberOfPictures = 0;
		int totalPrice = 0;
		SampleDlg.update_total(numberOfPictures, totalPrice);

		// Order confirmation.
		if (numberOfPictures > 0)
		{
			CString totalPriceTemplate;
			totalPriceTemplate.LoadString(IDS_TOTAL_PRICE_FORMAT);
			CString totalPriceStr;
			totalPriceStr.Format(totalPriceTemplate, (float)totalPrice / 100);
			CString orderText;
			orderText.FormatMessage(IDS_ORDER_CONFIRMATION, numberOfPictures, totalPriceStr);

			AfxMessageBox(orderText, MB_OK | MB_ICONINFORMATION);
		}
	}

	parent.Detach();

	for(vector<picture_data>::iterator it = picture_data_list.begin(); it != picture_data_list.end(); ++it)
	{
		delete it->m_buf;
	}

	return m_update_info;
}

