#pragma once

#include "FunctionPlugin.h"
#include <Vfw.h>


class CFunctionPluginTimeCapsule : public CFunctionPlugin
{
protected:
	CFunctionPluginTimeCapsule();

protected:
	HWND handle_wnd;
	HBITMAP poster_dib;
	CString title_text;
	int sort_mode;
	bool show_metadata;
	int thumbnail_width;
	int thumbnail_height;
	double location_cluster_radius_km;

public:
	virtual ~CFunctionPluginTimeCapsule();

public:
	static CFunctionPlugin* __stdcall GetInstance()
	{
		return new CFunctionPluginTimeCapsule;
	};

public:
	virtual struct plugin_data __stdcall get_plugin_data() const;
	virtual struct arg_count __stdcall get_arg_count() const;

public:
	virtual enum REQUEST_TYPE __stdcall start(const HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes);
	virtual bool __stdcall process_picture(const picture_data& picture_data);
	virtual const vector<update_data>& __stdcall end(const vector<picture_data>& picture_data_list);
};
