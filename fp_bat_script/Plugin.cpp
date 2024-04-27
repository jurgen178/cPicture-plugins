#include "resource.h"		// main symbols
#include "stdafx.h"
#include "plugin.h"
#include "locale.h"

#include <vector>
using namespace std;

// Plugin cpp_bat_script

enum PLUGIN_TYPE operator|(const enum PLUGIN_TYPE t1, const enum PLUGIN_TYPE t2)
{
	return (enum PLUGIN_TYPE)((const unsigned int)t1 | (const unsigned int)t2);
}

enum PLUGIN_TYPE operator&(const enum PLUGIN_TYPE t1, const enum PLUGIN_TYPE t2)
{
	return (enum PLUGIN_TYPE)((const unsigned int)t1 & (const unsigned int)t2);
}


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


const int __stdcall GetPluginInit()
{
	const CString ScriptMask(L"*.bat");
	WIN32_FIND_DATA c_file;
	HANDLE hFile;

	// Register all *.bat scripts.
	if((hFile = FindFirstFile(ScriptMask, &c_file)) != INVALID_HANDLE_VALUE)
	{
		int i = 0;
		do
		{
			if(!(c_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(c_file.cFileName, L".") && wcscmp(c_file.cFileName, L"..")) 
			{
				const CString script(c_file.cFileName);
				Scripts.push_back(script);
				PluginProcArray.push_back(GetInstanceList[i++]);
			}
		}
		while(i < maxscripts && FindNextFile(hFile, &c_file) != 0);

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


CFunctionPluginBatScript::CFunctionPluginBatScript(const CString& script)
  :	m_script(script),
	m_i(0),
	m_n(0)
{
	_wsetlocale(LC_ALL, L".ACP"); 
}

struct PluginData __stdcall CFunctionPluginBatScript::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.file_name = m_script;
	pluginData.name.FormatMessage(IDS_SCRIPT_SHORT, m_script);
	pluginData.desc.FormatMessage(IDS_SCRIPT_LONG, m_script.Left(m_script.ReverseFind(L'.')));

	return pluginData;
}


struct request_info __stdcall CFunctionPluginBatScript::start(HWND hwnd, const vector<const WCHAR*>& file_list) 
{
	m_i = 0;
	m_n = (int)file_list.size();

	const bool bScript(CheckFile(m_script));
	if(!bScript)
	{
		const CString path(L".");
		WCHAR abs_path[MAX_PATH] = { 0 };
		if(_wfullpath(abs_path, path, MAX_PATH-1) == NULL)
			wcsncpy_s(abs_path, MAX_PATH, path, MAX_PATH-1);

		CString msg, fmt;
		fmt.LoadString(IDS_ERROR_SCRIPT_MISSING);
		msg.FormatMessage(fmt, m_script, abs_path);

		AfxMessageBox(msg);
	}

	return request_info(bScript?PICTURE_REQUEST_INFO_FILE_NAME_ONLY:PICTURE_REQUEST_INFO_CANCEL_REQUEST);
}

bool __stdcall CFunctionPluginBatScript::process_picture(const picture_data& picture_data) 
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
	const int f(picture_data.m_FileName.ReverseFind(L'\\')+1);
	const CString name(picture_data.m_FileName.Mid(f));
	const CString dir(picture_data.m_FileName.Left(f));

	CString cmd;
	cmd.FormatMessage(cmd_format, 
		picture_data.m_FileName,
		name,
		dir,
		picture_data.m_OriginalPictureWidth,
		picture_data.m_OriginalPictureHeight,
		m_i, 
		m_n
		);

	CString script;
	script += L"\"";
	script += m_script;
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
	m_update_info.push_back(update_info(picture_data.m_FileName, UPDATE_TYPE_UPDATED));

	m_i++;

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

