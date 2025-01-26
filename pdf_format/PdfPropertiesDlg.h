#pragma once


// CPdfPropertiesDlg dialog

class CPdfPropertiesDlg : public CDialog
{
	DECLARE_DYNAMIC(CPdfPropertiesDlg)

public:
	CPdfPropertiesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPdfPropertiesDlg();

public:
	int m_compression_type;

protected:

// Dialog Data
	enum { IDD = IDD_DIALOG_PDF_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
	virtual BOOL OnInitDialog();
};
