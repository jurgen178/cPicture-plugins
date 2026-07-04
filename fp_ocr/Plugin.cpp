// Plugin.cpp — OCR function plugin for cPicture.
//
// Uses Windows.Media.Ocr (built into Windows 10+, no API key, no internet).
// Recognizes text in selected images and shows the result in a dialog.
//
// WinRT headers must be included BEFORE MFC to avoid macro conflicts.

// ── WinRT (before MFC) ──────────────────────────────────────────────────────
#pragma push_macro("GetCurrentTime")
#pragma push_macro("TRY")
#undef GetCurrentTime
#undef TRY

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Ocr.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>

#pragma pop_macro("TRY")
#pragma pop_macro("GetCurrentTime")

// ── MFC + plugin headers ────────────────────────────────────────────────────
#include "stdafx.h"
#include "Plugin.h"
#include "OcrResultDlg.h"
#include "resource.h"

#include <shcore.h>     // CreateRandomAccessStreamOverStream
#include <thread>       // std::thread for MTA background execution

// IMemoryBufferByteAccess — declared manually with uint8_t to avoid the
// ambiguous-symbol conflict that robuffer.h causes with C++17 std::byte.
struct __declspec(uuid("5b0d3235-4dba-4d44-865e-8f1d0e4fd04d"))
IMemoryBufferByteAccess : public ::IUnknown
{
	virtual HRESULT __stdcall GetBuffer(uint8_t** value, uint32_t* capacity) = 0;
};


// ── Plugin export functions ─────────────────────────────────────────────────

const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
#ifdef _DEBUG
	return L"1.7-debug";
#else
	return L"1.7";
#endif
}

const PLUGIN_TYPE __stdcall GetPluginType()
{
	return PLUGIN_TYPE_FUNCTION;
}

const int __stdcall GetPluginInit()
{
	return 1;
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int /*k*/)
{
	return CFunctionPluginOcr::GetInstance;
}


// ── OCR helper ──────────────────────────────────────────────────────────────

namespace
{
	// Load image from file path as SoftwareBitmap.
	// Uses StorageFile + BitmapDecoder — supports all formats Windows can decode
	// (JPEG, PNG, TIFF, HEIC, BMP, …).
	winrt::Windows::Graphics::Imaging::SoftwareBitmap LoadBitmapFromFile(const CString& file_name)
	{
		namespace WGI = winrt::Windows::Graphics::Imaging;

		// IStream → IRandomAccessStream (works with any local path, incl. UNC)
		winrt::com_ptr<IStream> fileStream;
		HRESULT hr = SHCreateStreamOnFileEx(
			file_name, STGM_READ | STGM_SHARE_DENY_WRITE,
			0, FALSE, nullptr, fileStream.put());
		winrt::check_hresult(hr);

		winrt::Windows::Storage::Streams::IRandomAccessStream raStream;
		winrt::check_hresult(::CreateRandomAccessStreamOverStream(
			fileStream.get(), BSOS_DEFAULT,
			winrt::guid_of<winrt::Windows::Storage::Streams::IRandomAccessStream>(),
			winrt::put_abi(raStream)));

		auto decoder = WGI::BitmapDecoder::CreateAsync(raStream).get();
		return decoder.GetSoftwareBitmapAsync().get();
	}

	// Fallback: create SoftwareBitmap from raw RGB pixel data already decoded
	// by cPicture. Covers formats cPicture supports but Windows BitmapDecoder
	// doesn’t (e.g. PDF pages decoded via the cPicture PDF plugin, exotic RAW).
	winrt::Windows::Graphics::Imaging::SoftwareBitmap LoadBitmapFromPixels(const picture_data& pd)
	{
		namespace WGI = winrt::Windows::Graphics::Imaging;

		if (pd.requested_data_list.empty())
			throw std::runtime_error("no pixel data");

		const auto& rd = pd.requested_data_list.front();
		const int w = rd.picture_width;
		const int h = rd.picture_height;
		if (!rd.data || w <= 0 || h <= 0)
			throw std::runtime_error("invalid pixel data");

		// Convert RGB (24 bpp) → BGRA (32 bpp) for SoftwareBitmap
		std::vector<uint8_t> bgra(static_cast<size_t>(w) * h * 4);
		for (int i = 0; i < w * h; ++i)
		{
			bgra[static_cast<size_t>(i) * 4 + 0] = rd.data[static_cast<size_t>(i) * 3 + 2]; // B
			bgra[static_cast<size_t>(i) * 4 + 1] = rd.data[static_cast<size_t>(i) * 3 + 1]; // G
			bgra[static_cast<size_t>(i) * 4 + 2] = rd.data[static_cast<size_t>(i) * 3 + 0]; // R
			bgra[static_cast<size_t>(i) * 4 + 3] = 255;                                      // A
		}

		WGI::SoftwareBitmap bitmap(WGI::BitmapPixelFormat::Bgra8, w, h,
			WGI::BitmapAlphaMode::Ignore);

		// Write pixels via IMemoryBufferByteAccess (manually declared above)
		{
			auto buffer    = bitmap.LockBuffer(WGI::BitmapBufferAccessMode::Write);
			auto ref       = buffer.CreateReference();
			auto byteAccess = ref.as<IMemoryBufferByteAccess>();
			uint8_t* dst{};
			uint32_t cap{};
			winrt::check_hresult(byteAccess->GetBuffer(&dst, &cap));
			memcpy(dst, bgra.data(), min(static_cast<size_t>(cap), bgra.size()));
		}

		return bitmap;
	}

	// Run Windows OCR on a SoftwareBitmap.
	// Returns the recognized plain text, or an empty string.
	CString RunOcr(const winrt::Windows::Graphics::Imaging::SoftwareBitmap& bitmap)
	{
		namespace WMO = winrt::Windows::Media::Ocr;

		auto engine = WMO::OcrEngine::TryCreateFromUserProfileLanguages();
		if (!engine)
			return L"";

		auto result = engine.RecognizeAsync(bitmap).get();

		// Build result text from individual lines with explicit \r\n.
		// Using result.Lines() instead of result.Text() avoids relying on
		// line-separator conventions (\n vs \r\n) across SDK versions.
		CString text;
		for (const auto& line : result.Lines())
		{
			if (!text.IsEmpty())
				text += L"\r\n";

			text += line.Text().c_str();
		}
		return text;
	}
}


// ── CFunctionPluginOcr ───────────────────────────────────────────────────────

CFunctionPluginOcr::CFunctionPluginOcr()
	: handle_wnd(NULL)
{
}

struct plugin_data __stdcall CFunctionPluginOcr::get_plugin_data() const
{
	struct plugin_data d;
	d.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	d.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	d.info.LoadString(IDS_PLUGIN_INFO);
	return d;
}

struct arg_count __stdcall CFunctionPluginOcr::get_arg_count() const
{
	// Accept 1..10 images per run
	return arg_count(1, 10);
}

enum REQUEST_TYPE __stdcall CFunctionPluginOcr::start(
	const HWND hwnd,
	const vector<const WCHAR*>& /*file_list*/,
	vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;

	// Request full-resolution RGB pixel data as fallback.
	// Used when BitmapDecoder can't decode the file (e.g. PDF via cPicture plugin,
	// RAW formats). cPicture has already decoded these to RGB pixels.
	request_data_sizes.emplace_back(-100, -100, DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA);

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginOcr::process_picture(const picture_data& /*picture_data*/)
{
	// Collect all pictures before processing — return true to continue loading
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginOcr::end(
	const vector<picture_data>& picture_data_list)
{
	const bool bMultiple = picture_data_list.size() > 1;
	CString resultText;

	for (const auto& pd : picture_data_list)
	{
		if (pd.file_name.IsEmpty())
			continue;

		CString pageText;
		// WinRT .get() blocks on STA threads (MFC UI thread) and triggers an assertion.
		// Solution: run OCR on a new background thread (MTA) and join synchronously.
		std::thread worker([&pd, &pageText]()
		{
			try
			{
				winrt::init_apartment();	// MTA — allows .get() without deadlock
				winrt::Windows::Graphics::Imaging::SoftwareBitmap bitmap{ nullptr };
				try
				{
					bitmap = LoadBitmapFromFile(pd.file_name);
				}
				catch (...)
				{
					// Fallback: use pixel data decoded by cPicture
					// (handles PDF, exotic RAW and other cPicture-only formats)
					bitmap = LoadBitmapFromPixels(pd);
				}
				pageText = RunOcr(bitmap);
			}
			catch (...) {}
		});
		worker.join();

		if (bMultiple)
		{
			// Prefix each section with the file name
			const int slash = pd.file_name.ReverseFind(L'\\');
			resultText += (slash >= 0 ? pd.file_name.Mid(slash + 1) : pd.file_name);
			resultText += L"\r\n────────────────────────────────\r\n";
		}

		resultText += pageText.IsEmpty() ? CString() : pageText;
		if (bMultiple)
			resultText += L"\r\n\r\n";
	}

	if (resultText.IsEmpty())
	{
		CString msg;
		msg.LoadString(IDS_OCR_NO_TEXT);
		::MessageBox(handle_wnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);
		return update_data_list;
	}

	// Show result in a resizable dialog with copy-to-clipboard button
	COcrResultDlg dlg(handle_wnd, resultText);
	dlg.DoModal();

	// No image files were modified — return empty update list
	return update_data_list;
}
