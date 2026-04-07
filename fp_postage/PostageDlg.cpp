#include "stdafx.h"
#include "PostageDlg.h"
#include <afxdlgs.h>

void CTextColorPreviewCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (lpDrawItemStruct == nullptr)
		return;

	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);

	CRect rect(lpDrawItemStruct->rcItem);
	dc.FrameRect(&rect, &CBrush(::GetSysColor(COLOR_WINDOWFRAME)));
	rect.DeflateRect(1, 1);
	dc.FillSolidRect(rect, m_previewColor);

	dc.Detach();
}

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
	DDX_Control(pDX, IDC_SLIDER_VALUE_MARGIN, m_sliderValueMargin);
	DDX_Control(pDX, IDC_STATIC_VALUE_MARGIN_VAL, m_staticValueMarginVal);
	DDX_Control(pDX, IDC_COMBO_PAPER, m_comboPaper);
	DDX_Control(pDX, IDC_BUTTON_TEXT_FONT, m_buttonFont);
	DDX_Control(pDX, IDC_BUTTON_TEXT_COLOR, m_buttonTextColor);
	DDX_Control(pDX, IDC_STATIC_TEXT_COLOR, m_staticTextColor);
	DDX_Control(pDX, IDC_EDIT_VALUE, m_editValue);
	DDX_Control(pDX, IDC_VALUE_CORNER, m_valueCorner);
	DDX_Control(pDX, IDC_PREVIEW, m_preview);
}

BEGIN_MESSAGE_MAP(CPostageDlg, CDialog)
	ON_WM_PAINT()
	ON_EN_CHANGE(IDC_EDIT_VALUE, OnChanged)
	ON_BN_CLICKED(IDC_VALUE_CORNER, OnChanged)
	ON_BN_CLICKED(IDC_BUTTON_TEXT_FONT, OnChooseFont)
	ON_BN_CLICKED(IDC_BUTTON_TEXT_COLOR, OnChooseTextColor)
	ON_CBN_SELCHANGE(IDC_COMBO_PAPER, OnChanged)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

BOOL CPostageDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_preview.GetClientRect(&preview_rect);
	m_preview.MapWindowPoints(this, &preview_rect);
	settings.border_percent = max(0, min(settings.border_percent, 10));
	if (settings.border_percent < 2)
		settings.border_percent = 0;

	m_sliderBorder.SetRange(0, 10);
	m_sliderBorder.SetPos(settings.border_percent);
	m_sliderValueMargin.SetRange(0, 20);
	m_sliderValueMargin.SetPos(max(0, min(settings.value_margin_percent, 20)));

	CString text;
	text.LoadString(IDS_PAPER_WHITE);
	m_comboPaper.AddString(text);
	text.LoadString(IDS_PAPER_OFFWHITE);
	m_comboPaper.AddString(text);
	text.LoadString(IDS_PAPER_CREAM);
	m_comboPaper.AddString(text);
	text.LoadString(IDS_PAPER_VINTAGE);
	m_comboPaper.AddString(text);
	m_comboPaper.SetCurSel(max(0, min(settings.paper_style, 3)));
	m_valueCorner.SetCorner(settings.value_corner);

	m_editValue.SetWindowText(settings.value_text);
	UpdateFontButtonLabel();
	UpdateTextColorBrush();

	UpdateLabels();
	return TRUE;
}

void CPostageDlg::OnOK()
{
	settings.border_percent = GetEffectiveBorderPercent();
	settings.value_margin_percent = m_sliderValueMargin.GetPos();
	settings.paper_style = max(0, m_comboPaper.GetCurSel());
	m_editValue.GetWindowText(settings.value_text);
	settings.value_corner = m_valueCorner.GetCorner();
	CDialog::OnOK();
}

int CPostageDlg::GetEffectiveBorderPercent() const
{
	const int border_percent = m_sliderBorder.GetSafeHwnd() ? m_sliderBorder.GetPos() : settings.border_percent;
	return border_percent < 2 ? 0 : border_percent;
}

void CPostageDlg::UpdateLabels()
{
	CString text;
	text.Format(L"%d", GetEffectiveBorderPercent());
	m_staticBorderVal.SetWindowText(text);
	text.Format(L"%d", m_sliderValueMargin.GetPos());
	m_staticValueMarginVal.SetWindowText(text);
}

void CPostageDlg::UpdateFontButtonLabel()
{
	CString face_name(settings.value_font.lfFaceName);
	face_name.Trim();
	if (face_name.IsEmpty())
		face_name = L"Segoe UI";
	if (m_buttonFont.GetSafeHwnd())
		m_buttonFont.SetWindowText(face_name);
}

void CPostageDlg::UpdateTextColorBrush()
{
	m_staticTextColor.SetPreviewColor(settings.value_color);
}

void CPostageDlg::OnChanged()
{
	Invalidate(FALSE);
}

void CPostageDlg::OnChooseFont()
{
	LOGFONT font_info = settings.value_font;
	CFontDialog font_dialog(&font_info, CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS, nullptr, this);
	if (font_dialog.DoModal() != IDOK)
		return;

	settings.value_font = font_info;
	UpdateFontButtonLabel();
	Invalidate(FALSE);
}

void CPostageDlg::OnChooseTextColor()
{
	CColorDialog color_dialog(settings.value_color, CC_ANYCOLOR | CC_FULLOPEN, this);
	if (color_dialog.DoModal() != IDOK)
		return;

	settings.value_color = color_dialog.GetColor();
	UpdateTextColorBrush();
	Invalidate(FALSE);
}

void CPostageDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar != nullptr && pScrollBar->GetSafeHwnd() == m_sliderBorder.GetSafeHwnd() && m_sliderBorder.GetPos() < 2)
		m_sliderBorder.SetPos(0);

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
	preview_settings.border_percent = GetEffectiveBorderPercent();
	preview_settings.value_margin_percent = m_sliderValueMargin.GetSafeHwnd() ? m_sliderValueMargin.GetPos() : settings.value_margin_percent;
	preview_settings.paper_style = m_comboPaper.GetSafeHwnd() ? max(0, m_comboPaper.GetCurSel()) : settings.paper_style;
	preview_settings.value_corner = m_valueCorner.GetSafeHwnd() ? m_valueCorner.GetCorner() : settings.value_corner;
	preview_settings.value_font = settings.value_font;
	preview_settings.value_color = settings.value_color;
	preview_settings.value_text = settings.value_text;
	if (m_editValue.GetSafeHwnd())
		m_editValue.GetWindowText(preview_settings.value_text);

	DrawPostagePreview(dc, preview_rect, source, preview_settings);
}
