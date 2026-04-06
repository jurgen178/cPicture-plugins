#include "stdafx.h"
#include "SettingsDlg.h"


CSettingsDlg::CSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSettingsDlg::IDD, pParent),
	  analysis_scale(60),
	  sensitivity(55),
	  show_grid(TRUE)
{
}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SCALE, analysis_scale);
	DDX_Text(pDX, IDC_EDIT_SENSITIVITY, sensitivity);
	DDX_Check(pDX, IDC_CHECK_GRID, show_grid);
	DDV_MinMaxInt(pDX, analysis_scale, 20, 100);
	DDV_MinMaxInt(pDX, sensitivity, 10, 100);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialog)
END_MESSAGE_MAP()


BOOL CSettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	UpdateData(FALSE);
	return TRUE;
}


void CSettingsDlg::OnOK()
{
	UpdateData(TRUE);
	CDialog::OnOK();
}
