// CornerPickerCtrl.cpp : implementation

#include "stdafx.h"
#include "CornerPickerCtrl.h"

BEGIN_MESSAGE_MAP(CCornerPickerCtrl, CStatic)
ON_WM_PAINT()
ON_WM_ERASEBKGND()
ON_WM_LBUTTONDOWN()
ON_WM_GETDLGCODE()
ON_WM_KEYDOWN()
ON_WM_SETFOCUS()
ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

CCornerPickerCtrl::CCornerPickerCtrl() : m_corner(3) {}

void CCornerPickerCtrl::PreSubclassWindow()
{
CStatic::PreSubclassWindow();
// SS_NOTIFY is required for a Static-class window to receive mouse messages.
ModifyStyle(0, SS_NOTIFY);
}

void CCornerPickerCtrl::SetCorner(int c)
{
m_corner = c;
if (GetSafeHwnd())
Invalidate(FALSE);
}

// Returns the pixel rect for cell 'index' in client coordinates.
// Layout: 0=TL, 1=TR, 2=BL, 3=BR
CRect CCornerPickerCtrl::GetCellRect(int index) const
{
CRect client;
GetClientRect(client);
const int w = client.Width();
const int h = client.Height();
const int col = index % 2;
const int row = index / 2;
return CRect(col * w / 2, row * h / 2, (col + 1) * w / 2, (row + 1) * h / 2);
}

// Draws cell 'index' as a standalone QR-code-style square.
// Selected cell gets a blue border; others get a thin gray border.
void CCornerPickerCtrl::DrawQrCell(CDC& dc, int index)
{
CRect cell = GetCellRect(index);
const bool selected = (index == m_corner);
const bool hasFocus  = (GetFocus() == this);

// ---- outer selection border ----
const int bw = selected ? 2 : 1;
const COLORREF bc = selected
? (hasFocus ? RGB(0, 90, 190) : RGB(70, 140, 220))
: RGB(160, 160, 160);
dc.FillSolidRect(cell, bc);

// ---- inner area ----
CRect inner(cell.left + bw, cell.top + bw, cell.right - bw, cell.bottom - bw);
if (inner.Width() < 8 || inner.Height() < 8)
return;

// Square QR area centred inside inner (small padding)
const int pad = 2;
const int sz  = min(inner.Width(), inner.Height()) - 2 * pad;
if (sz < 8)
return;

const int ox = inner.left + (inner.Width()  - sz) / 2;
const int oy = inner.top  + (inner.Height() - sz) / 2;

// QR background: white if active, light gray if inactive
const COLORREF clrBg  = selected ? RGB(255, 255, 255) : RGB(220, 220, 220);
const COLORREF clrFg  = selected ? RGB(  0,   0,   0) : RGB(140, 140, 140);
dc.FillSolidRect(inner, clrBg);

// ---- Finder patterns (~1/3 of QR size each) ----
const int fp = max(5, sz / 3);

auto drawFinder = [&](int fx, int fy)
{
// outer square
dc.FillSolidRect(fx, fy, fp, fp, clrFg);
// inner ring (background color)
if (fp > 2)
dc.FillSolidRect(fx + 1, fy + 1, fp - 2, fp - 2, clrBg);
// centre square
if (fp > 4)
dc.FillSolidRect(fx + 2, fy + 2, fp - 4, fp - 4, clrFg);
};

drawFinder(ox,           oy);           // TL
drawFinder(ox + sz - fp, oy);           // TR
drawFinder(ox,           oy + sz - fp); // BL

// ---- Data modules (scattered dots to look QR-like) ----
const int dot  = max(1, sz / 14);
const int da = fp + dot + 1;   // data area offset from ox/oy
const int de = sz - 2;         // data area right/bottom limit
const int step = max(dot + 1, (de - da) / 5);

// Fixed 6x5 sparse dot pattern
static const signed char kDots[14][2] = {
{0,0},{2,0},{4,0},{5,0},
{1,1},{3,1},{5,1},
{0,2},{2,2},{4,2},
{1,3},{3,3},{5,3},
{0,4}
};
for (const auto& d : kDots)
{
const int dx = ox + da + d[0] * step;
const int dy = oy + da + d[1] * step;
if (dx + dot <= ox + de && dy + dot <= oy + de)
dc.FillSolidRect(dx, dy, dot, dot, clrFg);
}
}

void CCornerPickerCtrl::OnPaint()
{
CPaintDC dc(this);
for (int i = 0; i < 4; ++i)
DrawQrCell(dc, i);
}

BOOL CCornerPickerCtrl::OnEraseBkgnd(CDC* /*pDC*/)
{
return TRUE;
}

void CCornerPickerCtrl::OnLButtonDown(UINT /*nFlags*/, CPoint point)
{
SetFocus();
for (int i = 0; i < 4; ++i)
{
if (GetCellRect(i).PtInRect(point))
{
if (m_corner != i)
{
m_corner = i;
Invalidate(FALSE);
GetParent()->SendMessage(WM_COMMAND,
MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED),
reinterpret_cast<LPARAM>(m_hWnd));
}
break;
}
}
}

UINT CCornerPickerCtrl::OnGetDlgCode()
{
return DLGC_WANTARROWS;
}

void CCornerPickerCtrl::OnKeyDown(UINT nChar, UINT /*nRepCnt*/, UINT /*nFlags*/)
{
int next = m_corner;
switch (nChar)
{
case VK_LEFT:  if (m_corner % 2 == 1) next = m_corner - 1; break;
case VK_RIGHT: if (m_corner % 2 == 0) next = m_corner + 1; break;
case VK_UP:    if (m_corner >= 2)     next = m_corner - 2; break;
case VK_DOWN:  if (m_corner < 2)      next = m_corner + 2; break;
}
if (next != m_corner)
{
m_corner = next;
Invalidate(FALSE);
GetParent()->SendMessage(WM_COMMAND,
MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED),
reinterpret_cast<LPARAM>(m_hWnd));
}
}

void CCornerPickerCtrl::OnSetFocus(CWnd* pOldWnd)
{
CStatic::OnSetFocus(pOldWnd);
Invalidate(FALSE);
}

void CCornerPickerCtrl::OnKillFocus(CWnd* pNewWnd)
{
CStatic::OnKillFocus(pNewWnd);
Invalidate(FALSE);
}
