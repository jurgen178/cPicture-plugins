
#include "FontSelectComboBox.h"


static BOOL CALLBACK EnumFontProc(LPLOGFONT lplf, LPTEXTMETRIC lptm, DWORD dwType, LPARAM lpData)
{
	CFontSelectComboBox* pThis = reinterpret_cast<CFontSelectComboBox*>(lpData);

	if ((lplf->lfPitchAndFamily & 0x03) == FIXED_PITCH &&	// MONO_FONT
		lplf->lfWeight == 400
		)
	{
		int index = pThis->AddString(lplf->lfFaceName);
		ASSERT(index != -1);

		int maxLen = lptm->tmMaxCharWidth * wcslen(lplf->lfFaceName);
		int ret = pThis->SetItemData(index, dwType);

		ASSERT(ret != -1);
	}

	return TRUE;
}

CFontSelectComboBox::CFontSelectComboBox()
{
	m_iFontHeight = 50;
	m_iMaxNameWidth = 0;
}

CFontSelectComboBox::~CFontSelectComboBox()
{
}

BEGIN_MESSAGE_MAP(CFontSelectComboBox, CComboBox)
	ON_WM_MEASUREITEM()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropdown)
END_MESSAGE_MAP()

void CFontSelectComboBox::Init()
{
	CClientDC dc(this);

	EnumFonts(dc, 0, (FONTENUMPROC)EnumFontProc, (LPARAM)this); //Enumerate font

	SetItemHeight(-1, m_iFontHeight);
	SetCurSel(0);
}

void CFontSelectComboBox::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	ASSERT(lpDIS->CtlType == ODT_COMBOBOX);

	CRect rc = lpDIS->rcItem;

	CDC dc;
	dc.Attach(lpDIS->hDC);

	if (lpDIS->itemState & ODS_FOCUS)
		dc.DrawFocusRect(&rc);

	if (lpDIS->itemID == -1)
		return;

	int nIndexDC = dc.SaveDC();

	CBrush br;

	if (lpDIS->itemState & ODS_SELECTED)
	{
		br.CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
		dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else
	{
		br.CreateSolidBrush(dc.GetBkColor());
	}

	dc.SetBkMode(TRANSPARENT);
	dc.FillRect(&rc, &br);

	CString csCurFontName;
	GetLBText(lpDIS->itemID, csCurFontName);

	int iOffsetX = 0;// SPACING;

	CFont cf;
	if (!cf.CreateFont(m_iFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, csCurFontName))
	{
		ASSERT(0);
		return;
	}

	CSize sz;
	int iPosY = 0;
	HFONT hf = (HFONT)dc.SelectObject(cf);
	sz = dc.GetTextExtent(csCurFontName);
	iPosY = (rc.Height() - sz.cy) / 2;
	dc.TextOut(rc.left + iOffsetX, rc.top + iPosY, csCurFontName);
	dc.SelectObject(hf);

	dc.RestoreDC(nIndexDC);

	dc.Detach();
}

void CFontSelectComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	CString csFontName;
	GetLBText(lpMeasureItemStruct->itemID, csFontName);

	CFont cf;
	if (!cf.CreateFont(m_iFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, csFontName))
	{
		ASSERT(0);
		return;
	}

	LOGFONT lf;
	cf.GetLogFont(&lf);

	CClientDC dc(this);
	HFONT hf = (HFONT)dc.SelectObject(cf);
	if (hf)
	{
	CSize sz = dc.GetTextExtent(csFontName);
	m_iMaxNameWidth = max(m_iMaxNameWidth, sz.cx);
	}
		dc.SelectObject(hf);

	lpMeasureItemStruct->itemHeight = lf.lfHeight + 4;
}

void CFontSelectComboBox::OnDropdown()
{
	int nScrollWidth = ::GetSystemMetrics(SM_CXVSCROLL);
	int nWidth = nScrollWidth;
	//nWidth += GLYPH_WIDTH;

	nWidth += m_iMaxNameWidth;

	SetDroppedWidth(nWidth);
}
