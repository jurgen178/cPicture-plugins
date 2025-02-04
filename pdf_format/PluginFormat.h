#pragma once
#include "pictureformat.h"
#include "include/fpdfview.h"
//#include "include/fpdf_thumbnail.h"


class CPdfFormat : public CPictureFormat
{
protected:
	CPdfFormat();

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
	static CString m_property_str;
	static int max_picture_x;
	static int max_picture_y;
	static int scaling;
	static int page_range_from;
	static int page_range_to;
	static int border_size;
	static int border_color;
	static int separator_border_color;

protected:
	int start_page;
	int end_page;
	int page_count;
	int pdf_page_width;
	int pdf_page_height;
	int num_cols;
	int num_rows;
	int border_size_pdf;
	int separator_border_size_pdf;

protected:
	void reset_properties();
	void validate_properties();
	BYTE* ReadFile(const CString& FileName,
		const int size_x, const int size_y,
		const int rel_size_z = 1, const int rel_size_n = 1);
	void get_page_range(FPDF_DOCUMENT document, int& start, int& end);
	BYTE* convert_to_rgb(FPDF_BITMAP rgba_bitmap);
	CStringA get_utf8_file_name(const CString& FileName);
	FPDF_BITMAP get_pages(FPDF_DOCUMENT document,
		const int abs_size_x = 0, const int abs_size_y = 0,
		const int rel_size_z = 1, const int rel_size_n = 1);
	void update_page_sizes(FPDF_DOCUMENT document,
		const int abs_size_x = 0, const int abs_size_y = 0,
		const int rel_size_z = 1, const int rel_size_n = 1);
};
