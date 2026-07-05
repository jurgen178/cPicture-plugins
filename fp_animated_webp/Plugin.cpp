// Plugin.cpp — Animated WebP function plugin for cPicture.
//
// Creates an animated WebP file from the selected images.
// Uses libwebp (shared from webp_format/external/) via WebPAnimEncoder API.
// No extra library download needed — libwebp is already part of the plugin suite.

#include "stdafx.h"
#include "Plugin.h"
#include "SettingsDlg.h"
#include "EncodeDlg.h"
#include "resource.h"

#include <vector>
using namespace std;


// ── Plugin export functions ─────────────────────────────────────────────────

const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
#ifdef _DEBUG
	return L"1.7-debug";
#else
	return L"1.7";
#endif
}

const PLUGIN_TYPE __stdcall GetPluginType()
{
	return PLUGIN_TYPE_FUNCTION;
}

const int __stdcall GetPluginInit()
{
	return 1;
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int /*k*/)
{
	return CFunctionPluginAnimatedWebP::GetInstance;
}


// ── CFunctionPluginAnimatedWebP ──────────────────────────────────────────────

CFunctionPluginAnimatedWebP::CFunctionPluginAnimatedWebP()
	: handle_wnd(NULL)
{
}

struct plugin_data __stdcall CFunctionPluginAnimatedWebP::get_plugin_data() const
{
	struct plugin_data d;
	d.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	d.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	d.info.LoadString(IDS_PLUGIN_INFO);
	return d;
}

struct arg_count __stdcall CFunctionPluginAnimatedWebP::get_arg_count() const
{
	// At least 2 images, no upper limit
	return arg_count(2, -1);
}

enum REQUEST_TYPE __stdcall CFunctionPluginAnimatedWebP::start(
	const HWND hwnd,
	const vector<const WCHAR*>& file_list,
	vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;

	if (file_list.size() < 2)
	{
		CString msg;
		msg.LoadString(IDS_ERR_MIN_PICTURES);
		::MessageBox(hwnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);
		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	// Show settings dialog before loading pixel data
	CSettingsDlg dlg(m_settings, CWnd::FromHandle(hwnd));
	if (dlg.DoModal() != IDOK)
		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;

	m_settings = dlg.GetSettings();

	// Request pixel data scaled to the user-configured output width.
	// Passing absolute dimensions keeps memory proportional to the output size,
	// not the original camera resolution. 0 = original full resolution (-100%).
	if (m_settings.output_width > 0)
		request_data_sizes.emplace_back(m_settings.output_width, m_settings.output_width,
			DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA);
	else
		request_data_sizes.emplace_back(-100, -100, DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA);

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginAnimatedWebP::process_picture(const picture_data& /*pd*/)
{
	// Collect all frames before encoding
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginAnimatedWebP::end(
	const vector<picture_data>& picture_data_list)
{
	if (picture_data_list.empty())
		return update_data_list;

	// Build output path: first image base name + "_animated.webp"
	const CString& first = picture_data_list.front().file_name;
	const int dot = first.ReverseFind(L'.');
	const CString outPath = (dot >= 0 ? first.Left(dot) : first) + L"_animated.webp";

	// Run encoding in a background thread with a progress + cancel dialog
	CEncodeDlg dlg(handle_wnd, picture_data_list, m_settings, outPath);
	dlg.DoModal();

	if (dlg.IsCancelled())
	{
		// Silently cancelled — nothing to report
	}
	else if (dlg.IsSuccess())
	{
		// Add the new file to the cPicture file list.
		// No pixel data needed — cPictureWnd now handles UPDATE_TYPE_ADDED with
		// data=null by using it->file_name directly (file was already written to disk).
		update_data_list.emplace_back(outPath, UPDATE_TYPE::UPDATE_TYPE_ADDED);

		CString msg;
		msg.LoadString(IDS_SUCCESS);
		const int slash = outPath.ReverseFind(L'\\');
		msg += L"\n" + (slash >= 0 ? outPath.Mid(slash + 1) : outPath);
		::MessageBox(handle_wnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		CString msg;
		msg.LoadString(IDS_ERR_ENCODE_FAILED);
		::MessageBox(handle_wnd, msg, get_plugin_data().name, MB_OK | MB_ICONERROR);
	}

	return update_data_list;
}
