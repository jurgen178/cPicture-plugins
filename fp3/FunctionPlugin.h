#pragma once
#include "stdafx.h"

#include "vector"
using namespace std;

// Request type used in the 'start' event.
enum REQUEST_TYPE
{
	REQUEST_TYPE_CANCEL = 0,
	REQUEST_TYPE_DATA,
};

enum DATA_REQUEST_TYPE
{
	// Picture format in RGB format.
	REQUEST_TYPE_RGB_DATA,
	// Picture format in BGR format DWORD aligned.
	REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA,
	// Picture data to be used in a device context for display purpose.
	REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA_DISPLAY,
};

struct request_data_size
{
	request_data_size(
		const int picture_width,
		const int picture_height,
		const enum DATA_REQUEST_TYPE data_request_type)
		: picture_width(picture_width),
		picture_height(picture_height),
		data_request_type(data_request_type)
	{
	};

	const int picture_width;
	const int picture_height;
	const enum DATA_REQUEST_TYPE data_request_type;
};


struct requested_data
{
	requested_data
	(
		int picture_width = 0,
		int picture_height = 0,
		BYTE* data = NULL,
		int len = 0
	)
		: picture_width(picture_width),
		picture_height(picture_height),
		data(data),
		len(len)
	{
	};

	int picture_width;
	int picture_height;
	BYTE* data;
	int len;
};


// Update-info used in the 'end' event.
enum UPDATE_TYPE
{
	UPDATE_TYPE_UNCHANGED = 0,
	UPDATE_TYPE_ADDED,
	UPDATE_TYPE_UPDATED,
	UPDATE_TYPE_DELETED,
};

struct update_data
{
	update_data(const CString file_name = L"",
		const UPDATE_TYPE update_type = UPDATE_TYPE::UPDATE_TYPE_UNCHANGED,
		const int picture_width = 0,
		const int picture_height = 0,
		BYTE* data = NULL,
		const DATA_REQUEST_TYPE request_type = DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA)
		: file_name(file_name),
		update_type(update_type),
		picture_width(picture_width),
		picture_height(picture_height),
		data(data),
		request_type(request_type)
	{
	};

	const CString file_name;
	const UPDATE_TYPE update_type;
	const int picture_width;
	const int picture_height;
	BYTE* data;
	const DATA_REQUEST_TYPE request_type;
};


struct picture_data
{
	picture_data(const CString file_name = L"")
		: file_name(file_name),
		picture_width(0),
		picture_height(0),
		error_msg(L""),
		audio(false),
		video(false),
		color_profile(false),
		gps(L""),
		exif_time(0),
		aperture(0.0),
		shutterspeed(0),
		iso(0),
		exif_datetime_display(L""),
		model(L""),
		lens(L""),
		cdata(L"")
	{
	};

	const CString file_name;

	int picture_width;
	int picture_height;
	vector<requested_data> requested_data_list;

	CString error_msg;
	bool audio;
	bool video;
	bool color_profile;
	CString gps;
	__int64 exif_time;
	float aperture;
	int shutterspeed;
	int iso;
	CString exif_datetime_display;
	CString model;
	CString lens;
	CString cdata;
};


enum PLUGIN_TYPE
{
	PLUGIN_TYPE_NONE = 0x0000,
	PLUGIN_TYPE_FORMAT = 0x0001,
	PLUGIN_TYPE_FUNCTION = 0x0002,
};


struct PluginData
{
public:
	PluginData() : type(PLUGIN_TYPE::PLUGIN_TYPE_NONE) { };

	CString name;
	CString desc;
	CString file_name;
	CString file_name2;
	CString info;
	CString version;
	enum PLUGIN_TYPE type;

	// Zum Sortieren der Einträge.
	bool operator < (PluginData& rhs)
	{
		return file_name2 < rhs.file_name2;
	}
};


class CFunctionPlugin
{
protected:
	CFunctionPlugin()
	{
	};

public:
	virtual __stdcall ~CFunctionPlugin() { };

protected:
	vector<update_data> update_data_list;

public:
	virtual struct PluginData __stdcall get_plugin_data() = 0;

public:
	virtual enum REQUEST_TYPE __stdcall start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes) { return REQUEST_TYPE::REQUEST_TYPE_DATA; };
	virtual bool __stdcall process_picture(const picture_data& picture_data) { return true; };
	virtual const vector<update_data>& __stdcall end(const vector<picture_data>& picture_data_list) { return update_data_list; };
};


typedef CFunctionPlugin* (__stdcall *lpfnFunctionGetInstanceProc)();
