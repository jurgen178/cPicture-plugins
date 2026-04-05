#pragma once

#include "Plugin.h"
#include "resource.h"


class CSettingsDlg : public CDialog
{
public:
	CSettingsDlg(CWnd* pParent = NULL);
	virtual ~CSettingsDlg();

	enum { IDD = IDD_DIALOG_TIMECAPSULE };

public:
	CString title_text;
	int sort_mode;
	BOOL show_metadata;
	int thumbnail_width;
	int thumbnail_height;
	double location_cluster_radius_km;

protected:
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
};
