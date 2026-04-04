#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "resource.h"

// Plugin cpp_copy_cb.
// Copy the picture to the clipboard.
// 
// This plugin uses the REQUEST_TYPE::REQUEST_TYPE_DATA,
// request_data_size type to get resized picture data,
// DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA to set the DIB.


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
	// Implement one function plugin.
	return 1;
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int k)
{
	// Plugin-Fabric: return the one function plugin.
	return CFunctionPluginCopyCB::GetInstance;
}


CFunctionPluginCopyCB::CFunctionPluginCopyCB()
	: handle_wnd(NULL)
{
}

struct plugin_data __stdcall CFunctionPluginCopyCB::get_plugin_data() const
{
	struct plugin_data pluginData;

	// Set plugin info.
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);

	return pluginData;
}

struct arg_count __stdcall CFunctionPluginCopyCB::get_arg_count() const
{
	// Only one picture.
	return arg_count(1, 1);
}

enum REQUEST_TYPE __stdcall CFunctionPluginCopyCB::start(const HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	// Start event.

	handle_wnd = hwnd;

	// Allow only one picture.
	if (file_list.size() != 1)
	{
		CString msg;
		msg.LoadString(IDS_STRING_SINGLE_FILE_ONLY);
		::MessageBox(hwnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);

		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}

	// Request one unresized picture data set.
	// A negative value requests a relative size for the picture data.
	// For example, -100 requests data for the original 100% picture size.
	// To get picture data for the half size, use
	// request_data_sizes.push_back(request_data_size(-50, -50, DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA));
	request_data_sizes.push_back(request_data_size(-100, -100, DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA));

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginCopyCB::process_picture(const picture_data& picture_data)
{ 
	// Process picture event.

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return false;
}

const vector<update_data>& __stdcall CFunctionPluginCopyCB::end(const vector<picture_data>& picture_data_list)
{ 
	// End event.

	// Get the picture data.
	picture_data picture_data = picture_data_list[0];
	requested_data requested_data = picture_data.requested_data_list[0];

	bool bSuccess(false);

	if (requested_data.picture_width && requested_data.picture_height)
	{
		// Create a BITMAPINFOHEADER.
		BITMAPINFOHEADER bi;
		memset(&bi, 0, sizeof(BITMAPINFOHEADER));
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = requested_data.picture_width;
		bi.biHeight = requested_data.picture_height;
		bi.biPlanes = 1;
		bi.biBitCount = 24;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
		bi.biXPelsPerMeter = 0;
		bi.biYPelsPerMeter = 0;
		bi.biClrUsed = 0;
		bi.biClrImportant = 0;

		// Calculate the size of the DIB.
		auto widthDwordAligned = [](const int pixel) -> int
			{
				return (((pixel * 24) + 31) >> 3) & ~0x03;
			};
		const __int64 size3 = static_cast<__int64>(widthDwordAligned(requested_data.picture_width)) * requested_data.picture_height;
		const __int64 dibSize = sizeof(BITMAPINFOHEADER) + size3;

		// Allocate global memory for the DIB.
		const HGLOBAL hMem = GlobalAlloc(GHND, dibSize);
		if (hMem)
		{
			// Lock the memory and copy the BITMAPINFOHEADER and RGB data.
			BYTE* pDIB = reinterpret_cast<BYTE*>(::GlobalLock(hMem));
			memcpy(pDIB, &bi, sizeof(BITMAPINFOHEADER));
			memcpy(pDIB + sizeof(BITMAPINFOHEADER), requested_data.data, size3);
			::GlobalUnlock(hMem);

			if (::OpenClipboard(NULL))
			{
				::EmptyClipboard();
				if (!::SetClipboardData(CF_DIB, hMem))
				{
					// When SetClipboardData succeeds, the system takes ownership of the memory block,
					// and you should not free it. However, if SetClipboardData fails, the ownership
					// remains with your application, and you are responsible for freeing the memory.
					::GlobalFree(hMem);
				}

				::CloseClipboard();

				CString msg;
				msg.LoadString(IDS_CLIPBOARD_PICTURE);
				::MessageBox(handle_wnd, msg, get_plugin_data().name, MB_OK | MB_ICONINFORMATION);

				bSuccess = true;
			}
			else
			{
				::GlobalFree(hMem);
			}
		}
	}

	if (!bSuccess)
	{
		CString msg;
		msg.LoadString(IDS_CLIPBOARD_PICTURE_ERROR);
		::MessageBox(handle_wnd, msg, get_plugin_data().name, MB_OK | MB_ICONERROR);
	}

	// Return list of pictures that are updated, added or deleted (enum UPDATE_TYPE).
	return update_data_list;
}
