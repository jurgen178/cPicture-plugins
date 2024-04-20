#pragma once
#include "stdafx.h"

#include "vector"
using namespace std;


enum PICTURE_LAYOUT
{
	PICTURE_LAYOUT_CANCEL_REQUEST = 0,
	PICTURE_LAYOUT_FILE_NAME_ONLY,
	PICTURE_LAYOUT_RGB,
	PICTURE_LAYOUT_BGR_DWORD_ALIGNED,
};


struct request_info
{
	request_info(const PICTURE_LAYOUT _picture_layout = PICTURE_LAYOUT_FILE_NAME_ONLY,
				 const int _PictureWidth = 0,
				 const int _PictureHeight = 0)
	  : m_picture_layout(_picture_layout),
		m_PictureWidth(_PictureWidth),
		m_PictureHeight(_PictureHeight)
	{
	};

	PICTURE_LAYOUT m_picture_layout;
	int m_PictureWidth;
	int m_PictureHeight;
};


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
	picture_data(const CString _name,
				 const int _PictureWidth = 0,
				 const int _PictureHeight = 0,
				 const int _OriginalPictureWidth = 0,
				 const int _OriginalPictureHeight = 0,
				 BYTE* _buf = NULL,
				 const int _len = 0)
	  : m_name(_name),
		m_PictureWidth(_PictureWidth),
		m_PictureHeight(_PictureHeight),
		m_OriginalPictureWidth(_OriginalPictureWidth),
		m_OriginalPictureHeight(_OriginalPictureHeight),
		m_buf(_buf),
		m_len(_len)
	{
	};

	CString m_name;
	int m_PictureWidth;
	int m_PictureHeight;
	int m_OriginalPictureWidth;
	int m_OriginalPictureHeight;
	BYTE* m_buf;
	int m_len;
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
	CString file_name_dll;
	CString info;
	CString version;
	enum PLUGIN_TYPE type;

	// Zum Sortieren der Einträge
	bool operator < (PluginData& rhs)
	{
		return file_name_dll < rhs.file_name_dll;
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
