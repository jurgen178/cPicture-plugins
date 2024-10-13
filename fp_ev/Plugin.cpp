#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "resource.h"
#include <functional>

// Example Plugin cpp_fp_ev.
// Function plugin to calculate the exposure difference.
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
	return CFunctionPluginEV::GetInstance;
}


CFunctionPluginEV::CFunctionPluginEV()
	: handle_wnd(NULL),
	picture_processed(0),
	pictures(0)
{
	_wsetlocale(LC_ALL, L".ACP");
}

struct PluginData __stdcall CFunctionPluginEV::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);

	return pluginData;
}

enum REQUEST_TYPE __stdcall CFunctionPluginEV::start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;

	// Requires at least 2 pictures.
	if (file_list.size() < 2)
	{
		CString msg;
		msg.Format(IDS_MIN_SELECTION);
		::MessageBox(handle_wnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);

		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginEV::process_picture(const picture_data& picture_data)
{ 
	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

double log_2(double x) 
{
	return log(x) / log(2.0);
}

// Aggregate vector to a string.
template<class T>
CString aggregate(typename T::iterator begin,
	typename T::iterator end,
	const std::function<CString(unsigned int)>& toStringFunction,
	const CString& startText,
	const CString& endText,
	const CString& separator)
{
	CString matchParameter(startText);
	CString text;
	for (typename T::const_iterator it2 = begin; it2 != end; ++it2)
	{
		// Add text.
		matchParameter += toStringFunction(*it2);

		// Skip separator for last element.
		if (it2 != end - 1)
		{
			matchParameter += separator;
		}
	}
	matchParameter += endText;

	return matchParameter;
}

const vector<update_data>& __stdcall CFunctionPluginEV::end(const vector<picture_data>& picture_data_list)
{
	CString list;
	vector<picture_data>::const_iterator it = picture_data_list.begin();

	if (it != picture_data_list.end())
	{
		// Reference picture A
		const int f(it->file_name.ReverseFind(L'\\') + 1);
		const CString nameA(it->file_name.Mid(f));

		const double shutterspeedA(it->shutterspeed);
		const double apertureA(it->aperture);
		const double isoA(it->iso);

		if (shutterspeedA < 1.0 && apertureA < 1.0 && isoA < 1.0)
		{
			list.Format(IDS_NO_EXIF, nameA);
		}
		else
		{
			// Calculate diff to the reference picture A

			++it;
			for (; it != picture_data_list.end(); ++it)
			{
				const int f(it->file_name.ReverseFind(L'\\') + 1);
				const CString nameB(it->file_name.Mid(f));

				const double shutterspeedB(it->shutterspeed);
				const double apertureB(it->aperture);
				const double isoB(it->iso);

				const bool shutterspeedMatch(shutterspeedA >= 1.0 && shutterspeedB >= 1.0);
				const bool apertureMatch(apertureA >= 1.0 && apertureB >= 1.0);
				const bool isoMatch(isoA >= 1.0 && isoB >= 1.0);

				CString evStr;
				vector <unsigned int> matchList;

				if (shutterspeedMatch || apertureMatch || isoMatch)
				{
					// Diff
					auto shutterspeedEV([&matchList](double shutterspeedA, double shutterspeedB) { matchList.push_back(IDS_SHUTTERSPEED); return log_2(shutterspeedA) - log_2(shutterspeedB); });
					const double shutterspeedAB(shutterspeedMatch ? shutterspeedEV(shutterspeedA, shutterspeedB) : 0.0);

					auto apertureEV([&matchList](double apertureA, double apertureB) { matchList.push_back(IDS_APERTURE); return 2 * (log_2(apertureA) - log_2(apertureB)); });
					const double apertureAB(apertureMatch ? apertureEV(apertureA, apertureB) : 0.0);

					auto isoEV([&matchList](double isoA, double isoB) { matchList.push_back(IDS_ISO); return log_2(isoB) - log_2(isoA); });
					const double isoAB(isoMatch ? isoEV(isoA, isoB) : 0.0);

					const double ev(shutterspeedAB + apertureAB + isoAB);

					// toString lambda expression
					auto getText([](unsigned int id) { CString text; text.LoadString(id); return text; });
					
					// Add list of matched parameter '(p1, p2, p3)'.		
					const CString matchParameter(aggregate<vector<unsigned int>>(matchList.begin(), matchList.end(), getText, L"(", L")", L", "));

					evStr.Format(L"%s - %s = %.2fEV %s\n", nameA, nameB, ev, matchParameter);
				}
				else
				{
					evStr.Format(L"%s - %s = --EV\n", nameA, nameB);
				}

				list += evStr;
			}

			CString msg;
			msg.LoadString(IDS_COPY);

			list += msg;
		}
	}

	::MessageBox(handle_wnd, list, get_plugin_data().desc, MB_ICONINFORMATION);

	// Return list of pictures that are updated, added or deleted (enum UPDATE_TYPE).
	return update_data_list;
}

