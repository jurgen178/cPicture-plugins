// QrCodeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "QrCodeDlg.h"


// CQrCodeDlg dialog

CQrCodeDlg::CQrCodeDlg(const vector<picture_data>& picture_data_list, CWnd* pParent /*=NULL*/)
	: CDialog(CQrCodeDlg::IDD, pParent),
	  picture_data_list(picture_data_list),
	  corner(3),
	  text(L""),
	  relative_size(20)
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
	DDX_Control(pDX, IDC_COMBO_CORNER, m_comboCorner);
	DDX_Control(pDX, IDC_EDIT_TEXT, m_editText);
	DDX_Control(pDX, IDC_EDIT_SIZE, m_editSize);
	DDX_Control(pDX, IDC_PREVIEW, m_preview);
}


BEGIN_MESSAGE_MAP(CQrCodeDlg, CDialog)
	ON_WM_PAINT()
	ON_CBN_SELCHANGE(IDC_COMBO_CORNER, OnChanged)
	ON_EN_CHANGE(IDC_EDIT_SIZE, OnChanged)
END_MESSAGE_MAP()


BOOL CQrCodeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Measure the preview control rect in dialog coordinates.
	m_preview.GetClientRect(&preview_rect);
	m_preview.MapWindowPoints(this, &preview_rect);

	CString str;
	str.LoadString(IDS_CORNER_TOP_LEFT);
	m_comboCorner.AddString(str);
	str.LoadString(IDS_CORNER_TOP_RIGHT);
	m_comboCorner.AddString(str);
	str.LoadString(IDS_CORNER_BOTTOM_LEFT);
	m_comboCorner.AddString(str);
	str.LoadString(IDS_CORNER_BOTTOM_RIGHT);
	m_comboCorner.AddString(str);
	m_comboCorner.SetCurSel(corner);

	// Set text edit.
	m_editText.SetWindowText(text);

	// Set size edit (show default percentage).
	CString sizeStr;
	sizeStr.Format(L"%d", relative_size);
	m_editSize.SetWindowText(sizeStr);

	return TRUE;
}


void CQrCodeDlg::OnOK()
{
	// Read corner selection.
	corner = m_comboCorner.GetCurSel();
	if (corner < 0)
		corner = 3; // fallback: Bottom-Right

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
	if (m_comboCorner.GetSafeHwnd())
	{
		const int sel = m_comboCorner.GetCurSel();
		if (sel >= 0)
			cur_corner = sel;
	}
	if (m_editSize.GetSafeHwnd())
	{
		CString sizeStr;
		m_editSize.GetWindowText(sizeStr);
		const int v = _wtoi(sizeStr);
		if (v >= 5)
			cur_size = min(v, 50);
	}

	// Calculate QR size in preview image coordinates.
	const int shorter_side = min(rd.picture_width, rd.picture_height);
	const int qr_size = shorter_side * cur_size / 100;
	if (qr_size < 8)
		return;

	const int margin = max(1, qr_size / 20);
	int qr_x, qr_y;
	switch (cur_corner)
	{
	case 0: // Top-Left
		qr_x = margin;
		qr_y = margin;
		break;
	case 1: // Top-Right
		qr_x = rd.picture_width - qr_size - margin;
		qr_y = margin;
		break;
	case 2: // Bottom-Left
		qr_x = margin;
		qr_y = rd.picture_height - qr_size - margin;
		break;
	default: // Bottom-Right
		qr_x = rd.picture_width - qr_size - margin;
		qr_y = rd.picture_height - qr_size - margin;
		break;
	}

	// Offset to dialog coordinates.
	qr_x += left;
	qr_y += top;

	// Draw white QR background.
	dc.FillSolidRect(qr_x, qr_y, qr_size, qr_size, RGB(255, 255, 255));

	// Draw black outer frame.
	const int bw = max(1, qr_size / 20);
	dc.FillSolidRect(qr_x, qr_y, qr_size, bw, RGB(0, 0, 0)); // top
	dc.FillSolidRect(qr_x, qr_y + qr_size - bw, qr_size, bw, RGB(0, 0, 0)); // bottom
	dc.FillSolidRect(qr_x, qr_y, bw, qr_size, RGB(0, 0, 0)); // left
	dc.FillSolidRect(qr_x + qr_size - bw, qr_y, bw, qr_size, RGB(0, 0, 0)); // right

	// Draw three finder pattern squares (TL, TR, BL of QR area).
	const int fp = max(5, qr_size / 4);
	const int fp_off = bw + 1;
	auto drawFinder = [&](int fx, int fy)
	{
		if (fp < 5)
			return;
		const int cell = max(1, fp / 7);
		const int core = cell * 2;
		dc.FillSolidRect(fx, fy, fp, fp, RGB(0, 0, 0));
		dc.FillSolidRect(fx + cell, fy + cell, fp - 2 * cell, fp - 2 * cell, RGB(255, 255, 255));
		dc.FillSolidRect(fx + core, fy + core, fp - 2 * core, fp - 2 * core, RGB(0, 0, 0));
	};
	drawFinder(qr_x + fp_off, qr_y + fp_off); // TL
	drawFinder(qr_x + qr_size - fp_off - fp, qr_y + fp_off); // TR
	drawFinder(qr_x + fp_off, qr_y + qr_size - fp_off - fp); // BL
}
