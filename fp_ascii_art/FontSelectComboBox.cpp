
#include "FontSelectComboBox.h"
#include "shellscalingapi.h"


static BOOL CALLBACK EnumFontProc(LPLOGFONT lplf, LPTEXTMETRIC lptm, DWORD dwType, LPARAM lpData)
{
	CFontSelectComboBox* pFontSelectComboBox = (CFontSelectComboBox*)lpData;

	if ((lplf->lfPitchAndFamily & 0x03) == FIXED_PITCH &&	// MONO_FONT
		lplf->lfWeight == 400
		)
	{
		const int index(pFontSelectComboBox->AddString(lplf->lfFaceName));
		pFontSelectComboBox->SetItemData(index, dwType);
	}

	return TRUE;
}

CFontSelectComboBox::CFontSelectComboBox()
{
	maxFontNameWidth = 0;
}

CFontSelectComboBox::~CFontSelectComboBox()
{
}

BEGIN_MESSAGE_MAP(CFontSelectComboBox, CComboBox)
	ON_WM_MEASUREITEM()
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropdown)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnCbnSelchange)
END_MESSAGE_MAP()

void CFontSelectComboBox::Init(CWnd* pParent)
{
	pParentWnd = pParent;

	// Get height of a large text line to be used for the font dropdown using the menu height property.
	UINT dpiX(96);
	UINT dpiY(96);
	HMONITOR hMonitor = MonitorFromWindow(pParentWnd->m_hWnd, MONITOR_DEFAULTTONEAREST);
	HRESULT hr = ::GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

	// Get the height of the menu for the combo box height.
	const int lineHeight(::GetSystemMetrics(SM_CYMENU) / 2);

	// Adjust for DPI scaling.
	ctrlHeight = hr == S_OK ? ctrlHeight = MulDiv(lineHeight, dpiY, 96) : 50;

	// Get all fixed pitch fonts.
	CClientDC dc(this);
	EnumFonts(dc, 0, (FONTENUMPROC)EnumFontProc, (LPARAM)this); //Enumerate font

	SetItemHeight(-1, ctrlHeight);
	SetCurSel(0);
}

void CFontSelectComboBox::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	if (lpDIS->itemID == -1)
		return;

	CString fontName;
	GetLBText(lpDIS->itemID, fontName);

	CFont cfont;
	const int fontHeight(8 * ctrlHeight / 10);	// 80%
	if (cfont.CreateFont(
		fontHeight,                // Height
		0,                         // Width
		0,                         // Escapement
		0,                         // Orientation
		FW_NORMAL,                 // Weight
		FALSE,                     // Italic
		FALSE,                     // Underline
		0,                         // StrikeOut
		DEFAULT_CHARSET,           // CharSet
		OUT_DEFAULT_PRECIS,        // OutPrecision
		CLIP_DEFAULT_PRECIS,       // ClipPrecision
		DEFAULT_QUALITY,           // Quality
		FIXED_PITCH,	           // PitchAndFamily
		fontName)                  // Facename
		)
	{
		CDC dc;
		dc.Attach(lpDIS->hDC);

		CRect rc(lpDIS->rcItem);

		if (lpDIS->itemState & ODS_FOCUS)
			dc.DrawFocusRect(&rc);

		const int indexDC(dc.SaveDC());

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

		HFONT hf = (HFONT)dc.SelectObject(cfont);

		const int iPosY((rc.Height() - dc.GetTextExtent(fontName).cy) / 2);
		dc.TextOut(rc.left, rc.top + iPosY, fontName);

		dc.SelectObject(hf);
		dc.RestoreDC(indexDC);
		dc.Detach();
	}
}

void CFontSelectComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	CString fontName;
	GetLBText(lpMeasureItemStruct->itemID, fontName);

	CFont cfont;
	const int fontHeight(8 * ctrlHeight / 10);	// 80%
	if (cfont.CreateFont(
		fontHeight,                // Height
		0,                         // Width
		0,                         // Escapement
		0,                         // Orientation
		FW_NORMAL,                 // Weight
		FALSE,                     // Italic
		FALSE,                     // Underline
		0,                         // StrikeOut
		DEFAULT_CHARSET,           // CharSet
		OUT_DEFAULT_PRECIS,        // OutPrecision
		CLIP_DEFAULT_PRECIS,       // ClipPrecision
		DEFAULT_QUALITY,           // Quality
		FIXED_PITCH,	           // PitchAndFamily
		fontName)                  // Facename
		)
	{
		LOGFONT lf;
		cfont.GetLogFont(&lf);
		lpMeasureItemStruct->itemHeight = 12 * lf.lfHeight / 10;	// lfHeight + 20% vertical distance

		// Get max width of the font name.
		CClientDC dc(this);
		HFONT hf = (HFONT)dc.SelectObject(cfont);
		maxFontNameWidth = max(maxFontNameWidth, dc.GetTextExtent(fontName).cx);
		dc.SelectObject(hf);
	}
}

void CFontSelectComboBox::OnDropdown()
{
	const int width(::GetSystemMetrics(SM_CXVSCROLL) + maxFontNameWidth);
	SetDroppedWidth(width);
}

void CFontSelectComboBox::OnCbnSelchange()
{
	const int selIndex(GetCurSel());
	if (selIndex != CB_ERR)
	{
		CString fontName;
		GetLBText(selIndex, fontName);

		CFont cfont;
		const int fontHeight(8 * ctrlHeight / 10);	// 80%
		if (cfont.CreateFont(
			fontHeight,                // Height
			0,                         // Width
			0,                         // Escapement
			0,                         // Orientation
			FW_NORMAL,                 // Weight
			FALSE,                     // Italic
			FALSE,                     // Underline
			0,                         // StrikeOut
			DEFAULT_CHARSET,           // CharSet
			OUT_DEFAULT_PRECIS,        // OutPrecision
			CLIP_DEFAULT_PRECIS,       // ClipPrecision
			DEFAULT_QUALITY,           // Quality
			FIXED_PITCH,	           // PitchAndFamily
			fontName)                  // Facename
			)
		{
			CPaintDC dc(this); // device context for painting

			// Create a memory DC and a bitmap
			CDC memDC;
			memDC.CreateCompatibleDC(&dc);

			// Select the font into the memory DC
			CFont* pOldFont = memDC.SelectObject(&cfont);

			// Character to rasterize
			WCHAR ch = L'A';

			// Measure the character size
			CSize size = memDC.GetTextExtent(&ch, 1);

			WCHAR letters[] = L"abcdefghijklmnopqrstuvwxyz";
			for (wchar_t wc : letters)
			{
				CSize size1 = memDC.GetTextExtent(&wc, 1);
				size1.cx++;
			}

			// Create a monochrome bitmap with the character size
			CBitmap bitmap;
			bitmap.CreateBitmap(size.cx, size.cy, 1, 1, nullptr);
			CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

			// Fill the background with white
			memDC.FillSolidRect(0, 0, size.cx, size.cy, RGB(255, 255, 255));

			// Set text color and background mode
			memDC.SetTextColor(RGB(0, 0, 0));
			memDC.SetBkMode(OPAQUE);
			memDC.SetBkColor(RGB(255, 255, 255));

			// Draw the character
			memDC.TextOut(0, 0, &ch, 1);

			CString text;

			// Extract the bitmap data
			BITMAP bm;
			bitmap.GetBitmap(&bm);
			int width = bm.bmWidth;
			int height = bm.bmHeight;
			int sizeBytes = bm.bmWidthBytes * bm.bmHeight;
			BYTE* pBits = new BYTE[sizeBytes];
			bitmap.GetBitmapBits(sizeBytes, pBits);

			// Output the bitmap data as 0s and 1s
			for (int y = 0; y < height; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					int byteIndex = y * bm.bmWidthBytes + x / 8;
					int bitIndex = 7 - (x % 8);
					bool bit = (pBits[byteIndex] & (1 << bitIndex)) == 0;
					text += bit ? L'1' : L'0';
				}
				text += L'\n';
			}

			// Clean up
			delete[] pBits;
			memDC.SelectObject(pOldBitmap);
			memDC.SelectObject(pOldFont);
		}
	}
}
