#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "SampleDlg.h"

// Example Plugin cpp_fp3.


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
  : handle_wnd(NULL)
{
	_wsetlocale(LC_ALL, L".ACP"); 
}

struct PluginData __stdcall CFunctionPluginSample3::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.name = L"Sample3";
	pluginData.desc = L"Sample function plugin 3";
	pluginData.info = L"Additional Info plugin 3";

	return pluginData;
}

enum REQUEST_TYPE __stdcall CFunctionPluginSample3::start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;

	// Request one picture data set for the preview picture 160x120 pixel.
	// A negative value requests a relative size for the picture data.
	// For example, -100 requests data for the original 100% picture size.
	// request_data_sizes.push_back(request_data_size(-50, -50));
	request_data_sizes.push_back(request_data_size(size_x, size_y));

	return REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA;
}

bool __stdcall CFunctionPluginSample3::process_picture(const picture_data& picture_data) 
{ 
	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginSample3::end(const vector<picture_data>& picture_data_list)
{ 
	CWnd parent;
	parent.Attach(handle_wnd);

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

	return update_data_list;
}

