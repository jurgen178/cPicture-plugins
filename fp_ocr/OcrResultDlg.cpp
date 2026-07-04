#include "stdafx.h"
#include "OcrResultDlg.h"


BEGIN_MESSAGE_MAP(COcrResultDlg, CDialog)
	ON_BN_CLICKED(IDC_OCR_COPY, OnCopy)
END_MESSAGE_MAP()


COcrResultDlg::COcrResultDlg(HWND hWndParent, const CString& text)
	: CDialog(IDD_OCR_RESULT, CWnd::FromHandle(hWndParent)),
	m_text(text)
{
}

void COcrResultDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL COcrResultDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set dialog title and button labels from string table
	CString s;
	s.LoadString(IDS_OCR_DIALOG_TITLE);
	SetWindowText(s);

	s.LoadString(IDS_OCR_COPY_BUTTON);
	GetDlgItem(IDC_OCR_COPY)->SetWindowText(s);

	s.LoadString(IDS_OCR_CLOSE_BUTTON);
	GetDlgItem(IDCANCEL)->SetWindowText(s);

	// Set OCR text in edit control
	SetDlgItemText(IDC_OCR_TEXT, m_text);
	// PostMessage defers EM_SETSEL until after Windows auto-selects on focus
	GetDlgItem(IDC_OCR_TEXT)->PostMessage(EM_SETSEL, static_cast<WPARAM>(-1), 0);

	// Move focus to Close button so the edit control doesn't auto-select all text
	GetDlgItem(IDCANCEL)->SetFocus();
	return FALSE;	// FALSE: focus was set manually
}

void COcrResultDlg::OnCopy()
{
	if (OpenClipboard())
	{
		EmptyClipboard();
		const int len = m_text.GetLength() + 1;
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(WCHAR));
		if (hMem)
		{
			WCHAR* pMem = static_cast<WCHAR*>(GlobalLock(hMem));
			if (pMem)
			{
				wmemcpy(pMem, static_cast<LPCWSTR>(m_text), len);
				GlobalUnlock(hMem);
				SetClipboardData(CF_UNICODETEXT, hMem);
			}
		}
		CloseClipboard();

		CString msg;
		msg.LoadString(IDS_OCR_COPIED);
		AfxMessageBox(msg, MB_ICONINFORMATION);
	}
}
