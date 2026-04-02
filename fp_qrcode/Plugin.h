#pragma once

#include "FunctionPlugin.h"


class CFunctionPluginQRCode : public CFunctionPlugin
{
protected:
	CFunctionPluginQRCode();

public:
	virtual ~CFunctionPluginQRCode() { };

protected:
	HWND handle_wnd;
	int qr_corner; // 0=Top-Left, 1=Top-Right, 2=Bottom-Left, 3=Bottom-Right
	CString qr_text; // text content for the QR code
	int qr_relative_size; // QR size as percentage of the shorter image side (5..50)

public:
	static CFunctionPlugin* __stdcall GetInstance()
	{
		return new CFunctionPluginQRCode;
	};

public:
	virtual struct plugin_data __stdcall get_plugin_data() const;
	virtual struct arg_count __stdcall get_arg_count() const;

public:
	virtual enum REQUEST_TYPE __stdcall start(const HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes);
	virtual bool __stdcall process_picture(const picture_data& picture_data);
	virtual const vector<update_data>& __stdcall end(const vector<picture_data>& picture_data_list);
};
