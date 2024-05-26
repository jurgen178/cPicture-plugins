#pragma once

#include "Plugin.h"
#include "resource.h"
#include "afxwin.h"


class CColorEdit : public CEdit
{
// Construction
public:
	CColorEdit();
	virtual ~CColorEdit();

// Attributes
protected:
	const CBrush brush;

public:

// Operations
public:

protected:
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	DECLARE_MESSAGE_MAP()
};


// ParameterDlg dialog

class ParameterDlg : public CDialog
{
public:
	ParameterDlg(const vector<const WCHAR*>& picture_list, CWnd* pParent = NULL);   // standard constructor
	virtual ~ParameterDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_HDR };

public:
	CString enfuse_exe_path;
	CString output_file;
	UINT jpeg_quality;

protected:
	const vector<const WCHAR*>& picture_list;
	CFont console_font;
	CColorEdit console;
	bool finished;

protected:
	void Go(LPCTSTR commandLine);
	HANDLE SpawnAndRedirect(LPCTSTR commandLine, HANDLE *hStdOutputReadPipe, LPCTSTR lpCurrentDirectory);
	void RemoveCtrl(const DWORD id);
	bool CheckFile(const WCHAR* pFile);

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedOk();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
