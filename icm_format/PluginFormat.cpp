#include "stdafx.h"
#include "resource.h"
#include "pluginformat.h"
#include "../shared/FormatPluginEnumOperators.h"
#include "../shared/FormatPluginHelpers.h"
#include "lcms2.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <mutex>
#include <vector>
using namespace std;

const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
#ifdef _DEBUG
	return L"1.1-debug";
#else
	return L"1.1";
#endif
}

const PLUGIN_TYPE __stdcall GetPluginType()
{
	return PLUGIN_TYPE_FORMAT;
}

const int __stdcall GetPluginInit()
{
	return 1;
}

lpfnFormatGetInstanceProc __stdcall GetPluginProc(const int k)
{
	return k == 0 ? CICMFormat::GetInstance : NULL;
}

const CString CICMFormat::type = L"ICM";

struct PointD
{
	double x;
	double y;
};

struct CmfRow
{
	int wavelength;
	double xBar;
	double yBar;
	double zBar;
};

struct ProfilePreviewData
{
	CString description;
	CString manufacturer;
	CString model;
	CString copyright;
	CString deviceClass;
	CString colorSpace;
	CString pcs;
	CString version;
	CString renderingIntent;
	CString cmm;
	CString creator;
	CString created;
	CString fileSize;
	PointD red;
	PointD green;
	PointD blue;
	PointD white;
	bool hasRgbColorants;
	bool hasWhitePoint;

	ProfilePreviewData()
		: red{ 0.0, 0.0 }, green{ 0.0, 0.0 }, blue{ 0.0, 0.0 }, white{ 0.0, 0.0 }, hasRgbColorants(false), hasWhitePoint(false)
	{
	}
};

vector<BYTE> ReadFileData(const CString& FileName, CString& errorMsg)
{
	vector<BYTE> fileData;
	const __int64 fileSize = GetFileSize64(FileName);
	if (fileSize <= 0)
	{
		errorMsg.Format(L"Invalid ICC profile size: %I64d bytes", fileSize);
		return fileData;
	}

	FILE* fp = NULL;
	const errno_t err(_wfopen_s(&fp, FileName, L"rb"));
	if (err != 0 || !fp)
	{
		errorMsg.Format(L"Error open file '%s' for reading", FileName);
		return fileData;
	}

	fileData.resize(static_cast<size_t>(fileSize));
	const size_t readBytes = fread(fileData.data(), 1, fileData.size(), fp);
	fclose(fp);

	if (readBytes != fileData.size())
	{
		errorMsg.Format(L"Error reading ICC profile '%s'", FileName);
		fileData.clear();
	}

	return fileData;
}

CString SignatureToString(const cmsUInt32Number signature)
{
	CString text;
	text.Format(L"%c%c%c%c",
		static_cast<wchar_t>((signature >> 24) & 0xff),
		static_cast<wchar_t>((signature >> 16) & 0xff),
		static_cast<wchar_t>((signature >> 8) & 0xff),
		static_cast<wchar_t>(signature & 0xff));
	return text;
}

CString SignatureName(const cmsUInt32Number signature)
{
	switch (signature)
	{
	case cmsSigInputClass: return L"Input device";
	case cmsSigDisplayClass: return L"Display device";
	case cmsSigOutputClass: return L"Output device";
	case cmsSigLinkClass: return L"Device link";
	case cmsSigAbstractClass: return L"Abstract";
	case cmsSigColorSpaceClass: return L"Color space";
	case cmsSigNamedColorClass: return L"Named color";
	case cmsSigRgbData: return L"RGB";
	case cmsSigGrayData: return L"Gray";
	case cmsSigCmykData: return L"CMYK";
	case cmsSigLabData: return L"Lab";
	case cmsSigXYZData: return L"XYZ";
	case cmsSigYCbCrData: return L"YCbCr";
	default:
		return SignatureToString(signature);
	}
}

CString GetProfileText(cmsHPROFILE profile, const cmsInfoType infoType)
{
	wchar_t buffer[512] = { 0 };
	cmsUInt32Number chars = cmsGetProfileInfo(profile, infoType, "en", "US", buffer, static_cast<cmsUInt32Number>(sizeof(buffer)));
	if (chars == 0 || buffer[0] == 0)
	{
		chars = cmsGetProfileInfo(profile, infoType, "de", "DE", buffer, static_cast<cmsUInt32Number>(sizeof(buffer)));
	}
	if (chars == 0 || buffer[0] == 0)
	{
		chars = cmsGetProfileInfo(profile, infoType, NULL, NULL, buffer, static_cast<cmsUInt32Number>(sizeof(buffer)));
	}

	return buffer;
}

bool XYZToXY(const cmsCIEXYZ* xyz, PointD& point)
{
	if (!xyz)
	{
		return false;
	}

	const double sum = xyz->X + xyz->Y + xyz->Z;
	if (sum <= 0.0)
	{
		return false;
	}

	point.x = xyz->X / sum;
	point.y = xyz->Y / sum;
	return point.x >= 0.0 && point.y >= 0.0;
}

CString FormatDate(const struct tm& dateTime)
{
	CString text;
	text.Format(L"%04d-%02d-%02d %02d:%02d:%02d", dateTime.tm_year, dateTime.tm_mon, dateTime.tm_mday, dateTime.tm_hour, dateTime.tm_min, dateTime.tm_sec);
	return text;
}

CString FormatVersion(const double version)
{
	CString text;
	text.Format(L"%.2f", version);
	return text;
}

CString FormatHexSignature(const cmsUInt32Number value)
{
	CString text;
	text.Format(L"%s (0x%08X)", SignatureToString(value), value);
	return text;
}

bool LoadProfileData(const CString& FileName, ProfilePreviewData& data, CString& errorMsg)
{
	const vector<BYTE> fileData = ReadFileData(FileName, errorMsg);
	if (fileData.empty())
	{
		return false;
	}

	cmsHPROFILE profile = cmsOpenProfileFromMem(fileData.data(), static_cast<cmsUInt32Number>(fileData.size()));
	if (!profile)
	{
		errorMsg = L"Little CMS could not open this ICC/ICM profile";
		return false;
	}

	data.description = GetProfileText(profile, cmsInfoDescription);
	data.manufacturer = GetProfileText(profile, cmsInfoManufacturer);
	data.model = GetProfileText(profile, cmsInfoModel);
	data.copyright = GetProfileText(profile, cmsInfoCopyright);
	data.deviceClass = SignatureName(cmsGetDeviceClass(profile));
	data.colorSpace = SignatureName(cmsGetColorSpace(profile));
	data.pcs = SignatureName(cmsGetPCS(profile));
	data.version = FormatVersion(cmsGetProfileVersion(profile));
	data.renderingIntent.Format(L"%u", cmsGetHeaderRenderingIntent(profile));
	data.cmm = FormatHexSignature(cmsGetHeaderCMM(profile));
	data.creator = FormatHexSignature(cmsGetHeaderCreator(profile));
	data.fileSize = GetSizeStr(static_cast<__int64>(fileData.size()));

	struct tm created = { 0 };
	if (cmsGetHeaderCreationDateTime(profile, &created))
	{
		data.created = FormatDate(created);
	}

	const cmsCIEXYZ* red = static_cast<const cmsCIEXYZ*>(cmsReadTag(profile, cmsSigRedColorantTag));
	const cmsCIEXYZ* green = static_cast<const cmsCIEXYZ*>(cmsReadTag(profile, cmsSigGreenColorantTag));
	const cmsCIEXYZ* blue = static_cast<const cmsCIEXYZ*>(cmsReadTag(profile, cmsSigBlueColorantTag));
	data.hasRgbColorants = XYZToXY(red, data.red) && XYZToXY(green, data.green) && XYZToXY(blue, data.blue);

	const cmsCIEXYZ* white = static_cast<const cmsCIEXYZ*>(cmsReadTag(profile, cmsSigMediaWhitePointTag));
	data.hasWhitePoint = XYZToXY(white, data.white);

	cmsCloseProfile(profile);
	return true;
}

CPoint PlotPoint(const CRect& rect, const PointD& point)
{
	const double xMax = 0.8;
	const double yMax = 0.9;
	const int x = rect.left + static_cast<int>((point.x / xMax) * rect.Width() + 0.5);
	const int y = rect.bottom - static_cast<int>((point.y / yMax) * rect.Height() + 0.5);
	return CPoint(x, y);
}

double SmoothInterpolateCmf(const CmfRow* rows, const int rowCount, const int wavelength, const int channel)
{
	int index = 0;
	while (index < rowCount && rows[index].wavelength < wavelength)
	{
		++index;
	}

	const int startIndex = max(0, min(index - 2, rowCount - 4));
	double value = 0.0;
	for (int i = 0; i < 4; ++i)
	{
		const int rowIndex = startIndex + i;
		const double xi = rows[rowIndex].wavelength;
		double term = channel == 0 ? rows[rowIndex].xBar : channel == 1 ? rows[rowIndex].yBar : rows[rowIndex].zBar;
		for (int j = 0; j < 4; ++j)
		{
			if (i == j)
			{
				continue;
			}
			const double xj = rows[startIndex + j].wavelength;
			term *= (wavelength - xj) / (xi - xj);
		}
		value += term;
	}

	return max(0.0, value);
}

vector<PointD> BuildSpectralLocus()
{
	static const CmfRow cmfRows[] = {
		{ 380, 0.001368000000, 0.000039000000, 0.006450001000 }, { 385, 0.002236000000, 0.000064000000, 0.010549990000 },
		{ 390, 0.004243000000, 0.000120000000, 0.020050010000 }, { 395, 0.007650000000, 0.000217000000, 0.036210000000 },
		{ 400, 0.014310000000, 0.000396000000, 0.067850010000 }, { 405, 0.023190000000, 0.000640000000, 0.110200000000 },
		{ 410, 0.043510000000, 0.001210000000, 0.207400000000 }, { 415, 0.077630000000, 0.002180000000, 0.371300000000 },
		{ 420, 0.134380000000, 0.004000000000, 0.645600000000 }, { 425, 0.214770000000, 0.007300000000, 1.039050100000 },
		{ 430, 0.283900000000, 0.011600000000, 1.385600000000 }, { 435, 0.328500000000, 0.016840000000, 1.622960000000 },
		{ 440, 0.348280000000, 0.023000000000, 1.747060000000 }, { 445, 0.348060000000, 0.029800000000, 1.782600000000 },
		{ 450, 0.336200000000, 0.038000000000, 1.772110000000 }, { 455, 0.318700000000, 0.048000000000, 1.744100000000 },
		{ 460, 0.290800000000, 0.060000000000, 1.669200000000 }, { 465, 0.251100000000, 0.073900000000, 1.528100000000 },
		{ 470, 0.195360000000, 0.090980000000, 1.287640000000 }, { 475, 0.142100000000, 0.112600000000, 1.041900000000 },
		{ 480, 0.095640000000, 0.139020000000, 0.812950100000 }, { 485, 0.057950010000, 0.169300000000, 0.616200000000 },
		{ 490, 0.032010000000, 0.208020000000, 0.465180000000 }, { 495, 0.014700000000, 0.258600000000, 0.353300000000 },
		{ 500, 0.004900000000, 0.323000000000, 0.272000000000 }, { 505, 0.002400000000, 0.407300000000, 0.212300000000 },
		{ 510, 0.009300000000, 0.503000000000, 0.158200000000 }, { 515, 0.029100000000, 0.608200000000, 0.111700000000 },
		{ 520, 0.063270000000, 0.710000000000, 0.078249990000 }, { 525, 0.109600000000, 0.793200000000, 0.057250010000 },
		{ 530, 0.165500000000, 0.862000000000, 0.042160000000 }, { 535, 0.225749900000, 0.914850100000, 0.029840000000 },
		{ 540, 0.290400000000, 0.954000000000, 0.020300000000 }, { 545, 0.359700000000, 0.980300000000, 0.013400000000 },
		{ 550, 0.433449900000, 0.994950100000, 0.008749999000 }, { 555, 0.512050100000, 1.000000000000, 0.005749999000 },
		{ 560, 0.594500000000, 0.995000000000, 0.003900000000 }, { 565, 0.678400000000, 0.978600000000, 0.002749999000 },
		{ 570, 0.762100000000, 0.952000000000, 0.002100000000 }, { 575, 0.842500000000, 0.915400000000, 0.001800000000 },
		{ 580, 0.916300000000, 0.870000000000, 0.001650001000 }, { 585, 0.978600000000, 0.816300000000, 0.001400000000 },
		{ 590, 1.026300000000, 0.757000000000, 0.001100000000 }, { 595, 1.056700000000, 0.694900000000, 0.001000000000 },
		{ 600, 1.062200000000, 0.631000000000, 0.000800000000 }, { 605, 1.045600000000, 0.566800000000, 0.000600000000 },
		{ 610, 1.002600000000, 0.503000000000, 0.000340000000 }, { 615, 0.938400000000, 0.441200000000, 0.000240000000 },
		{ 620, 0.854449900000, 0.381000000000, 0.000190000000 }, { 625, 0.751400000000, 0.321000000000, 0.000100000000 },
		{ 630, 0.642400000000, 0.265000000000, 0.000049999990 }, { 635, 0.541900000000, 0.217000000000, 0.000030000000 },
		{ 640, 0.447900000000, 0.175000000000, 0.000020000000 }, { 645, 0.360800000000, 0.138200000000, 0.000010000000 },
		{ 650, 0.283500000000, 0.107000000000, 0.000000000000 }, { 655, 0.218700000000, 0.081600000000, 0.000000000000 },
		{ 660, 0.164900000000, 0.061000000000, 0.000000000000 }, { 665, 0.121200000000, 0.044580000000, 0.000000000000 },
		{ 670, 0.087400000000, 0.032000000000, 0.000000000000 }, { 675, 0.063600000000, 0.023200000000, 0.000000000000 },
		{ 680, 0.046770000000, 0.017000000000, 0.000000000000 }, { 685, 0.032900000000, 0.011920000000, 0.000000000000 },
		{ 690, 0.022700000000, 0.008210000000, 0.000000000000 }, { 695, 0.015840000000, 0.005723000000, 0.000000000000 },
		{ 700, 0.011359160000, 0.004102000000, 0.000000000000 }, { 705, 0.008110916000, 0.002929000000, 0.000000000000 },
		{ 710, 0.005790346000, 0.002091000000, 0.000000000000 }, { 715, 0.004109457000, 0.001484000000, 0.000000000000 },
		{ 720, 0.002899327000, 0.001047000000, 0.000000000000 }, { 725, 0.002049190000, 0.000740000000, 0.000000000000 },
		{ 730, 0.001439971000, 0.000520000000, 0.000000000000 }, { 735, 0.000999949300, 0.000361100000, 0.000000000000 },
		{ 740, 0.000690078600, 0.000249200000, 0.000000000000 }, { 745, 0.000476021300, 0.000171900000, 0.000000000000 },
		{ 750, 0.000332301100, 0.000120000000, 0.000000000000 }, { 755, 0.000234826100, 0.000084800000, 0.000000000000 },
		{ 760, 0.000166150500, 0.000060000000, 0.000000000000 }, { 765, 0.000117413000, 0.000042400000, 0.000000000000 },
		{ 770, 0.000083075270, 0.000030000000, 0.000000000000 }, { 775, 0.000058706520, 0.000021200000, 0.000000000000 },
		{ 780, 0.000041509940, 0.000014990000, 0.000000000000 }
	};

	vector<PointD> locus;
	locus.reserve(401);
	for (int wavelength = 380; wavelength <= 780; ++wavelength)
	{
		const double xBar = SmoothInterpolateCmf(cmfRows, _countof(cmfRows), wavelength, 0);
		const double yBar = SmoothInterpolateCmf(cmfRows, _countof(cmfRows), wavelength, 1);
		const double zBar = SmoothInterpolateCmf(cmfRows, _countof(cmfRows), wavelength, 2);
		const double sum = xBar + yBar + zBar;
		if (sum > 0.0)
		{
			locus.push_back({ xBar / sum, yBar / sum });
		}
	}

	return locus;
}

const vector<PointD>& GetSpectralLocus()
{
	static const vector<PointD> locus = BuildSpectralLocus();
	return locus;
}

BYTE CompandSrgb(const double linear)
{
	const double value = linear <= 0.0031308 ? 12.92 * linear : 1.055 * pow(max(0.0, linear), 1.0 / 2.4) - 0.055;
	return static_cast<BYTE>(min(max(value * 0.95, 0.0), 1.0) * 255.0 + 0.5);
}

COLORREF XyToSrgb(const double x, const double y)
{
	if (y <= 0.0 || x < 0.0 || y < 0.0 || x + y > 1.0)
	{
		return RGB(255, 255, 255);
	}

	const double X = x / y;
	const double Y = 1.0;
	const double Z = (1.0 - x - y) / y;
	const double r = 3.2404542 * X - 1.5371385 * Y - 0.4985314 * Z;
	const double g = -0.9692660 * X + 1.8760108 * Y + 0.0415560 * Z;
	const double b = 0.0556434 * X - 0.2040259 * Y + 1.0572252 * Z;

	return RGB(CompandSrgb(r), CompandSrgb(g), CompandSrgb(b));
}

bool IsInsidePolygon(const vector<PointD>& polygon, const PointD& point)
{
	bool inside = false;
	for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++)
	{
		const PointD& a = polygon[i];
		const PointD& b = polygon[j];
		if (((a.y > point.y) != (b.y > point.y)) &&
			(point.x < (b.x - a.x) * (point.y - a.y) / (b.y - a.y) + a.x))
		{
			inside = !inside;
		}
	}

	return inside;
}

void DrawSpectralFill(CDC& dc, const CRect& rect, const vector<PointD>& locus)
{
	struct CachedSpectralBitmap
	{
		CachedSpectralBitmap() : width(0), height(0), bitmap(NULL) {}
		~CachedSpectralBitmap()
		{
			if (bitmap)
			{
				::DeleteObject(bitmap);
			}
		}

		int width;
		int height;
		HBITMAP bitmap;
	};

	static CachedSpectralBitmap cache;
	static std::mutex cacheMutex;
	std::lock_guard<std::mutex> lock(cacheMutex);

	const int width = rect.Width();
	const int height = rect.Height();
	if (cache.bitmap == NULL || cache.width != width || cache.height != height)
	{
		if (width <= 0 || height <= 0)
		{
			return;
		}
		if (cache.bitmap)
		{
			::DeleteObject(cache.bitmap);
			cache.bitmap = NULL;
			cache.width = 0;
			cache.height = 0;
		}

		BITMAPINFO bitmapInfo = { 0 };
		bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmapInfo.bmiHeader.biWidth = width;
		bitmapInfo.bmiHeader.biHeight = -height;
		bitmapInfo.bmiHeader.biPlanes = 1;
		bitmapInfo.bmiHeader.biBitCount = 32;
		bitmapInfo.bmiHeader.biCompression = BI_RGB;

		void* bits = NULL;
		HDC screen = ::GetDC(NULL);
		cache.bitmap = ::CreateDIBSection(screen, &bitmapInfo, DIB_RGB_COLORS, &bits, NULL, 0);
		::ReleaseDC(NULL, screen);
		if (!cache.bitmap || !bits)
		{
			return;
		}

		BYTE* pixels = static_cast<BYTE*>(bits);
		for (int pixelY = 0; pixelY < height; ++pixelY)
		{
			const double y = (height - pixelY - 0.5) / height * 0.9;
			for (int pixelX = 0; pixelX < width; ++pixelX)
			{
				const double x = (pixelX + 0.5) / width * 0.8;
				const PointD point = { x, y };
				const COLORREF color = IsInsidePolygon(locus, point) ? XyToSrgb(x, y) : RGB(255, 255, 255);
				const size_t offset = (static_cast<size_t>(pixelY) * width + pixelX) * 4;
				pixels[offset] = GetBValue(color);
				pixels[offset + 1] = GetGValue(color);
				pixels[offset + 2] = GetRValue(color);
				pixels[offset + 3] = 0;
			}
		}

		cache.width = width;
		cache.height = height;
	}

	CDC bitmapDc;
	bitmapDc.CreateCompatibleDC(&dc);
	HBITMAP oldBitmap = static_cast<HBITMAP>(bitmapDc.SelectObject(cache.bitmap));
	dc.BitBlt(rect.left, rect.top, width, height, &bitmapDc, 0, 0, SRCCOPY);
	bitmapDc.SelectObject(oldBitmap);
}

void DrawLine(CDC& dc, const CRect& rect, const PointD& a, const PointD& b, COLORREF color, const int width)
{
	CPen pen(PS_SOLID, width, color);
	CPen* oldPen = dc.SelectObject(&pen);
	dc.MoveTo(PlotPoint(rect, a));
	dc.LineTo(PlotPoint(rect, b));
	dc.SelectObject(oldPen);
}

void DrawLine(CDC& dc, const CRect& rect, const PointD& a, const PointD& b, COLORREF color, const int width, const int style)
{
	CPen pen(style, width, color);
	CPen* oldPen = dc.SelectObject(&pen);
	dc.MoveTo(PlotPoint(rect, a));
	dc.LineTo(PlotPoint(rect, b));
	dc.SelectObject(oldPen);
}

void DrawTriangle(CDC& dc, const CRect& rect, const PointD& red, const PointD& green, const PointD& blue, COLORREF color, const int width)
{
	DrawLine(dc, rect, red, green, color, width);
	DrawLine(dc, rect, green, blue, color, width);
	DrawLine(dc, rect, blue, red, color, width);
}

void DrawTriangle(CDC& dc, const CRect& rect, const PointD& red, const PointD& green, const PointD& blue, COLORREF color, const int width, const int style)
{
	DrawLine(dc, rect, red, green, color, width, style);
	DrawLine(dc, rect, green, blue, color, width, style);
	DrawLine(dc, rect, blue, red, color, width, style);
}

void DrawMarker(CDC& dc, const CRect& rect, const PointD& point, COLORREF color, const CString& label)
{
	const CPoint p = PlotPoint(rect, point);
	CBrush brush(color);
	CBrush* oldBrush = dc.SelectObject(&brush);
	CPen pen(PS_SOLID, 1, RGB(20, 20, 20));
	CPen* oldPen = dc.SelectObject(&pen);
	dc.Ellipse(p.x - 5, p.y - 5, p.x + 5, p.y + 5);
	dc.SelectObject(oldPen);
	dc.SelectObject(oldBrush);
	dc.TextOutW(p.x + 7, p.y - 8, label);
}

bool IsNearPoint(const PointD& a, const PointD& b, const double tolerance)
{
	const double dx = a.x - b.x;
	const double dy = a.y - b.y;
	return sqrt(dx * dx + dy * dy) <= tolerance;
}

void DrawWavelengthLabels(CDC& dc, const CRect& rect, const vector<PointD>& locus)
{
	CPen pen(PS_SOLID, 1, RGB(45, 45, 45));
	CPen* oldPen = dc.SelectObject(&pen);
	CFont wavelengthFont;
	wavelengthFont.CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
	CFont* oldFont = dc.SelectObject(&wavelengthFont);
	const CPoint center = PlotPoint(rect, { 0.33, 0.33 });
	vector<CRect> usedLabelRects;
	for (int wavelength = 380; wavelength <= 780; wavelength += 20)
	{
		const size_t index = static_cast<size_t>(wavelength - 380);
		if (index >= locus.size())
		{
			continue;
		}

		const CPoint p = PlotPoint(rect, locus[index]);
		double dx = static_cast<double>(p.x - center.x);
		double dy = static_cast<double>(p.y - center.y);
		const double length = sqrt(dx * dx + dy * dy);
		if (length > 0.0)
		{
			dx /= length;
			dy /= length;
		}

		const CPoint tickEnd(p.x + static_cast<int>(dx * 8.0 + 0.5), p.y + static_cast<int>(dy * 8.0 + 0.5));
		dc.MoveTo(p);
		dc.LineTo(tickEnd);

		CString label;
		label.Format(L"%d", wavelength);
		const int labelX = p.x + static_cast<int>(dx * 24.0 + 0.5);
		const int labelY = p.y + static_cast<int>(dy * 20.0 + 0.5);
		CRect labelRect(labelX - 24, labelY - 10, labelX + 24, labelY + 12);
		CRect testRect(labelRect);
		testRect.InflateRect(4, 2);

		bool overlaps = false;
		for (size_t i = 0; i < usedLabelRects.size(); ++i)
		{
			CRect intersection;
			if (intersection.IntersectRect(testRect, usedLabelRects[i]))
			{
				overlaps = true;
				break;
			}
		}
		if (overlaps)
		{
			continue;
		}

		usedLabelRects.push_back(testRect);
		dc.DrawTextW(label, labelRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}
	dc.SelectObject(oldFont);
	dc.SelectObject(oldPen);
}

void DrawCieDiagram(CDC& dc, const CRect& rect, const ProfilePreviewData& data)
{
	const vector<PointD>& locus = GetSpectralLocus();
	CPen gridPen(PS_DOT, 1, RGB(255, 255, 255));
	CPen axisPen(PS_SOLID, 1, RGB(70, 70, 70));
	CBrush whiteBrush(RGB(255, 255, 255));
	dc.FillRect(rect, &whiteBrush);
	DrawSpectralFill(dc, rect, locus);
	dc.Rectangle(rect);

	CPen* oldPen = dc.SelectObject(&gridPen);
	for (int i = 1; i < 8; ++i)
	{
		const int x = rect.left + i * rect.Width() / 8;
		dc.MoveTo(x, rect.top);
		dc.LineTo(x, rect.bottom);
	}
	for (int i = 1; i < 9; ++i)
	{
		const int y = rect.bottom - i * rect.Height() / 9;
		dc.MoveTo(rect.left, y);
		dc.LineTo(rect.right, y);
	}
	dc.SelectObject(&axisPen);
	dc.MoveTo(rect.left, rect.bottom);
	dc.LineTo(rect.right, rect.bottom);
	dc.MoveTo(rect.left, rect.top);
	dc.LineTo(rect.left, rect.bottom);
	dc.SelectObject(oldPen);

	CPen tickPen(PS_SOLID, 1, RGB(70, 70, 70));
	oldPen = dc.SelectObject(&tickPen);
	for (int i = 0; i <= 8; ++i)
	{
		const int x = rect.left + i * rect.Width() / 8;
		dc.MoveTo(x, rect.bottom);
		dc.LineTo(x, rect.bottom + 5);

		CString label;
		label.Format(L"%.1f", static_cast<double>(i) / 10.0);
		CRect labelRect(x - 18, rect.bottom + 7, x + 18, rect.bottom + 28);
		dc.DrawTextW(label, labelRect, DT_CENTER | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);
	}
	for (int i = 0; i <= 9; ++i)
	{
		const int y = rect.bottom - i * rect.Height() / 9;
		dc.MoveTo(rect.left - 5, y);
		dc.LineTo(rect.left, y);

		CString label;
		label.Format(L"%.1f", static_cast<double>(i) / 10.0);
		CRect labelRect(rect.left - 45, y - 10, rect.left - 8, y + 10);
		dc.DrawTextW(label, labelRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}
	dc.TextOutW(rect.right + 8, rect.bottom - 16, L"x");
	dc.TextOutW(rect.left - 62, rect.top - 10, L"y");
	dc.SelectObject(oldPen);

	CPen locusPen(PS_SOLID, 2, RGB(0, 0, 0));
	oldPen = dc.SelectObject(&locusPen);
	dc.MoveTo(PlotPoint(rect, locus[0]));
	for (size_t i = 1; i < locus.size(); ++i)
	{
		dc.LineTo(PlotPoint(rect, locus[i]));
	}
	dc.SelectObject(oldPen);
	DrawLine(dc, rect, locus[locus.size() - 1], locus[0], RGB(0, 0, 0), 2);
	DrawWavelengthLabels(dc, rect, locus);

	const PointD srgbR = { 0.6400, 0.3300 };
	const PointD srgbG = { 0.3000, 0.6000 };
	const PointD srgbB = { 0.1500, 0.0600 };
	const PointD p3R = { 0.6800, 0.3200 };
	const PointD p3G = { 0.2650, 0.6900 };
	const PointD p3B = { 0.1500, 0.0600 };
	const PointD adobeR = { 0.6400, 0.3300 };
	const PointD adobeG = { 0.2100, 0.7100 };
	const PointD adobeB = { 0.1500, 0.0600 };
	const PointD d65 = { 0.3127, 0.3290 };

	DrawTriangle(dc, rect, srgbR, srgbG, srgbB, RGB(255, 255, 255), 1, PS_DASH);
	DrawTriangle(dc, rect, p3R, p3G, p3B, RGB(0, 145, 255), 2);
	DrawTriangle(dc, rect, adobeR, adobeG, adobeB, RGB(255, 212, 0), 2);

	if (data.hasRgbColorants)
	{
		DrawTriangle(dc, rect, data.red, data.green, data.blue, RGB(230, 0, 0), 4);
		DrawMarker(dc, rect, data.red, RGB(245, 0, 0), L"R");
		DrawMarker(dc, rect, data.green, RGB(0, 185, 55), L"G");
		DrawMarker(dc, rect, data.blue, RGB(0, 90, 255), L"B");
	}
	if (data.hasWhitePoint && IsNearPoint(data.white, d65, 0.005))
	{
		DrawMarker(dc, rect, d65, RGB(250, 250, 250), L"D65/WP");
	}
	else
	{
		DrawMarker(dc, rect, d65, RGB(250, 250, 250), L"D65");
		if (data.hasWhitePoint)
		{
			DrawMarker(dc, rect, data.white, RGB(250, 250, 250), L"WP");
		}
	}

	dc.TextOutW(rect.left, rect.top - 25, L"CIE 1931 xy Chromaticity");
	dc.TextOutW(rect.left + 8, rect.bottom + 34, L"white dashed: sRGB   blue: Display P3   yellow: Adobe RGB   red: profile gamut");
	dc.TextOutW(rect.left + 8, rect.bottom + 56, L"D65: reference white   WP: profile media white point");
}

void AddInfoLine(vector<CString>& lines, const CString& label, const CString& value)
{
	if (value.GetLength() == 0)
	{
		return;
	}

	CString line;
	line.Format(L"%s: %s", label.GetString(), value.GetString());
	lines.push_back(line);
}

void DrawMetadata(CDC& dc, const CString& FileName, const ProfilePreviewData& data)
{
	vector<CString> lines;
	const WCHAR* fileTitle = wcsrchr(FileName, L'\\');
	fileTitle = fileTitle ? fileTitle + 1 : FileName.GetString();
	AddInfoLine(lines, L"File", fileTitle);
	AddInfoLine(lines, L"Description", data.description);
	AddInfoLine(lines, L"Class", data.deviceClass);
	AddInfoLine(lines, L"Color space", data.colorSpace);
	AddInfoLine(lines, L"PCS", data.pcs);
	AddInfoLine(lines, L"ICC version", data.version);
	AddInfoLine(lines, L"Rendering intent", data.renderingIntent);
	AddInfoLine(lines, L"Created", data.created);
	AddInfoLine(lines, L"Size", data.fileSize);
	AddInfoLine(lines, L"Manufacturer", data.manufacturer);
	AddInfoLine(lines, L"Model", data.model);
	AddInfoLine(lines, L"CMM", data.cmm);
	AddInfoLine(lines, L"Creator", data.creator);
	AddInfoLine(lines, L"Copyright", data.copyright);

	CRect textRect(40, 90, 470, 760);
	for (size_t i = 0; i < lines.size(); ++i)
	{
		CRect lineRect(textRect.left, textRect.top + static_cast<int>(i) * 38, textRect.right, textRect.top + static_cast<int>(i + 1) * 38);
		dc.DrawTextW(lines[i], lineRect, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
	}

	if (!data.hasRgbColorants)
	{
		CRect noteRect(40, 690, 470, 760);
		dc.DrawTextW(L"No RGB matrix colorants found. Complex LUT, CMYK and printer profiles are shown as metadata only.", noteRect, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);
	}
}

BYTE* RenderProfilePreview(const CString& FileName, const ProfilePreviewData& data, int& width, int& height, CString& errorMsg)
{
	width = 1200;
	height = 800;

	BITMAPINFO bitmapInfo = { 0 };
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = width;
	bitmapInfo.bmiHeader.biHeight = -height;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	void* dibBits = NULL;
	HDC screen = ::GetDC(NULL);
	HBITMAP dib = ::CreateDIBSection(screen, &bitmapInfo, DIB_RGB_COLORS, &dibBits, NULL, 0);
	::ReleaseDC(NULL, screen);
	if (!dib || !dibBits)
	{
		errorMsg = L"Could not create ICC preview bitmap";
		return NULL;
	}

	CDC dc;
	dc.CreateCompatibleDC(NULL);
	HBITMAP oldBitmap = static_cast<HBITMAP>(dc.SelectObject(dib));
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(RGB(40, 40, 40));

	CBrush background(RGB(248, 248, 246));
	dc.FillRect(CRect(0, 0, width, height), &background);

	CFont titleFont;
	titleFont.CreateFontW(34, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
	CFont textFont;
	textFont.CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

	CFont* oldFont = dc.SelectObject(&titleFont);
	dc.TextOutW(40, 32, L"ICC / ICM Profile");
	dc.SelectObject(&textFont);
	DrawMetadata(dc, FileName, data);
	DrawCieDiagram(dc, CRect(530, 90, 1130, 690), data);
	dc.SelectObject(oldFont);

	const __int64 rgbSize = static_cast<__int64>(width) * height * 3;
	BYTE* rgb = static_cast<BYTE*>(VirtualAlloc(NULL, rgbSize, MEM_COMMIT, PAGE_READWRITE));
	if (!rgb)
	{
		errorMsg.Format(L"ICC preview memory request failed: %I64d bytes", rgbSize);
		dc.SelectObject(oldBitmap);
		::DeleteObject(dib);
		return NULL;
	}

	const BYTE* bgra = static_cast<const BYTE*>(dibBits);
	for (__int64 pixel = 0; pixel < static_cast<__int64>(width) * height; ++pixel)
	{
		rgb[pixel * 3] = bgra[pixel * 4 + 2];
		rgb[pixel * 3 + 1] = bgra[pixel * 4 + 1];
		rgb[pixel * 3 + 2] = bgra[pixel * 4];
	}

	dc.SelectObject(oldBitmap);
	::DeleteObject(dib);
	return rgb;
}

CString __stdcall CICMFormat::get_ext() const
{
	return L"icc;icm";
}

struct plugin_data __stdcall CICMFormat::get_plugin_data() const
{
	struct plugin_data pluginData;
	pluginData.name.LoadString(IDS_SHORT_DESC);
	pluginData.desc.LoadString(IDS_LONG_DESC);
	return pluginData;
}

unsigned int __stdcall CICMFormat::get_cap() const
{
	return PICTURE_READ;
}

BYTE* __stdcall CICMFormat::FileToRGB(const CString& FileName,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n,
	const enum scaling_type picture_scaling_type,
	const bool b_scan)
{
	ProfilePreviewData data;
	if (!LoadProfileData(FileName, data, m_ErrorMsg))
	{
		return NULL;
	}

	int width = 0;
	int height = 0;
	BYTE* buffer = RenderProfilePreview(FileName, data, width, height, m_ErrorMsg);
	if (!buffer)
	{
		return NULL;
	}

	m_OriginalPictureWidth = m_PictureWidth = width;
	m_OriginalPictureHeight = m_PictureHeight = height;
	m_color_space = 2;
	m_bColorProfile = true;
	m_bIsValid = true;
	return buffer;
}

void CICMFormat::get_size(const CString& FileName)
{
	m_bIsValid = false;
	m_OriginalPictureWidth = m_PictureWidth = 1200;
	m_OriginalPictureHeight = m_PictureHeight = 800;

	ProfilePreviewData data;
	CString errorMsg;
	m_bIsValid = LoadProfileData(FileName, data, errorMsg);
}

vector<CString> info_template;

void __stdcall SetPluginInfoTemplates(const vector<CString>& _info_template)
{
	info_template = _info_template;
}

CString __stdcall CICMFormat::get_info(const CString& FileName, const enum info_type _info_type)
{
	if (_info_type & (info_type_std | info_type_size | info_type_short))
	{
		get_size(FileName);
	}

	if (_info_type & info_type_short)
	{
		CString msg, info;
		const int info_template_c(static_cast<int>(info_template.size()));
		if (info_template_c > 8)
		{
			const WCHAR* t1 = wcsrchr(FileName, L'\\');
			if (t1)
				++t1;
			else
				t1 = FileName;

			info.FormatMessage(info_template[0], t1);
			msg += info;
			msg += L'\n';

			info.FormatMessage(info_template[7], FileName.Left(FileName.ReverseFind(L'\\') + 1));
			msg += info;
			msg += L'\n';

			info.FormatMessage(info_template[3], m_OriginalPictureWidth, m_OriginalPictureHeight, L"1.0");
			msg += info;

			const __int64 file_size(::GetFileSize64(FileName));
			CString size_str;
			size_str.Format(L"%I64d", file_size);
			info.FormatMessage(info_template[1],
				GetSizeStr(file_size),
				size_str,
				::GetLongFileDateTime(FileName, filetime_type::date_created),
				::GetLongFileDateTime(FileName, filetime_type::date_modified));

			msg += L'\n';
			msg += info;
		}

		return msg;
	}

	return L"";
}
