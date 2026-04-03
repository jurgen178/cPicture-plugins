#pragma once

#include "FunctionPlugin.h"
#include "resource.h"
#include "CornerPickerCtrl.h"
#include <afxcmn.h>


// CQrCodeDlg dialog
// Settings dialog for the QR code plugin.
// Shown once before processing to collect: corner, text and relative size.

class CQrCodeDlg : public CDialog
{
public:
	CQrCodeDlg(const vector<picture_data>& picture_data_list, CWnd* pParent = NULL);
	virtual ~CQrCodeDlg();

	enum { IDD = IDD_DIALOG_QRCODE };

public:
	// Settings - pre-fill before DoModal(), read back after IDOK.
	int corner; // 0 = Top-Left, 1 = Top-Right, 2 = Bottom-Left, 3 = Bottom-Right
	CString text; // text content for the QR code
	int relative_size; // QR size as percentage of the shorter image side (5..50)
	int margin_percent; // gap between QR and image edge as % of shorter side (0..25)
	// Set during Create() - used by Plugin to size the data request.
	CRect preview_rect;

protected:
	const vector<picture_data>& picture_data_list;
	BITMAPINFOHEADER bmiHeader;

protected:
	CCornerPickerCtrl m_cornerPicker;
	CEdit m_editText;
	CSliderCtrl m_sliderSize;
	CSliderCtrl m_sliderMargin;
	CStatic m_staticSizeVal;
	CStatic m_staticMarginVal;
	CStatic m_preview;

protected:
	void DrawPreview(CDC& dc);

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnPaint();
	afx_msg void OnChanged();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
