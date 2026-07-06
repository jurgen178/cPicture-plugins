#include "stdafx.h"
#include "resource.h"
#include "pluginformat.h"
#include "../shared/FormatPluginEnumOperators.h"
#include "../shared/FormatPluginHelpers.h"
#include "webp/decode.h"
#include "webp/demux.h"
#include "webp/encode.h"

#include <algorithm>
#include <cstdio>
#include <vector>
using namespace std;

const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
#ifdef _DEBUG
	return L"1.2-debug";
#else
	return L"1.2";
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
	return k == 0 ? CWebPFormat::GetInstance : NULL;
}

const CString CWebPFormat::type = L"WebP";
CString CWebPFormat::m_property_str;

namespace
{
	constexpr __int64 MAX_WEBP_ANIMATION_FRAME_BYTES = 1024LL * 1024LL * 1024LL;

	bool CalculateRgbFrameSize(const int width, const int height, __int64& pixelCount, __int64& size, CString& errorMsg)
	{
		pixelCount = 0;
		size = 0;

		if (width <= 0 || height <= 0)
		{
			errorMsg.Format(L"Invalid WebP animation frame size: %d x %d", width, height);
			return false;
		}

		pixelCount = static_cast<__int64>(width) * height;
		if (pixelCount <= 0 || pixelCount > MAX_WEBP_ANIMATION_FRAME_BYTES / 3)
		{
			errorMsg.Format(L"WebP animation frame is too large: %d x %d", width, height);
			return false;
		}

		size = pixelCount * 3;
		return true;
	}
}

CWebPFormat::CWebPFormat()
	: m_animationDecoder(NULL),
	m_animationWidth(0),
	m_animationHeight(0),
	m_animationLoopCount(0),
	m_animationLoopIndex(0),
	m_animationPreviousTimestamp(0)
{
}

CWebPFormat::~CWebPFormat()
{
	CloseAnimation();
}

bool ReadWebPProperties(const CString& propertyStr, int& quality, int& lossless)
{
	quality = 80;
	lossless = 0;

	const int ret = swscanf_s(propertyStr, L"%d,%d", &quality, &lossless);
	if (ret != 2)
	{
		return false;
	}

	quality = min(max(quality, 0), 100);
	lossless = lossless != 0 ? 1 : 0;
	return true;
}

class CWebPPropertiesDlg : public CDialog
{
public:
	CWebPPropertiesDlg(CWnd* pParent = NULL)
		: CDialog(IDD_DIALOG_WEBP_PROPERTIES, pParent)
	{
	}

	CString m_property_str;

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBnClickedCheckWebPLossless();
	DECLARE_MESSAGE_MAP()

private:
	void update();
};

BEGIN_MESSAGE_MAP(CWebPPropertiesDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_WEBP_LOSSLESS, &CWebPPropertiesDlg::OnBnClickedCheckWebPLossless)
END_MESSAGE_MAP()

BOOL CWebPPropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	int quality = 80;
	int lossless = 0;
	ReadWebPProperties(m_property_str, quality, lossless);

	CString qualityText;
	qualityText.Format(L"%d", quality);
	SetDlgItemText(IDC_EDIT_WEBP_QUALITY, qualityText);
	CheckDlgButton(IDC_CHECK_WEBP_LOSSLESS, lossless != 0 ? BST_CHECKED : BST_UNCHECKED);
	update();

	return TRUE;
}

void CWebPPropertiesDlg::OnOK()
{
	CString qualityText;
	GetDlgItemText(IDC_EDIT_WEBP_QUALITY, qualityText);
	const int quality = min(max(_wtoi(qualityText), 0), 100);
	const bool lossless = IsDlgButtonChecked(IDC_CHECK_WEBP_LOSSLESS) == BST_CHECKED;
	m_property_str.Format(L"%d,%d", quality, lossless ? 1 : 0);

	CDialog::OnOK();
}

void CWebPPropertiesDlg::OnBnClickedCheckWebPLossless()
{
	update();
}

void CWebPPropertiesDlg::update()
{
	CWnd* qualityEdit = GetDlgItem(IDC_EDIT_WEBP_QUALITY);
	if (qualityEdit)
	{
		qualityEdit->EnableWindow(IsDlgButtonChecked(IDC_CHECK_WEBP_LOSSLESS) != BST_CHECKED);
	}
}

vector<BYTE> ReadFileData(const CString& FileName, CString& errorMsg)
{
	vector<BYTE> fileData;
	const __int64 fileSize = GetFileSize64(FileName);
	if (fileSize <= 0)
	{
		errorMsg.Format(L"Invalid WebP file size: %I64d bytes", fileSize);
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
		errorMsg.Format(L"Error reading WebP file '%s'", FileName);
		fileData.clear();
	}

	return fileData;
}

bool GetWebPImageInfo(const vector<BYTE>& fileData, int& width, int& height, int& frameCount)
{
	width = 0;
	height = 0;
	frameCount = 0;

	const WebPData webpData = { fileData.data(), fileData.size() };
	WebPDemuxer* demux = WebPDemux(&webpData);
	if (demux)
	{
		width = WebPDemuxGetI(demux, WEBP_FF_CANVAS_WIDTH);
		height = WebPDemuxGetI(demux, WEBP_FF_CANVAS_HEIGHT);
		frameCount = WebPDemuxGetI(demux, WEBP_FF_FRAME_COUNT);
		WebPDemuxDelete(demux);

		return width > 0 && height > 0;
	}

	return WebPGetInfo(fileData.data(), fileData.size(), &width, &height) && width > 0 && height > 0;
}

BYTE* DecodeFirstAnimatedWebPFrame(const vector<BYTE>& fileData, int& width, int& height, CString& errorMsg)
{
	WebPAnimDecoderOptions options;
	if (!WebPAnimDecoderOptionsInit(&options))
	{
		errorMsg = L"WebP animation decoder options initialization failed";
		return NULL;
	}

	options.color_mode = MODE_RGBA;

	const WebPData webpData = { fileData.data(), fileData.size() };
	WebPAnimDecoder* decoder = WebPAnimDecoderNew(&webpData, &options);
	if (!decoder)
	{
		errorMsg = L"WebP animation decoder allocation failed";
		return NULL;
	}

	WebPAnimInfo animInfo = { 0 };
	if (!WebPAnimDecoderGetInfo(decoder, &animInfo) || animInfo.canvas_width <= 0 || animInfo.canvas_height <= 0)
	{
		errorMsg = L"Invalid animated WebP image header";
		WebPAnimDecoderDelete(decoder);
		return NULL;
	}

	uint8_t* decodedFrame = NULL;
	int timestamp = 0;
	if (!WebPAnimDecoderGetNext(decoder, &decodedFrame, &timestamp) || !decodedFrame)
	{
		errorMsg = L"Animated WebP first frame decode failed";
		WebPAnimDecoderDelete(decoder);
		return NULL;
	}

	__int64 pixelCount = 0;
	__int64 size = 0;
	if (!CalculateRgbFrameSize(animInfo.canvas_width, animInfo.canvas_height, pixelCount, size, errorMsg))
	{
		WebPAnimDecoderDelete(decoder);
		return NULL;
	}

	BYTE* buffer = static_cast<BYTE*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
	if (!buffer)
	{
		errorMsg.Format(L"WebP memory request failed: %I64d bytes", size);
		WebPAnimDecoderDelete(decoder);
		return NULL;
	}

	for (__int64 pixel = 0; pixel < pixelCount; ++pixel)
	{
		buffer[pixel * 3] = decodedFrame[pixel * 4];
		buffer[pixel * 3 + 1] = decodedFrame[pixel * 4 + 1];
		buffer[pixel * 3 + 2] = decodedFrame[pixel * 4 + 2];
	}

	width = animInfo.canvas_width;
	height = animInfo.canvas_height;
	WebPAnimDecoderDelete(decoder);
	return buffer;
}

CString __stdcall CWebPFormat::get_ext() const
{
	return L"webp";
}

struct plugin_data __stdcall CWebPFormat::get_plugin_data() const
{
	struct plugin_data pluginData;
	pluginData.name.LoadString(IDS_SHORT_DESC);
	pluginData.desc.LoadString(IDS_LONG_DESC);
	return pluginData;
}

unsigned int __stdcall CWebPFormat::get_cap() const
{
	return PICTURE_READ | PICTURE_WRITE | PICTURE_QUALITY;
}

PictureMediaType __stdcall CWebPFormat::GetMediaType(const CString& FileName)
{
	CString errorMsg;
	const vector<BYTE> fileData = ReadFileData(FileName, errorMsg);
	if (fileData.empty())
	{
		return PictureMediaType::Unknown;
	}

	int width = 0;
	int height = 0;
	int frameCount = 0;
	if (!GetWebPImageInfo(fileData, width, height, frameCount))
	{
		return PictureMediaType::Unknown;
	}

	return frameCount > 1 ? PictureMediaType::AnimatedImage : PictureMediaType::Image;
}

bool __stdcall CWebPFormat::OpenAnimation(const CString& FileName, int& width, int& height)
{
	CloseAnimation();

	m_animationFileData = ReadFileData(FileName, m_ErrorMsg);
	if (m_animationFileData.empty())
	{
		return false;
	}

	WebPAnimDecoderOptions options;
	if (!WebPAnimDecoderOptionsInit(&options))
	{
		m_ErrorMsg = L"WebP animation decoder options initialization failed";
		return false;
	}

	options.color_mode = MODE_RGBA;

	const WebPData webpData = { m_animationFileData.data(), m_animationFileData.size() };
	m_animationDecoder = WebPAnimDecoderNew(&webpData, &options);
	if (!m_animationDecoder)
	{
		m_ErrorMsg = L"WebP animation decoder allocation failed";
		m_animationFileData.clear();
		return false;
	}

	WebPAnimInfo animInfo = { 0 };
	if (!WebPAnimDecoderGetInfo(m_animationDecoder, &animInfo) || animInfo.canvas_width <= 0 || animInfo.canvas_height <= 0)
	{
		m_ErrorMsg = L"Invalid animated WebP image header";
		CloseAnimation();
		return false;
	}

	m_animationWidth = animInfo.canvas_width;
	m_animationHeight = animInfo.canvas_height;
	__int64 pixelCount = 0;
	__int64 size = 0;
	if (!CalculateRgbFrameSize(m_animationWidth, m_animationHeight, pixelCount, size, m_ErrorMsg))
	{
		CloseAnimation();
		return false;
	}

	m_animationLoopCount = animInfo.loop_count;
	m_animationLoopIndex = 0;
	m_animationPreviousTimestamp = 0;
	width = m_animationWidth;
	height = m_animationHeight;
	return true;
}

bool __stdcall CWebPFormat::ReadAnimationFrame(BYTE*& data, int& width, int& height, int& delay_ms)
{
	data = NULL;
	width = 0;
	height = 0;
	delay_ms = 100;

	if (!m_animationDecoder)
	{
		return false;
	}

	uint8_t* decodedFrame = NULL;
	int timestamp = 0;
	if (!WebPAnimDecoderGetNext(m_animationDecoder, &decodedFrame, &timestamp) || !decodedFrame)
	{
		if (m_animationLoopCount != 0 && m_animationLoopIndex + 1 >= m_animationLoopCount)
		{
			return false;
		}

		++m_animationLoopIndex;
		WebPAnimDecoderReset(m_animationDecoder);
		m_animationPreviousTimestamp = 0;
		if (!WebPAnimDecoderGetNext(m_animationDecoder, &decodedFrame, &timestamp) || !decodedFrame)
		{
			return false;
		}
	}

	__int64 pixelCount = 0;
	__int64 size = 0;
	if (!CalculateRgbFrameSize(m_animationWidth, m_animationHeight, pixelCount, size, m_ErrorMsg))
	{
		return false;
	}

	BYTE* buffer = static_cast<BYTE*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
	if (!buffer)
	{
		m_ErrorMsg.Format(L"WebP memory request failed: %I64d bytes", size);
		return false;
	}

	for (__int64 pixel = 0; pixel < pixelCount; ++pixel)
	{
		buffer[pixel * 3] = decodedFrame[pixel * 4];
		buffer[pixel * 3 + 1] = decodedFrame[pixel * 4 + 1];
		buffer[pixel * 3 + 2] = decodedFrame[pixel * 4 + 2];
	}

	width = m_animationWidth;
	height = m_animationHeight;
	delay_ms = max(10, timestamp - m_animationPreviousTimestamp);
	m_animationPreviousTimestamp = timestamp;
	data = buffer;
	return true;
}

void __stdcall CWebPFormat::CloseAnimation()
{
	if (m_animationDecoder)
	{
		WebPAnimDecoderDelete(m_animationDecoder);
		m_animationDecoder = NULL;
	}

	m_animationFileData.clear();
	m_animationWidth = 0;
	m_animationHeight = 0;
	m_animationLoopCount = 0;
	m_animationLoopIndex = 0;
	m_animationPreviousTimestamp = 0;
}

bool __stdcall CWebPFormat::properties_dlg(const HWND hwnd)
{
	CWnd parent;
	if (hwnd)
	{
		parent.Attach(hwnd);
	}

	CWebPPropertiesDlg dlg(hwnd ? &parent : NULL);
	const CString previousProperty = get_properties();
	dlg.m_property_str = previousProperty;
	bool propertiesUpdated = false;
	if (dlg.DoModal() == IDOK)
	{
		set_properties(dlg.m_property_str);
		propertiesUpdated = previousProperty != get_properties();
	}

	if (hwnd)
	{
		parent.Detach();
	}

	return propertiesUpdated;
}

void __stdcall CWebPFormat::set_properties(const CString& property_str)
{
	int quality = 80;
	int lossless = 0;
	if (!ReadWebPProperties(property_str, quality, lossless))
	{
		m_property_str = L"80,0";
		return;
	}

	m_property_str.Format(L"%d,%d", quality, lossless);
}

CString __stdcall CWebPFormat::get_properties() const
{
	return m_property_str.GetLength() ? static_cast<LPCWSTR>(m_property_str) : L"80,0";
}

BYTE* __stdcall CWebPFormat::FileToRGB(const CString& FileName,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n,
	const enum scaling_type picture_scaling_type,
	const bool b_scan)
{
	const vector<BYTE> fileData = ReadFileData(FileName, m_ErrorMsg);
	if (fileData.empty())
	{
		return NULL;
	}

	int width = 0;
	int height = 0;
	int frameCount = 0;
	if (!GetWebPImageInfo(fileData, width, height, frameCount))
	{
		m_ErrorMsg = L"Invalid WebP image header";
		return NULL;
	}

	if (frameCount > 1)
	{
		BYTE* animatedBuffer = DecodeFirstAnimatedWebPFrame(fileData, width, height, m_ErrorMsg);
		if (!animatedBuffer)
		{
			return NULL;
		}

		m_OriginalPictureWidth = m_PictureWidth = width;
		m_OriginalPictureHeight = m_PictureHeight = height;
		m_color_space = 2;
		m_bIsValid = true;
		return animatedBuffer;
	}

	const __int64 rowBytes = static_cast<__int64>(width) * 3;
	const __int64 size = rowBytes * height;
	BYTE* buffer = static_cast<BYTE*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
	if (!buffer)
	{
		m_ErrorMsg.Format(L"WebP memory request failed: %I64d bytes", size);
		return NULL;
	}

	if (!WebPDecodeRGBInto(fileData.data(), fileData.size(), buffer, static_cast<size_t>(size), static_cast<int>(rowBytes)))
	{
		m_ErrorMsg = L"WebP decode failed";
		VirtualFree(buffer, 0, MEM_RELEASE);
		return NULL;
	}

	m_OriginalPictureWidth = m_PictureWidth = width;
	m_OriginalPictureHeight = m_PictureHeight = height;
	m_color_space = 2;
	m_bIsValid = true;
	return buffer;
}

bool __stdcall CWebPFormat::RGBToFile(const CString& FileName,
	const BYTE* dataBuf,
	const int width,
	const int height,
	const int quality_L,
	const int quality_C,
	const int jpeg_lossless)
{
	if (!dataBuf || width <= 0 || height <= 0)
	{
		m_ErrorMsg = L"Invalid WebP input buffer";
		return false;
	}

	int webpQuality = 80;
	int losslessSetting = 0;
	const CString propertyStr = get_properties();
	ReadWebPProperties(propertyStr, webpQuality, losslessSetting);

	const int quality = quality_L >= 0 ? quality_L : webpQuality;
	const bool useLossless = jpeg_lossless < 0 ? losslessSetting != 0 : jpeg_lossless != 0;

	uint8_t* output = NULL;
	const size_t outputSize = useLossless
		? WebPEncodeLosslessRGB(dataBuf, width, height, width * 3, &output)
		: WebPEncodeRGB(dataBuf, width, height, width * 3, static_cast<float>(min(max(quality, 0), 100)), &output);
	if (outputSize == 0 || !output)
	{
		m_ErrorMsg = L"WebP encode failed";
		return false;
	}

	FILE* fp = NULL;
	const errno_t err(_wfopen_s(&fp, FileName, L"wb"));
	if (err != 0 || !fp)
	{
		m_ErrorMsg.Format(L"Error open file '%s' for writing", FileName);
		WebPFree(output);
		return false;
	}

	const bool ok = fwrite(output, 1, outputSize, fp) == outputSize;
	fclose(fp);

	if (!ok)
	{
		m_ErrorMsg.Format(L"Error writing WebP file '%s'", FileName);
	}

	WebPFree(output);
	return ok;
}

void CWebPFormat::get_size(const CString& FileName)
{
	m_bIsValid = false;
	m_OriginalPictureWidth = m_OriginalPictureHeight = 0;
	m_PictureWidth = m_PictureHeight = 0;

	CString errorMsg;
	const vector<BYTE> fileData = ReadFileData(FileName, errorMsg);
	if (fileData.empty())
	{
		return;
	}

	int width = 0;
	int height = 0;
	int frameCount = 0;
	if (GetWebPImageInfo(fileData, width, height, frameCount))
	{
		m_OriginalPictureWidth = m_PictureWidth = width;
		m_OriginalPictureHeight = m_PictureHeight = height;
		m_color_space = 2;
		m_bIsValid = m_OriginalPictureWidth > 0 && m_OriginalPictureHeight > 0;
	}
}

vector<CString> info_template;

void __stdcall SetPluginInfoTemplates(const vector<CString>& _info_template)
{
	info_template = _info_template;
}

CString __stdcall CWebPFormat::get_info(const CString& FileName, const enum info_type _info_type)
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

			const float f_mp(static_cast<float>(m_OriginalPictureWidth) * m_OriginalPictureHeight / 1000 / 1000);
			CString mp;
			mp.Format(L"%.1f", (f_mp < 0.1) ? 0.1 : f_mp);

			info.FormatMessage(info_template[3], m_OriginalPictureWidth, m_OriginalPictureHeight, mp);
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
