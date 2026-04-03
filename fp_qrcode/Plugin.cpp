#include "stdafx.h"
#include "Plugin.h"
#include "locale.h"
#include "QrCodeDlg.h"
#include "qrcode.h"

// Plugin cpp_qrcode.
// Overlays a QR code onto the selected pictures at a chosen corner.

// RGB data is stored packed (stride = width * 3), top-to-bottom, R-G-B byte order.

static void DrawRealQRCode(BYTE* data, int img_width, int img_height,
	int corner, int qr_size_px, int margin_px, const CString& text)
{
	const std::string utf8 = CStringToUTF8(text);
	if (utf8.empty()) return;

	std::vector<bool> bitmap;
	int modules = 0;
	if (!GenerateQRCode(utf8, QRErrorLevel::M, bitmap, modules) || modules == 0)
		return;

	const int modulePx = max(1, qr_size_px / modules);
	const int drawSize = modules * modulePx;

	int qr_x, qr_y;
	switch (corner)
	{
	case 0: qr_x = margin_px;                         qr_y = margin_px; break;
	case 1: qr_x = img_width  - drawSize - margin_px; qr_y = margin_px; break;
	case 2: qr_x = margin_px;                         qr_y = img_height - drawSize - margin_px; break;
	default: qr_x = img_width - drawSize - margin_px; qr_y = img_height - drawSize - margin_px; break;
	}

	for (int my = 0; my < modules; my++)
	{
		for (int mx = 0; mx < modules; mx++)
		{
			const BYTE c = bitmap[my * modules + mx] ? 0 : 255;
			const int px0 = qr_x + mx * modulePx;
			const int py0 = qr_y + my * modulePx;
			for (int dy = 0; dy < modulePx; dy++)
			{
				const int py = py0 + dy;
				if (py < 0 || py >= img_height) continue;
				for (int dx = 0; dx < modulePx; dx++)
				{
					const int px = px0 + dx;
					if (px < 0 || px >= img_width) continue;
					BYTE* p = data + (py * img_width + px) * 3;
					p[0] = p[1] = p[2] = c;
				}
			}
		}
	}
}

// Fake QR helpers kept for reference (no longer called):

static inline void SetPixelRGB(BYTE* data, int img_width, int img_height,
	int x, int y, BYTE r, BYTE g, BYTE b)
{
	if (x < 0 || x >= img_width || y < 0 || y >= img_height)
		return;
	BYTE* p = data + (y * img_width + x) * 3;
	p[0] = r;
	p[1] = g;
	p[2] = b;
}

static void FillRectRGB(BYTE* data, int img_width, int img_height,
	int rx, int ry, int rw, int rh, BYTE r, BYTE g, BYTE b)
{
	for (int y = ry; y < ry + rh; ++y)
		for (int x = rx; x < rx + rw; ++x)
			SetPixelRGB(data, img_width, img_height, x, y, r, g, b);
}

// Draws one "finder pattern" square (the three corner squares of a real QR code).
// Position (fx, fy), total size fs x fs pixels.
// Pattern: black outer ring, white gap (1/7th), black inner square (3/7th).
static void DrawFinderPattern(BYTE* data, int img_width, int img_height,
	int fx, int fy, int fs)
{
	if (fs < 7) return;

	// Black outer square
	FillRectRGB(data, img_width, img_height, fx, fy, fs, fs, 0, 0, 0);

	// White inner (leave 1 cell border around the edge; cell = fs/7)
	const int cell = max(1, fs / 7);
	FillRectRGB(data, img_width, img_height,
		fx + cell, fy + cell, fs - 2 * cell, fs - 2 * cell,
		255, 255, 255);

	// Black inner core (3 cells from each side)
	const int core = cell * 2;
	FillRectRGB(data, img_width, img_height,
		fx + core, fy + core, fs - 2 * core, fs - 2 * core,
		0, 0, 0);
}

// Draws the fake (placeholder) QR code onto the RGB image.
// corner: 0=Top-Left, 1=Top-Right, 2=Bottom-Left, 3=Bottom-Right
static void DrawFakeQRCode(BYTE* data, int img_width, int img_height,
	int corner, int qr_size, int margin_px)
{
	const int margin = max(1, margin_px);

	int qr_x, qr_y;
	switch (corner)
	{
	case 0: // Top-Left
		qr_x = margin;
		qr_y = margin;
		break;
	case 1: // Top-Right
		qr_x = img_width - qr_size - margin;
		qr_y = margin;
		break;
	case 2: // Bottom-Left
		qr_x = margin;
		qr_y = img_height - qr_size - margin;
		break;
	default: // Bottom-Right
		qr_x = img_width - qr_size - margin;
		qr_y = img_height - qr_size - margin;
		break;
	}

	// White background
	FillRectRGB(data, img_width, img_height, qr_x, qr_y, qr_size, qr_size, 255, 255, 255);

	// Black outer frame
	const int bw = max(2, qr_size / 20); // border width
	FillRectRGB(data, img_width, img_height, qr_x, qr_y, qr_size, bw, 0, 0, 0); // top
	FillRectRGB(data, img_width, img_height, qr_x, qr_y + qr_size - bw, qr_size, bw, 0, 0, 0); // bottom
	FillRectRGB(data, img_width, img_height, qr_x, qr_y, bw, qr_size, 0, 0, 0); // left
	FillRectRGB(data, img_width, img_height, qr_x + qr_size - bw, qr_y, bw, qr_size, 0, 0, 0); // right

	// Three finder patterns (top-left, top-right, bottom-left of the QR area)
	const int fp = max(7, qr_size / 4); // finder pattern size
	const int fp_off = bw + 2; // offset from inner edge

	DrawFinderPattern(data, img_width, img_height,
		qr_x + fp_off, qr_y + fp_off, fp); // TL
	DrawFinderPattern(data, img_width, img_height,
		qr_x + qr_size - fp_off - fp, qr_y + fp_off, fp); // TR
	DrawFinderPattern(data, img_width, img_height,
		qr_x + fp_off, qr_y + qr_size - fp_off - fp, fp); // BL
}


// -------------------------------------------------------------------------
//  DLL entry points
// -------------------------------------------------------------------------

const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
	return L"1.7";
}

const PLUGIN_TYPE __stdcall GetPluginType()
{
	return PLUGIN_TYPE_FUNCTION;
}

const int __stdcall GetPluginInit()
{
	return 1;
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int k)
{
	return CFunctionPluginQRCode::GetInstance;
}


// -------------------------------------------------------------------------
//  Plugin implementation
// -------------------------------------------------------------------------

CFunctionPluginQRCode::CFunctionPluginQRCode()
	: handle_wnd(NULL),
	  qr_corner(3),
	  qr_text(L""),
	  qr_relative_size(20),
	  qr_margin_percent(2)
{
	_wsetlocale(LC_ALL, L".ACP");
}

struct plugin_data __stdcall CFunctionPluginQRCode::get_plugin_data() const
{
	struct plugin_data pluginData;
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);
	return pluginData;
}

struct arg_count __stdcall CFunctionPluginQRCode::get_arg_count() const
{
	// At least one picture; no upper limit.
	return arg_count(1, -1);
}

enum REQUEST_TYPE __stdcall CFunctionPluginQRCode::start(
	const HWND hwnd,
	const vector<const WCHAR*>& file_list,
	vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;

	CWnd parent;
	parent.Attach(handle_wnd);

	// Create the dialog temporarily just to measure the preview rect, then destroy it.
	vector<picture_data> empty_list;
	CQrCodeDlg tempDlg(empty_list, &parent);
	tempDlg.Create(IDD_DIALOG_QRCODE, &parent);
	const CRect previewRect = tempDlg.preview_rect;
	tempDlg.DestroyWindow();

	parent.Detach();

	// Request [0]: preview-sized BGR display data (first picture only, shown in the dialog).
	// Request [1]: full-size RGB data for the actual modification of every picture.
	request_data_sizes.push_back(
		request_data_size(previewRect.Width(), previewRect.Height(),
			DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA_DISPLAY));
	request_data_sizes.push_back(
		request_data_size(-100, -100, DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA));

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginQRCode::process_picture(const picture_data& picture_data)
{
	// Nothing to do per-picture; all work is done in end().
	// Return true to continue loading the next picture.
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginQRCode::end(
	const vector<picture_data>& picture_data_list)
{
	// Show the settings dialog with a live preview of the first picture.
	CWnd parent;
	parent.Attach(handle_wnd);

	CQrCodeDlg dlg(picture_data_list, &parent);
	dlg.corner = qr_corner;
	dlg.text = qr_text;
	dlg.relative_size = qr_relative_size;
	dlg.margin_percent = qr_margin_percent;

	if (dlg.DoModal() != IDOK)
	{
		parent.Detach();
		return update_data_list;
	}

	qr_corner = dlg.corner;
	qr_text = dlg.text;
	qr_relative_size = dlg.relative_size;
	qr_margin_percent = dlg.margin_percent;

	parent.Detach();

	constexpr int MIN_QR_SIZE = 32; // pixels – skip pictures whose QR would be smaller

	CString skipped;

	for (const picture_data& pd : picture_data_list)
	{
		if (pd.requested_data_list.empty())
			continue;

		// Index 1 is the full-size RGB data requested in start().
		if (pd.requested_data_list.size() < 2)
			continue;
		const requested_data& rd = pd.requested_data_list[1];

		if (rd.data == nullptr || rd.picture_width == 0 || rd.picture_height == 0)
			continue;

		// Calculate QR code size in pixels.
		const int shorter_side = min(rd.picture_width, rd.picture_height);
		const int qr_size      = (shorter_side * qr_relative_size) / 100;

		if (qr_size < MIN_QR_SIZE)
		{
			// Picture too small – collect name and skip.
			skipped += pd.file_name + L"\n";
			continue;
		}

		const int margin_px = max(1, shorter_side * qr_margin_percent / 100);

		// Draw the fake QR code directly onto the pixel buffer.
		// rd.data is BYTE* (non-const through the struct member), so bytes are writable.
		DrawRealQRCode(rd.data, rd.picture_width, rd.picture_height,
			qr_corner, qr_size, margin_px, qr_text);

		// Signal that this picture was updated.
		update_data_list.push_back(update_data(
			pd.file_name,
			UPDATE_TYPE::UPDATE_TYPE_UPDATED,
			rd.picture_width,
			rd.picture_height,
			rd.data,
			DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA));
	}

	// Inform the user about any skipped pictures.
	if (!skipped.IsEmpty())
	{
		CString msg;
		msg.LoadString(IDS_TOO_SMALL);
		msg += L"\n\n" + skipped;
		::MessageBox(handle_wnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);
	}

	return update_data_list;
}
