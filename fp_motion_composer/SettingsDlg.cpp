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
	DDV_MinMaxInt(pDX, output_width, 320, 8000);
	DDX_Text(pDX, IDC_EDIT_OUTPUT_HEIGHT, output_height);
	DDV_MinMaxInt(pDX, output_height, 180, 6000);
	DDX_Text(pDX, IDC_EDIT_THRESHOLD, difference_threshold);
	DDV_MinMaxInt(pDX, difference_threshold, 8, 160);
	DDX_Check(pDX, IDC_CHECK_COLORIZE, colorize_frames);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialog)
END_MESSAGE_MAP()


BOOL CSettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	UpdateData(FALSE);
	return TRUE;
}
