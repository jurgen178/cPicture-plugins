#pragma once

#include "FunctionPlugin.h"
#include <Vfw.h>

enum POSTAGE_PAPER_STYLE
{
	POSTAGE_PAPER_WHITE = 0,
	POSTAGE_PAPER_CREAM,
	POSTAGE_PAPER_VINTAGE,
};

struct PostageSettings
{
	PostageSettings()
		: border_percent(5),
		  paper_style(POSTAGE_PAPER_CREAM),
		  value_text(L"50 Cent"),
		  value_corner(1),
		  value_margin_percent(4),
		  value_color(RGB(92, 71, 39))
	{
		ZeroMemory(&value_font, sizeof(value_font));
		value_font.lfHeight = -16;
		value_font.lfWeight = FW_BOLD;
		value_font.lfCharSet = DEFAULT_CHARSET;
		value_font.lfQuality = PROOF_QUALITY;
		value_font.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
		lstrcpynW(value_font.lfFaceName, L"Segoe UI", LF_FACESIZE);
	}

	int border_percent;
	int paper_style;
	CString value_text;
	int value_corner;
	int value_margin_percent;
	LOGFONT value_font;
	COLORREF value_color;
};

void DrawPostagePreview(CDC& dc, const CRect& preview_rect, const requested_data& source, const PostageSettings& settings);
HBITMAP RenderPostageBitmap(const requested_data& source, const PostageSettings& settings, void*& dib_bits, int& output_width, int& output_height);
