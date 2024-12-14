#pragma once

#include "stdafx.h"


class CDPI
{
public:
	CDPI(const CWnd* pWnd)
		: m_dpi(96)
	{
		if (pWnd)
		{
			m_dpi = GetDpiForWindow(pWnd->m_hWnd);
		}
	}

	int Scale(const int x) const
	{
		// Per-Monitor DPI Aware:  Return the input value scaled by the factor for the display which contains most of the window.
		// These apps render themselves for any DPI, and re-render when the DPI changes (as indicated by the WM_DPICHANGED window message).
		return MulDiv(x, m_dpi, 96);
	}

private:
	UINT m_dpi;
};