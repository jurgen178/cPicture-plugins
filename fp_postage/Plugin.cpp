#include "stdafx.h"
#include "Plugin.h"
#include "PostageDlg.h"

namespace
{
	CString AppendToBaseName(const CString& file_name, const CString& append)
	{
		const int extension_dot = file_name.ReverseFind(L'.');
		return file_name.Left(extension_dot) + append + file_name.Mid(extension_dot);
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
	return CFunctionPluginPostage::GetInstance;
}

CFunctionPluginPostage::CFunctionPluginPostage()
	: handle_wnd(NULL)
{
}

CFunctionPluginPostage::~CFunctionPluginPostage()
{
	ClearDibs();
}

void CFunctionPluginPostage::ClearDibs()
{
	for (HBITMAP dib : dib_list)
		::DeleteObject(dib);
	dib_list.clear();
}

struct plugin_data __stdcall CFunctionPluginPostage::get_plugin_data() const
{
	struct plugin_data pluginData;
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);
	return pluginData;
}

struct arg_count __stdcall CFunctionPluginPostage::get_arg_count() const
{
	return arg_count(1, -1);
}

enum REQUEST_TYPE __stdcall CFunctionPluginPostage::start(
	const HWND hwnd,
	const vector<const WCHAR*>& file_list,
	vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;
	if (file_list.empty())
		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;

	CWnd parent;
	parent.Attach(handle_wnd);

	vector<picture_data> empty_list;
	CPostageDlg temp_dlg(empty_list, &parent);
	temp_dlg.Create(IDD_DIALOG_POSTAGE, &parent);
	const CRect preview_rect = temp_dlg.preview_rect;
	temp_dlg.DestroyWindow();

	parent.Detach();

	request_data_sizes.emplace_back(
		max(64, preview_rect.Width() * 3 / 4),
		max(64, preview_rect.Height() * 3 / 4),
		DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA_DISPLAY);
	request_data_sizes.emplace_back(-100, -100, DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA);
	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginPostage::process_picture(const picture_data& picture_data)
{
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginPostage::end(const vector<picture_data>& picture_data_list)
{
	update_data_list.clear();
	ClearDibs();
	if (picture_data_list.empty())
		return update_data_list;

	CWnd parent;
	parent.Attach(handle_wnd);

	CPostageDlg dlg(picture_data_list, &parent);
	dlg.settings = settings;
	if (dlg.DoModal() != IDOK)
	{
		parent.Detach();
		return update_data_list;
	}

	parent.Detach();
	settings = dlg.settings;

	for (const picture_data& picture : picture_data_list)
	{
		if (picture.requested_data_list.size() < 2)
			continue;

		const requested_data& source = picture.requested_data_list[1];
		void* dib_bits = nullptr;
		int output_width = 0;
		int output_height = 0;
		HBITMAP dib = RenderPostageBitmap(source, settings, dib_bits, output_width, output_height);
		if (dib == NULL || dib_bits == nullptr)
			continue;

		dib_list.emplace_back(dib);

		const CString output_file(AppendToBaseName(picture.file_name, L"-postage"));
		update_data_list.emplace_back(
			output_file,
			UPDATE_TYPE::UPDATE_TYPE_ADDED,
			output_width,
			output_height,
			reinterpret_cast<BYTE*>(dib_bits),
			DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA);
	}

	return update_data_list;
}
