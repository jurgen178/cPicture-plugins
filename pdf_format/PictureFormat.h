#pragma once

#include "vector"
using namespace std;


enum info_type
{
	info_type_std = 0x0001,
	info_type_short = 0x0002,
	info_type_date = 0x0004,
	info_type_size = 0x0008,
	info_type_detail = 0x0010,
};

enum info_type operator|(const enum info_type t1, const enum info_type t2);
enum info_type operator&(const enum info_type t1, const enum info_type t2);
enum info_type operator|=(enum info_type& t1, const enum info_type t2);


enum scaling_type
{
	scaling_type_none = 0x0000,
	scaling_type_upscaling = 0x0001,
	scaling_type_keep_aspect_ratio = 0x0002,
	scaling_type_not_keep_aspect_ratio = 0x0004,
};

enum scaling_type operator|(const enum scaling_type t1, const enum scaling_type t2);
enum scaling_type operator&(const enum scaling_type t1, const enum scaling_type t2);


constexpr unsigned int PICTURE_READ = 0x00000001;			// Plugin can read the picture format
constexpr unsigned int PICTURE_WRITE = 0x00000002;			// Plugin can write the picture format
constexpr unsigned int PICTURE_QUALITY = 0x00000004;		// Plugin supports different quality levels (for example, JPEG has quality levels from 0 to 100)

constexpr unsigned int PICTURE_AUTO_ROTATE = 0x00000008;	// Plugin supports transformations
constexpr unsigned int PICTURE_ROTATERIGHT = 0x00000010;
constexpr unsigned int PICTURE_ROTATELEFT = 0x00000020;
constexpr unsigned int PICTURE_ROTATE180 = 0x00000040;
constexpr unsigned int PICTURE_FLIPH = 0x00000080;
constexpr unsigned int PICTURE_FLIPV = 0x00000100;
constexpr unsigned int PICTURE_TRANSPOSE = 0x00000200;
constexpr unsigned int PICTURE_TRANSVERSE = 0x00000400;
constexpr unsigned int PICTURE_GRAYSCALE = 0x00000800;
constexpr unsigned int PICTURE_CROP = 0x00001000;
constexpr unsigned int PICTURE_EXIF_READ = 0x00002000;		// Plugin can read EXIF
constexpr unsigned int PICTURE_EXIF_WRITE = 0x00004000;		// Plugin can write EXIF
constexpr unsigned int PICTURE_JPEG_STRUCTURE = 0x00008000;	// Plugin supports JPEG structure display
constexpr unsigned int PICTURE_COMMENT = 0x00010000;		// Plugin supports comments
constexpr unsigned int PICTURE_ORIENTATION = 0x00020000;	// Plugin supports an orientation flag
constexpr unsigned int PICTURE_GPS = 0x00040000;			// Plugin supports GPS data


struct GPSdata
{
	GPSdata()
		: LatitudeRef(L'\0'),
		LongitudeRef(L'\0'),
		fLat(0.0),
		fLong(0.0),
		fAltitude(0.0),
		GPSFormat(-1)
	{
	};

	bool Parse(CString data, bool bShowError = true);
	bool Parse(const unsigned int gpsdata[32]);
	CString ToString();

	void SetLat(const double _fLat);
	void SetLong(const double _fLong);
	void SetLat(const WCHAR _LatRef, const double _fLat);
	void SetLong(const WCHAR _LongRef, const double _fLong);
	void SetLat(const WCHAR _LatRef, const CString _Lat);
	void SetLong(const WCHAR _LongRef, const CString _Long);
	void SetAltitude(const CString _Altitude);

	bool empty() const
	{
		return !(LatitudeRef != L'\0' && Latitude.GetLength() && LongitudeRef != L'\0' && Longitude.GetLength());
	};

	int GPSFormat;

	WCHAR LatitudeRef;		// North/South Latitude
	CString Latitude;		// Latitude
	double fLat;

	WCHAR LongitudeRef;		// East/West Longitude
	CString Longitude;		// Longitude
	double fLong;

	double fAltitude;
};


class CPictureFormat
{
protected:
	CPictureFormat()
		: m_PictureWidth(0),
		m_PictureHeight(0),
		m_OriginalPictureWidth(0),
		m_OriginalPictureHeight(0),
		m_h_samp_factor8(1),
		m_v_samp_factor8(1),
		m_Orientation(-1),
		m_jpeg_streamsize(0),
		m_color_space(0),
		m_bExternalLibMem(true),
		m_bIsValid(false),
		m_bPanDispMode(false),
		m_low_quality(false),
		m_bAudio(false),
		m_bVideo(false),
		m_bIPTC(false),
		m_bXMP(false),
		m_bColorProfile(false),
		m_bCFlag(false),
		m_bJFXX(true),
		m_bUseColorProfile(false),
		m_fAperture(0.0),
		m_Shutterspeed(0),
		m_ISO(0),
		m_bLossless(false)
	{
		m_exiftime.dwLowDateTime = m_exiftime.dwHighDateTime = 0;
	};

public:
	virtual ~CPictureFormat() { };

public:
	int m_PictureWidth;
	int m_PictureHeight;

	int	m_OriginalPictureWidth;
	int m_OriginalPictureHeight;

	int	m_h_samp_factor8;
	int m_v_samp_factor8;

	int m_Orientation;

	int m_jpeg_streamsize;
	int m_color_space;
	CString m_ErrorMsg;
	bool m_bExternalLibMem;

	bool m_bIsValid;
	bool m_bPanDispMode;
	bool m_low_quality;
	bool m_bLossless;

	bool m_bAudio;
	bool m_bVideo;
	bool m_bIPTC;
	bool m_bXMP;
	bool m_bColorProfile;
	bool m_bCFlag;
	bool m_bJFXX;
	bool m_bUseColorProfile;

	GPSdata m_GPSdata;
	FILETIME m_exiftime;
	float m_fAperture;
	int m_Shutterspeed;
	int m_ISO;
	CString m_ExifDateTime_display;
	CString m_LongExifDateTime;
	CString m_Model;
	CString m_ExifComment;
	CString m_Comment;
	CString m_iptc;
	CString m_xmp;
	CString m_Lens;

protected:
	vector<pair<CString, CString> > m_exiflist;

public:
	virtual const CString __stdcall getType() const { return L""; };
	virtual CString __stdcall get_ext() const = 0;
	virtual struct plugin_data __stdcall get_plugin_data() const = 0;
	virtual unsigned int __stdcall get_cap() const = 0;

	virtual void __stdcall set_properties(const CString& property_str) { };
	virtual CString __stdcall get_properties() const { return L""; };
	virtual bool __stdcall properties_dlg(const HWND hwnd) { return false; };

	virtual bool __stdcall IsPanorama() const
	{
		if (!m_bPanDispMode || m_PictureWidth == 0 || m_PictureHeight == 0)
			return false;

		const int s1 = m_PictureWidth / m_PictureHeight;
		const int s2 = m_PictureHeight / m_PictureWidth;

		const int s12 = max(s1, s2);

		return s12 >= 2;
	};

	virtual CString __stdcall get_info(const CString& FileName, const enum info_type _info_type = info_type_std) { return L""; };
	virtual bool __stdcall GetAudioData(const CString& FileName, BYTE*& pAudioBuf, int& size) { return false; };
	virtual bool __stdcall GetColorProfile(const CString& FileName, BYTE*& pProfile, int& size) { return false; };
	virtual bool __stdcall set_comment(const CString& FileName, const CString& comment) { return false; };
	virtual CString __stdcall GetWriteExtension(const CString& file) { return file; };

	virtual bool __stdcall SetGPSData(const CString& FileName, const struct GPSdata& data) { return false; };
	virtual struct GPSdata __stdcall GetGPSData(const CString& FileName) { return struct GPSdata(); };

	virtual BYTE* __stdcall FileToRGB(const CString& FileName,
		const int abs_size_x = 0, const int abs_size_y = 0,
		const int rel_size_z = 0, const int rel_size_n = 0,
		const enum scaling_type picture_scaling_type = scaling_type::scaling_type_none,
		const bool b_scan = true)
	{
		return NULL;
	};
	virtual BYTE* __stdcall FileToPreview(const CString& FileName, int& len, int& size_x, int& size_y, const bool bScanAudio = false, const bool bMaxSize = false) { return NULL; };
	virtual BYTE* __stdcall MemToRGB(const BYTE* buf, const int bufsize,
		const int abs_size_x = 0, const int abs_size_y = 0,
		const int rel_size_z = 0, const int rel_size_n = 0,
		const enum scaling_type picture_scaling_type = scaling_type::scaling_type_none,
		const bool b_scan = true)
	{
		return NULL;
	};

	virtual bool __stdcall RGBToFile(const CString& FileName,
		const BYTE* dataBuf,
		const int width,
		const int height,
		const int quality_L = -1,
		const int quality_C = -1,
		const int jpeg_lossless = -1) // -1: Einstellung verwenden, 0: kein lossless, 1: lossless
	{
		return false;
	};

	virtual bool __stdcall RotateLeft(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly) { return false; };
	virtual bool __stdcall RotateRight(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly) { return false; };
	virtual bool __stdcall Rotate180(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly) { return false; };
	virtual bool __stdcall FlipH(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly) { return false; };
	virtual bool __stdcall FlipV(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly) { return false; };
	virtual bool __stdcall Transpose(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly) { return false; };
	virtual bool __stdcall Transverse(const CString& inFileName, const bool bModifyPreview, const bool bModifyPreviewOnly) { return false; };
	virtual bool __stdcall GrayScale(const CString& inFileName, const bool bModifyPreview) { return false; };
	virtual bool __stdcall AutoRotate(const CString& inFileName, const bool bModifyPreview) { return false; };
	virtual int __stdcall SetOrientationFlag(const CString& inFileName, const int orientation = 1) { return -1; };
	virtual int __stdcall GetOrientationFlag(const CString& inFileName) { return -1; };
	virtual bool __stdcall Crop(const CString& inFileName, const bool bModifyPreview,
		const int x, const int y, const int b, const int h) { return false;	};
	virtual bool __stdcall Crop(const CString& inFileName, const CString& outFileName, const bool bModifyPreview,
		const int x, const int y, const int b, const int h) { return false; }
	virtual vector<pair<CString, CString> >& __stdcall GetExifList(const CString& FileName) { return m_exiflist; };
};


typedef CPictureFormat* (__stdcall *lpfnFormatGetInstanceProc)();
