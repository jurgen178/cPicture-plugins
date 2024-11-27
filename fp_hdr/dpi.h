#pragma once

#include "stdafx.h"
#include "shellscalingapi.h"


class CDPI
{
public:
	CDPI(const CWnd* pWnd)
		: m_dpiX(96),
		m_dpiY(96)
	{
		if (pWnd)
		{
			UINT dpiX = m_dpiX;
			UINT dpiY = m_dpiY;
			HMONITOR hMonitor = MonitorFromWindow(pWnd->m_hWnd, MONITOR_DEFAULTTONEAREST);
			HRESULT hr = ::GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
			if (hr == S_OK)
			{
				m_dpiX = dpiX;
				m_dpiY = dpiY;
			}
		}
	}

	int Scale(const int x) const
	{
		// Per-Monitor DPI Aware:  Return the input value scaled by the factor for the display which contains most of the window.
		// These apps render themselves for any DPI, and re-render when the DPI changes (as indicated by the WM_DPICHANGED window message).
		return MulDiv(x, m_dpiX, 96);
	}

private:
	UINT m_dpiX;
	UINT m_dpiY;
};