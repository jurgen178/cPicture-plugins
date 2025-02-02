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


class CFunctionPluginBatScript : public CFunctionPlugin
{
protected:
public:
	CFunctionPluginBatScript(const script_info script_infoe);

protected:
	HWND handle_wnd;
	CString script_file;
	int sequence;
	int max_files;

public:
	virtual ~CFunctionPluginBatScript() { };

public:
	virtual struct plugin_data __stdcall get_plugin_data() const;
	virtual struct arg_count __stdcall get_arg_count() const;

public:
	virtual enum REQUEST_TYPE __stdcall start(const HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes);
	virtual bool __stdcall process_picture(const picture_data& picture_data);
};


class CFunctionPluginPS1Script : public CFunctionPlugin
{
public:
	CFunctionPluginPS1Script(const script_info script_info);

protected:
	const CString m_PowerShellExe;
	HWND handle_wnd;
	script_info m_script_info;

public:
	virtual ~CFunctionPluginPS1Script() { };

public:
	virtual struct plugin_data __stdcall get_plugin_data() const;
	virtual struct arg_count __stdcall get_arg_count() const;

public:
	virtual enum REQUEST_TYPE __stdcall start(const HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes);
	virtual bool __stdcall process_picture(const picture_data& picture_data);
	virtual const vector<update_data>& __stdcall end(const vector<picture_data>& picture_data_list);
};


class CFunctionPluginPyScript : public CFunctionPlugin
{
public:
	CFunctionPluginPyScript(const script_info script_info);

protected:
	const CString m_PythonExe;
	HWND handle_wnd;
	script_info m_script_info;

public:
	virtual ~CFunctionPluginPyScript() { };

public:
	virtual struct plugin_data __stdcall get_plugin_data() const;
	virtual struct arg_count __stdcall get_arg_count() const;

public:
	virtual enum REQUEST_TYPE __stdcall start(const HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes);
	virtual bool __stdcall process_picture(const picture_data& picture_data);
	virtual const vector<update_data>& __stdcall end(const vector<picture_data>& picture_data_list);
};
