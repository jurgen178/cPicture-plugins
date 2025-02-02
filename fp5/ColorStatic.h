// ColorStatic.h
#pragma once
#include "afxwin.h"

class CColorStatic : public CStatic
{
public:
    CColorStatic();
    virtual ~CColorStatic();

    void SetColor(COLORREF color);
    COLORREF GetColor();

protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point); 
    virtual void PreSubclassWindow(); 
    DECLARE_MESSAGE_MAP()

private:
    COLORREF m_color;
    HCURSOR m_hLinkCursor;
};
