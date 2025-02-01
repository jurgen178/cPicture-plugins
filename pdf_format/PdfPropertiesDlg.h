#pragma once
#include "ColorStatic.h"
#include "afxcmn.h"


// CPdfPropertiesDlg dialog

class CPdfPropertiesDlg : public CDialog
{
	DECLARE_DYNAMIC(CPdfPropertiesDlg)

public:
	CPdfPropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPdfPropertiesDlg();

public:
	int max_picture_x;
	int max_picture_y;
	int border_size;
	int page_range_from;
	int page_range_to;
	COLORREF border_color;
	CColorStatic m_colorStatic;

protected:
	CString page_range;
	CToolTipCtrl m_ToolTip;

// Dialog Data
	enum { IDD = IDD_DIALOG_PDF_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnClickedButtonBorderColor();
	afx_msg void OnClickSyslinkPdf(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnKillfocusEditControlMaxX();
	afx_msg void OnEnKillfocusEditControlMaxY();
	afx_msg void OnEnKillfocusEditControlBorderSize();
};
