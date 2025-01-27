// PluginPropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PdfPropertiesDlg.h"


// CPdfPropertiesDlg dialog

IMPLEMENT_DYNAMIC(CPdfPropertiesDlg, CDialog)
CPdfPropertiesDlg::CPdfPropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPdfPropertiesDlg::IDD, pParent),
	m_pdf_display_mode(pdf_display_mode::first_page_only)
{
}

CPdfPropertiesDlg::~CPdfPropertiesDlg()
{
}

void CPdfPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_FIRST_PAGE_ONLY, m_pdf_display_mode);
}


BEGIN_MESSAGE_MAP(CPdfPropertiesDlg, CDialog)
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

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
