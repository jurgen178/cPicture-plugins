#pragma once
#include "stdafx.h"
#include "resource.h"
#include "Plugin.h"


class CColorPreviewCtrl : public CStatic
{
public:
	void SetPreviewColor(const COLORREF color)
	{
		m_previewColor = color;
		if (GetSafeHwnd() != NULL)
			RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
	}

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()

private:
	COLORREF m_previewColor = RGB(0, 0, 0);
};


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
	afx_msg void OnChooseBackgroundColor();

	DECLARE_MESSAGE_MAP()

private:
	AnimSettings m_settings;
	CColorPreviewCtrl m_backgroundColorPreview;

	void UpdateQualityState();
	void UpdateBackgroundColorPreview();
};
