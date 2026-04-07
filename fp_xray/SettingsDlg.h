#pragma once

#include "Plugin.h"
#include "resource.h"


class CSettingsDlg : public CDialog
{
public:
	CSettingsDlg(CWnd* pParent = NULL);
	virtual ~CSettingsDlg();

	enum { IDD = IDD_DIALOG_XRAY };

public:
	int analysis_scale;
	int sensitivity;
	BOOL show_grid;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
};
