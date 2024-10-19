#pragma once

#include "Plugin.h"
#include "FontSelectComboBox.h"
#include "resource.h"


// CAsciiArtDlg dialog

class CAsciiArtDlg : public CDialog
{
public:
	CAsciiArtDlg(const vector<picture_data>& picture_data_list, CWnd* pParent = NULL);   // standard constructor
	virtual ~CAsciiArtDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_ASCII_ART
	};

public:
	CRect preview_position_rect;
	CFontSelectComboBox	fontSelectComboBox;
	CWnd* pParentWnd;

protected:
	CStatic	preview_position;
	CEdit ascii_display;
	CFont ascii_display_font;
	const vector<picture_data>& picture_data_list;
	BITMAPINFOHEADER bmiHeader;
	int index;
	CSliderCtrl fontSizeSliderCtrl;

protected:
	void Update(const CString fontName);
	void UpdateDisplayFont(const CString fontName, const int fontsize);

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnPostInitDialog(WPARAM wParam, LPARAM lParam);
};
