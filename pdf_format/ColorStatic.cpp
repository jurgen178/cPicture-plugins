#include "stdafx.h"
#include "afxdlgs.h"
#include "ColorStatic.h"

// ColorStatic.cpp

CColorStatic::CColorStatic()
    : m_color(RGB(255, 255, 255)) // Default color is white
{
    m_hLinkCursor = ::LoadCursor(NULL, IDC_HAND); // Load the hand cursor
}

CColorStatic::~CColorStatic()
{
}

BEGIN_MESSAGE_MAP(CColorStatic, CStatic)
    ON_WM_PAINT()
    ON_WM_SETCURSOR()
    ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void CColorStatic::SetColor(COLORREF color)
{
    m_color = color;
    Invalidate(); // Force the control to repaint
}

COLORREF CColorStatic::GetColor()
{
    return m_color;
}

void CColorStatic::OnPaint()
{
    CPaintDC dc(this); // device context for painting

    // Get the client rectangle of the control
    CRect rect;
    GetClientRect(&rect);

    // Fill the rectangle with the selected color
    CBrush brush(m_color);
    dc.FillRect(&rect, &brush);
}

BOOL CColorStatic::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    ::SetCursor(m_hLinkCursor);
    return TRUE; // Indicate that we handled the cursor setting
}

void CColorStatic::OnLButtonDown(UINT nFlags, CPoint point)
{
    CColorDialog ColorDialog(m_color, CC_ANYCOLOR | CC_FULLOPEN);
    if (ColorDialog.DoModal() == IDOK)
    {
        m_color = ColorDialog.GetColor();
        Invalidate(); // Force the control to repaint
    }

    CStatic::OnLButtonDown(nFlags, point);
}

void CColorStatic::PreSubclassWindow()
{
    CStatic::PreSubclassWindow();
    ModifyStyle(0, SS_NOTIFY); // Ensure the control has SS_NOTIFY style
}
