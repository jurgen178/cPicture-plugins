#pragma once
#include "FunctionPlugin.h"


class CFunctionPluginScript : public CFunctionPlugin
{
protected:
public:
	CFunctionPluginScript(const CString& script_file);

protected:
	CString script_file;
	int sequence;
	int max_files;

public:
	virtual __stdcall ~CFunctionPluginScript() { };

public:
	virtual struct PluginData __stdcall get_plugin_data();

public:
	virtual enum REQUEST_TYPE __stdcall start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes);
	virtual bool __stdcall process_picture(const picture_data& picture_data);
};
