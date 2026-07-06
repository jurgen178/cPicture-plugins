#include "stdafx.h"
#include "EncodeDlg.h"


BEGIN_MESSAGE_MAP(CEncodeDlg, CDialog)
	ON_BN_CLICKED(IDCANCEL, OnCancel)
	ON_MESSAGE(WM_ENCODE_PROGRESS, OnEncodeProgress)
	ON_MESSAGE(WM_ENCODE_DONE,     OnEncodeDone)
END_MESSAGE_MAP()


CEncodeDlg::CEncodeDlg(
	HWND                        hWndParent,
	const vector<picture_data>& pictureData,
	const AnimSettings&         settings,
	const CString&              outputPath)
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
	const int total   = static_cast<int>(lParam);

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

	// Determine dimensions from first valid frame
	int width  = 0;
	int height = 0;
	for (const auto& pd : m_pictureData)
	{
		if (!pd.requested_data_list.empty()
			&& pd.requested_data_list.front().picture_width > 0)
		{
			width  = pd.requested_data_list.front().picture_width;
			height = pd.requested_data_list.front().picture_height;
			break;
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
	config.quality  = static_cast<float>(m_settings.quality);

	bool encodeOk    = true;
	int  timestamp   = 0;
	int  framesDone  = 0;

	for (const auto& pd : m_pictureData)
	{
		if (m_cancelled.load())
			break;

		if (pd.requested_data_list.empty())
			continue;

		const auto& rd = pd.requested_data_list.front();
		if (!rd.data || rd.picture_width <= 0 || rd.picture_height <= 0)
			continue;

		WebPPicture pic;
		WebPPictureInit(&pic);
		pic.width    = rd.picture_width;
		pic.height   = rd.picture_height;
		pic.use_argb = 1;

		if (!WebPPictureImportRGB(&pic, rd.data, rd.picture_width * 3)
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
