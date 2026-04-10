#pragma once

#include "FunctionPlugin.h"
#include "PostageRender.h"
#include <Vfw.h>

class CFunctionPluginPostage : public CFunctionPlugin
{
protected:
	CFunctionPluginPostage();

public:
	virtual ~CFunctionPluginPostage();

protected:
	HWND handle_wnd;
	vector<HBITMAP> dib_list;
	PostageSettings settings;

	void ClearDibs();

public:
	static CFunctionPlugin* __stdcall GetInstance()
	{
		return new CFunctionPluginPostage;
	};

public:
	virtual struct plugin_data __stdcall get_plugin_data() const;
	virtual struct arg_count __stdcall get_arg_count() const;

public:
	virtual enum REQUEST_TYPE __stdcall start(const HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes);
	virtual bool __stdcall process_picture(const picture_data& picture_data);
	virtual const vector<update_data>& __stdcall end(const vector<picture_data>& picture_data_list);
};
