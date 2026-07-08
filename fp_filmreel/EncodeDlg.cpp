#include "stdafx.h"
#include "EncodeDlg.h"


BEGIN_MESSAGE_MAP(CEncodeDlg, CDialog)
	ON_BN_CLICKED(IDCANCEL, OnCancel)
	ON_MESSAGE(WM_ENCODE_PROGRESS, OnEncodeProgress)
	ON_MESSAGE(WM_ENCODE_DONE, OnEncodeDone)
END_MESSAGE_MAP()

namespace
{
	constexpr int RGB_CHANNELS = 3;

	int MinInt(const int a, const int b)
	{
		return a < b ? a : b;
	}

	int MaxInt(const int a, const int b)
	{
		return a > b ? a : b;
	}

	bool BuildCanvasRgbFrame(
		const unsigned char* src,
		const int                  srcWidth,
		const int                  srcHeight,
		const int                  canvasWidth,
		const int                  canvasHeight,
		const COLORREF             backgroundColor,
		std::vector<unsigned char>& canvas)
	{
		if (!src || srcWidth <= 0 || srcHeight <= 0 || canvasWidth <= 0 || canvasHeight <= 0)
			return false;

		canvas.resize(static_cast<size_t>(canvasWidth) * canvasHeight * RGB_CHANNELS);

		const unsigned char backgroundRed = static_cast<unsigned char>(GetRValue(backgroundColor));
		const unsigned char backgroundGreen = static_cast<unsigned char>(GetGValue(backgroundColor));
		const unsigned char backgroundBlue = static_cast<unsigned char>(GetBValue(backgroundColor));
		// Common case: black/white/gray backgrounds have identical RGB bytes and can
		// fill the whole packed RGB canvas with a single memset.
		if (backgroundRed == backgroundGreen && backgroundGreen == backgroundBlue)
		{
			memset(canvas.data(), backgroundRed, canvas.size());
		}
		else
		{
			// For colored backgrounds, build one packed RGB row and duplicate it.
			// That avoids per-pixel work across the full canvas height.
			const size_t rowSize = static_cast<size_t>(canvasWidth) * RGB_CHANNELS;
			unsigned char* firstRow = canvas.data();
			for (int x = 0; x < canvasWidth; ++x)
			{
				unsigned char* pixel = firstRow + static_cast<size_t>(x) * RGB_CHANNELS;
				pixel[0] = backgroundRed;
				pixel[1] = backgroundGreen;
				pixel[2] = backgroundBlue;
			}

			for (int row = 1; row < canvasHeight; ++row)
				memcpy(canvas.data() + static_cast<size_t>(row) * rowSize, firstRow, rowSize);
		}

		const int copyWidth = MinInt(srcWidth, canvasWidth);
		const int copyHeight = MinInt(srcHeight, canvasHeight);
		const int offsetX = (canvasWidth - copyWidth) / 2;
		const int offsetY = (canvasHeight - copyHeight) / 2;
		const size_t copyRowSize = static_cast<size_t>(copyWidth) * RGB_CHANNELS;
		const size_t canvasRowSize = static_cast<size_t>(canvasWidth) * RGB_CHANNELS;

		if (srcWidth == canvasWidth)
		{
			// Same row stride: the centered frame only has vertical padding, so the
			// source block is contiguous in the destination and can be copied at once.
			memcpy(canvas.data() + static_cast<size_t>(offsetY) * canvasRowSize, src, static_cast<size_t>(copyHeight) * canvasRowSize);
		}
		else
		{
			// Different row strides need one copy per source row to preserve horizontal
			// centering inside the canvas.
			for (int y = 0; y < copyHeight; ++y)
			{
				const unsigned char* srcRow = src + (static_cast<size_t>(y) * srcWidth * RGB_CHANNELS);
				unsigned char* dstRow = canvas.data() + (static_cast<size_t>(offsetY + y) * canvasRowSize) + (static_cast<size_t>(offsetX) * RGB_CHANNELS);
				memcpy(dstRow, srcRow, copyRowSize);
			}
		}

		return true;
	}
}


CEncodeDlg::CEncodeDlg(
	HWND                        hWndParent,
	const vector<picture_data>& pictureData,
	const AnimSettings& settings,
	const CString& outputPath)
	: CDialog(IDD_ENCODE_PROGRESS, CWnd::FromHandle(hWndParent)),
	m_pictureData(pictureData),
	m_settings(settings),
	m_outputPath(outputPath),
	m_cancelled(false),
	m_success(false)
{
}

BOOL CEncodeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Initialise progress bar range
	auto* pBar = reinterpret_cast<CProgressCtrl*>(GetDlgItem(IDC_ENCODE_PROGRESS_BAR));
	pBar->SetRange32(0, static_cast<int>(m_pictureData.size()));
	pBar->SetPos(0);

	// Start worker thread
	m_worker = std::thread([this]() { WorkerProc(); });

	return TRUE;
}

void CEncodeDlg::OnCancel()
{
	// Signal worker to stop; disable button to prevent double-click
	m_cancelled = true;
	GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
	// Dialog stays open — OnEncodeDone closes it once the worker finishes
}

LRESULT CEncodeDlg::OnEncodeProgress(WPARAM wParam, LPARAM lParam)
{
	const int current = static_cast<int>(wParam);
	const int total = static_cast<int>(lParam);

	reinterpret_cast<CProgressCtrl*>(GetDlgItem(IDC_ENCODE_PROGRESS_BAR))->SetPos(current);

	// Show current filename (basename only)
	if (current > 0 && current <= static_cast<int>(m_pictureData.size()))
	{
		const CString& fullPath = m_pictureData[current - 1].file_name;
		const int slash = fullPath.ReverseFind(L'\\');
		SetDlgItemText(IDC_ENCODE_FILENAME, slash >= 0 ? fullPath.Mid(slash + 1) : fullPath);
	}

	// Show frame counter
	CString fmt, text;
	fmt.LoadString(IDS_ENCODING_FRAME);
	text.Format(fmt, current, total);
	SetDlgItemText(IDC_ENCODE_TEXT, text);

	return 0;
}

LRESULT CEncodeDlg::OnEncodeDone(WPARAM wParam, LPARAM lParam)
{
	m_success = (wParam != 0) && !m_cancelled.load();

	// Join worker before closing
	if (m_worker.joinable())
		m_worker.join();

	EndDialog(IDOK);
	return 0;
}


// ── Worker thread ────────────────────────────────────────────────────────────

void CEncodeDlg::WorkerProc()
{
	const int totalFrames = static_cast<int>(m_pictureData.size());

	if (totalFrames == 0)
	{
		PostMessage(WM_ENCODE_DONE, 0, 0);
		return;
	}

	// WebP has one fixed animation canvas. Use the largest cPicture-resized
	// frame so smaller frames can be centered without a second resize.
	int width = 0;
	int height = 0;
	for (const auto& pd : m_pictureData)
	{
		if (!pd.requested_data_list.empty()
			&& pd.requested_data_list.front().picture_width > 0
			&& pd.requested_data_list.front().picture_height > 0)
		{
			width = MaxInt(width, pd.requested_data_list.front().picture_width);
			height = MaxInt(height, pd.requested_data_list.front().picture_height);
		}
	}

	if (width <= 0 || height <= 0)
	{
		PostMessage(WM_ENCODE_DONE, 0, 0);
		return;
	}

	// Set up WebP animation encoder
	WebPAnimEncoderOptions enc_options;
	WebPAnimEncoderOptionsInit(&enc_options);
	enc_options.anim_params.loop_count = m_settings.loop_count;

	WebPAnimEncoder* enc = WebPAnimEncoderNew(width, height, &enc_options);
	if (!enc)
	{
		PostMessage(WM_ENCODE_DONE, 0, 0);
		return;
	}

	WebPConfig config;
	WebPConfigInit(&config);
	config.lossless = m_settings.lossless ? 1 : 0;
	config.quality = static_cast<float>(m_settings.quality);

	bool encodeOk = true;
	int  timestamp = 0;
	int  framesDone = 0;

	for (const auto& pd : m_pictureData)
	{
		if (m_cancelled.load())
			break;

		if (pd.requested_data_list.empty())
			continue;

		const auto& rd = pd.requested_data_list.front();
		if (!rd.data || rd.picture_width <= 0 || rd.picture_height <= 0)
			continue;

		std::vector<unsigned char> canvasFrame;
		const unsigned char* frameData = rd.data;
		int frameStride = rd.picture_width * RGB_CHANNELS;
		int frameWidth = rd.picture_width;
		int frameHeight = rd.picture_height;

		// Fast path: same-size frames are passed through unchanged. Smaller frames
		// are only copied into a colored canvas; cPicture already did the resize.
		if (rd.picture_width != width || rd.picture_height != height)
		{
			if (!BuildCanvasRgbFrame(rd.data, rd.picture_width, rd.picture_height, width, height, m_settings.background_color, canvasFrame))
			{
				encodeOk = false;
				break;
			}

			frameData = canvasFrame.data();
			frameStride = width * RGB_CHANNELS;
			frameWidth = width;
			frameHeight = height;
		}

		WebPPicture pic;
		WebPPictureInit(&pic);
		pic.width = frameWidth;
		pic.height = frameHeight;
		pic.use_argb = 1;

		if (!WebPPictureImportRGB(&pic, frameData, frameStride)
			|| !WebPAnimEncoderAdd(enc, &pic, timestamp, &config))
		{
			WebPPictureFree(&pic);
			encodeOk = false;
			break;
		}

		WebPPictureFree(&pic);
		timestamp += m_settings.delay_ms;
		++framesDone;

		// Report progress to UI thread
		PostMessage(WM_ENCODE_PROGRESS, framesDone, totalFrames);
	}

	// Flush animation with final empty frame
	if (encodeOk && !m_cancelled.load())
		WebPAnimEncoderAdd(enc, nullptr, timestamp, nullptr);

	WebPData webp_data;
	WebPDataInit(&webp_data);
	bool writeOk = false;

	if (encodeOk && !m_cancelled.load() && WebPAnimEncoderAssemble(enc, &webp_data))
	{
		FILE* f = nullptr;
		if (_wfopen_s(&f, m_outputPath, L"wb") == 0 && f)
		{
			fwrite(webp_data.bytes, 1, webp_data.size, f);
			fclose(f);
			writeOk = true;
		}
	}

	WebPDataClear(&webp_data);
	WebPAnimEncoderDelete(enc);

	PostMessage(WM_ENCODE_DONE, (encodeOk && writeOk) ? 1 : 0, 0);
}
