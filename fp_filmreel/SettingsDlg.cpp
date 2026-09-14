#include "stdafx.h"
#include "SettingsDlg.h"

#include <afxdlgs.h>


BEGIN_MESSAGE_MAP(CColorPreviewCtrl, CStatic)
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CColorPreviewCtrl::OnPaint()
{
	CPaintDC dc(this);
	CRect rect;
	GetClientRect(&rect);
	dc.FrameRect(&rect, &CBrush(::GetSysColor(COLOR_WINDOWFRAME)));
	rect.DeflateRect(1, 1);
	dc.FillSolidRect(rect, m_previewColor);
}

BEGIN_MESSAGE_MAP(CSettingsDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_LOSSLESS, OnLosslessChanged)
	ON_BN_CLICKED(IDC_BUTTON_BACKGROUND_COLOR, OnChooseBackgroundColor)
END_MESSAGE_MAP()


CSettingsDlg::CSettingsDlg(const AnimSettings& defaults, CWnd* pParent)
	: CDialog(IDD_SETTINGS, pParent),
	m_settings(defaults)
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_BACKGROUND_COLOR_PREVIEW, m_backgroundColorPreview);
}

BOOL CSettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Spinners
	reinterpret_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_DELAY))->SetRange32(10, 60000);
	reinterpret_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_LOOP))->SetRange32(0, 999);
	reinterpret_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_QUALITY))->SetRange32(0, 100);

	SetDlgItemInt(IDC_EDIT_DELAY,   m_settings.delay_ms);
	SetDlgItemInt(IDC_EDIT_LOOP,    m_settings.loop_count);
	SetDlgItemInt(IDC_EDIT_QUALITY, m_settings.quality);
	CheckDlgButton(IDC_CHECK_LOSSLESS, m_settings.lossless ? BST_CHECKED : BST_UNCHECKED);
	UpdateBackgroundColorPreview();

	// Output width combo
	CComboBox* pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_WIDTH));
	const int presets[] = { 640, 800, 1280, 1920, 2560, 3840 };
	for (int px : presets)
	{
		CString s;
		s.Format(L"%d", px);
		pCombo->AddString(s);
	}
	CString sOriginal;
	sOriginal.LoadString(IDS_DLG_WIDTH_ORIGINAL);
	pCombo->AddString(sOriginal);

	if (m_settings.output_width == 0)
		pCombo->SelectString(-1, sOriginal);
	else
	{
		CString sWidth;
		sWidth.Format(L"%d", m_settings.output_width);
		if (pCombo->SelectString(-1, sWidth) == CB_ERR)
			pCombo->SetWindowText(sWidth);	// custom value
	}

	UpdateQualityState();
	return TRUE;
}

void CSettingsDlg::OnOK()
{
	m_settings.delay_ms    = static_cast<int>(GetDlgItemInt(IDC_EDIT_DELAY));
	m_settings.loop_count  = static_cast<int>(GetDlgItemInt(IDC_EDIT_LOOP));
	m_settings.quality     = static_cast<int>(GetDlgItemInt(IDC_EDIT_QUALITY));
	m_settings.lossless    = (IsDlgButtonChecked(IDC_CHECK_LOSSLESS) == BST_CHECKED);

	// Read output width
	{
		CString sWidth, sOriginal;
		GetDlgItemText(IDC_COMBO_WIDTH, sWidth);
		sOriginal.LoadString(IDS_DLG_WIDTH_ORIGINAL);
		if (sWidth.CompareNoCase(sOriginal) == 0)
			m_settings.output_width = 0;	// original full resolution
		else
			m_settings.output_width = max(64, min(9999, static_cast<int>(_wtoi(sWidth))));
	}

	m_settings.delay_ms   = max(10,  min(60000, m_settings.delay_ms));
	m_settings.loop_count = max(0,   min(999,   m_settings.loop_count));
	m_settings.quality    = max(0,   min(100,   m_settings.quality));

	CDialog::OnOK();
}

void CSettingsDlg::OnLosslessChanged()
{
	UpdateQualityState();
}

void CSettingsDlg::OnChooseBackgroundColor()
{
	CColorDialog dlg(m_settings.background_color, CC_ANYCOLOR | CC_FULLOPEN, this);
	if (dlg.DoModal() == IDOK)
	{
		m_settings.background_color = dlg.GetColor();
		UpdateBackgroundColorPreview();
	}
}

void CSettingsDlg::UpdateQualityState()
{
	const bool lossless = (IsDlgButtonChecked(IDC_CHECK_LOSSLESS) == BST_CHECKED);
	GetDlgItem(IDC_EDIT_QUALITY)->EnableWindow(!lossless);
	GetDlgItem(IDC_SPIN_QUALITY)->EnableWindow(!lossless);
	GetDlgItem(IDC_STATIC_QUALITY)->EnableWindow(!lossless);
}

void CSettingsDlg::UpdateBackgroundColorPreview()
{
	m_backgroundColorPreview.SetPreviewColor(m_settings.background_color);
}
