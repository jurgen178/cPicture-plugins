#pragma once

#include "stdafx.h"

class CFontSelectComboBox : public CComboBox
{
public:
	CFontSelectComboBox();
	virtual ~CFontSelectComboBox();

public:
	int m_iFontHeight;
	int m_iMaxNameWidth;

public:
	void Init();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDropdown();
};
