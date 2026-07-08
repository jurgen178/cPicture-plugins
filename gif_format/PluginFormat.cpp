#include "stdafx.h"
#include "resource.h"
#include "pluginformat.h"
#include "../shared/FormatPluginEnumOperators.h"
#include "../shared/FormatPluginHelpers.h"

#include <algorithm>
#include <memory>
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
	return k == 0 ? CGifFormat::GetInstance : NULL;
}

const CString CGifFormat::type = L"GIF";

namespace
{
	constexpr __int64 MAX_GIF_FRAME_BYTES = 1024LL * 1024LL * 1024LL;

	bool CalculateRgbFrameSize(const int width, const int height, __int64& pixelCount, __int64& size, CString& errorMsg)
	{
		pixelCount = 0;
		size = 0;

		if (width <= 0 || height <= 0)
		{
			errorMsg.Format(L"Invalid GIF frame size: %d x %d", width, height);
			return false;
		}

		pixelCount = static_cast<__int64>(width) * height;
		if (pixelCount <= 0 || pixelCount > MAX_GIF_FRAME_BYTES / 3)
		{
			errorMsg.Format(L"GIF frame is too large: %d x %d", width, height);
			return false;
		}

		size = pixelCount * 3;
		return true;
	}

	bool GetTimeFrameDimension(Gdiplus::Image& image, GUID& dimension, UINT& frameCount)
	{
		frameCount = 1;
		const UINT dimensionCount = image.GetFrameDimensionsCount();
		if (dimensionCount == 0)
		{
			return false;
		}

		vector<GUID> dimensions(dimensionCount);
		if (image.GetFrameDimensionsList(dimensions.data(), dimensionCount) != Gdiplus::Ok)
		{
			return false;
		}

		for (UINT index = 0; index < dimensionCount; ++index)
		{
			const UINT count = image.GetFrameCount(&dimensions[index]);
			if (dimensions[index] == Gdiplus::FrameDimensionTime && count > 0)
			{
				dimension = dimensions[index];
				frameCount = count;
				return true;
			}
		}

		return false;
	}

	vector<UINT> ReadFrameDelays(Gdiplus::Image& image, const UINT frameCount)
	{
		vector<UINT> delays(frameCount, 100);
		const UINT propertySize = image.GetPropertyItemSize(PropertyTagFrameDelay);
		if (propertySize == 0)
		{
			return delays;
		}

		unique_ptr<BYTE[]> propertyBuffer(new BYTE[propertySize]);
		Gdiplus::PropertyItem* propertyItem = reinterpret_cast<Gdiplus::PropertyItem*>(propertyBuffer.get());
		if (image.GetPropertyItem(PropertyTagFrameDelay, propertySize, propertyItem) != Gdiplus::Ok || propertyItem->value == NULL)
		{
			return delays;
		}

		const UINT valueCount = min(frameCount, propertyItem->length / static_cast<UINT>(sizeof(UINT)));
		const UINT* values = reinterpret_cast<const UINT*>(propertyItem->value);
		for (UINT index = 0; index < valueCount; ++index)
		{
			delays[index] = max(10U, values[index] * 10U);
		}

		return delays;
	}

	UINT ReadLoopCount(Gdiplus::Image& image)
	{
		const UINT propertySize = image.GetPropertyItemSize(PropertyTagLoopCount);
		if (propertySize == 0)
		{
			return 1;
		}

		unique_ptr<BYTE[]> propertyBuffer(new BYTE[propertySize]);
		Gdiplus::PropertyItem* propertyItem = reinterpret_cast<Gdiplus::PropertyItem*>(propertyBuffer.get());
		if (image.GetPropertyItem(PropertyTagLoopCount, propertySize, propertyItem) != Gdiplus::Ok || propertyItem->value == NULL || propertyItem->length < sizeof(USHORT))
		{
			return 1;
		}

		const USHORT* loopCount = reinterpret_cast<const USHORT*>(propertyItem->value);
		return *loopCount;
	}

	bool HasTransparentGifColor(Gdiplus::Image& image)
	{
		constexpr UINT imageFlagsHasAlpha = 0x00000002;
		return (image.GetFlags() & imageFlagsHasAlpha) != 0;
	}

	bool ReadGifAnimationInfo(const CString& fileName, int& durationMs, int& loopCount, int& minFrameDurationMs, int& maxFrameDurationMs, bool& hasTransparency)
	{
		durationMs = 0;
		loopCount = -1;
		minFrameDurationMs = 0;
		maxFrameDurationMs = 0;
		hasTransparency = false;

		Gdiplus::Image image(fileName);
		if (image.GetLastStatus() != Gdiplus::Ok)
		{
			return false;
		}

		GUID dimension = Gdiplus::FrameDimensionTime;
		UINT frameCount = 1;
		if (!GetTimeFrameDimension(image, dimension, frameCount) || frameCount <= 1)
		{
			return false;
		}

		const vector<UINT> delays = ReadFrameDelays(image, frameCount);
		minFrameDurationMs = INT_MAX;
		for (const UINT delay : delays)
		{
			const int delayMs = static_cast<int>(delay);
			minFrameDurationMs = min(minFrameDurationMs, delayMs);
			maxFrameDurationMs = max(maxFrameDurationMs, delayMs);
			if (durationMs <= INT_MAX - static_cast<int>(delay))
			{
				durationMs += delayMs;
			}
			else
			{
				durationMs = INT_MAX;
				break;
			}
		}
		if (minFrameDurationMs == INT_MAX)
		{
			minFrameDurationMs = 0;
		}

		loopCount = static_cast<int>(ReadLoopCount(image));
		hasTransparency = HasTransparentGifColor(image);
		return true;
	}

	BYTE* RenderImageToRgb(Gdiplus::Image& image, const int width, const int height, CString& errorMsg)
	{
		__int64 pixelCount = 0;
		__int64 size = 0;
		if (!CalculateRgbFrameSize(width, height, pixelCount, size, errorMsg))
		{
			return NULL;
		}

		Gdiplus::Bitmap bitmap(width, height, PixelFormat24bppRGB);
		if (bitmap.GetLastStatus() != Gdiplus::Ok)
		{
			errorMsg = L"GIF render bitmap allocation failed";
			return NULL;
		}

		Gdiplus::Graphics graphics(&bitmap);
		graphics.Clear(Gdiplus::Color(255, 255, 255, 255));
		if (graphics.DrawImage(&image, 0, 0, width, height) != Gdiplus::Ok)
		{
			errorMsg = L"GIF frame render failed";
			return NULL;
		}

		Gdiplus::Rect rect(0, 0, width, height);
		Gdiplus::BitmapData bitmapData = { 0 };
		if (bitmap.LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat24bppRGB, &bitmapData) != Gdiplus::Ok)
		{
			errorMsg = L"GIF frame lock failed";
			return NULL;
		}

		BYTE* buffer = static_cast<BYTE*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
		if (buffer == NULL)
		{
			bitmap.UnlockBits(&bitmapData);
			errorMsg.Format(L"GIF memory request failed: %I64d bytes", size);
			return NULL;
		}

		const int stride = bitmapData.Stride;
		const BYTE* scan0 = static_cast<const BYTE*>(bitmapData.Scan0);
		for (int y = 0; y < height; ++y)
		{
			const BYTE* src = stride >= 0 ? scan0 + static_cast<__int64>(y) * stride : scan0 + static_cast<__int64>(height - 1 - y) * -stride;
			BYTE* dst = buffer + static_cast<__int64>(y) * width * 3;
			CopyBgrToRgbRow(src, dst, width);
		}

		bitmap.UnlockBits(&bitmapData);
		return buffer;
	}
}

CGifFormat::GdiplusSession::GdiplusSession()
	: Token(0),
	Started(false)
{
	Gdiplus::GdiplusStartupInput input;
	Started = Gdiplus::GdiplusStartup(&Token, &input, NULL) == Gdiplus::Ok;
}

CGifFormat::GdiplusSession::~GdiplusSession()
{
	if (Started)
	{
		Gdiplus::GdiplusShutdown(Token);
	}
}

bool CGifFormat::GdiplusSession::IsValid() const noexcept
{
	return Started;
}

CGifFormat::CGifFormat()
	: m_animationImage(NULL),
	m_animationFrameDimension(Gdiplus::FrameDimensionTime),
	m_animationFrameCount(0),
	m_animationFrameIndex(0),
	m_animationLoopCount(1),
	m_animationLoopIndex(0)
{
}

CGifFormat::~CGifFormat()
{
	CloseAnimation();
}

CString __stdcall CGifFormat::get_ext() const
{
	return L"gif";
}

struct plugin_data __stdcall CGifFormat::get_plugin_data() const
{
	struct plugin_data pluginData;
	pluginData.name.LoadString(IDS_SHORT_DESC);
	pluginData.desc.LoadString(IDS_LONG_DESC);
	return pluginData;
}

unsigned int __stdcall CGifFormat::get_cap() const
{
	return PICTURE_READ;
}

bool __stdcall CGifFormat::properties_dlg(const HWND hwnd)
{
	CString msg;
	msg.LoadString(IDS_PROPERTY_DLG_TEXT);
	::MessageBox(hwnd, msg, get_plugin_data().desc, MB_ICONINFORMATION);
	return false;
}

PictureMediaType __stdcall CGifFormat::GetMediaType(const CString& FileName)
{
	if (FileName.IsEmpty())
	{
		return PictureMediaType::Unknown;
	}

	if (!m_gdiplus.IsValid())
	{
		return PictureMediaType::Unknown;
	}

	Gdiplus::Image image(FileName);
	if (image.GetLastStatus() != Gdiplus::Ok)
	{
		return PictureMediaType::Unknown;
	}

	GUID dimension = Gdiplus::FrameDimensionTime;
	UINT frameCount = 1;
	if (!GetTimeFrameDimension(image, dimension, frameCount))
	{
		return PictureMediaType::Image;
	}

	return frameCount > 1 ? PictureMediaType::AnimatedImage : PictureMediaType::Image;
}

bool __stdcall CGifFormat::OpenAnimation(const CString& FileName, int& width, int& height)
{
	CloseAnimation();
	width = 0;
	height = 0;

	if (!m_gdiplus.IsValid())
	{
		m_ErrorMsg = L"GDI+ initialization failed";
		return false;
	}

	m_animationImage = new Gdiplus::Image(FileName);
	if (m_animationImage == NULL || m_animationImage->GetLastStatus() != Gdiplus::Ok)
	{
		m_ErrorMsg = L"GIF animation open failed";
		CloseAnimation();
		return false;
	}

	if (!GetTimeFrameDimension(*m_animationImage, m_animationFrameDimension, m_animationFrameCount) || m_animationFrameCount <= 1)
	{
		m_ErrorMsg = L"GIF file is not animated";
		CloseAnimation();
		return false;
	}

	width = static_cast<int>(m_animationImage->GetWidth());
	height = static_cast<int>(m_animationImage->GetHeight());
	__int64 pixelCount = 0;
	__int64 size = 0;
	if (!CalculateRgbFrameSize(width, height, pixelCount, size, m_ErrorMsg))
	{
		CloseAnimation();
		return false;
	}

	m_animationFrameDelays = ReadFrameDelays(*m_animationImage, m_animationFrameCount);
	m_animationLoopCount = ReadLoopCount(*m_animationImage);
	m_animationFrameIndex = 0;
	m_animationLoopIndex = 0;
	m_OriginalPictureWidth = m_PictureWidth = width;
	m_OriginalPictureHeight = m_PictureHeight = height;
	m_color_space = 2;
	m_bIsValid = true;
	return true;
}

bool __stdcall CGifFormat::ReadAnimationFrame(BYTE*& data, int& width, int& height, int& delay_ms, bool allowLoop)
{
	data = NULL;
	width = 0;
	height = 0;
	delay_ms = 100;

	if (m_animationImage == NULL || m_animationFrameCount == 0)
	{
		return false;
	}

	if (m_animationFrameIndex >= m_animationFrameCount)
	{
		if (!allowLoop)
		{
			return false;
		}

		if (m_animationLoopCount != 0 && m_animationLoopIndex + 1 >= m_animationLoopCount)
		{
			return false;
		}

		++m_animationLoopIndex;
		m_animationFrameIndex = 0;
	}

	if (m_animationImage->SelectActiveFrame(&m_animationFrameDimension, m_animationFrameIndex) != Gdiplus::Ok)
	{
		m_ErrorMsg = L"GIF animation frame select failed";
		return false;
	}

	width = static_cast<int>(m_animationImage->GetWidth());
	height = static_cast<int>(m_animationImage->GetHeight());
	BYTE* buffer = RenderImageToRgb(*m_animationImage, width, height, m_ErrorMsg);
	if (buffer == NULL)
	{
		return false;
	}

	if (m_animationFrameIndex < m_animationFrameDelays.size())
	{
		delay_ms = static_cast<int>(m_animationFrameDelays[m_animationFrameIndex]);
	}

	++m_animationFrameIndex;
	data = buffer;
	return true;
}

void __stdcall CGifFormat::CloseAnimation()
{
	delete m_animationImage;
	m_animationImage = NULL;
	m_animationFrameDimension = Gdiplus::FrameDimensionTime;
	m_animationFrameCount = 0;
	m_animationFrameIndex = 0;
	m_animationLoopCount = 1;
	m_animationLoopIndex = 0;
	m_animationFrameDelays.clear();
}

BYTE* __stdcall CGifFormat::FileToRGB(const CString& FileName,
	const int abs_size_x, const int abs_size_y,
	const int rel_size_z, const int rel_size_n,
	const enum scaling_type picture_scaling_type,
	const bool b_scan)
{
	UNREFERENCED_PARAMETER(abs_size_x);
	UNREFERENCED_PARAMETER(abs_size_y);
	UNREFERENCED_PARAMETER(rel_size_z);
	UNREFERENCED_PARAMETER(rel_size_n);
	UNREFERENCED_PARAMETER(picture_scaling_type);
	UNREFERENCED_PARAMETER(b_scan);

	if (!m_gdiplus.IsValid())
	{
		m_ErrorMsg = L"GDI+ initialization failed";
		return NULL;
	}

	Gdiplus::Image image(FileName);
	if (image.GetLastStatus() != Gdiplus::Ok)
	{
		m_ErrorMsg = L"GIF open failed";
		return NULL;
	}

	GUID dimension = Gdiplus::FrameDimensionTime;
	UINT frameCount = 1;
	if (GetTimeFrameDimension(image, dimension, frameCount))
	{
		image.SelectActiveFrame(&dimension, 0);
	}

	const int width = static_cast<int>(image.GetWidth());
	const int height = static_cast<int>(image.GetHeight());
	BYTE* buffer = RenderImageToRgb(image, width, height, m_ErrorMsg);
	if (buffer == NULL)
	{
		return NULL;
	}

	m_OriginalPictureWidth = m_PictureWidth = width;
	m_OriginalPictureHeight = m_PictureHeight = height;
	m_color_space = 2;
	m_bIsValid = true;
	return buffer;
}

vector<CString> info_template;

void __stdcall SetPluginInfoTemplates(const vector<CString>& _info_template)
{
	info_template = _info_template;
}

CString __stdcall CGifFormat::get_info(const CString& FileName, const enum info_type _info_type)
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
			if (ReadGifAnimationInfo(FileName, animationDurationMs, animationLoopCount, minFrameDurationMs, maxFrameDurationMs, hasTransparency))
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

void CGifFormat::get_size(const CString& FileName)
{
	m_OriginalPictureWidth = m_PictureWidth = 0;
	m_OriginalPictureHeight = m_PictureHeight = 0;
	m_Shutterspeed = 0;
	m_FrameCount = 0;
	m_bIsValid = false;

	if (!m_gdiplus.IsValid())
	{
		return;
	}

	Gdiplus::Image image(FileName);
	if (image.GetLastStatus() != Gdiplus::Ok)
	{
		return;
	}

	m_OriginalPictureWidth = m_PictureWidth = static_cast<int>(image.GetWidth());
	m_OriginalPictureHeight = m_PictureHeight = static_cast<int>(image.GetHeight());
	m_bIsValid = m_OriginalPictureWidth > 0 && m_OriginalPictureHeight > 0;

	GUID dimension = Gdiplus::FrameDimensionTime;
	UINT frameCount = 1;
	if (m_bIsValid && GetTimeFrameDimension(image, dimension, frameCount) && frameCount > 1)
	{
		m_FrameCount = static_cast<int>(frameCount);
	}
}
