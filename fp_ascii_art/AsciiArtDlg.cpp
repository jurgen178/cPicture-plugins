// AsciiDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AsciiArtDlg.h"
#include "vfw.h"
#include <sys/stat.h>
#include <map>
#include "locale.h"
#include <commctrl.h>


extern BOOL CreateFont2(CFont& font, const CString& fontName, const int fontHeight);

#define WM_POST_INITDIALOG (WM_USER + 1)


// CAsciiArtDlg dialog

CAsciiArtDlg::CAsciiArtDlg(const vector<picture_data>& picture_data_list, CWnd* pParent /*=NULL*/)
	: CDialog(CAsciiArtDlg::IDD, pParent),
	picture_data_list(picture_data_list),
	index(0),
	blocksize(72),
	brightness(0),
	contrast(0),
	pParentWnd(pParent)
{
	memset(&bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader.biPlanes = 1;
	bmiHeader.biBitCount = 24;
	bmiHeader.biCompression = BI_RGB;
}

CAsciiArtDlg::~CAsciiArtDlg()
{
}

void CAsciiArtDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PREVIEW, preview_position);
	DDX_Control(pDX, IDC_FONT_SELECT_COMBO, fontSelectComboBox);
	DDX_Control(pDX, IDC_EDIT_ASCII, ascii_display);
	DDX_Control(pDX, IDC_SLIDER_FONTSIZE, fontSizeSliderCtrl);
	DDX_Control(pDX, IDC_SLIDER_BLOCKSIZE, blockSizeSliderCtrl);
	DDX_Control(pDX, IDC_SLIDER_BRIGHTNESS, brightnessSliderCtrl);
	DDX_Control(pDX, IDC_SLIDER_CONTRAST, contrastSliderCtrl);
	DDX_Text(pDX, IDC_STATIC_TEXT_FONTSIZE, static_text_fontsize);
	DDX_Text(pDX, IDC_STATIC_TEXT_BRIGHTNESS, static_text_brightness);
	DDX_Text(pDX, IDC_STATIC_TEXT_CONTRAST, static_text_contrast);
	DDX_Text(pDX, IDC_STATIC_INFO, static_text_info);
}


BEGIN_MESSAGE_MAP(CAsciiArtDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_HSCROLL()
	ON_MESSAGE(WM_POST_INITDIALOG, &CAsciiArtDlg::OnPostInitDialog)
	ON_BN_CLICKED(IDC_BUTTON_COPY, &CAsciiArtDlg::OnClickedButtonCopy)
END_MESSAGE_MAP()


BOOL CAsciiArtDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	preview_position.GetClientRect(&preview_position_rect);
	preview_position.MapWindowPoints(this, &preview_position_rect);

	fontSizeSliderCtrl.SetRange(1, 40);  // Set the range.
	fontSizeSliderCtrl.SetPos(12);       // Set initial position.

	blockSizeSliderCtrl.SetRange(4, 200);	// Set the range.
	blockSizeSliderCtrl.SetPos(blocksize);	// Set initial position.

	brightnessSliderCtrl.SetRange(-127, 127);	// Set the range.
	brightnessSliderCtrl.SetPos(1);				// Default brightness=0 does not move the slider to the middle.
	brightnessSliderCtrl.SetPos(brightness);	// Set initial position.

	contrastSliderCtrl.SetRange(-100, 100);	// Set the range.
	contrastSliderCtrl.SetPos(1);			// Default contrast=0 does not move the slider to the middle.
	contrastSliderCtrl.SetPos(contrast);	// Set initial position.

	// Set text wrap mode for static control.
	CStatic* pStaticText = (CStatic*)GetDlgItem(IDC_STATIC_INFO);
	LONG_PTR styles = ::GetWindowLongPtr(pStaticText->GetSafeHwnd(), GWL_STYLE);
	styles |= SS_EDITCONTROL;
	::SetWindowLongPtr(pStaticText->GetSafeHwnd(), GWL_STYLE, styles);

	const size_t size(picture_data_list.size());
	CString str;
	str.LoadString(IDS_PLUGIN_LONG_DESC);
	SetWindowText(str);

	fontSelectComboBox.Init(pParentWnd, &CAsciiArtDlg::Update, this);

	// Post the custom message to be handled after OnInitDialog.
	PostMessage(WM_POST_INITDIALOG);

	return TRUE;
}

LRESULT CAsciiArtDlg::OnPostInitDialog(WPARAM wParam, LPARAM lParam)
{
	Update(fontSelectComboBox.GetSelectedFont());

	return 0;
}

void CAsciiArtDlg::UpdateDisplayFont(const CString fontName, const int fontsize)
{
	// Delete the old font if it exists.
	if (ascii_display_font.m_hObject)
		ascii_display_font.DeleteObject();

	CreateFont2(ascii_display_font, fontName, fontsize);

	ascii_display.SetFont(&ascii_display_font, FALSE);
	ascii_display.Invalidate();

	static_text_fontsize.Format(L"%d", fontsize);
	UpdateData(false); // write the data
}

void CAsciiArtDlg::Update(const CString fontName)
{
	const int pos(fontSizeSliderCtrl.GetPos());
	UpdateDisplayFont(fontName, pos);

	CFont cfont;
	const int fontHeight(blocksize);
	if (CreateFont2(cfont, fontName, fontHeight))
	{
		CPaintDC dc(this); // device context for painting

		// Create a memory DC and a bitmap.
		CDC memDC;
		memDC.CreateCompatibleDC(&dc);

		// Select the font into the memory DC.
		CFont* pOldFont = memDC.SelectObject(&cfont);

		// Character to rasterize. Fixed pitch fonts have the same size for all chars.
		WCHAR ch = L'A';

		// Measure the character size.
		const CSize size(memDC.GetTextExtent(&ch, 1));
		if (!(size.cx > 0 && size.cy > 0))
		{
			memDC.SelectObject(pOldFont);
			AfxMessageBox(IDS_CHAR_SIZE_ERROR, MB_ICONERROR);
			return;
		}

		// Create a monochrome bitmap with the character size.
		CBitmap bitmap;
		bitmap.CreateBitmap(size.cx, size.cy, 1, 1, nullptr);
		CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

		// Set text color and background mode.
		memDC.SetTextColor(RGB(255, 255, 255));
		memDC.SetBkMode(OPAQUE);
		memDC.SetBkColor(RGB(0, 0, 0));

		// Extract the bitmap data.
		BITMAP bm;
		bitmap.GetBitmap(&bm);
		const int width = bm.bmWidth;
		const int height = bm.bmHeight;
		const int area = width * height;
		const int sizeBytes = bm.bmWidthBytes * bm.bmHeight;
		BYTE* pBits = new BYTE[sizeBytes];

		// Fill the background with white.
		memDC.FillSolidRect(0, 0, size.cx, size.cy, RGB(0, 0, 0));

		// Create a map to store all the densities.
		std::map<double, WCHAR> densities;

		// Get the char densities.
		WCHAR letters[] = L" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
		for (wchar_t wc : letters)
		{
			if (wc == 0)
				continue;

			// Draw the character.
			memDC.TextOut(0, 0, &wc, 1);

			bitmap.GetBitmapBits(sizeBytes, pBits);

#ifdef DEBUG
			CString text;
#endif

			int ones = 0;
			int yOffset = 0;

			// Output the bitmap data as 0s and 1s.
			for (int y = 0; y < height; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					const int byteIndex = yOffset + x / 8;
					const int bitIndex = 7 - (x % 8);
					const bool bit = (pBits[byteIndex] & (1 << bitIndex)) != 0;

					if (bit)
						ones++;

#ifdef DEBUG
					text += bit ? L'1' : L'0';
#endif
				}

				// Next line.
				yOffset += bm.bmWidthBytes;

#ifdef DEBUG
				text += L'\n';
#endif
			}

			densities[(double)ones / area] = wc;
		}


#ifdef DEBUG
		_wsetlocale(LC_ALL, L"en-US");
		CString density_map_test_case;
		CString f;
		for (const auto& pair : densities)
		{
			f.Format(L"densities[%f] = L'%c';\n", pair.first, pair.second);
			density_map_test_case += f;
		}
#endif


		// Clean up.
		delete[] pBits;
		memDC.SelectObject(pOldBitmap);
		memDC.SelectObject(pOldFont);

		// Segment the grey scale picture and assign matching letters from the density map.
		// Each Segment is a rect.
		const int rect_w(size.cx);
		const int rect_h(size.cy);
		const int rect_area(rect_w * rect_h);

		// Array to map the normalized grey values 0..255 to the matching char.
		WCHAR densities_index[256] = { 0 };

		// End values of the density map.
		const double largestDensity(densities.rbegin()->first);
		const WCHAR smallestDensityChar(densities.begin()->second);
		const WCHAR largestDensityChar(densities.rbegin()->second);

		for (int i = 0; i < 256; ++i)
		{
			const double d(i * largestDensity / 255);

			// Find best matching density.
			for (const auto& pair : densities)
			{
				if (pair.first >= d)
				{
					// Invert (255-i)
					// white is 255 with the least density and black 0 with the max density.
					int index = 255 - i;

					// Contrast.
					// Shift the range to be symmetrical around x=0 (-127..0..127).
					// Stretch the range
					//  -100..0, factor 2..1 (low contrast)
					//  0, factor 1 (no contrast change)
					//  0..+100, factor 1..0 (high contrast)
					// Shift back the range (0..+127..+255)
					index = (index - 127) * (100 - contrast) / 100 + 127;

					// Brightness.
					// Shift the range by the brightness value.
					index -= brightness;

					if (index >= 0 && index < 256)
						densities_index[index] = pair.second;

					break;
				}
			}
		}

		// Brightness shifts the values. Fill the end gaps.
		// Right side gap.
		int i = 255;
		while (i > 0)
		{
			if (!densities_index[i])
				densities_index[i--] = smallestDensityChar;
			else
				break;
		}

		// Left side gap.
		i = 0;
		while (i < 255)
		{
			if (!densities_index[i])
				densities_index[i++] = largestDensityChar;
			else
				break;
		}

		// Contrast stretches the index. Fill any gaps with the neighboring value.
		i = 1;
		while (i < 255)
		{
			if (!densities_index[i])
				densities_index[i] = densities_index[i - 1];

			i++;
		}

		//// Brightness shifts the values. Fill the end gaps.
		//if (brightness > 0)
		//{
		//	for (int i = 0; i < brightness; ++i)
		//	{
		//		densities_index[255 - i] = smallestDensityChar;
		//	}
		//}
		//else
		//if (brightness < 0)
		//{
		//	for (int i = 0; i >= brightness; --i)
		//	{
		//		densities_index[-i] = largestDensityChar;
		//	}
		//}


		// Get the second requested data set (unresized picture resized, 100%).
		vector<picture_data>::const_iterator it = picture_data_list.begin();
		vector<requested_data> requested_data_list = it->requested_data_list;
		requested_data requested_data2 = requested_data_list.back();

		BYTE* data = requested_data2.data;

		// Map the segments to the chars.
		CString ascii_art;

		// Enumerate all rect segments.
		for (register int rect_y = 0; rect_y < requested_data2.picture_height - rect_h; rect_y += rect_h)
		{
			for (register int rect_x = 0; rect_x < requested_data2.picture_width - rect_w; rect_x += rect_w)
			{
				__int64 grey_sum = 0;

				// Read the rect segment at (rect_x, rect_y).
				for (register int y = 0; y < rect_h; y++)
				{
					for (register int x = 0; x < rect_w; x++)
					{
						const int index(3 * ((rect_y + y) * requested_data2.picture_width + rect_x + x));
						const BYTE grey(data[index]);
						grey_sum += grey;
					}
				}

				// average grey value mapped to 0..255
				const int density_index((int)(grey_sum / rect_area));

				// Add density matching char.
				const WCHAR w(densities_index[density_index]);
				ascii_art += w;
			}

			ascii_art += L"\r\n";
		}

		ascii_display.SetWindowText(ascii_art);

		CString usedChars;
		for (const auto& pair : densities)
		{
			usedChars += pair.second;
		}

		// L"%+d" outputs '+0' for the zero value
		// \x200B is zero width space (ZWSP) because L'' is invalid for %c
		static_text_brightness.Format(L"%c%d", brightness > 0 ? L'+' : L'\x200B', brightness);
		static_text_contrast.Format(L"%c%d", contrast > 0 ? L'+' : L'\x200B', contrast);

		static_text_info.Format(IDS_STRING_INFO,
			requested_data2.picture_width / rect_w,
			requested_data2.picture_height / rect_h,
			size.cx, size.cy,
			densities.size(),
			usedChars);

		UpdateData(false); // write the data
	}
}

void CAsciiArtDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	const HWND hwnd = pScrollBar->GetSafeHwnd();

	if (hwnd == fontSizeSliderCtrl.GetSafeHwnd())
	{
		const int pos = fontSizeSliderCtrl.GetPos();
		UpdateDisplayFont(fontSelectComboBox.GetSelectedFont(), pos);
	}
	else
	if (hwnd == blockSizeSliderCtrl.GetSafeHwnd())
	{
		blocksize = blockSizeSliderCtrl.GetPos();
		Update(fontSelectComboBox.GetSelectedFont());
	}
	else
	if (hwnd == brightnessSliderCtrl.GetSafeHwnd())
	{
		brightness = brightnessSliderCtrl.GetPos();
		Update(fontSelectComboBox.GetSelectedFont());
	}
	else
	if (hwnd == contrastSliderCtrl.GetSafeHwnd())
	{
		contrast = contrastSliderCtrl.GetPos();
		Update(fontSelectComboBox.GetSelectedFont());
	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CAsciiArtDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	const int size(static_cast<int>(picture_data_list.size()));
	if (index >= 0 && index < size)
	{
		vector<picture_data>::const_iterator it = picture_data_list.begin() + index;

		vector<requested_data> requested_data_list = it->requested_data_list;
		// Get the data for the requested dialog preview picture size.
		requested_data requested_data1 = requested_data_list.front();

		// Draw the selected picture.
		bmiHeader.biWidth = requested_data1.picture_width;
		bmiHeader.biHeight = requested_data1.picture_height;
		const int left(preview_position_rect.left + (preview_position_rect.Width() - (int)requested_data1.picture_width) / 2);
		const int top(preview_position_rect.top + (preview_position_rect.Height() - (int)requested_data1.picture_height) / 2);

		HDRAWDIB hdd = DrawDibOpen();

		DrawDibDraw(hdd,
			dc.m_hDC,
			left,
			top,
			requested_data1.picture_width,
			requested_data1.picture_height,
			&bmiHeader,
			requested_data1.data,
			0,
			0,
			requested_data1.picture_width,
			requested_data1.picture_height,
			0
		);

		DrawDibClose(hdd);
	}
}

void CAsciiArtDlg::TextToClipboard(CString text)
{
	const int text_lenW = text.GetLength();

	// Allocate a global memory object for the text. 
	HGLOBAL hglbCopyW = GlobalAlloc(GMEM_MOVEABLE, (text_lenW + 1) * sizeof(WCHAR));
	if (hglbCopyW == NULL)
	{
		return;
	}

	// Lock the handle and copy the text to the buffer. 
	LPWSTR lpwstrCopyW = (WCHAR*)GlobalLock(hglbCopyW);
	memcpy(lpwstrCopyW, text, text_lenW * sizeof(WCHAR));
	lpwstrCopyW[text_lenW] = (WCHAR)0;    // null character 
	GlobalUnlock(hglbCopyW);

	if (::OpenClipboard(NULL))
	{
		// Place the handle on the clipboard. 
		::SetClipboardData(CF_UNICODETEXT, hglbCopyW);

		// Close the clipboard. 
		::CloseClipboard();
	}
	else
	{
		::GlobalFree(hglbCopyW);
	}
}

void CAsciiArtDlg::OnClickedButtonCopy()
{
	CString text;
	ascii_display.GetWindowText(text);
	TextToClipboard(text);

	AfxMessageBox(IDS_CLIPBOARD_COPY_TEXT, MB_ICONINFORMATION);
}
