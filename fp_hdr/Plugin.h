#pragma once
#include "FunctionPlugin.h"


class CFunctionPluginHDR : public CFunctionPlugin
{
protected:
	CFunctionPluginHDR();

public:
	virtual __stdcall ~CFunctionPluginHDR() { };

protected:
	HWND m_hwnd;
	CWnd m_parent;
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
	virtual request_info __stdcall start(HWND hwnd, const vector<const WCHAR*>& file_list);
	virtual bool __stdcall process_picture(const picture_data& _picture_data);
	virtual const vector<update_info>& __stdcall end();
};
