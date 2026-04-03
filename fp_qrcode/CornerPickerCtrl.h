#pragma once

#include <afxwin.h>

// CCornerPickerCtrl
//
// Custom CStatic subclass: draws a 2x2 grid of QR-code-style cells.
// Clicking a cell selects that corner (0=TL, 1=TR, 2=BL, 3=BR).
// Arrow keys change selection. Sends BN_CLICKED via WM_COMMAND to parent on change.

class CCornerPickerCtrl : public CStatic
{
public:
	CCornerPickerCtrl();

	int  GetCorner() const { return m_corner; }
	void SetCorner(int c);

protected:
	int m_corner; // 0=TL, 1=TR, 2=BL, 3=BR

	CRect GetCellRect(int index) const;
	void  DrawQrCell(CDC& dc, int index);

	virtual void PreSubclassWindow() override;

	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);

	DECLARE_MESSAGE_MAP()
};
