#pragma once

#include "Plugin.h"
#include "resource.h"


// CSampleDlg dialog

class CSampleDlg : public CDialog
{
public:
	CSampleDlg(const vector<picture_data>& picture_data_list, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSampleDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_SAMPLE2 };

public:
	CRect preview_position_rect;

protected:
	CStatic	preview_position;
	CStatic	info;
	CStatic	info2;
	CStatic	counter;
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
	afx_msg void OnBnClickedButtonNext();
	afx_msg void OnBnClickedButtonPrev();
};
