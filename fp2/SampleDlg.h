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
	CRect m_PreviewPositionRect;

protected:
	CStatic	m_PreviewPosition;
	CStatic	m_Info;
	CStatic	m_Info2;
	CStatic	m_Counter;
	const vector<picture_data>& picture_data_list;
	BITMAPINFOHEADER m_bmiHeader;
	int m_index;

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
