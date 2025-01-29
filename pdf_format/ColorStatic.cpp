#include "stdafx.h"
#include "ColorStatic.h"

// ColorStatic.cpp

CColorStatic::CColorStatic()
    : m_color(RGB(255, 255, 255)) // Default color is white
{
}

CColorStatic::~CColorStatic()
{
}

BEGIN_MESSAGE_MAP(CColorStatic, CStatic)
    ON_WM_PAINT()
END_MESSAGE_MAP()

void CColorStatic::SetColor(COLORREF color)
{
    m_color = color;
    Invalidate(); // Force the control to repaint
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
