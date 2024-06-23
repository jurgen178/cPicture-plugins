// SampleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SampleDlg.h"
#include "vfw.h"
#include <sys/stat.h>


unsigned int GetFileSize(const WCHAR* pFile)
{
	unsigned int file_size = 0;

	FILE* infile = NULL;
	const errno_t err(_wfopen_s(&infile, pFile, L"rb"));

	if (err == 0)
	{
		struct _stat fbuf;
		_fstat(_fileno(infile), &fbuf);

		file_size = fbuf.st_size;

		fclose(infile);
	}

	return file_size;
}


// CSampleDlg dialog

CSampleDlg::CSampleDlg(const vector<picture_data>& picture_data_list, CWnd* pParent /*=NULL*/)
  : CDialog(CSampleDlg::IDD, pParent),
	picture_data_list(picture_data_list),
	index(0)
{
	memset(&bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmiHeader.biPlanes = 1;
	bmiHeader.biBitCount = 24;
	bmiHeader.biCompression = BI_RGB;
}

CSampleDlg::~CSampleDlg()
{
}

void CSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PREVIEW, preview_position);
	DDX_Control(pDX, IDC_INFO, info);
	DDX_Control(pDX, IDC_INFO2, info2);
	DDX_Control(pDX, IDC_COUNTER, counter);
}


BEGIN_MESSAGE_MAP(CSampleDlg, CDialog)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_BUTTON_NEXT, OnBnClickedButtonNext)
	ON_BN_CLICKED(IDC_BUTTON_PREV, OnBnClickedButtonPrev)
END_MESSAGE_MAP()


BOOL CSampleDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	update_button_state();

	preview_position.GetClientRect(&preview_position_rect);
	preview_position.MapWindowPoints(this, &preview_position_rect);

	const size_t size(picture_data_list.size());
	CString str;

	if(size == 0)
		str.LoadString(IDS_EMPTY_LIST);
	else
		str.Format(IDS_N_ITEMS_IN_LIST, picture_data_list.size());
	
	SetWindowText(str);

	return TRUE;
}

void CSampleDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	const int size(static_cast<int>(picture_data_list.size()));
	if(index >= 0 && index < size)
	{
		vector<picture_data>::const_iterator it = picture_data_list.begin() + index;

		vector<requested_data> requested_data_list = it->requested_data_list;
		// Get the data for the requested dialog preview picture size.
		requested_data requested_data1 = requested_data_list.front();

		info.SetWindowText(it->file_name);

		CString str;
		str.Format(L"%d/%d", index+1, size);
		counter.SetWindowText(str);

		// Set Description text.
		const unsigned int fsize = ::GetFileSize(it->file_name);
		str.Format(IDS_PICTURE_SIZE, it->picture_width, it->picture_height, fsize >> 10);
		info2.SetWindowText(str);

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

void CSampleDlg::OnBnClickedButtonNext()
{
	if(index < (int)picture_data_list.size() - 1)
	{
		index++;
		RedrawWindow();
		update_button_state();
	}
}

void CSampleDlg::OnBnClickedButtonPrev()
{
	if(index > 0)
	{
		index--;
		RedrawWindow();
		update_button_state();
	}
}

void CSampleDlg::update_button_state()
{
	GetDlgItem(IDC_BUTTON_PREV)->EnableWindow(index > 0);
	GetDlgItem(IDC_BUTTON_NEXT)->EnableWindow(index < (int)picture_data_list.size() - 1);
}
