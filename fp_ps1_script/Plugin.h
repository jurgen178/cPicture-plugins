#pragma once

#include "FunctionPlugin.h"


struct script_info
{
	script_info(const CString& script, const CString& desc)
		: script(script),
		desc(desc)
	{
	};

	CString script;
	CString desc;
};


class CFunctionPluginScript : public CFunctionPlugin
{
public:
	CFunctionPluginScript(const script_info script_info);

protected:
	const CString m_PowerShellExe;
	script_info m_script_info;

public:
	virtual __stdcall ~CFunctionPluginScript() { };

public:
	virtual struct PluginData __stdcall get_plugin_data();

public:
	virtual enum REQUEST_TYPE __stdcall start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes);
	virtual bool __stdcall process_picture(const picture_data& picture_data);
	virtual const vector<update_data>& __stdcall end(const vector<picture_data>& picture_data_list);
};
