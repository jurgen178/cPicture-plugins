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
	if((infile = _wfopen(pFile, L"rb"))) 
	{
		struct _stat fbuf;
		_fstat(_fileno(infile), &fbuf);

		file_size = fbuf.st_size;

		fclose(infile);
	}

	return file_size;
}


// CSampleDlg dialog

CSampleDlg::CSampleDlg(const vector<picture_data>& _picture_data_list, CWnd* pParent /*=NULL*/)
  : CDialog(CSampleDlg::IDD, pParent),
	m_picture_data_list(_picture_data_list),
	m_index(0)
{
	memset(&m_bmiHeader, 0, sizeof(BITMAPINFOHEADER));
	m_bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bmiHeader.biPlanes = 1;
	m_bmiHeader.biBitCount = 24;
	m_bmiHeader.biCompression = BI_RGB;
}

CSampleDlg::~CSampleDlg()
{
}

void CSampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PREVIEW, m_PreviewPosition);
	DDX_Control(pDX, IDC_INFO, m_Info);
	DDX_Control(pDX, IDC_INFO2, m_Info2);
	DDX_Control(pDX, IDC_COUNTER, m_Counter);
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

	m_PreviewPosition.GetClientRect(&m_PreviewPositionRect);
	m_PreviewPosition.MapWindowPoints(this, &m_PreviewPositionRect);

	const size_t size(m_picture_data_list.size());
	CString str;

	if(size == 0)
		str.LoadString(IDS_EMPTY_LIST);
	else
	if(size == 1)
		str.LoadString(IDS_ONE_ITEM_IN_LIST);
	else
		str.Format(IDS_N_ITEMS_IN_LIST, m_picture_data_list.size());
	
	SetWindowText(str);

	return TRUE;
}

void CSampleDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	const int size(static_cast<int>(m_picture_data_list.size()));
	if(m_index >= 0 && m_index < size)
	{
		vector<picture_data>::const_iterator it = m_picture_data_list.begin() + m_index;

		m_Info.SetWindowText(it->m_name);

		CString str;
		str.Format(L"%d/%d", m_index+1, size);
		m_Counter.SetWindowText(str);

		const unsigned int fsize = ::GetFileSize(it->m_name);
		str.Format(L"%dx%d, %dKB", it->m_OriginalPictureWidth, it->m_OriginalPictureHeight, fsize >> 10);
		m_Info2.SetWindowText(str);


		m_bmiHeader.biWidth = it->m_PictureWidth;
		m_bmiHeader.biHeight = it->m_PictureHeight;
		const int left(m_PreviewPositionRect.left + (m_PreviewPositionRect.Width() - (int)it->m_PictureWidth) / 2);
		const int top(m_PreviewPositionRect.top + (m_PreviewPositionRect.Height() - (int)it->m_PictureHeight) / 2);

		HDRAWDIB hdd = DrawDibOpen();

		DrawDibDraw(hdd,             
					dc.m_hDC,                  
					left,                 
					top,                 
					it->m_PictureWidth,                
					it->m_PictureHeight, 
					&m_bmiHeader,  
					it->m_buf,            
					0,                 
					0,                 
					it->m_PictureWidth,                
					it->m_PictureHeight, 
					0
					);

		DrawDibClose(hdd);
	}
}

void CSampleDlg::OnBnClickedButtonNext()
{
	if(m_index < (int)m_picture_data_list.size() - 1)
	{
		m_index++;
		RedrawWindow();
		update_button_state();
	}
}

void CSampleDlg::OnBnClickedButtonPrev()
{
	if(m_index > 0)
	{
		m_index--;
		RedrawWindow();
		update_button_state();
	}
}

void CSampleDlg::update_button_state()
{
	GetDlgItem(IDC_BUTTON_PREV)->EnableWindow(m_index > 0);
	GetDlgItem(IDC_BUTTON_NEXT)->EnableWindow(m_index < (int)m_picture_data_list.size() - 1);
}
