#include "stdafx.h"
#include "Plugin.h"
#include "SettingsDlg.h"

#include <array>
#include <algorithm>


namespace
{
	// Plugins run in a DLL context, so the cursor handling stays on raw Win32 instead of MFC helpers.
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

	struct TrailColor
	{
		BYTE red;
		BYTE green;
		BYTE blue;
	};

	inline int PixelIndex(const int x, const int y, const int width)
	{
		return 3 * (y * width + x);
	}

	TrailColor GetTrailColor(const int frame_index)
	{
		static const std::array<TrailColor, 6> palette =
		{{
			{ 255, 92, 92 },
			{ 255, 174, 66 },
			{ 255, 232, 92 },
			{ 88, 216, 255 },
			{ 180, 132, 255 },
			{ 255, 120, 216 }
		}};

		return palette[frame_index % palette.size()];
	}

	CString AppendToBaseName(const CString& file_name, const CString& append)
	{
		const int extension_dot = file_name.ReverseFind(L'.');
		return file_name.Left(extension_dot) + append + file_name.Mid(extension_dot);
	}

	bool HasExpectedRgbBuffer(const requested_data& request)
	{
		if (request.data == nullptr || request.picture_width <= 0 || request.picture_height <= 0)
			return false;

		const __int64 expected_len = 3LL * request.picture_width * request.picture_height;
		return request.len >= expected_len;
	}

	// Every frame is centered on the requested output canvas so mixed aspect ratios still line up predictably.
	void CalcCenteredOffset(const int frame_width, const int frame_height,
		const int canvas_width, const int canvas_height,
		int& offset_x, int& offset_y)
	{
		offset_x = max(0, (canvas_width - frame_width) / 2);
		offset_y = max(0, (canvas_height - frame_height) / 2);
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
	return CFunctionPluginMotionComposer::GetInstance;
}


CFunctionPluginMotionComposer::CFunctionPluginMotionComposer()
	: handle_wnd(nullptr),
	  output_width(1280),
	  output_height(720),
	  difference_threshold(48),
	  colorize_frames(true),
	  composite_data(nullptr)
{
}

CFunctionPluginMotionComposer::~CFunctionPluginMotionComposer()
{
	delete[] composite_data;
}

struct plugin_data __stdcall CFunctionPluginMotionComposer::get_plugin_data() const
{
	struct plugin_data pluginData;
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);
	return pluginData;
}

struct arg_count __stdcall CFunctionPluginMotionComposer::get_arg_count() const
{
	return arg_count(2, -1);
}

enum REQUEST_TYPE __stdcall CFunctionPluginMotionComposer::start(
	const HWND hwnd,
	const vector<const WCHAR*>& file_list,
	vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;
	update_data_list.clear();

	if (file_list.size() < 2)
	{
		CString msg;
		msg.LoadString(IDS_MIN_SELECTION);
		::MessageBox(hwnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);
		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	CWnd parent;
	parent.Attach(handle_wnd);
	CSettingsDlg settings(&parent);
	settings.output_width = output_width;
	settings.output_height = output_height;
	settings.difference_threshold = difference_threshold;
	settings.colorize_frames = colorize_frames ? TRUE : FALSE;

	if (settings.DoModal() != IDOK)
	{
		parent.Detach();
		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	parent.Detach();

	output_width = settings.output_width;
	output_height = settings.output_height;
	difference_threshold = settings.difference_threshold;
	colorize_frames = settings.colorize_frames == TRUE;

	request_data_sizes.emplace_back(output_width, output_height, DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA);
	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginMotionComposer::process_picture(const picture_data& picture_data)
{
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginMotionComposer::end(const vector<picture_data>& picture_data_list)
{
	ScopedWaitCursor wait_cursor;
	update_data_list.clear();

	if (picture_data_list.size() < 2)
		return update_data_list;

	vector<const requested_data*> frames;
	for (const picture_data& picture : picture_data_list)
	{
		if (!picture.requested_data_list.empty() && picture.requested_data_list.front().data != nullptr)
			frames.emplace_back(&picture.requested_data_list.front());
	}

	if (frames.size() < 2)
		return update_data_list;

	const int canvas_width = output_width;
	const int canvas_height = output_height;
	if (canvas_width <= 0 || canvas_height <= 0)
		return update_data_list;

	if (!HasExpectedRgbBuffer(*frames.front()))
		return update_data_list;

	const __int64 canvas_len = 3LL * canvas_width * canvas_height;
	vector<BYTE> base_canvas(static_cast<size_t>(canvas_len), 0);

	delete[] composite_data;
	composite_data = new BYTE[static_cast<size_t>(canvas_len)];

	int base_offset_x = 0;
	int base_offset_y = 0;
	CalcCenteredOffset(frames.front()->picture_width, frames.front()->picture_height,
		canvas_width, canvas_height,
		base_offset_x, base_offset_y);

	// The first frame defines the visual baseline; later frames only add motion deltas on top of it.
	const BYTE* base_frame = frames.front()->data;
	for (int y = 0; y < frames.front()->picture_height; ++y)
	{
		for (int x = 0; x < frames.front()->picture_width; ++x)
		{
			const int src_index = PixelIndex(x, y, frames.front()->picture_width);
			const int canvas_x = base_offset_x + x;
			const int canvas_y = base_offset_y + y;
			if (canvas_x < 0 || canvas_x >= canvas_width || canvas_y < 0 || canvas_y >= canvas_height)
				continue;

			const int dst_index = PixelIndex(canvas_x, canvas_y, canvas_width);
			base_canvas[dst_index] = base_frame[src_index];
			base_canvas[dst_index + 1] = base_frame[src_index + 1];
			base_canvas[dst_index + 2] = base_frame[src_index + 2];
		}
	}

	for (__int64 index = 0; index < canvas_len; ++index)
		composite_data[index] = base_canvas[static_cast<size_t>(index)];

	for (size_t frame_index = 1; frame_index < frames.size(); ++frame_index)
	{
		if (!HasExpectedRgbBuffer(*frames[frame_index]))
			continue;

		// Compare every later frame against the baseline canvas and only keep meaningful per-pixel changes.
		const BYTE* frame = frames[frame_index]->data;
		const TrailColor trail = GetTrailColor(static_cast<int>(frame_index - 1));
		int frame_offset_x = 0;
		int frame_offset_y = 0;
		CalcCenteredOffset(frames[frame_index]->picture_width, frames[frame_index]->picture_height,
			canvas_width, canvas_height,
			frame_offset_x, frame_offset_y);

		for (int y = 0; y < frames[frame_index]->picture_height; ++y)
		{
			for (int x = 0; x < frames[frame_index]->picture_width; ++x)
			{
				const int index = PixelIndex(x, y, frames[frame_index]->picture_width);
				const int canvas_x = frame_offset_x + x;
				const int canvas_y = frame_offset_y + y;
				if (canvas_x < 0 || canvas_x >= canvas_width || canvas_y < 0 || canvas_y >= canvas_height)
					continue;

				const int canvas_index = PixelIndex(canvas_x, canvas_y, canvas_width);
				const int diff =
					abs(frame[index] - base_canvas[canvas_index]) +
					abs(frame[index + 1] - base_canvas[canvas_index + 1]) +
					abs(frame[index + 2] - base_canvas[canvas_index + 2]);

				if (diff < difference_threshold)
					continue;

				BYTE red = frame[index];
				BYTE green = frame[index + 1];
				BYTE blue = frame[index + 2];

				if (colorize_frames)
				{
					red = static_cast<BYTE>((red * 2 + trail.red) / 3);
					green = static_cast<BYTE>((green * 2 + trail.green) / 3);
					blue = static_cast<BYTE>((blue * 2 + trail.blue) / 3);
				}

				composite_data[canvas_index] = max(composite_data[canvas_index], red);
				composite_data[canvas_index + 1] = max(composite_data[canvas_index + 1], green);
				composite_data[canvas_index + 2] = max(composite_data[canvas_index + 2], blue);
			}
		}
	}

	const CString output_file(AppendToBaseName(picture_data_list.front().file_name, L"-motion-composer"));
	update_data_list.emplace_back(
		output_file,
		UPDATE_TYPE::UPDATE_TYPE_ADDED,
		canvas_width,
		canvas_height,
		composite_data,
		DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA);

	return update_data_list;
}

