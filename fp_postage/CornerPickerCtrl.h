#pragma once

#include <afxwin.h>

class CCornerPickerCtrl : public CStatic
{
public:
	CCornerPickerCtrl();

	int GetCorner() const { return m_corner; }
	void SetCorner(int corner);

protected:
	int m_corner;

	CRect GetCellRect(int index) const;
	void DrawCell(CDC& dc, int index);

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