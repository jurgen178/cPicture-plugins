#pragma once
#include "FunctionPlugin.h"


class CFunctionPluginScript : public CFunctionPlugin
{
public:
	CFunctionPluginScript(const CString& script);

protected:
	CString m_script;
	int m_i;
	int m_n;

public:
	virtual __stdcall ~CFunctionPluginScript() { };

public:
	virtual struct PluginData __stdcall get_plugin_data();

public:
	virtual request_info __stdcall start(HWND hwnd, const vector<const WCHAR*>& file_list);
	virtual bool __stdcall process_picture(const picture_data& _picture_data);
};
