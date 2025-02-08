#pragma once

#include "FunctionPlugin.h"


class CFunctionPluginCS : public CFunctionPlugin
{
protected:
	CFunctionPluginCS();

protected:
	HWND handle_wnd;
	int picture_processed;

public:
	virtual ~CFunctionPluginCS();

public:
	static CFunctionPlugin* __stdcall GetInstance()
	{
		return new CFunctionPluginCS;
	};

public:
	virtual struct plugin_data __stdcall get_plugin_data() const;
	virtual struct arg_count __stdcall get_arg_count() const;

public:
	virtual enum REQUEST_TYPE __stdcall start(const HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes);
	virtual bool __stdcall process_picture(const picture_data& picture_data);
	virtual const vector<update_data>& __stdcall end(const vector<picture_data>& picture_data_list);
};
