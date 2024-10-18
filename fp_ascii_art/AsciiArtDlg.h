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
	const vector<picture_data>& picture_data_list;
	BITMAPINFOHEADER bmiHeader;
	int index;

protected:
	void update_button_state();

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnBnClickedButtonFont();
};
