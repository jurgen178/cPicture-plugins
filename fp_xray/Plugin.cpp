#include "stdafx.h"
#include "Plugin.h"
#include "SettingsDlg.h"

namespace
{
	// Plugins run as DLLs, so the wait cursor must not depend on an MFC app object being present.
	class ScopedWaitCursor
	{
	public:
		ScopedWaitCursor()
			: previous_cursor(::SetCursor(::LoadCursor(nullptr, IDC_WAIT)))
		{
		}

		~ScopedWaitCursor()
		{
			::SetCursor(previous_cursor != nullptr ? previous_cursor : ::LoadCursor(nullptr, IDC_ARROW));
		}

	private:
		HCURSOR previous_cursor = nullptr;
	};

	inline int PixelIndex(const int x, const int y, const int width)
	{
		return 3 * (y * width + x);
	}

	inline BYTE ClampToByte(const int value)
	{
		return static_cast<BYTE>(max(0, min(255, value)));
	}

	void BlendPixel(BYTE* dst, const int index, const BYTE red, const BYTE green, const BYTE blue, const BYTE alpha)
	{
		dst[index] = static_cast<BYTE>((dst[index] * (255 - alpha) + red * alpha) / 255);
		dst[index + 1] = static_cast<BYTE>((dst[index + 1] * (255 - alpha) + green * alpha) / 255);
		dst[index + 2] = static_cast<BYTE>((dst[index + 2] * (255 - alpha) + blue * alpha) / 255);
	}

	CString GetBaseName(const CString& file_name)
	{
		const int slash = file_name.ReverseFind(L'\\');
		CString name = slash >= 0 ? file_name.Mid(slash + 1) : file_name;
		const int dot = name.ReverseFind(L'.');
		return dot > 0 ? name.Left(dot) : name;
	}

	CString GetDirectory(const CString& file_name)
	{
		const int slash = file_name.ReverseFind(L'\\');
		return slash >= 0 ? file_name.Left(slash + 1) : CString();
	}

	CString GetExtension(const CString& file_name)
	{
		const int dot = file_name.ReverseFind(L'.');
		return dot >= 0 ? file_name.Mid(dot) : L".jpg";
	}

	void CopyPanel(const BYTE* src, const int src_width, const int src_height,
		BYTE* dst, const int dst_width, const int offset_x, const int offset_y)
	{
		for (int y = 0; y < src_height; ++y)
		{
			for (int x = 0; x < src_width; ++x)
			{
				const int src_index = PixelIndex(x, y, src_width);
				const int dst_index = PixelIndex(offset_x + x, offset_y + y, dst_width);
				dst[dst_index] = src[src_index];
				dst[dst_index + 1] = src[src_index + 1];
				dst[dst_index + 2] = src[src_index + 2];
			}
		}
	}

	void CopyGrayPanel(const vector<BYTE>& gray, const int width, const int height,
		BYTE* dst, const int dst_width, const int offset_x, const int offset_y)
	{
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const BYTE value = gray[y * width + x];
				const int dst_index = PixelIndex(offset_x + x, offset_y + y, dst_width);
				dst[dst_index] = value;
				dst[dst_index + 1] = value;
				dst[dst_index + 2] = value;
			}
		}
	}

	void HeatColor(const BYTE value, BYTE& red, BYTE& green, BYTE& blue)
	{
		if (value < 85)
		{
			red = 0;
			green = static_cast<BYTE>(value * 3);
			blue = static_cast<BYTE>(128 + value / 2);
		}
		else if (value < 170)
		{
			const int t = value - 85;
			red = static_cast<BYTE>(t * 3);
			green = 255;
			blue = static_cast<BYTE>(255 - t * 3);
		}
		else
		{
			const int t = value - 170;
			red = 255;
			green = static_cast<BYTE>(max(0, 255 - t * 3));
			blue = 0;
		}
	}

	void CopyHeatPanel(const vector<BYTE>& heat, const int width, const int height,
		BYTE* dst, const int dst_width, const int offset_x, const int offset_y)
	{
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				BYTE red;
				BYTE green;
				BYTE blue;
				HeatColor(heat[y * width + x], red, green, blue);
				const int dst_index = PixelIndex(offset_x + x, offset_y + y, dst_width);
				dst[dst_index] = red;
				dst[dst_index + 1] = green;
				dst[dst_index + 2] = blue;
			}
		}
	}

	// The labels are drawn from a tiny built-in bitmap font so the output does not depend on GDI text rendering.
	const unsigned char* GetGlyph5x7(const char ch)
	{
		static const unsigned char glyph_space[7] = { 0, 0, 0, 0, 0, 0, 0 };
		static const unsigned char glyph_0[7] = { 14, 17, 19, 21, 25, 17, 14 };
		static const unsigned char glyph_1[7] = { 4, 12, 4, 4, 4, 4, 14 };
		static const unsigned char glyph_2[7] = { 14, 17, 1, 2, 4, 8, 31 };
		static const unsigned char glyph_3[7] = { 30, 1, 1, 14, 1, 1, 30 };
		static const unsigned char glyph_4[7] = { 2, 6, 10, 18, 31, 2, 2 };
		static const unsigned char glyph_5[7] = { 31, 16, 16, 30, 1, 1, 30 };
		static const unsigned char glyph_6[7] = { 14, 16, 16, 30, 17, 17, 14 };
		static const unsigned char glyph_7[7] = { 31, 1, 2, 4, 8, 8, 8 };
		static const unsigned char glyph_8[7] = { 14, 17, 17, 14, 17, 17, 14 };
		static const unsigned char glyph_9[7] = { 14, 17, 17, 15, 1, 1, 14 };
		static const unsigned char glyph_a[7] = { 14, 17, 17, 31, 17, 17, 17 };
		static const unsigned char glyph_b[7] = { 30, 17, 17, 30, 17, 17, 30 };
		static const unsigned char glyph_c[7] = { 14, 17, 16, 16, 16, 17, 14 };
		static const unsigned char glyph_d[7] = { 30, 17, 17, 17, 17, 17, 30 };
		static const unsigned char glyph_e[7] = { 31, 16, 16, 30, 16, 16, 31 };
		static const unsigned char glyph_f[7] = { 31, 16, 16, 30, 16, 16, 16 };
		static const unsigned char glyph_g[7] = { 14, 17, 16, 16, 19, 17, 14 };
		static const unsigned char glyph_h[7] = { 17, 17, 17, 31, 17, 17, 17 };
		static const unsigned char glyph_i[7] = { 31, 4, 4, 4, 4, 4, 31 };
		static const unsigned char glyph_j[7] = { 7, 2, 2, 2, 2, 18, 12 };
		static const unsigned char glyph_k[7] = { 17, 18, 20, 24, 20, 18, 17 };
		static const unsigned char glyph_l[7] = { 16, 16, 16, 16, 16, 16, 31 };
		static const unsigned char glyph_m[7] = { 17, 27, 21, 21, 17, 17, 17 };
		static const unsigned char glyph_n[7] = { 17, 25, 21, 19, 17, 17, 17 };
		static const unsigned char glyph_o[7] = { 14, 17, 17, 17, 17, 17, 14 };
		static const unsigned char glyph_p[7] = { 30, 17, 17, 30, 16, 16, 16 };
		static const unsigned char glyph_q[7] = { 14, 17, 17, 17, 21, 18, 13 };
		static const unsigned char glyph_r[7] = { 30, 17, 17, 30, 20, 18, 17 };
		static const unsigned char glyph_s[7] = { 14, 17, 16, 14, 1, 17, 14 };
		static const unsigned char glyph_t[7] = { 31, 4, 4, 4, 4, 4, 4 };
		static const unsigned char glyph_u[7] = { 17, 17, 17, 17, 17, 17, 14 };
		static const unsigned char glyph_v[7] = { 17, 17, 17, 17, 17, 10, 4 };
		static const unsigned char glyph_w[7] = { 17, 17, 17, 21, 21, 21, 10 };
		static const unsigned char glyph_x[7] = { 17, 17, 10, 4, 10, 17, 17 };
		static const unsigned char glyph_y[7] = { 17, 17, 10, 4, 4, 4, 4 };
		static const unsigned char glyph_z[7] = { 31, 1, 2, 4, 8, 16, 31 };

		const char upper = ch >= 'a' && ch <= 'z'
			? static_cast<char>(ch - 'a' + 'A')
			: ch;

		switch (upper)
		{
		case '0': return glyph_0;
		case '1': return glyph_1;
		case '2': return glyph_2;
		case '3': return glyph_3;
		case '4': return glyph_4;
		case '5': return glyph_5;
		case '6': return glyph_6;
		case '7': return glyph_7;
		case '8': return glyph_8;
		case '9': return glyph_9;
		case 'A': return glyph_a;
		case 'B': return glyph_b;
		case 'C': return glyph_c;
		case 'D': return glyph_d;
		case 'E': return glyph_e;
		case 'F': return glyph_f;
		case 'G': return glyph_g;
		case 'H': return glyph_h;
		case 'I': return glyph_i;
		case 'J': return glyph_j;
		case 'K': return glyph_k;
		case 'L': return glyph_l;
		case 'M': return glyph_m;
		case 'N': return glyph_n;
		case 'O': return glyph_o;
		case 'P': return glyph_p;
		case 'Q': return glyph_q;
		case 'R': return glyph_r;
		case 'S': return glyph_s;
		case 'T': return glyph_t;
		case 'U': return glyph_u;
		case 'V': return glyph_v;
		case 'W': return glyph_w;
		case 'X': return glyph_x;
		case 'Y': return glyph_y;
		case 'Z': return glyph_z;
		case ' ': return glyph_space;
		default: return glyph_space;
		}
	}

	void DrawChar5x7(BYTE* dst, const int image_width, const int image_height,
		const int origin_x, const int origin_y, const int scale,
		const char ch, const BYTE red, const BYTE green, const BYTE blue, const BYTE alpha)
	{
		const unsigned char* glyph = GetGlyph5x7(ch);
		for (int row = 0; row < 7; ++row)
		{
			for (int col = 0; col < 5; ++col)
			{
				if ((glyph[row] & (1 << (4 - col))) == 0)
					continue;

				for (int dy = 0; dy < scale; ++dy)
				{
					for (int dx = 0; dx < scale; ++dx)
					{
						const int x = origin_x + col * scale + dx;
						const int y = origin_y + row * scale + dy;
						if (x < 0 || x >= image_width || y < 0 || y >= image_height)
							continue;

						BlendPixel(dst, PixelIndex(x, y, image_width), red, green, blue, alpha);
					}
				}
			}
		}
	}

	void DrawLabel(BYTE* dst, const int image_width, const int image_height,
		const int panel_x, const int panel_y, const int panel_width, const int panel_height,
		const char* text)
	{
		const int scale = max(1, min(3, min(panel_width, panel_height) / 140));
		const int padding = 3 * scale;
		const int char_width = 5 * scale;
		const int char_spacing = scale;
		const int char_height = 7 * scale;
		const int text_len = static_cast<int>(strlen(text));
		const int banner_width = min(panel_width - padding * 2, padding * 2 + text_len * char_width + max(0, text_len - 1) * char_spacing);
		const int banner_height = char_height + padding * 2;

		for (int y = 0; y < banner_height; ++y)
		{
			for (int x = 0; x < banner_width; ++x)
			{
				const int px = panel_x + x;
				const int py = panel_y + y;
				if (px < 0 || px >= image_width || py < 0 || py >= image_height)
					continue;

				BlendPixel(dst, PixelIndex(px, py, image_width), 12, 18, 22, 132);
			}
		}

		int cursor_x = panel_x + padding;
		const int cursor_y = panel_y + padding;
		for (int index = 0; index < text_len; ++index)
		{
			DrawChar5x7(dst, image_width, image_height, cursor_x, cursor_y, scale, text[index], 245, 245, 238, 220);
			cursor_x += char_width + char_spacing;
		}
	}

	void OverlayGrid(BYTE* dst, const int width, const int height, const int offset_x, const int offset_y)
	{
		for (int y = 0; y < height; y += 8)
		{
			for (int x = 0; x < width; ++x)
			{
				const int index = PixelIndex(offset_x + x, offset_y + y, width * 2);
				BlendPixel(dst, index, 120, 190, 120, 56);
			}
		}

		for (int x = 0; x < width; x += 8)
		{
			for (int y = 0; y < height; ++y)
			{
				const int index = PixelIndex(offset_x + x, offset_y + y, width * 2);
				BlendPixel(dst, index, 120, 190, 120, 56);
			}
		}
	}

	void BuildGrayImage(const BYTE* src, const int width, const int height, vector<BYTE>& gray)
	{
		gray.resize(width * height);
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				const int index = PixelIndex(x, y, width);
				gray[y * width + x] = static_cast<BYTE>((src[index] * 77 + src[index + 1] * 150 + src[index + 2] * 29) >> 8);
			}
		}
	}

	void BuildEdgeMap(const vector<BYTE>& gray, const int width, const int height, vector<BYTE>& edge)
	{
		edge.assign(width * height, 0);
		for (int y = 1; y < height - 1; ++y)
		{
			for (int x = 1; x < width - 1; ++x)
			{
				const int gx = abs(gray[y * width + (x + 1)] - gray[y * width + (x - 1)]);
				const int gy = abs(gray[(y + 1) * width + x] - gray[(y - 1) * width + x]);
				edge[y * width + x] = ClampToByte(gx + gy);
			}
		}
	}

	// This highlights JPEG-style 8x8 discontinuities by sampling intensity jumps across block borders.
	void BuildBlockMap(const vector<BYTE>& gray, const int width, const int height, const int sensitivity, vector<BYTE>& block)
	{
		block.assign(width * height, 0);
		for (int x = 8; x < width; x += 8)
		{
			for (int y = 0; y < height; ++y)
			{
				const int diff = abs(gray[y * width + x] - gray[y * width + x - 1]);
				const BYTE value = ClampToByte(diff * sensitivity / 18);
				block[y * width + x] = max(block[y * width + x], value);
				block[y * width + x - 1] = max(block[y * width + x - 1], value);
			}
		}

		for (int y = 8; y < height; y += 8)
		{
			for (int x = 0; x < width; ++x)
			{
				const int diff = abs(gray[y * width + x] - gray[(y - 1) * width + x]);
				const BYTE value = ClampToByte(diff * sensitivity / 18);
				block[y * width + x] = max(block[y * width + x], value);
				block[(y - 1) * width + x] = max(block[(y - 1) * width + x], value);
			}
		}
	}

	void BuildCombinedHeat(const vector<BYTE>& edge, const vector<BYTE>& block, vector<BYTE>& heat)
	{
		heat.resize(edge.size());
		for (size_t index = 0; index < edge.size(); ++index)
			heat[index] = ClampToByte(block[index] + edge[index] / 3);
	}
}


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
	return CFunctionPluginXRay::GetInstance;
}


CFunctionPluginXRay::CFunctionPluginXRay()
	: handle_wnd(nullptr),
	  analysis_scale(60),
	  sensitivity(55),
	  show_grid(true)
{
}

CFunctionPluginXRay::~CFunctionPluginXRay()
{
	for (BYTE* buffer : generated_buffers)
		delete[] buffer;
}

struct plugin_data __stdcall CFunctionPluginXRay::get_plugin_data() const
{
	struct plugin_data pluginData;
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);
	return pluginData;
}

struct arg_count __stdcall CFunctionPluginXRay::get_arg_count() const
{
	return arg_count(1, -1);
}

enum REQUEST_TYPE __stdcall CFunctionPluginXRay::start(
	const HWND hwnd,
	const vector<const WCHAR*>& file_list,
	vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;
	update_data_list.clear();

	if (file_list.empty())
	{
		CString msg;
		msg.LoadString(IDS_MIN_SELECTION);
		::MessageBox(hwnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);
		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	CWnd parent;
	parent.Attach(handle_wnd);
	CSettingsDlg settings(&parent);
	settings.analysis_scale = analysis_scale;
	settings.sensitivity = sensitivity;
	settings.show_grid = show_grid ? TRUE : FALSE;

	if (settings.DoModal() != IDOK)
	{
		parent.Detach();
		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	parent.Detach();

	analysis_scale = settings.analysis_scale;
	sensitivity = settings.sensitivity;
	show_grid = settings.show_grid == TRUE;

	request_data_sizes.emplace_back(-analysis_scale, -analysis_scale, DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA);
	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginXRay::process_picture(const picture_data& picture_data)
{
	ScopedWaitCursor wait_cursor;

	if (picture_data.requested_data_list.empty())
		return true;

	const requested_data& request = picture_data.requested_data_list.front();
	if (request.data == nullptr || request.picture_width <= 0 || request.picture_height <= 0)
		return true;

	const int width = request.picture_width;
	const int height = request.picture_height;
	const int out_width = width * 2;
	const int out_height = height * 2;

	BYTE* composite = new BYTE[out_width * out_height * 3];
	generated_buffers.emplace_back(composite);
	for (int index = 0; index < out_width * out_height * 3; ++index)
		composite[index] = 18;

	vector<BYTE> gray;
	vector<BYTE> edge;
	vector<BYTE> block;
	vector<BYTE> heat;
	BuildGrayImage(request.data, width, height, gray);
	BuildEdgeMap(gray, width, height, edge);
	BuildBlockMap(gray, width, height, sensitivity, block);
	BuildCombinedHeat(edge, block, heat);

	// Assemble a 2x2 analysis board: original, edges, block boundaries and a combined heat view.
	CopyPanel(request.data, width, height, composite, out_width, 0, 0);
	CopyGrayPanel(edge, width, height, composite, out_width, width, 0);
	CopyGrayPanel(block, width, height, composite, out_width, 0, height);
	CopyHeatPanel(heat, width, height, composite, out_width, width, height);

	if (show_grid)
	{
		OverlayGrid(composite, width, height, 0, height);
		OverlayGrid(composite, width, height, width, height);
	}

	DrawLabel(composite, out_width, out_height, 0, 0, width, height, "ORIGINAL");
	DrawLabel(composite, out_width, out_height, width, 0, width, height, "EDGES");
	DrawLabel(composite, out_width, out_height, 0, height, width, height, "BLOCK 8X8");
	DrawLabel(composite, out_width, out_height, width, height, width, height, "COMBINED");

	const CString output_file(GetDirectory(picture_data.file_name) + GetBaseName(picture_data.file_name) + L"-xray" + GetExtension(picture_data.file_name));
	update_data_list.emplace_back(
		output_file,
		UPDATE_TYPE::UPDATE_TYPE_ADDED,
		out_width,
		out_height,
		composite,
		DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA);

	return true;
}

const vector<update_data>& __stdcall CFunctionPluginXRay::end(const vector<picture_data>& picture_data_list)
{
	return update_data_list;
}

