// PluginPropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "afxdlgs.h"
#include "resource.h"
#include "PdfPropertiesDlg.h"


// CPdfPropertiesDlg dialog

IMPLEMENT_DYNAMIC(CPdfPropertiesDlg, CDialog)
CPdfPropertiesDlg::CPdfPropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPdfPropertiesDlg::IDD, pParent),
	pdf_display_mode(pdf_display_mode_enum::first_page_only),
	max_x(8000),
	max_y(8000),
	max_pages(0),
	border_size(25),
	border_color(RGB(255, 216, 0))
{
}

CPdfPropertiesDlg::~CPdfPropertiesDlg()
{
}

void CPdfPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_FIRST_PAGE_ONLY, pdf_display_mode);
	DDX_Control(pDX, IDC_STATIC_COLOR, m_colorStatic);
	DDX_Text(pDX, IDC_EDIT_PDF_MAX_X, max_x);
	DDX_Text(pDX, IDC_EDIT_PDF_MAX_Y, max_y);
	DDX_Text(pDX, IDC_EDIT_MAX_PAGES, max_pages);
	DDX_Text(pDX, IDC_EDIT_BORDER_SIZE, border_size);
}


BEGIN_MESSAGE_MAP(CPdfPropertiesDlg, CDialog)
	ON_BN_CLICKED(IDC_RADIO_FIRST_PAGE_ONLY, &CPdfPropertiesDlg::OnClickedRadioFirstPageOnly)
	ON_BN_CLICKED(IDC_RADIO_ALL_PAGES, &CPdfPropertiesDlg::OnClickedRadioAllPages)
	ON_BN_CLICKED(IDC_BUTTON_BORDER_COLOR, &CPdfPropertiesDlg::OnClickedButtonBorderColor)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK_PDF, &CPdfPropertiesDlg::OnClickSyslinkPdf)
END_MESSAGE_MAP()


// CPdfPropertiesDlg message handlers

void CPdfPropertiesDlg::OnOK()
{
	UpdateData(true); // read the data

	CDialog::OnOK();
}

BOOL CPdfPropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	UpdateData(false); // write the data

	update();
	m_colorStatic.SetColor(border_color);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPdfPropertiesDlg::update()
{
	UpdateData(true); // read the data

	const BOOL bAllPages = pdf_display_mode == pdf_display_mode_enum::all_pages;

	GetDlgItem(IDC_EDIT_MAX_PAGES)->EnableWindow(bAllPages);
	GetDlgItem(IDC_BUTTON_BORDER_COLOR)->EnableWindow(bAllPages);
	GetDlgItem(IDC_STATIC_MAX_PAGES_TEXT)->EnableWindow(bAllPages);
}	

void CPdfPropertiesDlg::OnClickedRadioFirstPageOnly()
{
	update();
}

void CPdfPropertiesDlg::OnClickedRadioAllPages()
{
	update();
}


void CPdfPropertiesDlg::OnClickedButtonBorderColor()
{
	CColorDialog ColorDialog(border_color, CC_ANYCOLOR | CC_FULLOPEN);
	if (ColorDialog.DoModal() == IDOK)
	{
		border_color = ColorDialog.GetColor();
		m_colorStatic.SetColor(border_color);
	}
}

void CPdfPropertiesDlg::OnClickSyslinkPdf(NMHDR* pNMHDR, LRESULT* pResult)
{
	PNMLINK pNMLink = reinterpret_cast<PNMLINK>(pNMHDR);
	LITEM   item = pNMLink->item;

	ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);

	*pResult = 0;
}
