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
	int border_percent = 12;
	int perforation_percent = 6;
	int inner_padding_percent = 4;
	int paper_style = POSTAGE_PAPER_CREAM;
	CString value_text = L"50 Cent";
	bool show_stamp = true;
	CString stamp_text = L"AIR MAIL";
	int stamp_angle = -18;
	int stamp_strength = 60;
};

void DrawPostagePreview(CDC& dc, const CRect& preview_rect, const requested_data& source, const PostageSettings& settings);
HBITMAP RenderPostageBitmap(const requested_data& source, const PostageSettings& settings, void*& dib_bits, int& output_width, int& output_height);
