#include "stdafx.h"
#include "plugin.h"
#include "resource.h"

// C# Plugin cpp_cs.


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
	return CFunctionPluginCS::GetInstance;
}


CFunctionPluginCS::CFunctionPluginCS()
	: handle_wnd(NULL),
	picture_processed(0)
{
}

CFunctionPluginCS::~CFunctionPluginCS()
{
}

struct plugin_data __stdcall CFunctionPluginCS::get_plugin_data() const
{
	struct plugin_data pluginData;

	// Set plugin info.
	pluginData.name.LoadString(IDS_PLUGIN_SHORT_DESC);
	pluginData.desc.LoadString(IDS_PLUGIN_LONG_DESC);
	pluginData.info.LoadString(IDS_PLUGIN_INFO);

	return pluginData;
}

struct arg_count __stdcall CFunctionPluginCS::get_arg_count() const
{
	// At least one picture.
	return arg_count(1, -1);
}


typedef void(__stdcall* TransferBytesFunc)(unsigned char*, int);


enum REQUEST_TYPE __stdcall CFunctionPluginCS::start(const HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	// Start event.

	handle_wnd = hwnd;


	WCHAR cwd[_MAX_PATH];
	_wgetcwd(cwd, _MAX_PATH);

	HINSTANCE hinstLib = LoadLibrary(L"CppCli.dll");

	if (hinstLib != NULL)
	{
		TransferBytesFunc TransferBytesFromCpp = (TransferBytesFunc)GetProcAddress(hinstLib, "TransferBytesFromCpp");
		if (TransferBytesFromCpp != NULL)
		{
			unsigned char data[] = { 1, 2, 3, 4, 5 };
			int length = sizeof(data) / sizeof(data[0]);
			TransferBytesFromCpp(data, length);
		}
		else
		{
			//std::cerr << "Could not locate the function." << std::endl;
		}
		FreeLibrary(hinstLib);
	}

	return REQUEST_TYPE::REQUEST_TYPE_DATA;
}

bool __stdcall CFunctionPluginCS::process_picture(const picture_data& picture_data)
{ 
	// Process picture event.

	picture_processed++;

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginCS::end(const vector<picture_data>& picture_data_list)
{ 
	// End event.

	//// Signal that the picture is added (enum UPDATE_TYPE).
	//update_data_list.push_back(update_data(filename,
	//	UPDATE_TYPE::UPDATE_TYPE_ADDED,
	//	// Picture will be added with this data:
	//	bitmap_width,
	//	bitmap_height,
	//	(BYTE*)ppvBits,
	//	// DIB data is using this format. When the end function return, the picture is created.
	//	// ppvBits is still valid until the plugin call is finished, and by that time
	//	// ~CFunctionPluginCS() release the DIB Section.
	//	DATA_REQUEST_TYPE::REQUEST_TYPE_BGR_DWORD_ALIGNED_DATA));


	// Return list of pictures that are updated, added or deleted (enum UPDATE_TYPE).
	return update_data_list;
}

typedef void(__cdecl* MyCSharpFunction)(unsigned char*, int);
