#include "stdafx.h"
#include "SettingsDlg.h"


CSettingsDlg::CSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSettingsDlg::IDD, pParent),
	  sort_mode(0),
	  show_metadata(TRUE),
	  thumbnail_width(240),
	  thumbnail_height(150),
	  location_cluster_radius_km(2.0)
{
}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_TITLE, title_text);
	DDX_CBIndex(pDX, IDC_COMBO_SORT_MODE, sort_mode);
	DDX_Check(pDX, IDC_CHECK_METADATA, show_metadata);
	DDX_Text(pDX, IDC_EDIT_THUMB_WIDTH, thumbnail_width);
	DDV_MinMaxInt(pDX, thumbnail_width, 120, 480);
	DDX_Text(pDX, IDC_EDIT_THUMB_HEIGHT, thumbnail_height);
	DDV_MinMaxInt(pDX, thumbnail_height, 90, 320);
	DDX_Text(pDX, IDC_EDIT_LOCATION_RADIUS, location_cluster_radius_km);
	DDV_MinMaxDouble(pDX, location_cluster_radius_km, 0.1, 250.0);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialog)
END_MESSAGE_MAP()


BOOL CSettingsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CComboBox* sort_combo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SORT_MODE));
	if (sort_combo != NULL)
	{
		CString text;
		text.LoadString(IDS_SORT_MODE_EXIF);
		sort_combo->AddString(text);
		text.LoadString(IDS_SORT_MODE_SELECTION);
		sort_combo->AddString(text);
		text.LoadString(IDS_SORT_MODE_WEST_EAST);
		sort_combo->AddString(text);
		text.LoadString(IDS_SORT_MODE_NORTH_SOUTH);
		sort_combo->AddString(text);
		text.LoadString(IDS_SORT_MODE_EAST_WEST);
		sort_combo->AddString(text);
		text.LoadString(IDS_SORT_MODE_SOUTH_NORTH);
		sort_combo->AddString(text);
		sort_combo->SetCurSel(max(0, min(5, sort_mode)));
	}

	if (title_text.IsEmpty())
		title_text.LoadString(IDS_DEFAULT_TITLE);

	UpdateData(FALSE);

	return TRUE;
}


void CSettingsDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return;
	title_text.Trim();

	if (title_text.IsEmpty())
		title_text.LoadString(IDS_DEFAULT_TITLE);

	EndDialog(IDOK);
}
