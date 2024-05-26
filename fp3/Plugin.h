#pragma once
#include "FunctionPlugin.h"

const int size_x = 80;
const int size_y = 80;

class CFunctionPluginSample3 : public CFunctionPlugin
{
protected:
	CFunctionPluginSample3();

public:
	virtual __stdcall ~CFunctionPluginSample3() { };

protected:
	HWND m_hwnd;
	CWnd m_parent;
	vector<picture_data> m_picture_data_list;

public:
	static CFunctionPlugin* __stdcall GetInstance()
	{
		return new CFunctionPluginSample3;
	};

public:
	virtual struct PluginData __stdcall get_plugin_data();

public:
	virtual request_info __stdcall start(HWND hwnd, const vector<const WCHAR*>& file_list);
	virtual bool __stdcall process_picture(const picture_data& _picture_data);
	virtual const vector<update_info>& __stdcall end();
};
