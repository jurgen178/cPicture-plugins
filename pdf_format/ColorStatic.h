// ColorStatic.h
#pragma once
#include "afxwin.h"

class CColorStatic : public CStatic
{
public:
    CColorStatic();
    virtual ~CColorStatic();

    void SetColor(COLORREF color);

protected:
    afx_msg void OnPaint();
    DECLARE_MESSAGE_MAP()

private:
    COLORREF m_color;
};
