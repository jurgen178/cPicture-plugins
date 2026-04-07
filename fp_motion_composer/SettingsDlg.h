#pragma once

#include "Plugin.h"
#include "resource.h"


class CSettingsDlg : public CDialog
{
public:
	CSettingsDlg(CWnd* pParent = NULL);
	virtual ~CSettingsDlg();

	enum { IDD = IDD_DIALOG_MOTION_COMPOSER };

public:
	int output_width;
	int output_height;
	int difference_threshold;
	BOOL colorize_frames;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
};
