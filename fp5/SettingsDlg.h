#pragma once

#include "Plugin.h"
#include "resource.h"
#include "ColorStatic.h"


// CSettingsDlg dialog

class CSettingsDlg : public CDialog
{
public:
	CSettingsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSettingsDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_SAMPLE5 };

protected:
	CEdit HeadlineEditCtrl;

public:
	CString headline_text;
	COLORREF border_color;
	CColorStatic m_colorStatic;

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonBorderColor();
};
