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
	int border_percent = 5;
	int paper_style = POSTAGE_PAPER_CREAM;
	CString value_text = L"50 Cent";
	int value_corner = 1;
	int value_margin_percent = 4;
};

void DrawPostagePreview(CDC& dc, const CRect& preview_rect, const requested_data& source, const PostageSettings& settings);
HBITMAP RenderPostageBitmap(const requested_data& source, const PostageSettings& settings, void*& dib_bits, int& output_width, int& output_height);
