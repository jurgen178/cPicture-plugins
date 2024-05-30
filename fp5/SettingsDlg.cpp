// SettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "afxdlgs.h"
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
	DDX_Control(pDX, IDC_EDIT_HEADLINE_TEXT, HeadlineEditCtrl);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CSettingsDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_BORDER_COLOR, &CSettingsDlg::OnBnClickedButtonBorderColor)
END_MESSAGE_MAP()


BOOL CSettingsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	HeadlineEditCtrl.SetWindowText(L"Sample5 Headline");

	return TRUE;
}


void CSettingsDlg::OnBnClickedOk()
{
	HeadlineEditCtrl.GetWindowText(headline_text);

	CDialog::OnOK();
}


void CSettingsDlg::OnBnClickedButtonBorderColor()
{
	CColorDialog ColorDialog(border_color, CC_ANYCOLOR | CC_FULLOPEN);
	if (ColorDialog.DoModal() == IDOK)
	{
		border_color = ColorDialog.GetColor();
	}
}
