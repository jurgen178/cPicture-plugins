#pragma once

#include "stdafx.h"

class CFontSelectComboBox : public CComboBox
{
public:
	CFontSelectComboBox();
	virtual ~CFontSelectComboBox();

public:
	int ctrlHeight;
	int maxFontNameWidth;
	CWnd* pParentWnd;

public:
	void Init(CWnd* pParent);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDropdown();
	afx_msg void OnCbnSelchange();
};
