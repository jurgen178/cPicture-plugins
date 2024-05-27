#pragma once
#include "FunctionPlugin.h"


class CFunctionPluginHDR : public CFunctionPlugin
{
protected:
	CFunctionPluginHDR();

public:
	virtual __stdcall ~CFunctionPluginHDR() { };

protected:
	HWND handle_wnd;
	CWnd parent_wnd;
	vector<const WCHAR*> picture_list;

protected:

public:
	static CFunctionPlugin* __stdcall GetInstance()
	{
		return new CFunctionPluginHDR;
	};

public:
	virtual struct PluginData __stdcall get_plugin_data();

public:
	virtual enum REQUEST_TYPE __stdcall start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes);
	virtual bool __stdcall process_picture(const picture_data& picture_data);
	virtual const vector<update_data>& __stdcall end(const vector<picture_data>& picture_data_list);
};
