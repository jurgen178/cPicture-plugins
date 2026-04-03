// QrCodeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "QrCodeDlg.h"
#include "CornerPickerCtrl.h"
#include "qrcode.h"


// CQrCodeDlg dialog

CQrCodeDlg::CQrCodeDlg(const vector<picture_data>& picture_data_list, CWnd* pParent /*=NULL*/)
	: CDialog(CQrCodeDlg::IDD, pParent),
	  picture_data_list(picture_data_list),
	  corner(3),
	  text(L""),
	  relative_size(20),
	  margin_percent(2)
{
	memset(&bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader.biPlanes = 1;
	bmiHeader.biBitCount = 24;
	bmiHeader.biCompression = BI_RGB;
}

CQrCodeDlg::~CQrCodeDlg()
{
}

void CQrCodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CORNER_PICKER, m_cornerPicker);
	DDX_Control(pDX, IDC_EDIT_TEXT, m_editText);
	DDX_Control(pDX, IDC_EDIT_SIZE, m_editSize);
	DDX_Control(pDX, IDC_EDIT_MARGIN, m_editMargin);
	DDX_Control(pDX, IDC_PREVIEW, m_preview);
}


BEGIN_MESSAGE_MAP(CQrCodeDlg, CDialog)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_CORNER_PICKER, OnChanged)
	ON_EN_CHANGE(IDC_EDIT_TEXT, OnChanged)
	ON_EN_CHANGE(IDC_EDIT_SIZE, OnChanged)
	ON_EN_CHANGE(IDC_EDIT_MARGIN, OnChanged)
END_MESSAGE_MAP()


BOOL CQrCodeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Measure the preview control rect in dialog coordinates.
	m_preview.GetClientRect(&preview_rect);
	m_preview.MapWindowPoints(this, &preview_rect);

	m_cornerPicker.SetCorner(corner);

	// Set text edit.
	m_editText.SetWindowText(text);

	// Set size edit (show default percentage).
	CString sizeStr;
	sizeStr.Format(L"%d", relative_size);
	m_editSize.SetWindowText(sizeStr);

	// Set margin edit.
	margin_percent = max(0, min(margin_percent, 25));
	CString marginStr;
	marginStr.Format(L"%d", margin_percent);
	m_editMargin.SetWindowText(marginStr);

	return TRUE;
}


void CQrCodeDlg::OnOK()
{
	// Read corner selection.
	corner = m_cornerPicker.GetCorner();

	// Read text.
	m_editText.GetWindowText(text);

	// Read and validate relative size.
	CString sizeStr;
	m_editSize.GetWindowText(sizeStr);
	relative_size = _wtoi(sizeStr);
	if (relative_size < 5)
		relative_size = 5;
	if (relative_size > 50)
		relative_size = 50;

	// Read and validate margin.
	CString marginStr;
	m_editMargin.GetWindowText(marginStr);
	margin_percent = _wtoi(marginStr);
	if (margin_percent < 0)
		margin_percent = 0;
	if (margin_percent > 25)
		margin_percent = 25;

	CDialog::OnOK();
}


void CQrCodeDlg::OnPaint()
{
	CPaintDC dc(this);
	DrawPreview(dc);
}


void CQrCodeDlg::OnChanged()
{
	Invalidate(FALSE);
}


void CQrCodeDlg::DrawPreview(CDC& dc)
{
	if (picture_data_list.empty())
		return;

	const picture_data& pd = picture_data_list.front();
	if (pd.requested_data_list.empty())
		return;

	// Index 0 = preview-sized BGR display data (first picture only).
	const requested_data& rd = pd.requested_data_list[0];
	if (rd.data == nullptr || rd.picture_width == 0 || rd.picture_height == 0)
		return;

	// Center image in preview_rect.
	const int left = preview_rect.left + (preview_rect.Width() - rd.picture_width) / 2;
	const int top = preview_rect.top + (preview_rect.Height() - rd.picture_height) / 2;

	bmiHeader.biWidth = rd.picture_width;
	bmiHeader.biHeight = rd.picture_height;

	StretchDIBits(dc.m_hDC,
		left, top, rd.picture_width, rd.picture_height,
		0, 0, rd.picture_width, rd.picture_height,
		rd.data,
		reinterpret_cast<const BITMAPINFO*>(&bmiHeader),
		DIB_RGB_COLORS, SRCCOPY);

	// Read current settings from controls for live preview.
	int cur_corner = corner;
	int cur_size = relative_size;
	int cur_margin = margin_percent;
	if (m_cornerPicker.GetSafeHwnd())
		cur_corner = m_cornerPicker.GetCorner();
	if (m_editSize.GetSafeHwnd())
	{
		CString sizeStr;
		m_editSize.GetWindowText(sizeStr);
		const int v = _wtoi(sizeStr);
		if (v >= 5)
			cur_size = min(v, 50);
	}
	if (m_editMargin.GetSafeHwnd())
	{
		CString marginStr;
		m_editMargin.GetWindowText(marginStr);
		cur_margin = max(0, min(_wtoi(marginStr), 25));
	}

	// Calculate QR size in preview image coordinates.
	const int shorter_side = min(rd.picture_width, rd.picture_height);
	const int qr_size = shorter_side * cur_size / 100;
	if (qr_size < 8)
		return;

	const int margin = max(0, shorter_side * cur_margin / 100);

	// Read current text for real QR generation.
	CString cur_text = text;
	if (m_editText.GetSafeHwnd())
		m_editText.GetWindowText(cur_text);

	// Generate real QR code.
	std::string utf8;
	{
		int len = WideCharToMultiByte(CP_UTF8, 0, cur_text, -1, nullptr, 0, nullptr, nullptr);
		if (len > 1) { utf8.resize(len - 1); WideCharToMultiByte(CP_UTF8, 0, cur_text, -1, &utf8[0], len, nullptr, nullptr); }
	}

	std::vector<bool> qrBitmap;
	int modules = 0;
	bool qrOk = !utf8.empty() && GenerateQRCode(utf8, QRErrorLevel::M, qrBitmap, modules) && modules > 0;

	const int modulePx  = qrOk ? max(1, qr_size / modules) : 1;
	const int drawSize  = qrOk ? modules * modulePx : qr_size;

	// Anchor position: use drawSize so the QR code always touches the margin edge.
	int qr_x, qr_y;
	switch (cur_corner)
	{
	case 0: // Top-Left
		qr_x = margin;
		qr_y = margin;
		break;
	case 1: // Top-Right
		qr_x = rd.picture_width - drawSize - margin;
		qr_y = margin;
		break;
	case 2: // Bottom-Left
		qr_x = margin;
		qr_y = rd.picture_height - drawSize - margin;
		break;
	default: // Bottom-Right
		qr_x = rd.picture_width - drawSize - margin;
		qr_y = rd.picture_height - drawSize - margin;
		break;
	}

	// Offset to dialog coordinates.
	qr_x += left;
	qr_y += top;

	if (qrOk)
	{
		for (int my = 0; my < modules; my++)
			for (int mx = 0; mx < modules; mx++)
			{
				COLORREF c = qrBitmap[my * modules + mx] ? RGB(0, 0, 0) : RGB(255, 255, 255);
				dc.FillSolidRect(qr_x + mx * modulePx, qr_y + my * modulePx, modulePx, modulePx, c);
			}
	}
	else
	{
		// Fallback: white placeholder when text is empty.
		dc.FillSolidRect(qr_x, qr_y, qr_size, qr_size, RGB(255, 255, 255));
	}
}
