#pragma once
#include "pictureformat.h"


class CPdfFormat : public CPictureFormat
{
protected:
	CPdfFormat()
		: border_size(10),
		separator_border_size(10)
	{
	};

public:
	virtual ~CPdfFormat() {};

	static CPictureFormat* __stdcall GetInstance()
	{
		return new CPdfFormat;
	};

	virtual void __stdcall get_size(const CString& FileName);
	virtual CString __stdcall get_ext();
	virtual struct PluginData __stdcall get_plugin_data();
	virtual CString __stdcall get_properties();
	virtual bool __stdcall properties_dlg(const HWND hwnd);

public:
	virtual BYTE* __stdcall FileToRGB(const CString& FileName,
		const int abs_size_x = 0, const int abs_size_y = 0,
		const int rel_size_z = 0, const int rel_size_n = 0,
		const enum scaling_type picture_scaling_type = scaling_type::scaling_type_none,
		const bool b_scan = true);

	virtual CString __stdcall get_info(const CString& FileName, const enum info_type _info_type);
	virtual unsigned int __stdcall get_cap() const;

protected:
	static CString m_property_str;

public:
	int border_size;
	int separator_border_size;
};
