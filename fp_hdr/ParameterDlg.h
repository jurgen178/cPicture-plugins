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
	const CBrush m_Brush;

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
	ParameterDlg(const vector<const WCHAR*>& _picture_list, CWnd* pParent = NULL);   // standard constructor
	virtual ~ParameterDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_HDR };

public:
	CString m_Enfuse;
	CString m_OutputFile;
	UINT m_JpegQuality;

protected:
	const vector<const WCHAR*>& m_picture_list;
	CFont m_ConsoleFont;
	CColorEdit m_Console;
	bool m_bFinished;

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
	afx_msg void OnBnClickedCancel();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};
