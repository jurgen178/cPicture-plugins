#pragma once
#include "ColorStatic.h"


enum pdf_display_mode
{
	first_page_only = 0,
	all_pages = 1
};


// CPdfPropertiesDlg dialog

class CPdfPropertiesDlg : public CDialog
{
	DECLARE_DYNAMIC(CPdfPropertiesDlg)

public:
	CPdfPropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPdfPropertiesDlg();

public:
	int m_pdf_display_mode;
	COLORREF border_color;
	CColorStatic m_colorStatic;

protected:

// Dialog Data
	enum { IDD = IDD_DIALOG_PDF_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void update();

	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnClickedRadioFirstPageOnly();
	afx_msg void OnClickedRadioAllPages();
	afx_msg void OnClickedButtonBorderColor();
};
