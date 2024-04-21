#pragma once
#include "FunctionPlugin.h"


class CFunctionPluginPs1Script : public CFunctionPlugin
{
public:
	CFunctionPluginPs1Script(const CString& script);

protected:
	const CString m_PowerShellExe;
	CString m_script;
	vector<picture_data> picture_data_list;

public:
	virtual __stdcall ~CFunctionPluginPs1Script() { };

public:
	virtual struct PluginData __stdcall get_plugin_data();

public:
	virtual request_info __stdcall start(HWND hwnd, const vector<const WCHAR*>& file_list);
	virtual bool __stdcall process_picture(const picture_data& _picture_data);
	virtual const vector<update_info>& __stdcall end();
};
