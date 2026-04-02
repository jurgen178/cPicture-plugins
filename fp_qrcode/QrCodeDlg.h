#pragma once

#include "FunctionPlugin.h"
#include "resource.h"
#include <afxcmn.h>


// CQrCodeDlg dialog
// Settings dialog for the QR code plugin.
// Shown once before processing to collect: corner, text and relative size.

class CQrCodeDlg : public CDialog
{
public:
	CQrCodeDlg(CWnd* pParent = NULL);
	virtual ~CQrCodeDlg();

	enum { IDD = IDD_DIALOG_QRCODE };

public:
	// Results read by Plugin after DoModal() == IDOK.
	int    corner;         // 0 = Top-Left, 1 = Top-Right, 2 = Bottom-Left, 3 = Bottom-Right
	CString text;          // text content for the QR code (used later by real QR implementation)
	int    relative_size;  // QR size as percentage of the shorter image side (5..50)

protected:
	CComboBox m_comboCorner;
	CEdit     m_editText;
	CEdit     m_editSize;

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
};
