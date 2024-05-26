#pragma once
#include "stdafx.h"

#include "vector"
using namespace std;

// Request-info used in the 'start' event.
enum PICTURE_REQUEST_INFO
{
	PICTURE_REQUEST_INFO_CANCEL_REQUEST = 0,
	PICTURE_REQUEST_INFO_FILE_NAME_ONLY,
	PICTURE_REQUEST_INFO_RGB_DATA,
	PICTURE_REQUEST_INFO_BGR_DWORD_ALIGNED_DATA,
};


struct request_info
{
	request_info(const PICTURE_REQUEST_INFO picture_request_info = PICTURE_REQUEST_INFO_FILE_NAME_ONLY,
		const int PictureWidth1 = 0,
		const int PictureHeight1 = 0,
		const int PictureWidth2 = 0,
		const int PictureHeight2 = 0)
		: m_picture_request_info(picture_request_info),
		m_PictureWidth1(PictureWidth1),
		m_PictureHeight1(PictureHeight1),
		m_PictureWidth2(PictureWidth2),
		m_PictureHeight2(PictureHeight2)
	{
	};

	PICTURE_REQUEST_INFO m_picture_request_info;
	int m_PictureWidth1;
	int m_PictureHeight1;
	int m_PictureWidth2;
	int m_PictureHeight2;
};


// Update-info used in the 'end' event.
enum UPDATE_TYPE
{
	UPDATE_TYPE_UNCHANGED = 0,
	UPDATE_TYPE_ADDED,
	UPDATE_TYPE_UPDATED,
	UPDATE_TYPE_DELETED,
};

struct update_info
{
	update_info(const CString& _list_obj = L"", const UPDATE_TYPE _update_type = UPDATE_TYPE_UNCHANGED)
		: m_list_obj(_list_obj),
		m_update_type(_update_type)
	{
	};

	CString m_list_obj;
	UPDATE_TYPE m_update_type;
};


struct picture_data
{
	picture_data(const CString FileName,
		const int PictureWidth1 = 0,
		const int PictureHeight1 = 0,
		const int OriginalPictureWidth1 = 0,
		const int OriginalPictureHeight1 = 0,
		BYTE* buf1 = NULL,
		const int len1 = 0,
		const int PictureWidth2 = 0,
		const int PictureHeight2 = 0,
		const int OriginalPictureWidth2 = 0,
		const int OriginalPictureHeight2 = 0,
		BYTE* buf2 = NULL,
		const int len2 = 0,

		const CString ErrorMsg = L"",
		const bool bAudio = false,
		const bool bVideo = false,
		const bool bColorProfile = false,
		const CString GPSdata = L"",
		const __int64 exiftime = 0,
		const float fAperture = 0.0,
		const int Shutterspeed = 0,
		const int ISO = 0,
		const CString ExifDateTime_display = L"",
		const CString Model = L"",
		const CString Lens = L"",
		const CString cdata = L"")
		: m_FileName(FileName),
		m_PictureWidth1(PictureWidth1),
		m_PictureHeight1(PictureHeight1),
		m_OriginalPictureWidth1(OriginalPictureWidth1),
		m_OriginalPictureHeight1(OriginalPictureHeight1),
		m_buf1(buf1),
		m_len1(len1),
		m_PictureWidth2(PictureWidth2),
		m_PictureHeight2(PictureHeight2),
		m_OriginalPictureWidth2(OriginalPictureWidth2),
		m_OriginalPictureHeight2(OriginalPictureHeight2),
		m_buf2(buf2),
		m_len2(len2),

		m_ErrorMsg(ErrorMsg),
		m_bAudio(bAudio),
		m_bVideo(bVideo),
		m_bColorProfile(bColorProfile),
		m_GPSdata(GPSdata),
		m_exiftime(exiftime),
		m_fAperture(fAperture),
		m_Shutterspeed(Shutterspeed),
		m_ISO(ISO),
		m_ExifDateTime_display(ExifDateTime_display),
		m_Model(Model),
		m_Lens(Lens),
		m_cdata(cdata)
	{
	};

	CString m_FileName;
	int m_PictureWidth1;
	int m_PictureHeight1;
	int m_OriginalPictureWidth1;
	int m_OriginalPictureHeight1;
	BYTE* m_buf1;
	int m_len1;

	int m_PictureWidth2;
	int m_PictureHeight2;
	int m_OriginalPictureWidth2;
	int m_OriginalPictureHeight2;
	BYTE* m_buf2;
	int m_len2;

	CString m_ErrorMsg;
	bool m_bAudio;
	bool m_bVideo;
	bool m_bColorProfile;
	CString m_GPSdata;
	__int64 m_exiftime;
	float m_fAperture;
	int m_Shutterspeed;
	int m_ISO;
	CString m_ExifDateTime_display;
	CString m_Model;
	CString m_Lens;
	CString m_cdata;
};


enum PLUGIN_TYPE
{
	PLUGIN_TYPE_NONE = 0x0000,
	PLUGIN_TYPE_FORMAT = 0x0001,
	PLUGIN_TYPE_FUNCTION = 0x0002,
};

enum PLUGIN_TYPE operator|(const enum PLUGIN_TYPE t1, const enum PLUGIN_TYPE t2);
enum PLUGIN_TYPE operator&(const enum PLUGIN_TYPE t1, const enum PLUGIN_TYPE t2);


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
	vector<update_info> m_update_info;

public:
	virtual struct PluginData __stdcall get_plugin_data() = 0;

public:
	virtual request_info __stdcall start(HWND hwnd, const vector<const WCHAR*>& file_list) { return request_info(); };
	virtual bool __stdcall process_picture(const picture_data& _picture_data) { return true; };
	virtual const vector<update_info>& __stdcall end() { return m_update_info; };
};


typedef CFunctionPlugin* (__stdcall *lpfnFunctionGetInstanceProc)();
