#include "stdafx.h"
#include "PostageRender.h"

#include <cmath>

namespace
{
	struct PostageMetrics
	{
		int border_px = 18;
		int padding_px = 6;
		int hole_radius = 4;
		int hole_step = 8;
	};

	COLORREF GetPaperColor(const int paper_style)
	{
		switch (paper_style)
		{
		case POSTAGE_PAPER_WHITE:
			return RGB(250, 250, 246);
		case POSTAGE_PAPER_VINTAGE:
			return RGB(236, 223, 191);
		case POSTAGE_PAPER_CREAM:
		default:
			return RGB(246, 238, 216);
		}
	}

	COLORREF MixColor(const COLORREF base, const COLORREF overlay, const int amount)
	{
		const int clamped = max(0, min(amount, 100));
		const int inv = 100 - clamped;
		return RGB(
			(GetRValue(base) * inv + GetRValue(overlay) * clamped) / 100,
			(GetGValue(base) * inv + GetGValue(overlay) * clamped) / 100,
			(GetBValue(base) * inv + GetBValue(overlay) * clamped) / 100);
	}

	PostageMetrics GetMetrics(const requested_data& source, const PostageSettings& settings)
	{
		PostageMetrics metrics;
		const int shorter_side = max(1, min(source.picture_width, source.picture_height));
		metrics.border_px = max(18, shorter_side * settings.border_percent / 100);
		metrics.padding_px = max(6, shorter_side * settings.inner_padding_percent / 100);
		metrics.hole_radius = max(4, metrics.border_px * settings.perforation_percent / 24);
		metrics.hole_radius = min(metrics.hole_radius, max(4, metrics.border_px - 3));
		metrics.hole_step = max(metrics.hole_radius * 2, metrics.hole_radius + 6);
		return metrics;
	}

	CRect BuildCanvasRect(const requested_data& source, const PostageMetrics& metrics)
	{
		const int margin = metrics.border_px + metrics.padding_px;
		return CRect(0, 0, source.picture_width + margin * 2, source.picture_height + margin * 2);
	}

	CRect BuildImageRect(const CRect& canvas_rect, const requested_data& source, const PostageMetrics& metrics)
	{
		const int margin = metrics.border_px + metrics.padding_px;
		return CRect(
			canvas_rect.left + margin,
			canvas_rect.top + margin,
			canvas_rect.left + margin + source.picture_width,
			canvas_rect.top + margin + source.picture_height);
	}

	CRect BuildPaperRect(const CRect& canvas_rect)
	{
		CRect paper_rect(canvas_rect);
		paper_rect.DeflateRect(1, 1);
		return paper_rect;
	}

	void DrawHalfCircleCuts(CDC& dc, const CRect& paper_rect, const PostageMetrics& metrics, const COLORREF background, const COLORREF outline)
	{
		CPen pen(PS_SOLID, 1, outline);
		CBrush brush(background);
		CPen* old_pen = dc.SelectObject(&pen);
		CBrush* old_brush = dc.SelectObject(&brush);

		const int left = paper_rect.left;
		const int top = paper_rect.top;
		const int right = paper_rect.right - 1;
		const int bottom = paper_rect.bottom - 1;
		const int radius = metrics.hole_radius;
		const int step = metrics.hole_step;

		for (int x = left + radius; x <= right - radius; x += step)
		{
			dc.Ellipse(x - radius, top - radius, x + radius, top + radius);
			dc.Ellipse(x - radius, bottom - radius, x + radius, bottom + radius);
		}

		for (int y = top + radius; y <= bottom - radius; y += step)
		{
			dc.Ellipse(left - radius, y - radius, left + radius, y + radius);
			dc.Ellipse(right - radius, y - radius, right + radius, y + radius);
		}

		dc.Ellipse(left - radius, top - radius, left + radius, top + radius);
		dc.Ellipse(right - radius, top - radius, right + radius, top + radius);
		dc.Ellipse(left - radius, bottom - radius, left + radius, bottom + radius);
		dc.Ellipse(right - radius, bottom - radius, right + radius, bottom + radius);

		dc.SelectObject(old_brush);
		dc.SelectObject(old_pen);
	}

	void DrawPaperShape(CDC& dc, const CRect& paper_rect, const PostageMetrics& metrics, const PostageSettings& settings, const COLORREF background)
	{
		const COLORREF paper = GetPaperColor(settings.paper_style);
		const COLORREF outline = MixColor(paper, RGB(120, 94, 66), 45);
		const COLORREF highlight = MixColor(paper, RGB(255, 255, 255), 25);

		CPen outline_pen(PS_SOLID, 1, outline);
		CBrush paper_brush(paper);
		CPen* old_pen = dc.SelectObject(&outline_pen);
		CBrush* old_brush = dc.SelectObject(&paper_brush);
		dc.Rectangle(paper_rect);

		DrawHalfCircleCuts(dc, paper_rect, metrics, background, outline);

		CPen highlight_pen(PS_SOLID, 1, highlight);
		dc.SelectObject(&highlight_pen);
		dc.MoveTo(paper_rect.left + metrics.hole_radius, paper_rect.top + 1);
		dc.LineTo(paper_rect.right - metrics.hole_radius, paper_rect.top + 1);
		dc.MoveTo(paper_rect.left + 1, paper_rect.top + metrics.hole_radius);
		dc.LineTo(paper_rect.left + 1, paper_rect.bottom - metrics.hole_radius);

		dc.SelectObject(old_brush);
		dc.SelectObject(old_pen);
	}

	void DrawSourcePicture(CDC& dc, const requested_data& source, const CRect& image_rect)
	{
		HDRAWDIB draw_dib = DrawDibOpen();
		if (draw_dib == NULL)
			return;

		BITMAPINFOHEADER bmi_header = { 0 };
		bmi_header.biSize = sizeof(BITMAPINFOHEADER);
		bmi_header.biPlanes = 1;
		bmi_header.biBitCount = 24;
		bmi_header.biCompression = BI_RGB;
		bmi_header.biWidth = source.picture_width;
		bmi_header.biHeight = source.picture_height;

		DrawDibDraw(
			draw_dib,
			dc.m_hDC,
			image_rect.left,
			image_rect.top,
			image_rect.Width(),
			image_rect.Height(),
			&bmi_header,
			source.data,
			0,
			0,
			source.picture_width,
			source.picture_height,
			0);

		DrawDibClose(draw_dib);
	}

	void DrawInnerFrame(CDC& dc, const CRect& image_rect, const PostageSettings& settings)
	{
		const int shorter_side = min(image_rect.Width(), image_rect.Height());
		const int padding_px = max(6, shorter_side * settings.inner_padding_percent / 100);
		CRect inner_rect(image_rect);
		inner_rect.InflateRect(padding_px, padding_px);

		CPen pen(PS_SOLID, 2, RGB(171, 148, 108));
		CPen* old_pen = dc.SelectObject(&pen);
		CBrush* old_brush = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH));
		dc.Rectangle(inner_rect);
		dc.SelectObject(old_brush);
		dc.SelectObject(old_pen);
	}

	void DrawValueText(CDC& dc, const CRect& canvas_rect, const PostageSettings& settings)
	{
		CString text(settings.value_text);
		text.Trim();
		if (text.IsEmpty())
			return;

		const int font_height = max(18, min(canvas_rect.Width(), canvas_rect.Height()) / 10);
		CFont font;
		font.CreateFont(font_height, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
			OUT_DEVICE_PRECIS, CLIP_CHARACTER_PRECIS, PROOF_QUALITY,
			VARIABLE_PITCH | FF_SWISS, L"Georgia");
		CFont* old_font = dc.SelectObject(&font);
		const int old_mode = dc.SetBkMode(TRANSPARENT);
		const COLORREF old_color = dc.SetTextColor(RGB(92, 71, 39));

		CRect text_rect(canvas_rect);
		text_rect.DeflateRect(16, 10);
		dc.DrawText(text, text_rect, DT_RIGHT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);

		dc.SetTextColor(old_color);
		dc.SetBkMode(old_mode);
		dc.SelectObject(old_font);
	}

	void DrawStamp(CDC& dc, const CRect& canvas_rect, const PostageSettings& settings)
	{
		if (!settings.show_stamp)
			return;

		CString text(settings.stamp_text);
		text.Trim();
		if (text.IsEmpty())
			return;

		const COLORREF paper = GetPaperColor(settings.paper_style);
		const COLORREF stamp_color = MixColor(paper, RGB(174, 43, 43), settings.stamp_strength);
		const int font_height = max(20, min(canvas_rect.Width(), canvas_rect.Height()) / 9);
		CFont font;
		font.CreateFont(font_height, 0, settings.stamp_angle * 10, settings.stamp_angle * 10, FW_BOLD, 0, 0, 0,
			DEFAULT_CHARSET, OUT_DEVICE_PRECIS, CLIP_CHARACTER_PRECIS, PROOF_QUALITY,
			VARIABLE_PITCH | FF_SWISS, L"Arial");
		CFont* old_font = dc.SelectObject(&font);
		const int old_mode = dc.SetBkMode(TRANSPARENT);
		const COLORREF old_color = dc.SetTextColor(stamp_color);

		CPen pen(PS_SOLID, max(1, settings.stamp_strength / 20), stamp_color);
		CPen* old_pen = dc.SelectObject(&pen);
		CBrush* old_brush = static_cast<CBrush*>(dc.SelectStockObject(NULL_BRUSH));

		CRect stamp_rect(canvas_rect);
		stamp_rect.DeflateRect(canvas_rect.Width() / 6, canvas_rect.Height() / 3);
		dc.Ellipse(stamp_rect);
		dc.DrawText(text, stamp_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

		dc.SelectObject(old_brush);
		dc.SelectObject(old_pen);
		dc.SetTextColor(old_color);
		dc.SetBkMode(old_mode);
		dc.SelectObject(old_font);
	}

	void DrawPostage(CDC& dc, const CRect& canvas_rect, const requested_data& source, const PostageSettings& settings, const COLORREF background)
	{
		const PostageMetrics metrics = GetMetrics(source, settings);
		dc.FillSolidRect(canvas_rect, background);

		const CRect paper_rect = BuildPaperRect(canvas_rect);
		DrawPaperShape(dc, paper_rect, metrics, settings, background);

		const CRect image_rect = BuildImageRect(canvas_rect, source, metrics);
		DrawSourcePicture(dc, source, image_rect);
		DrawInnerFrame(dc, image_rect, settings);

		CRect border_rect(paper_rect);
		border_rect.DeflateRect(metrics.hole_radius + 4, metrics.hole_radius + 4);
		DrawValueText(dc, border_rect, settings);
		DrawStamp(dc, border_rect, settings);
	}
}

void DrawPostagePreview(CDC& dc, const CRect& preview_rect, const requested_data& source, const PostageSettings& settings)
{
	if (source.data == nullptr || source.picture_width <= 0 || source.picture_height <= 0)
		return;

	const PostageMetrics metrics = GetMetrics(source, settings);
	const CRect natural_canvas = BuildCanvasRect(source, metrics);
	CRect available(preview_rect);
	available.DeflateRect(4, 4);
	if (available.Width() <= 0 || available.Height() <= 0)
		return;

	double scale = min(
		static_cast<double>(available.Width()) / max(1, natural_canvas.Width()),
		static_cast<double>(available.Height()) / max(1, natural_canvas.Height()));
	scale = min(1.0, scale);

	const int target_width = max(1, static_cast<int>(std::lround(natural_canvas.Width() * scale)));
	const int target_height = max(1, static_cast<int>(std::lround(natural_canvas.Height() * scale)));
	const CRect target_rect(
		preview_rect.left + (preview_rect.Width() - target_width) / 2,
		preview_rect.top + (preview_rect.Height() - target_height) / 2,
		preview_rect.left + (preview_rect.Width() - target_width) / 2 + target_width,
		preview_rect.top + (preview_rect.Height() - target_height) / 2 + target_height);

	CDC mem_dc;
	mem_dc.CreateCompatibleDC(&dc);
	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(&dc, natural_canvas.Width(), natural_canvas.Height());
	CBitmap* old_bitmap = mem_dc.SelectObject(&bitmap);

	const CRect local_rect(0, 0, natural_canvas.Width(), natural_canvas.Height());
	DrawPostage(mem_dc, local_rect, source, settings, GetSysColor(COLOR_BTNFACE));

	if (target_width == natural_canvas.Width() && target_height == natural_canvas.Height())
	{
		dc.BitBlt(target_rect.left, target_rect.top, target_width, target_height, &mem_dc, 0, 0, SRCCOPY);
	}
	else
	{
		dc.SetStretchBltMode(HALFTONE);
		dc.StretchBlt(target_rect.left, target_rect.top, target_width, target_height, &mem_dc, 0, 0, natural_canvas.Width(), natural_canvas.Height(), SRCCOPY);
	}

	mem_dc.SelectObject(old_bitmap);
}

HBITMAP RenderPostageBitmap(const requested_data& source, const PostageSettings& settings, void*& dib_bits, int& output_width, int& output_height)
{
	dib_bits = nullptr;
	output_width = 0;
	output_height = 0;

	if (source.data == nullptr || source.picture_width <= 0 || source.picture_height <= 0)
		return NULL;

	const PostageMetrics metrics = GetMetrics(source, settings);
	const CRect canvas_rect = BuildCanvasRect(source, metrics);
	output_width = canvas_rect.Width();
	output_height = canvas_rect.Height();

	BITMAPINFO bitmap_info = { 0 };
	bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 24;
	bitmap_info.bmiHeader.biCompression = BI_RGB;
	bitmap_info.bmiHeader.biWidth = output_width;
	bitmap_info.bmiHeader.biHeight = output_height;

	HBITMAP dib = ::CreateDIBSection(NULL, &bitmap_info, DIB_RGB_COLORS, &dib_bits, NULL, 0);
	if (dib == NULL || dib_bits == nullptr)
		return NULL;

	CDC mem_dc;
	mem_dc.CreateCompatibleDC(NULL);
	CBitmap* bitmap = CBitmap::FromHandle(dib);
	CBitmap* old_bitmap = mem_dc.SelectObject(bitmap);

	DrawPostage(mem_dc, canvas_rect, source, settings, RGB(255, 255, 255));

	mem_dc.SelectObject(old_bitmap);
	return dib;
}
