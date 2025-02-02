// SettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SettingsDlg.h"


// CSettingsDlg dialog

CSettingsDlg::CSettingsDlg(CWnd* pParent /*=NULL*/)
  : CDialog(CSettingsDlg::IDD, pParent),
	border_color(RGB(255, 255, 0))
{
}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_HEADLINE_TEXT, headline_text);
	DDX_Control(pDX, IDC_STATIC_COLOR, m_colorStatic);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialog)
END_MESSAGE_MAP()


BOOL CSettingsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	headline_text.LoadString(IDS_HEADLINE_TEXT);
	m_colorStatic.SetColor(border_color);

	UpdateData(false); // write the data

	return TRUE;
}


void CSettingsDlg::OnOK()
{
	UpdateData(true); // read the data

	border_color = m_colorStatic.GetColor();

	CDialog::OnOK();
}
