#pragma once
#include "stdafx.h"
#include "resource.h"
#include "Plugin.h"


// CSettingsDlg — frame delay, loop count, quality / lossless

class CSettingsDlg : public CDialog
{
public:
	explicit CSettingsDlg(const AnimSettings& defaults, CWnd* pParent = nullptr);

	enum { IDD = IDD_SETTINGS };

	AnimSettings GetSettings() const { return m_settings; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnLosslessChanged();

	DECLARE_MESSAGE_MAP()

private:
	AnimSettings m_settings;

	void UpdateQualityState();
};
