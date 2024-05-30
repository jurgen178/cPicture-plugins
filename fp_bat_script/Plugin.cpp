#include "resource.h"		// main symbols
#include "stdafx.h"
#include "plugin.h"
#include "locale.h"

#include <vector>
using namespace std;

// Plugin cpp_bat_script
// Runs a .bat script for each selected picture.

static vector<lpfnFunctionGetInstanceProc> PluginProcArray;
static vector<CString> Scripts;


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


#include "GetInstance.h"

const int maxscripts(sizeof(GetInstanceList) / sizeof(lpfnFunctionGetInstanceProc));

// Each .bat file will be set as a plugin.
const int __stdcall GetPluginInit()
{
	Scripts.clear();
	PluginProcArray.clear();

	WIN32_FIND_DATA c_file;
	HANDLE hFile;

	// Register all *.bat scripts.
	const CString ScriptMask(L"*.bat");
	if((hFile = FindFirstFile(ScriptMask, &c_file)) != INVALID_HANDLE_VALUE)
	{
		int sequence = 0;

		do
		{
			if(!(c_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(c_file.cFileName, L".") && wcscmp(c_file.cFileName, L"..")) 
			{
				const CString script(c_file.cFileName);
				Scripts.push_back(script);
				PluginProcArray.push_back(GetInstanceList[sequence++]);
			}
		}
		while(sequence < maxscripts && FindNextFile(hFile, &c_file) != 0);

		FindClose(hFile);
	}

	// Anzahl als Negativwert wenn externe Moduldateien (*.bat, *.ps1) verwendet werden.
	return -(int)PluginProcArray.size();
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int k)
{
	if(k >= 0 && k < PluginProcArray.size())
		return PluginProcArray[k];
	else
		return NULL;
}


bool CheckFile(const WCHAR* pFile)
{
	if (wcslen(pFile))
	{
		FILE* infile = NULL;
		const errno_t err(_wfopen_s(&infile, pFile, L"r"));

		if (err == 0)
		{
			fclose(infile);
			return true;
		}
	}

	return false;
}


CFunctionPluginScript::CFunctionPluginScript(const CString& script_file)
  : script_file(script_file),
	sequence(0),
	max_files(0)
{
	_wsetlocale(LC_ALL, L".ACP"); 
}

struct PluginData __stdcall CFunctionPluginScript::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.file_name = script_file;
	pluginData.name.FormatMessage(IDS_SCRIPT_SHORT, script_file);
	pluginData.desc.FormatMessage(IDS_SCRIPT_LONG, script_file.Left(script_file.ReverseFind(L'.')));

	return pluginData;
}


enum REQUEST_TYPE __stdcall CFunctionPluginScript::start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	sequence = 0;
	max_files = (int)file_list.size();

	const bool bScript(CheckFile(script_file));
	if(!bScript)
	{
		const CString path(L".");
		WCHAR abs_path[MAX_PATH] = { 0 };
		if(_wfullpath(abs_path, path, MAX_PATH-1) == NULL)
			wcsncpy_s(abs_path, MAX_PATH, path, MAX_PATH-1);

		CString msg;
		msg.FormatMessage(IDS_ERROR_SCRIPT_MISSING, script_file, abs_path);

		::MessageBox(hwnd, msg, get_plugin_data().desc, MB_ICONEXCLAMATION);
	}

	return bScript ? REQUEST_TYPE::REQUEST_TYPE_DATA : REQUEST_TYPE::REQUEST_TYPE_CANCEL;
}

bool __stdcall CFunctionPluginScript::process_picture(const picture_data& picture_data)
{ 
	// %1 file
	// %2 name
	// %3 dir
	// %4 PictureWidth
	// %5 PictureHeight
	// %6 sequence number
	// %7 number of files

	// Example:
	// file = "c:\picture_folder\picture.jpg"
	// name = "picture.jpg"
	// dir = "c:\picture_folder\"
	// width = 1200
	// height = 900
	// sequence number = 0
	// number of files = 1

	const CString cmd_format(L" \"%1\" \"%2\" \"%3\" %4!d! %5!d! %6!d! %7!d!");
	const int f(picture_data.file_name.ReverseFind(L'\\')+1);
	const CString name(picture_data.file_name.Mid(f));
	const CString dir(picture_data.file_name.Left(f));

	CString cmd;
	cmd.FormatMessage(cmd_format, 
		picture_data.file_name,
		name,
		dir,
		picture_data.picture_width,
		picture_data.picture_height,
		sequence, 
		max_files
		);

	CString script;
	script += L"\"";
	script += script_file;
	script += L"\"";

	SHELLEXECUTEINFO shInfo = { 0 };
	shInfo.cbSize = sizeof(shInfo);

	shInfo.lpFile = script;
	shInfo.lpParameters = cmd;
	shInfo.nShow = SW_SHOWNORMAL;
	shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;

	ShellExecuteEx(&shInfo);
	WaitForSingleObject(shInfo.hProcess, INFINITE);

	// Signal that the picture could be updated.
	// This info will be submitted in the 'end' event.
	update_data_list.push_back(update_data(picture_data.file_name, UPDATE_TYPE::UPDATE_TYPE_UPDATED));

	sequence++;

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

