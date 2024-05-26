// SampleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SampleDlg.h"
#include "vfw.h"
#include <sys/stat.h>


// CSampleDlg dialog

CSampleDlg::CSampleDlg(const vector<picture_data>& picture_data_list, CWnd* pParent /*=NULL*/)
  : CDialog(CSampleDlg::IDD, pParent),
	m_hImageList(NULL),
	m_picture_data_list(picture_data_list)
{
	// Setup the price for the prints in cents.
	picture_price[PICTURE_FORMAT_9] = 10;
	picture_price[PICTURE_FORMAT_10] = 20;
}

CSampleDlg::~CSampleDlg()
{
	if(m_hImageList)
		ImageList_Destroy(m_hImageList);
}

void CSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PICTURE_LIST, m_PictureListCtrl);
	DDX_Control(pDX, IDC_COMBO_FORMAT, m_Format);
	DDX_Control(pDX, IDC_EDIT_NUMBER_OF_PRINTS, m_Prints);
	DDX_Control(pDX, IDC_STATIC_TOTAL_PRICE, m_StaticTotalPrice);
	DDX_Control(pDX, IDC_STATIC_NUMBER_OF_PICTURES, m_StaticNumberOfPictures);
}


BEGIN_MESSAGE_MAP(CSampleDlg, CDialog)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_PICTURE_LIST, OnNMCustomdrawPictureList)
	ON_BN_CLICKED(IDC_BUTTON_SET, OnBnClickedButtonSet)
END_MESSAGE_MAP()


BOOL CSampleDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CString str;
	str.LoadString(IDS_PICTURE_FORMAT_9);
	m_Format.AddString(str);
	str.LoadString(IDS_PICTURE_FORMAT_10);
	m_Format.AddString(str);

	m_Format.SetCurSel(PICTURE_FORMAT_10);
	m_Prints.SetWindowText(L"1");

	const int size(static_cast<int>(m_picture_data_list.size()));
	m_hImageList = ImageList_Create(size_x, size_y, ILC_COLOR24, 0, size);
	ImageList_SetImageCount(m_hImageList, size);
	::SendMessage(m_PictureListCtrl.m_hWnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)m_hImageList);
	m_PictureListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_FLATSB);

	str = L"";
	int col = 0;
	m_PictureListCtrl.InsertColumn(col++, str, LVCFMT_LEFT, size_x+2);
	str.LoadString(IDS_COLUMNHEADER_NAME);
	m_PictureListCtrl.InsertColumn(col++, str, LVCFMT_LEFT);

	LVITEM LviData;
	memset(&LviData, 0, sizeof(LVITEM));
	LviData.mask = LVIF_IMAGE;

	LVITEM lvi;
	lvi.stateMask = LVIS_STATEIMAGEMASK;
	lvi.state = INDEXTOSTATEIMAGEMASK(2);

	BITMAPINFO bmInfo;
	memset(&bmInfo, 0, sizeof(BITMAPINFO));
	bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmInfo.bmiHeader.biPlanes = 1;
	bmInfo.bmiHeader.biBitCount = 24;
	bmInfo.bmiHeader.biCompression = BI_RGB;
	bmInfo.bmiHeader.biWidth = size_x;
	bmInfo.bmiHeader.biHeight = size_y;

	const int size3(3*size_x*size_y);
	BYTE draw_buf[size3];	// the new buffer
	BYTE bufBk[size3];
	memset(bufBk, 255, size3);

	CDC* pDC = GetDC();
	HBITMAP hBmp = ::CreateCompatibleBitmap(pDC->m_hDC, size_x, size_y);

	m_picture_orders.reserve(m_picture_data_list.size());

	int i = 0;
	for(vector<picture_data>::const_iterator it = m_picture_data_list.begin(); it != m_picture_data_list.end(); ++it, i++)
	{
		vector<requested_data> requested_data_set = it->requested_data_set;
		requested_data requested_data1 = requested_data_set.back();

		m_picture_orders.push_back(picture_order(it->m_FileName));

		m_PictureListCtrl.InsertItem(i, L"");

		// reset the background of the bitmap bits
		memcpy(draw_buf, bufBk, size3);

		if(requested_data1.buf)
		{
			// copy and center the bitmap
			const int XDest = (requested_data1.picture_width < size_x) ? (size_x - requested_data1.picture_width) / 2 : 0;
			const int XDest2 = 3 * requested_data1.picture_width;
			const int YDest = (requested_data1.picture_height < size_y) ? (size_y - requested_data1.picture_height) / 2 : 0;

			int index = 0;
			int index2 = 3*(XDest+YDest*size_x);
			#define WIDTH_DWORD_ALIGNED(pixel)    ((((pixel * 24) + 31) >> 3) & ~0x03)
			const UINT WidthBytes = WIDTH_DWORD_ALIGNED(requested_data1.picture_width);

			for(int y = YDest; y < YDest + requested_data1.picture_height; y++)
			{ 
				memcpy(draw_buf + index2, requested_data1.buf + index, XDest2);
				index += WidthBytes;
				index2 += 3*size_x;
			}
		}

		::SetDIBits(pDC->m_hDC, hBmp, 0, size_y, draw_buf, &bmInfo, DIB_RGB_COLORS);
		ImageList_Replace(m_hImageList, i, hBmp, NULL);
		LviData.iImage = i;
		LviData.iItem = i;
		::SendMessage(m_PictureListCtrl.m_hWnd, LVM_SETITEM, 0, (LPARAM)&LviData);
	}

	m_PictureListCtrl.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	ReleaseDC(pDC);
	
	update_total();

	return TRUE;
}

void CSampleDlg::update_total(int& numberOfPictures, int& totalPrice)
{
	numberOfPictures = 0;
	totalPrice = 0;
	for (vector<picture_order>::const_iterator it = m_picture_orders.begin(); it != m_picture_orders.end(); ++it)
	{
		numberOfPictures += it->m_number_of_prints;
		totalPrice += picture_price[it->m_picture_format] * it->m_number_of_prints;
	}
}

void CSampleDlg::update_total()
{
	int numberOfPictures = 0;
	int totalPrice = 0;
	update_total(numberOfPictures, totalPrice);

	CString str;
	str.Format(L"%d", numberOfPictures);
	m_StaticNumberOfPictures.SetWindowText(str);
	CString fmt;
	fmt.LoadString(IDS_TOTAL_PRICE_FORMAT);
	str.Format(fmt, (float)totalPrice / 100);
	m_StaticTotalPrice.SetWindowText(str);

	UpdateData(false); // write the data
}

void CSampleDlg::OnNMCustomdrawPictureList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

    *pResult = CDRF_DODEFAULT;

    if(CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else
	if(CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	}
	else
	if((CDDS_ITEMPREPAINT | CDDS_SUBITEM | CDDS_ITEM) == pLVCD->nmcd.dwDrawStage)
	{
		if(pLVCD->iSubItem == 1)
		{
			const int iItem(static_cast<int>(pLVCD->nmcd.dwItemSpec));
			CRect rect;
			m_PictureListCtrl.GetItemRect(iItem, rect, LVIR_BOUNDS);
			LVCOLUMN lvcolumn;
			memset(&lvcolumn, 0, sizeof(LVCOLUMN));
			lvcolumn.mask = LVCF_WIDTH;
			m_PictureListCtrl.GetColumn(0, &lvcolumn);
			rect.left += lvcolumn.cx + 2;
			rect.top += 2;
			
			const bool selected((pLVCD->nmcd.uItemState & (CDIS_FOCUS | CDIS_SELECTED)) != 0);

			CString format;
			m_Format.GetLBText(m_picture_orders[iItem].m_picture_format, format);

			CString text;
			text.FormatMessage(IDS_DISPLAY_FORMAT,
				m_picture_data_list[iItem].m_FileName.Mid(m_picture_data_list[iItem].m_FileName.ReverseFind('\\')+1),
				m_picture_data_list[iItem].picture_width,
				m_picture_data_list[iItem].picture_height,
				m_picture_orders[iItem].m_number_of_prints,
				format
				);			
			
			FillRect(pLVCD->nmcd.hdc, &rect, (HBRUSH) (COLOR_3DHILIGHT+1));
			DrawText(pLVCD->nmcd.hdc, text, static_cast<int>(wcslen(text)), rect, DT_END_ELLIPSIS | DT_EDITCONTROL | DT_WORDBREAK | DT_NOPREFIX);
	
			*pResult = CDRF_SKIPDEFAULT;
		}
	}
}

void CSampleDlg::OnBnClickedButtonSet()
{
	UpdateData(true); // read the data

	int nHitItem = m_PictureListCtrl.GetNextItem(-1, LVNI_SELECTED);

	if (nHitItem != -1)
	{
		PICTURE_FORMAT picture_format((PICTURE_FORMAT)m_Format.GetCurSel());
		CString str;
		m_Prints.GetWindowText(str);

		const int number_of_prints(_wtol(str));

		// Simple validation.

		// Ignore negative numbers.
		if (number_of_prints < 0)
		{
			return;
		}

		// Only max 100 prints can be ordered.
		if (number_of_prints > 100)
		{
			AfxMessageBox(IDS_MAX_PRINTS);
			return;
		}

		while (nHitItem != -1 && nHitItem < static_cast<int>(m_picture_orders.size()))
		{
			m_picture_orders[nHitItem].m_picture_format = picture_format;
			m_picture_orders[nHitItem].m_number_of_prints = number_of_prints;

			nHitItem = m_PictureListCtrl.GetNextItem(nHitItem, LVNI_SELECTED);
		}

		update_total();
		m_PictureListCtrl.RedrawWindow();
	}
}
