#pragma once

#include "stdafx.h"

class CAsciiArtDlg;

class CFontSelectComboBox : public CComboBox
{
public:
	CFontSelectComboBox();
	virtual ~CFontSelectComboBox();

public:
	int ctrlHeight;
	int maxFontNameWidth;
	CWnd* pParentWnd;

	// Function pointer to a member function of CAsciiArtDlg.
	typedef void (CAsciiArtDlg::* CallbackFunc)(const CString fontName);
	CallbackFunc callback;

	// Pointer to an instance of CAsciiArtDlg.
	CAsciiArtDlg* callbackObj;

public:
	void Init(CWnd* pParent, CallbackFunc ptr, CAsciiArtDlg* obj);
	BOOL CreateFont(CFont& font, const CString& fontName, const int fontHeight);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDropdown();
	afx_msg void OnCbnSelchange();
};
