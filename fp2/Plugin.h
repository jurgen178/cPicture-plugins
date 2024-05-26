#pragma once
#include "FunctionPlugin.h"


class CFunctionPluginSample2 : public CFunctionPlugin
{
protected:
	CFunctionPluginSample2();

public:
	virtual __stdcall ~CFunctionPluginSample2() { };

protected:
	HWND m_hwnd;
	CWnd m_parent;
	vector<picture_data> m_picture_data_list;

public:
	static CFunctionPlugin* __stdcall GetInstance()
	{
		return new CFunctionPluginSample2;
	};

public:
	virtual struct PluginData __stdcall get_plugin_data();

public:
	virtual request_info __stdcall start(HWND hwnd, const vector<const WCHAR*>& file_list);
	virtual bool __stdcall process_picture(const picture_data& _picture_data);
	virtual const vector<update_info>& __stdcall end();
};
