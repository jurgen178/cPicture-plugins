#pragma once
#include "pictureformat.h"


class CPluginFormat : public CPictureFormat
{
protected:
	CPluginFormat();

public:
	virtual ~CPluginFormat() {};

protected:
	CStringA get_utf8_file_name(const CString& FileName);

public:
	virtual bool __stdcall RGBToFile(const CString& FileName,
		const BYTE* dataBuf,
		const int width,
		const int height,
		const int quality_L = -1,
		const int quality_C = -1,
		const int jpeg_lossless = -1) = 0;

	virtual BYTE* __stdcall FileToRGB(const CString& FileName,
		const int abs_size_x = 0, const int abs_size_y = 0,
		const int rel_size_z = 0, const int rel_size_n = 0,
		const enum scaling_type picture_scaling_type = scaling_type::scaling_type_none,
		const bool b_scan = true) = 0;

	virtual void __stdcall get_size(const CString& FileName) = 0;
	virtual CString __stdcall get_info(const CString& FileName, const enum info_type _info_type);
	virtual unsigned int __stdcall get_cap() const;
};

class CTiffFormat : public CPluginFormat
{
public:
	CTiffFormat() {};

public:
	virtual ~CTiffFormat() {};

protected:
	static CString m_property_str;

public:
	static const CString type;
	virtual const CString __stdcall getType() const
	{
		return type;
	}

	static CPictureFormat* __stdcall GetInstance()
	{
		return new CTiffFormat;
	};

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

	virtual void __stdcall get_size(const CString& FileName);
	virtual CString __stdcall get_ext() const;
	virtual struct plugin_data __stdcall get_plugin_data() const;

	virtual bool __stdcall properties_dlg(const HWND hwnd);
};


class CPngFormat : public CPluginFormat
{
public:
	CPngFormat() {};

public:
	virtual ~CPngFormat() {};

protected:
	static CString m_property_str;

public:
	static const CString type;
	virtual const CString __stdcall getType() const
	{
		return type;
	}

	static CPictureFormat* __stdcall GetInstance()
	{
		return new CPngFormat;
	};

	virtual bool __stdcall RGBToFile(const CString& FileName,
		const BYTE* dataBuf,
		const int width,
		const int height,
		const int quality_L = -1,
		const int quality_C = -1,
		const int jpeg_lossless = -1);

	//virtual BYTE* __stdcall FileToPreview(const CString& FileName, int& len, int& size_x, int& size_y, const bool bScanAudio = false, const bool bMaxSize = false);

	virtual BYTE* __stdcall FileToRGB(const CString& FileName,
		const int abs_size_x = 0, const int abs_size_y = 0,
		const int rel_size_z = 0, const int rel_size_n = 0,
		const enum scaling_type picture_scaling_type = scaling_type::scaling_type_none,
		const bool b_scan = true);

	virtual void __stdcall get_size(const CString& FileName);
	virtual CString __stdcall get_ext() const;
	virtual struct plugin_data __stdcall get_plugin_data() const;

	virtual bool __stdcall properties_dlg(const HWND hwnd);
};
