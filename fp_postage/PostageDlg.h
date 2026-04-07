#pragma once

#include "FunctionPlugin.h"
#include "PostageRender.h"
#include "resource.h"
#include <afxcmn.h>

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
	CSliderCtrl m_sliderPadding;
	CSliderCtrl m_sliderStampAngle;
	CSliderCtrl m_sliderStampStrength;
	CStatic m_staticBorderVal;
	CStatic m_staticPerforationVal;
	CStatic m_staticPaddingVal;
	CStatic m_staticStampAngleVal;
	CStatic m_staticStampStrengthVal;
	CComboBox m_comboPaper;
	CEdit m_editValue;
	CButton m_checkStamp;
	CEdit m_editStampText;
	CStatic m_preview;

	void UpdateLabels();
	void UpdateStampControls();
	void DrawPreview(CDC& dc);

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnChanged();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
