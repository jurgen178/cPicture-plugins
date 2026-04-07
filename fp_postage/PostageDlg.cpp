#include "stdafx.h"
#include "PostageDlg.h"

CPostageDlg::CPostageDlg(const vector<picture_data>& picture_data_list, CWnd* pParent /*=NULL*/)
	: CDialog(CPostageDlg::IDD, pParent),
	  picture_data_list(picture_data_list)
{
}

CPostageDlg::~CPostageDlg()
{
}

void CPostageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER_BORDER, m_sliderBorder);
	DDX_Control(pDX, IDC_STATIC_BORDER_VAL, m_staticBorderVal);
	DDX_Control(pDX, IDC_COMBO_PAPER, m_comboPaper);
	DDX_Control(pDX, IDC_EDIT_VALUE, m_editValue);
	DDX_Control(pDX, IDC_CHECK_STAMP, m_checkStamp);
	DDX_Control(pDX, IDC_EDIT_STAMP_TEXT, m_editStampText);
	DDX_Control(pDX, IDC_SLIDER_STAMP_ANGLE, m_sliderStampAngle);
	DDX_Control(pDX, IDC_STATIC_STAMP_ANGLE_VAL, m_staticStampAngleVal);
	DDX_Control(pDX, IDC_SLIDER_STAMP_STRENGTH, m_sliderStampStrength);
	DDX_Control(pDX, IDC_STATIC_STAMP_STRENGTH_VAL, m_staticStampStrengthVal);
	DDX_Control(pDX, IDC_PREVIEW, m_preview);
}

BEGIN_MESSAGE_MAP(CPostageDlg, CDialog)
	ON_WM_PAINT()
	ON_EN_CHANGE(IDC_EDIT_VALUE, OnChanged)
	ON_BN_CLICKED(IDC_CHECK_STAMP, OnChanged)
	ON_EN_CHANGE(IDC_EDIT_STAMP_TEXT, OnChanged)
	ON_CBN_SELCHANGE(IDC_COMBO_PAPER, OnChanged)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

BOOL CPostageDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_preview.GetClientRect(&preview_rect);
	m_preview.MapWindowPoints(this, &preview_rect);

	m_sliderBorder.SetRange(0, 10);
	m_sliderBorder.SetPos(max(0, min(settings.border_percent, 10)));

	m_sliderStampAngle.SetRange(-45, 45);
	m_sliderStampAngle.SetPos(max(-45, min(settings.stamp_angle, 45)));

	m_sliderStampStrength.SetRange(10, 100);
	m_sliderStampStrength.SetPos(max(10, min(settings.stamp_strength, 100)));

	CString text;
	text.LoadString(IDS_PAPER_WHITE);
	m_comboPaper.AddString(text);
	text.LoadString(IDS_PAPER_CREAM);
	m_comboPaper.AddString(text);
	text.LoadString(IDS_PAPER_VINTAGE);
	m_comboPaper.AddString(text);
	m_comboPaper.SetCurSel(max(0, min(settings.paper_style, 2)));

	m_editValue.SetWindowText(settings.value_text);
	m_checkStamp.SetCheck(settings.show_stamp ? BST_CHECKED : BST_UNCHECKED);
	m_editStampText.SetWindowText(settings.stamp_text);

	UpdateLabels();
	UpdateStampControls();
	return TRUE;
}

void CPostageDlg::OnOK()
{
	settings.border_percent = m_sliderBorder.GetPos();
	settings.paper_style = max(0, m_comboPaper.GetCurSel());
	m_editValue.GetWindowText(settings.value_text);
	settings.show_stamp = m_checkStamp.GetCheck() == BST_CHECKED;
	m_editStampText.GetWindowText(settings.stamp_text);
	settings.stamp_angle = m_sliderStampAngle.GetPos();
	settings.stamp_strength = m_sliderStampStrength.GetPos();
	CDialog::OnOK();
}

void CPostageDlg::UpdateLabels()
{
	CString text;
	text.Format(L"%d", m_sliderBorder.GetPos());
	m_staticBorderVal.SetWindowText(text);
	text.Format(L"%d", m_sliderStampAngle.GetPos());
	m_staticStampAngleVal.SetWindowText(text);
	text.Format(L"%d", m_sliderStampStrength.GetPos());
	m_staticStampStrengthVal.SetWindowText(text);
}

void CPostageDlg::UpdateStampControls()
{
	const BOOL enabled = m_checkStamp.GetCheck() == BST_CHECKED;
	m_editStampText.EnableWindow(enabled);
	m_sliderStampAngle.EnableWindow(enabled);
	m_sliderStampStrength.EnableWindow(enabled);
}

void CPostageDlg::OnChanged()
{
	UpdateStampControls();
	Invalidate(FALSE);
}

void CPostageDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	UpdateLabels();
	Invalidate(FALSE);
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPostageDlg::OnPaint()
{
	CPaintDC dc(this);
	const int width = preview_rect.Width();
	const int height = preview_rect.Height();
	if (width <= 0 || height <= 0)
		return;

	CDC mem_dc;
	mem_dc.CreateCompatibleDC(&dc);
	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&dc, width, height);
	CBitmap* old_bitmap = mem_dc.SelectObject(&bitmap);

	CRect local_rect(0, 0, width, height);
	mem_dc.FillSolidRect(local_rect, GetSysColor(COLOR_BTNFACE));
	mem_dc.SetViewportOrg(-preview_rect.left, -preview_rect.top);
	DrawPreview(mem_dc);
	mem_dc.SetViewportOrg(0, 0);

	dc.BitBlt(preview_rect.left, preview_rect.top, width, height, &mem_dc, 0, 0, SRCCOPY);
	mem_dc.SelectObject(old_bitmap);
}

void CPostageDlg::DrawPreview(CDC& dc)
{
	if (picture_data_list.empty())
		return;
	const picture_data& picture = picture_data_list.front();
	if (picture.requested_data_list.empty())
		return;

	const requested_data& source = picture.requested_data_list.front();
	if (source.data == nullptr || source.picture_width <= 0 || source.picture_height <= 0)
		return;

	PostageSettings preview_settings;
	preview_settings.border_percent = m_sliderBorder.GetSafeHwnd() ? m_sliderBorder.GetPos() : settings.border_percent;
	preview_settings.paper_style = m_comboPaper.GetSafeHwnd() ? max(0, m_comboPaper.GetCurSel()) : settings.paper_style;
	preview_settings.show_stamp = m_checkStamp.GetSafeHwnd() ? (m_checkStamp.GetCheck() == BST_CHECKED) : settings.show_stamp;
	preview_settings.stamp_angle = m_sliderStampAngle.GetSafeHwnd() ? m_sliderStampAngle.GetPos() : settings.stamp_angle;
	preview_settings.stamp_strength = m_sliderStampStrength.GetSafeHwnd() ? m_sliderStampStrength.GetPos() : settings.stamp_strength;
	preview_settings.value_text = settings.value_text;
	preview_settings.stamp_text = settings.stamp_text;
	if (m_editValue.GetSafeHwnd())
		m_editValue.GetWindowText(preview_settings.value_text);
	if (m_editStampText.GetSafeHwnd())
		m_editStampText.GetWindowText(preview_settings.stamp_text);

	DrawPostagePreview(dc, preview_rect, source, preview_settings);
}
