#include "stdafx.h"
#include "SettingsDlg.h"


CSettingsDlg::CSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSettingsDlg::IDD, pParent),
	  output_width(1280),
	  output_height(720),
	  difference_threshold(48),
	  colorize_frames(TRUE)
{
}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_OUTPUT_WIDTH, output_width);
	DDX_Text(pDX, IDC_EDIT_OUTPUT_HEIGHT, output_height);
	DDX_Text(pDX, IDC_EDIT_THRESHOLD, difference_threshold);
	DDX_Check(pDX, IDC_CHECK_COLORIZE, colorize_frames);
	DDV_MinMaxInt(pDX, output_width, 320, 1920);
	DDV_MinMaxInt(pDX, output_height, 180, 1080);
	DDV_MinMaxInt(pDX, difference_threshold, 8, 160);
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
