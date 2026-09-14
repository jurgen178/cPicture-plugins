#include "stdafx.h"
#include "resource.h"
#include "pluginformat.h"
#include "../shared/FormatPluginEnumOperators.h"
#include "../shared/FormatPluginHelpers.h"
#include "avif/avif.h"

#include <algorithm>
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
	return k == 0 ? CAvifFormat::GetInstance : NULL;
}

const CString CAvifFormat::type = L"AVIF";

namespace
{
	constexpr __int64 MAX_AVIF_ANIMATION_FRAME_BYTES = 1024LL * 1024LL * 1024LL;

	bool CalculateRgbFrameSize(const int width, const int height, __int64& size, CString& errorMsg)
	{
		size = 0;

		if (width <= 0 || height <= 0)
		{
			errorMsg.Format(L"Invalid AVIF animation frame size: %d x %d", width, height);
			return false;
		}

		const __int64 pixelCount = static_cast<__int64>(width) * height;
		if (pixelCount <= 0 || pixelCount > MAX_AVIF_ANIMATION_FRAME_BYTES / 3)
		{
			errorMsg.Format(L"AVIF animation frame is too large: %d x %d", width, height);
			return false;
		}

		size = pixelCount * 3;
		return true;
	}

	bool IsAnimatedAvifSequence(const avifDecoder* decoder)
	{
		return decoder != NULL && decoder->imageCount > 1 && decoder->progressiveState == AVIF_PROGRESSIVE_STATE_UNAVAILABLE;
	}

	void GetAvifAnimationInfo(const avifDecoder* decoder, int& durationMs, int& loopCount, int& minFrameDurationMs, int& maxFrameDurationMs, bool& hasTransparency)
	{
		durationMs = 0;
		loopCount = -1;
		minFrameDurationMs = 0;
		maxFrameDurationMs = 0;
		hasTransparency = false;

		if (decoder == NULL || !IsAnimatedAvifSequence(decoder))
		{
			return;
		}

		if (decoder->durationInTimescales > 0 && decoder->timescale > 0)
		{
			const double durationMsDouble = static_cast<double>(decoder->durationInTimescales) * 1000.0 / static_cast<double>(decoder->timescale);
			durationMs = durationMsDouble >= static_cast<double>(INT_MAX) ? INT_MAX : static_cast<int>(durationMsDouble + 0.5);
		}

		if (decoder->repetitionCount == AVIF_REPETITION_COUNT_INFINITE)
		{
			loopCount = 0;
		}
		else
		if (decoder->repetitionCount >= 0)
		{
			loopCount = decoder->repetitionCount == INT_MAX ? INT_MAX : decoder->repetitionCount + 1;
		}

		hasTransparency = decoder->alphaPresent != AVIF_FALSE;
		minFrameDurationMs = INT_MAX;
		for (uint32_t frameIndex = 0; frameIndex < static_cast<uint32_t>(decoder->imageCount); ++frameIndex)
		{
			avifImageTiming timing = { 0 };
			if (avifDecoderNthImageTiming(decoder, frameIndex, &timing) != AVIF_RESULT_OK || timing.duration <= 0.0)
			{
				continue;
			}

			const double frameDurationMsDouble = timing.duration * 1000.0;
			const int frameDurationMs = frameDurationMsDouble >= static_cast<double>(INT_MAX) ? INT_MAX : static_cast<int>(frameDurationMsDouble + 0.5);
			minFrameDurationMs = min(minFrameDurationMs, frameDurationMs);
			maxFrameDurationMs = max(maxFrameDurationMs, frameDurationMs);
		}
		if (minFrameDurationMs == INT_MAX)
		{
			minFrameDurationMs = 0;
		}
	}

	bool ReadAvifAnimationInfo(const CStringA& fileNameUtf8, int& durationMs, int& loopCount, int& minFrameDurationMs, int& maxFrameDurationMs, bool& hasTransparency)
	{
		durationMs = 0;
		loopCount = -1;
		minFrameDurationMs = 0;
		maxFrameDurationMs = 0;
		hasTransparency = false;

		avifDecoder* decoder = avifDecoderCreate();
		if (!decoder)
		{
			return false;
		}

		decoder->ignoreExif = AVIF_TRUE;
		decoder->ignoreXMP = AVIF_TRUE;
		avifResult result = avifDecoderSetIOFile(decoder, fileNameUtf8);
		if (result == AVIF_RESULT_OK)
		{
			result = avifDecoderParse(decoder);
		}

		const bool ok = result == AVIF_RESULT_OK && IsAnimatedAvifSequence(decoder);
		if (ok)
		{
			GetAvifAnimationInfo(decoder, durationMs, loopCount, minFrameDurationMs, maxFrameDurationMs, hasTransparency);
		}

		avifDecoderDestroy(decoder);
		return ok;
	}
}

CAvifFormat::CAvifFormat()
	: m_animationDecoder(NULL),
	m_animationWidth(0),
	m_animationHeight(0),
	m_animationRepetitionCount(AVIF_REPETITION_COUNT_UNKNOWN),
	m_animationLoopIndex(0)
{
}

CAvifFormat::~CAvifFormat()
{
	CloseAnimation();
}

CStringA CAvifFormat::get_utf8_file_name(const CString& FileName) const
{
	CStringA pictureNameUtf8;
	const int bufferSize = WideCharToMultiByte(CP_UTF8, 0, FileName, -1, NULL, 0, NULL, NULL);
	if (bufferSize)
	{
		vector<char> utf8Buffer(bufferSize, 0);
		const int utf8Length = WideCharToMultiByte(CP_UTF8, 0, FileName, -1, utf8Buffer.data(), bufferSize, NULL, NULL);
		if (utf8Length)
		{
			pictureNameUtf8 = utf8Buffer.data();
		}
	}

	return pictureNameUtf8;
}

CString __stdcall CAvifFormat::get_ext() const
{
	return L"avif;avifs";
}

struct plugin_data __stdcall CAvifFormat::get_plugin_data() const
{
	struct plugin_data pluginData;
	pluginData.name.LoadString(IDS_SHORT_DESC);
	pluginData.desc.LoadString(IDS_LONG_DESC);
	return pluginData;
}

unsigned int __stdcall CAvifFormat::get_cap() const
{
	return PICTURE_READ | PICTURE_WRITE | PICTURE_QUALITY;
}

PictureMediaType __stdcall CAvifFormat::GetMediaType(const CString& FileName)
{
	// Empty filename is the host's registration-time probe for a static MediaType.
	// AVIF can be either a still image or an image sequence, so Unknown keeps .avif
	// and .avifs out of the static MediaType cache and preserves content detection.
	if (FileName.IsEmpty())
	{
		return PictureMediaType::Unknown;
	}

	avifDecoder* decoder = avifDecoderCreate();
	if (!decoder)
	{
		return PictureMediaType::Unknown;
	}

	decoder->ignoreExif = AVIF_TRUE;
	decoder->ignoreXMP = AVIF_TRUE;
	avifResult result = avifDecoderSetIOFile(decoder, get_utf8_file_name(FileName));
	if (result == AVIF_RESULT_OK)
	{
		result = avifDecoderParse(decoder);
	}

	PictureMediaType mediaType = PictureMediaType::Unknown;
	if (result == AVIF_RESULT_OK)
	{
		mediaType = IsAnimatedAvifSequence(decoder) ? PictureMediaType::AnimatedImage : PictureMediaType::Image;
	}

	avifDecoderDestroy(decoder);
	return mediaType;
}

bool __stdcall CAvifFormat::OpenAnimation(const CString& FileName, int& width, int& height)
{
	CloseAnimation();

	m_animationDecoder = avifDecoderCreate();
	if (!m_animationDecoder)
	{
		m_ErrorMsg = L"AVIF animation decoder allocation failed";
		return false;
	}

	m_animationDecoder->maxThreads = 0;
	m_animationDecoder->ignoreExif = AVIF_TRUE;
	m_animationDecoder->ignoreXMP = AVIF_TRUE;
	avifResult result = avifDecoderSetIOFile(m_animationDecoder, get_utf8_file_name(FileName));
	if (result == AVIF_RESULT_OK)
	{
		result = avifDecoderParse(m_animationDecoder);
	}

	if (result != AVIF_RESULT_OK || !m_animationDecoder->image || !IsAnimatedAvifSequence(m_animationDecoder))
	{
		m_ErrorMsg.Format(L"AVIF animation parse failed: %S", avifResultToString(result));
		CloseAnimation();
		return false;
	}

	m_animationWidth = static_cast<int>(m_animationDecoder->image->width);
	m_animationHeight = static_cast<int>(m_animationDecoder->image->height);
	__int64 size = 0;
	if (!CalculateRgbFrameSize(m_animationWidth, m_animationHeight, size, m_ErrorMsg))
	{
		CloseAnimation();
		return false;
	}

	m_animationRepetitionCount = m_animationDecoder->repetitionCount;
	m_animationLoopIndex = 0;
	width = m_animationWidth;
	height = m_animationHeight;
	return true;
}

bool __stdcall CAvifFormat::ReadAnimationFrame(BYTE*& data, int& width, int& height, int& delay_ms, bool allowLoop)
{
	data = NULL;
	width = 0;
	height = 0;
	delay_ms = 100;

	if (!m_animationDecoder)
	{
		return false;
	}

	avifResult result = avifDecoderNextImage(m_animationDecoder);
	if (result != AVIF_RESULT_OK)
	{
		if (!allowLoop)
		{
			return false;
		}

		if (m_animationRepetitionCount >= 0 && m_animationLoopIndex >= m_animationRepetitionCount)
		{
			return false;
		}

		++m_animationLoopIndex;
		avifDecoderReset(m_animationDecoder);
		result = avifDecoderNextImage(m_animationDecoder);
		if (result != AVIF_RESULT_OK)
		{
			m_ErrorMsg.Format(L"AVIF animation frame decode failed: %S", avifResultToString(result));
			return false;
		}
	}

	avifImage* image = m_animationDecoder->image;
	if (!image)
	{
		m_ErrorMsg = L"AVIF animation decoder returned no image";
		return false;
	}

	__int64 size = 0;
	if (!CalculateRgbFrameSize(static_cast<int>(image->width), static_cast<int>(image->height), size, m_ErrorMsg))
	{
		return false;
	}

	BYTE* buffer = static_cast<BYTE*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
	if (!buffer)
	{
		m_ErrorMsg.Format(L"AVIF memory request failed: %I64d bytes", size);
		return false;
	}

	avifRGBImage rgb;
	avifRGBImageSetDefaults(&rgb, image);
	rgb.format = AVIF_RGB_FORMAT_RGB;
	rgb.depth = 8;
	rgb.rowBytes = image->width * 3;
	rgb.pixels = buffer;

	result = avifImageYUVToRGB(image, &rgb);
	if (result != AVIF_RESULT_OK)
	{
		m_ErrorMsg.Format(L"AVIF RGB conversion failed: %S", avifResultToString(result));
		VirtualFree(buffer, 0, MEM_RELEASE);
		return false;
	}

	width = static_cast<int>(image->width);
	height = static_cast<int>(image->height);
	if (m_animationDecoder->imageTiming.duration > 0.0)
		delay_ms = max(10, static_cast<int>(m_animationDecoder->imageTiming.duration * 1000.0 + 0.5));
	data = buffer;
	return true;
}

void __stdcall CAvifFormat::CloseAnimation()
{
	if (m_animationDecoder)
	{
		avifDecoderDestroy(m_animationDecoder);
		m_animationDecoder = NULL;
	}

	m_animationWidth = 0;
	m_animationHeight = 0;
	m_animationRepetitionCount = AVIF_REPETITION_COUNT_UNKNOWN;
	m_animationLoopIndex = 0;
}

bool __stdcall CAvifFormat::properties_dlg(const HWND hwnd)
{
	CString msg;
	msg.LoadString(IDS_PROPERTY_DLG_TEXT);
	::MessageBox(hwnd, msg, get_plugin_data().desc, MB_ICONINFORMATION);
	return false;
}

BYTE* __stdcall CAvifFormat::FileToRGB(const CString& FileName,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n,
	const enum scaling_type picture_scaling_type,
	const bool b_scan)
{
	avifImage* image = avifImageCreateEmpty();
	if (!image)
	{
		m_ErrorMsg = L"AVIF image allocation failed";
		return NULL;
	}

	avifDecoder* decoder = avifDecoderCreate();
	if (!decoder)
	{
		avifImageDestroy(image);
		m_ErrorMsg = L"AVIF decoder allocation failed";
		return NULL;
	}

	decoder->maxThreads = 0;
	decoder->ignoreExif = AVIF_TRUE;
	decoder->ignoreXMP = AVIF_TRUE;

	const avifResult result = avifDecoderReadFile(decoder, image, get_utf8_file_name(FileName));
	if (result != AVIF_RESULT_OK)
	{
		m_ErrorMsg.Format(L"AVIF decode failed: %S", avifResultToString(result));
		avifDecoderDestroy(decoder);
		avifImageDestroy(image);
		return NULL;
	}

	avifRGBImage rgb;
	avifRGBImageSetDefaults(&rgb, image);
	rgb.format = AVIF_RGB_FORMAT_RGB;
	rgb.depth = 8;
	rgb.rowBytes = image->width * 3;

	const __int64 size = static_cast<__int64>(rgb.rowBytes) * image->height;
	BYTE* buffer = static_cast<BYTE*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
	if (!buffer)
	{
		m_ErrorMsg.Format(L"AVIF memory request failed: %I64d bytes", size);
		avifDecoderDestroy(decoder);
		avifImageDestroy(image);
		return NULL;
	}

	rgb.pixels = buffer;
	const avifResult rgbResult = avifImageYUVToRGB(image, &rgb);
	if (rgbResult != AVIF_RESULT_OK)
	{
		m_ErrorMsg.Format(L"AVIF RGB conversion failed: %S", avifResultToString(rgbResult));
		VirtualFree(buffer, 0, MEM_RELEASE);
		buffer = NULL;
	}
	else
	{
		m_OriginalPictureWidth = m_PictureWidth = static_cast<int>(image->width);
		m_OriginalPictureHeight = m_PictureHeight = static_cast<int>(image->height);
		m_color_space = 2;
		m_bIsValid = true;
	}

	avifDecoderDestroy(decoder);
	avifImageDestroy(image);
	return buffer;
}

bool __stdcall CAvifFormat::RGBToFile(const CString& FileName,
	const BYTE* dataBuf,
	const int width,
	const int height,
	const int quality_L,
	const int quality_C,
	const int jpeg_lossless)
{
	if (!dataBuf || width <= 0 || height <= 0)
	{
		m_ErrorMsg = L"Invalid AVIF input buffer";
		return false;
	}

	avifImage* image = avifImageCreate(width, height, 8, AVIF_PIXEL_FORMAT_YUV420);
	if (!image)
	{
		m_ErrorMsg = L"AVIF image allocation failed";
		return false;
	}

	image->colorPrimaries = AVIF_COLOR_PRIMARIES_SRGB;
	image->transferCharacteristics = AVIF_TRANSFER_CHARACTERISTICS_SRGB;
	image->matrixCoefficients = AVIF_MATRIX_COEFFICIENTS_BT709;
	image->yuvRange = AVIF_RANGE_FULL;

	avifRGBImage rgb;
	avifRGBImageSetDefaults(&rgb, image);
	rgb.format = AVIF_RGB_FORMAT_RGB;
	rgb.depth = 8;
	rgb.pixels = const_cast<BYTE*>(dataBuf);
	rgb.rowBytes = width * 3;

	avifResult result = avifImageRGBToYUV(image, &rgb);
	if (result != AVIF_RESULT_OK)
	{
		m_ErrorMsg.Format(L"AVIF YUV conversion failed: %S", avifResultToString(result));
		avifImageDestroy(image);
		return false;
	}

	avifEncoder* encoder = avifEncoderCreate();
	if (!encoder)
	{
		m_ErrorMsg = L"AVIF encoder allocation failed";
		avifImageDestroy(image);
		return false;
	}

	const int quality = quality_L >= 0 ? min(max(quality_L, 0), 100) : 80;
	encoder->quality = quality;
	encoder->qualityAlpha = quality;
	encoder->speed = AVIF_SPEED_DEFAULT;
	encoder->maxThreads = 0;

	avifRWData output = AVIF_DATA_EMPTY;
	result = avifEncoderWrite(encoder, image, &output);
	if (result != AVIF_RESULT_OK)
	{
		m_ErrorMsg.Format(L"AVIF encode failed: %S", avifResultToString(result));
		avifEncoderDestroy(encoder);
		avifImageDestroy(image);
		return false;
	}

	FILE* fp = NULL;
	const errno_t err(_wfopen_s(&fp, FileName, L"wb"));
	if (err != 0 || !fp)
	{
		m_ErrorMsg.Format(L"Error open file '%s' for writing", FileName);
		avifRWDataFree(&output);
		avifEncoderDestroy(encoder);
		avifImageDestroy(image);
		return false;
	}

	const bool ok = fwrite(output.data, 1, output.size, fp) == output.size;
	fclose(fp);

	if (!ok)
	{
		m_ErrorMsg.Format(L"Error writing AVIF file '%s'", FileName);
	}

	avifRWDataFree(&output);
	avifEncoderDestroy(encoder);
	avifImageDestroy(image);
	return ok;
}

void CAvifFormat::get_size(const CString& FileName)
{
	m_bIsValid = false;
	m_OriginalPictureWidth = m_OriginalPictureHeight = 0;
	m_PictureWidth = m_PictureHeight = 0;
	m_Shutterspeed = 0;
	m_FrameCount = 0;

	avifDecoder* decoder = avifDecoderCreate();
	if (!decoder)
	{
		return;
	}

	decoder->ignoreExif = AVIF_TRUE;
	decoder->ignoreXMP = AVIF_TRUE;
	avifResult result = avifDecoderSetIOFile(decoder, get_utf8_file_name(FileName));
	if (result == AVIF_RESULT_OK)
	{
		result = avifDecoderParse(decoder);
	}

	if (result == AVIF_RESULT_OK && decoder->image)
	{
		m_OriginalPictureWidth = m_PictureWidth = static_cast<int>(decoder->image->width);
		m_OriginalPictureHeight = m_PictureHeight = static_cast<int>(decoder->image->height);
		m_color_space = decoder->image->yuvFormat == AVIF_PIXEL_FORMAT_YUV400 ? 1 : 2;
		m_bIsValid = m_OriginalPictureWidth > 0 && m_OriginalPictureHeight > 0;
		if (m_bIsValid && IsAnimatedAvifSequence(decoder))
		{
			m_FrameCount = static_cast<int>(decoder->imageCount);
		}
	}

	avifDecoderDestroy(decoder);
}

vector<CString> info_template;

void __stdcall SetPluginInfoTemplates(const vector<CString>& _info_template)
{
	info_template = _info_template;
}

CString __stdcall CAvifFormat::get_info(const CString& FileName, const enum info_type _info_type)
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

			CString frameCountInfoTemplate;
			frameCountInfoTemplate.LoadString(IDS_ANIMATED_FRAME_COUNT_INFO);
			AppendAnimatedFrameCountInfo(msg, frameCountInfoTemplate, m_FrameCount);

			int animationDurationMs = 0;
			int animationLoopCount = -1;
			int minFrameDurationMs = 0;
			int maxFrameDurationMs = 0;
			bool hasTransparency = false;
			if (ReadAvifAnimationInfo(get_utf8_file_name(FileName), animationDurationMs, animationLoopCount, minFrameDurationMs, maxFrameDurationMs, hasTransparency))
			{
				CString durationInfoTemplate;
				durationInfoTemplate.LoadString(IDS_ANIMATED_DURATION_INFO);
				AppendAnimatedDurationInfo(msg, durationInfoTemplate, animationDurationMs);

				CString frameRateInfoTemplate;
				frameRateInfoTemplate.LoadString(IDS_ANIMATED_FRAME_RATE_INFO);
				AppendAnimatedFrameRateInfo(msg, frameRateInfoTemplate, m_FrameCount, animationDurationMs);

				CString frameDurationInfoTemplate;
				frameDurationInfoTemplate.LoadString(IDS_ANIMATED_FRAME_DURATION_INFO);
				AppendAnimatedFrameDurationInfo(msg, frameDurationInfoTemplate, minFrameDurationMs, maxFrameDurationMs);

				CString loopCountInfoTemplate;
				CString infiniteLoopText;
				loopCountInfoTemplate.LoadString(IDS_ANIMATED_LOOP_COUNT_INFO);
				infiniteLoopText.LoadString(IDS_ANIMATED_LOOP_INFINITE);
				AppendAnimatedLoopCountInfo(msg, loopCountInfoTemplate, infiniteLoopText, animationLoopCount);

				CString transparencyInfoTemplate;
				CString transparencyText;
				transparencyInfoTemplate.LoadString(IDS_ANIMATED_TRANSPARENCY_INFO);
				transparencyText.LoadString(hasTransparency ? IDS_ANIMATED_TRANSPARENCY_YES : IDS_ANIMATED_TRANSPARENCY_NO);
				if (!transparencyInfoTemplate.IsEmpty())
				{
					CString transparencyInfo;
					transparencyInfo.FormatMessage(transparencyInfoTemplate, transparencyText);
					msg += L'\n';
					msg += transparencyInfo;
				}
			}

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
