#pragma once

#include "stdafx.h"
#include "resource.h"


// COcrResultDlg
// Shows the OCR-recognized text in a scrollable read-only edit control.

class COcrResultDlg : public CDialog
{
public:
	COcrResultDlg(HWND hWndParent, const CString& text);
	virtual ~COcrResultDlg() { }

	enum { IDD = IDD_OCR_RESULT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnCopy();

	DECLARE_MESSAGE_MAP()

private:
	const CString m_text;
};
