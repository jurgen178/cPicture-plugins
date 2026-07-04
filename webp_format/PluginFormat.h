#pragma once
#include "pictureformat.h"

enum PLUGIN_TYPE
{
	PLUGIN_TYPE_NONE = 0x0000,
	PLUGIN_TYPE_FORMAT = 0x0001,
	PLUGIN_TYPE_FUNCTION = 0x0002,
};

struct plugin_data
{
public:
	plugin_data() noexcept : type(PLUGIN_TYPE::PLUGIN_TYPE_NONE) {};

	CString name;
	CString desc;
	CString file_name;
	CString file_name2;
	CString info;
	CString version;
	enum PLUGIN_TYPE type;

	bool operator < (const plugin_data& rhs) noexcept
	{
		return file_name2 < rhs.file_name2;
	}
};

class CWebPFormat : public CPictureFormat
{
public:
	CWebPFormat() {};
	virtual ~CWebPFormat() {};

public:
	static const CString type;
	static CString m_property_str;
	virtual const CString __stdcall getType() const
	{
		return type;
	}

	static CPictureFormat* __stdcall GetInstance()
	{
		return new CWebPFormat;
	};

	virtual CString __stdcall get_ext() const;
	virtual struct plugin_data __stdcall get_plugin_data() const;
	virtual unsigned int __stdcall get_cap() const;
	virtual bool __stdcall properties_dlg(const HWND hwnd);
	virtual void __stdcall set_properties(const CString& property_str);
	virtual CString __stdcall get_properties() const;

	virtual bool __stdcall RGBToFile(const CString& FileName,
		const BYTE* dataBuf,
		const int width,
		const int height,
		const int quality_L = -1,
		const int quality_C = -1,
		const int jpeg_lossless = -1);

	virtual BYTE* __stdcall FileToRGB(const CString& FileName,
		const int abs_size_x = 0, const int abs_size_y = 0,
		const int rel_size_z = 0, const int rel_size_n = 0,
		const enum scaling_type picture_scaling_type = scaling_type::scaling_type_none,
		const bool b_scan = true);

	virtual CString __stdcall get_info(const CString& FileName, const enum info_type _info_type);

protected:
	void get_size(const CString& FileName);
};
