// QrCodeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "QrCodeDlg.h"


// CQrCodeDlg dialog

CQrCodeDlg::CQrCodeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CQrCodeDlg::IDD, pParent),
	  corner(3),          // default: Bottom-Right
	  text(L""),
	  relative_size(20)   // default: 20%
{
}

CQrCodeDlg::~CQrCodeDlg()
{
}

void CQrCodeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_CORNER, m_comboCorner);
	DDX_Control(pDX, IDC_EDIT_TEXT,    m_editText);
	DDX_Control(pDX, IDC_EDIT_SIZE,    m_editSize);
}


BEGIN_MESSAGE_MAP(CQrCodeDlg, CDialog)
END_MESSAGE_MAP()


BOOL CQrCodeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Populate corner combo box.
	CString str;
	str.LoadString(IDS_CORNER_TOP_LEFT);     m_comboCorner.AddString(str);
	str.LoadString(IDS_CORNER_TOP_RIGHT);    m_comboCorner.AddString(str);
	str.LoadString(IDS_CORNER_BOTTOM_LEFT);  m_comboCorner.AddString(str);
	str.LoadString(IDS_CORNER_BOTTOM_RIGHT); m_comboCorner.AddString(str);
	m_comboCorner.SetCurSel(corner);

	// Set text edit.
	m_editText.SetWindowText(text);

	// Set size edit (show default percentage).
	CString sizeStr;
	sizeStr.Format(L"%d", relative_size);
	m_editSize.SetWindowText(sizeStr);

	return TRUE;
}


void CQrCodeDlg::OnOK()
{
	// Read corner selection.
	corner = m_comboCorner.GetCurSel();
	if (corner < 0)
		corner = 3; // fallback: Bottom-Right

	// Read text.
	m_editText.GetWindowText(text);

	// Read and validate relative size.
	CString sizeStr;
	m_editSize.GetWindowText(sizeStr);
	relative_size = _wtoi(sizeStr);
	if (relative_size < 5)
		relative_size = 5;
	if (relative_size > 50)
		relative_size = 50;

	CDialog::OnOK();
}
