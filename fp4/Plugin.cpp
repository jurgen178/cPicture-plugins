#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include "resource.h"

// Example Plugin cpp_fp4.
// Resize and invert pictures.
// 
// This example uses the REQUEST_TYPE::REQUEST_TYPE_DATA,
// request_data_size type to get resized picture data,
// DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA for picture modification 
// and the update_data type to update/create a picture.


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
	return CFunctionPluginSample4::GetInstance;
}


CFunctionPluginSample4::CFunctionPluginSample4()
	: handle_wnd(NULL),
	picture_processed(0)
{
}

struct PluginData __stdcall CFunctionPluginSample4::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);

	return pluginData;
}

enum REQUEST_TYPE __stdcall CFunctionPluginSample4::start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	// Start event.

	handle_wnd = hwnd;
	
	CString msg;
	msg.LoadString(IDS_START_CONFIRM);

	if (::MessageBox(handle_wnd, msg, get_plugin_data().desc, MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		// Request a half size picture size data in RGB layout.
		// A negative value requests a relative size for the picture data.
		request_data_sizes.push_back(request_data_size(-50, -50, DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA));

		return REQUEST_TYPE::REQUEST_TYPE_DATA;
	}
	else
	{
		return REQUEST_TYPE::REQUEST_TYPE_CANCEL;
	}
}

bool __stdcall CFunctionPluginSample4::process_picture(const picture_data& picture_data)
{ 
	// Process picture event.

	// Get the requested data (picture resized to 50%).
	// requested_data_list is one element as only one size was requested.
	requested_data requested_data1 = picture_data.requested_data_list.front();

	BYTE* data = requested_data1.data;

	// Modify the picture data.
	for (register unsigned int y = requested_data1.picture_height; y != 0; y--)
	{
		for (register unsigned int x = requested_data1.picture_width; x != 0; x--)
		{
			// Invert colors.
			*data = 255 - *data++;	// red
			*data = 255 - *data++;	// green
			*data = 255 - *data++;	// blue

			//// Or make a grey scale picture.
			//const BYTE grey((306 * *(data) + 601 * *(data + 1) + 117 * *(data + 2)) >> 10);
			//*data++ = grey;	// red
			//*data++ = grey;	// green
			//*data++ = grey;	// blue
		}
	}

	// Or modify only a part of the picture:
	//// Process only the upper left half side with a border of 20 pixel.
	//// 
	////  --------
	//// |XXXX    |
	//// |XXXX    |
	//// |XXXX    |
	//// |        |
	//// |        |
	//// |        |
	////  --------
	////
	//for (register unsigned int y = 20; y < requested_data1.picture_height / 2; y++)
	//{
	//	for (register unsigned int x = 20; x < requested_data1.picture_width / 2; x++)
	//	{
	//		const int index(3 * (y * requested_data1.picture_width + x));

	//		const BYTE red(data[index]);
	//		const BYTE green(data[index + 1]);
	//		const BYTE blue(data[index + 2]);

	//		// Swap colors.
	//		data[index] = blue;			// red
	//		data[index + 1] = red;		// green
	//		data[index + 2] = green;	// blue
	//	}
	//}


	// Signal that the picture is updated (enum UPDATE_TYPE).
	// Use UPDATE_TYPE_ADDED if the file name is changed.
	update_data_list.push_back(update_data(picture_data.file_name,
		UPDATE_TYPE_UPDATED,
		// Picture will be updated with this data:
		requested_data1.picture_width,
		requested_data1.picture_height,
		requested_data1.data,
		DATA_REQUEST_TYPE::REQUEST_TYPE_RGB_DATA));

	picture_processed++;

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginSample4::end(const vector<picture_data>& picture_data_list)
{ 
	// End event.

	CString msg;
	msg.Format(IDS_END_MSG, picture_processed);
	::MessageBox(handle_wnd, msg, get_plugin_data().desc, MB_ICONINFORMATION);

	// Return list of pictures that are updated, added or deleted (enum UPDATE_TYPE).
	return update_data_list;
}

