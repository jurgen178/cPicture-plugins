#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "resource.h"

// Example Plugin cpp_fp_ev.
// Function plugin to calculate expose difference.
// This example uses the REQUEST_TYPE::REQUEST_TYPE_DATA.


const CString __stdcall GetPluginVersion()
{
	return L"1.0";
}

const CString __stdcall GetPluginInterfaceVersion()
{
	return L"1.6";
}

const PLUGIN_TYPE __stdcall GetPluginType()
{
	return PLUGIN_TYPE_FUNCTION;
}

const int __stdcall GetPluginInit()
{
	// Implement one function plugin.
	return 1;
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int k)
{
	// Plugin-Fabric: return the one function plugin.
	return CFunctionPluginSample1::GetInstance;
}


CFunctionPluginSample1::CFunctionPluginSample1()
	: handle_wnd(NULL),
	picture_processed(0),
	pictures(0)
{
	_wsetlocale(LC_ALL, L".ACP");
}

struct PluginData __stdcall CFunctionPluginSample1::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.name = L"Belichtungsunterschied";
	pluginData.desc = L"Berechnet den Belichtungsunterschied";
	pluginData.info = L"Berechnet den Belichtungsunterschied der ausgewählten Bilder mit dem ersten Bild als Referenzbild.";

	return pluginData;
}

enum REQUEST_TYPE __stdcall CFunctionPluginSample1::start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;
	// Requires at least 2 pictures.
	if (file_list.size() < 2)
	{
		CString msg;
		msg.Format(IDS_MIN_SELECTION);
		::MessageBox(hwnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);

		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginSample1::process_picture(const picture_data& picture_data)
{ 
	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

double log2(double x) 
{
	return log(x) / log(2.0);
}

const vector<update_data>& __stdcall CFunctionPluginSample1::end(const vector<picture_data>& picture_data_list)
{
	CString list;
	vector<picture_data>::const_iterator it = picture_data_list.begin();

	if (it != picture_data_list.end())
	{
		const int f(it->file_name.ReverseFind(L'\\') + 1);
		const CString nameA(it->file_name.Mid(f));

		const double shutterspeedA(it->shutterspeed);
		const double apertureA(it->aperture);
		const double isoA(it->iso);

		++it;
		for (; it != picture_data_list.end(); ++it)
		{
			const int f(it->file_name.ReverseFind(L'\\') + 1);
			const CString nameB(it->file_name.Mid(f));

			const double shutterspeedB(it->shutterspeed);
			const double apertureB(it->aperture);
			const double isoB(it->iso);

			const double shutterspeedAB(log2(shutterspeedA) - log2(shutterspeedB));
			const double apertureAB(2 * (log2(apertureA) - log2(apertureB)));
			const double isoAB(log2(isoB) - log2(isoA));

			const double ev(shutterspeedAB + apertureAB + isoAB);

			CString evStr;
			evStr.Format(L"%s - %s = %.2fEV\n", nameA, nameB, ev);
			list += evStr;
		}

		CString msg;
		msg.LoadString(IDS_COPY);
		list += msg;
	}

	::MessageBox(handle_wnd, list, get_plugin_data().desc, MB_ICONINFORMATION);

	// Return list of pictures that are updated, added or deleted (enum UPDATE_TYPE).
	return update_data_list;
}

