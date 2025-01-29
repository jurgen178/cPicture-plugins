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
	m_pdf_display_mode(pdf_display_mode::first_page_only),
	border_color(RGB(255, 216, 0))
{
}

CPdfPropertiesDlg::~CPdfPropertiesDlg()
{
}

void CPdfPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_FIRST_PAGE_ONLY, m_pdf_display_mode);
	DDX_Control(pDX, IDC_STATIC_COLOR, m_colorStatic);
}


BEGIN_MESSAGE_MAP(CPdfPropertiesDlg, CDialog)
	ON_BN_CLICKED(IDC_RADIO_FIRST_PAGE_ONLY, &CPdfPropertiesDlg::OnClickedRadioFirstPageOnly)
	ON_BN_CLICKED(IDC_RADIO_ALL_PAGES, &CPdfPropertiesDlg::OnClickedRadioAllPages)
	ON_BN_CLICKED(IDC_BUTTON_BORDER_COLOR, &CPdfPropertiesDlg::OnClickedButtonBorderColor)
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

	const BOOL bAllPages = m_pdf_display_mode == pdf_display_mode::all_pages;

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
