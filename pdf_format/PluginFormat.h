#pragma once
#include "pictureformat.h"
#include "include/fpdfview.h"
#include "include/fpdf_formfill.h"


class CPdfFormat : public CPictureFormat
{
protected:
	CPdfFormat();

protected:
	static CString m_property_str;
	static int m_compression_type;

public:
	virtual ~CPdfFormat();

	static CPictureFormat* __stdcall GetInstance()
	{
		return new CPdfFormat;
	};

	virtual void __stdcall get_size(const CString& FileName);
	virtual CString __stdcall get_ext();
	virtual struct PluginData __stdcall get_plugin_data();

	virtual void __stdcall set_properties(const CString& property_str);
	virtual CString __stdcall get_properties();
	virtual bool __stdcall properties_dlg(const HWND hwnd);

public:
	virtual BYTE* __stdcall FileToPreview(const CString& FileName, int& len, int& size_x, int& size_y, const bool bScanAudio = false, const bool bMaxSize = false);
	virtual BYTE* __stdcall FileToRGB(const CString& FileName,
		const int abs_size_x = 0, const int abs_size_y = 0,
		const int rel_size_z = 0, const int rel_size_n = 0,
		const enum scaling_type picture_scaling_type = scaling_type::scaling_type_none,
		const bool b_scan = true);

	virtual CString __stdcall get_info(const CString& FileName, const enum info_type _info_type);
	virtual unsigned int __stdcall get_cap() const;

protected:
	CStringA get_file_name(const CString& FileName);
	FPDF_BITMAP get_first_page(FPDF_DOCUMENT document,
		FPDF_FORMHANDLE form,
		const int abs_size_x, const int abs_size_y);
	FPDF_BITMAP get_all_pages(FPDF_DOCUMENT document,
		FPDF_FORMHANDLE form,
		const int abs_size_x, const int abs_size_y);

public:
	int border_size;
	int separator_border_size;
};
