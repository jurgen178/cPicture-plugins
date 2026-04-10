#pragma once

#include "FunctionPlugin.h"
#include "PostageRender.h"
#include "CornerPickerCtrl.h"
#include "resource.h"
#include <afxcmn.h>

class CTextColorPreviewCtrl : public CStatic
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

class CPostageDlg : public CDialog
{
public:
	CPostageDlg(const vector<picture_data>& picture_data_list, CWnd* pParent = NULL);
	virtual ~CPostageDlg();

	enum { IDD = IDD_DIALOG_POSTAGE };

public:
	PostageSettings settings;
	CRect preview_rect;

protected:
	const vector<picture_data>& picture_data_list;

	CSliderCtrl m_sliderBorder;
	CSliderCtrl m_sliderPerforation;
	CSliderCtrl m_sliderValueMargin;
	CStatic m_staticBorderVal;
	CStatic m_staticPerforationVal;
	CStatic m_staticValueMarginVal;
	CComboBox m_comboPaper;
	CButton m_buttonFont;
	CButton m_buttonTextColor;
	CTextColorPreviewCtrl m_staticTextColor;
	CCornerPickerCtrl m_valueCorner;
	CEdit m_editValue;
	CStatic m_preview;

	int GetEffectiveBorderPercent() const;
	void UpdateLabels();
	void UpdateFontButtonLabel();
	void UpdateTextColorBrush();
	void DrawPreview(CDC& dc);

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnChanged();
	afx_msg void OnChooseFont();
	afx_msg void OnChooseTextColor();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
