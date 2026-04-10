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

CCornerPickerCtrl::CCornerPickerCtrl()
	: m_corner(1)
{
}

void CCornerPickerCtrl::PreSubclassWindow()
{
	CStatic::PreSubclassWindow();
	ModifyStyle(0, SS_NOTIFY);
}

void CCornerPickerCtrl::SetCorner(int corner)
{
	m_corner = max(0, min(corner, 3));
	if (GetSafeHwnd())
		Invalidate(FALSE);
}

CRect CCornerPickerCtrl::GetCellRect(int index) const
{
	CRect client;
	GetClientRect(client);
	const int col = index % 2;
	const int row = index / 2;
	return CRect(
		client.left + col * client.Width() / 2,
		client.top + row * client.Height() / 2,
		client.left + (col + 1) * client.Width() / 2,
		client.top + (row + 1) * client.Height() / 2);
}

void CCornerPickerCtrl::DrawCell(CDC& dc, int index)
{
	const CRect cell = GetCellRect(index);
	const bool selected = index == m_corner;
	const bool has_focus = GetFocus() == this;

	const COLORREF border = selected
		? (has_focus ? RGB(0, 90, 190) : RGB(70, 140, 220))
		: RGB(160, 160, 160);
	dc.FillSolidRect(cell, border);

	CRect inner(cell);
	inner.DeflateRect(selected ? 2 : 1, selected ? 2 : 1);
	dc.FillSolidRect(inner, RGB(255, 255, 255));

	const int pad = max(2, min(inner.Width(), inner.Height()) / 6);
	CRect marker(inner);
	marker.DeflateRect(pad, pad);
	const COLORREF fill = selected ? RGB(35, 35, 35) : RGB(150, 150, 150);

	switch (index)
	{
	case 0:
		dc.FillSolidRect(marker.left, marker.top, marker.Width() / 2, marker.Height() / 2, fill);
		break;
	case 1:
		dc.FillSolidRect(marker.CenterPoint().x, marker.top, marker.Width() / 2, marker.Height() / 2, fill);
		break;
	case 2:
		dc.FillSolidRect(marker.left, marker.CenterPoint().y, marker.Width() / 2, marker.Height() / 2, fill);
		break;
	case 3:
		dc.FillSolidRect(marker.CenterPoint().x, marker.CenterPoint().y, marker.Width() / 2, marker.Height() / 2, fill);
		break;
	}
}

void CCornerPickerCtrl::OnPaint()
{
	CPaintDC dc(this);
	for (int index = 0; index < 4; ++index)
		DrawCell(dc, index);
}

BOOL CCornerPickerCtrl::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CCornerPickerCtrl::OnLButtonDown(UINT /*nFlags*/, CPoint point)
{
	SetFocus();
	for (int index = 0; index < 4; ++index)
	{
		if (!GetCellRect(index).PtInRect(point))
			continue;
		if (m_corner != index)
		{
			m_corner = index;
			Invalidate(FALSE);
			GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), reinterpret_cast<LPARAM>(m_hWnd));
		}
		break;
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
	case VK_LEFT:
		if (m_corner % 2 == 1)
			next = m_corner - 1;
		break;
	case VK_RIGHT:
		if (m_corner % 2 == 0)
			next = m_corner + 1;
		break;
	case VK_UP:
		if (m_corner >= 2)
			next = m_corner - 2;
		break;
	case VK_DOWN:
		if (m_corner < 2)
			next = m_corner + 2;
		break;
	}

	if (next != m_corner)
	{
		m_corner = next;
		Invalidate(FALSE);
		GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), reinterpret_cast<LPARAM>(m_hWnd));
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