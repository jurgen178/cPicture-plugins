#include "resource.h"		// main symbols
#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include <io.h>

#include <vector>
using namespace std;

// PowerShell script plugin cpp_ps1_script.

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


bool scanVar(char* Text, char* TxtANSI, char* trueTxtANSI, WCHAR* TxtUnicode, WCHAR* trueTxtUnicode, bool def)
{
	char* p = strstr((char*)Text, TxtANSI);
	if(p)
	{
		p += strlen(TxtANSI);
		return strstr(p, trueTxtANSI) != NULL;
	}
	else
	{
		WCHAR* pw = wcsstr((WCHAR*)Text, TxtUnicode);
		if(pw)
		{
			pw += wcslen(TxtUnicode);
			return wcsstr(pw, trueTxtUnicode) != NULL;
		}
	}

	return def;
}


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
	return L"1.1";
}

const CString __stdcall GetPluginInterfaceVersion()
{
	return L"1.6";
}

const PLUGIN_TYPE __stdcall GetPluginType()
{
	return PLUGIN_TYPE_FUNCTION;
}


//#define GetInstance(n) static CFunctionPlugin* __stdcall GetInstance##n() { return new CFunctionPluginScript(Scripts[n]); };
//GetInstance(0)
//static CFunctionPlugin* __stdcall GetInstance0() { return new CFunctionPluginScript(Scripts[0]); };

#include "GetInstance.h"

const int maxscripts(sizeof(GetInstanceList)/sizeof(lpfnFunctionGetInstanceProc));

// Each .ps1 file will be set as a plugin.
const int __stdcall GetPluginInit()
{
	Scripts.clear();
	PluginProcArray.clear();

	const CString ScriptMask(L"*.ps1");
	WIN32_FIND_DATA c_file;
	HANDLE hFile;

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
	if(k < PluginProcArray.size())
		return PluginProcArray[k];
	else
		return NULL;
}

CFunctionPluginScript::CFunctionPluginScript(const CString& script)
  :	m_script(script),
	m_i(0),
	m_n(0)
{
	_wsetlocale(LC_ALL, L".ACP"); 
}

struct PluginData __stdcall CFunctionPluginScript::get_plugin_data()
{
	struct PluginData pluginData;

	pluginData.file_name = m_script;
	pluginData.name.FormatMessage(IDS_SCRIPT_SHORT, m_script);
	pluginData.desc.FormatMessage(IDS_SCRIPT_LONG, m_script.Left(m_script.ReverseFind(L'.')));

	return pluginData;
}


struct request_info __stdcall CFunctionPluginScript::start(HWND hwnd, const vector<const WCHAR*>& file_list)
{
	m_i = 0;
	m_n = (int)file_list.size();

	const bool bScript(CheckFile(m_script));
	if(!bScript)
	{
		const CString path(L".");
		WCHAR abs_path[MAX_PATH];
		memset(abs_path, 0, sizeof(abs_path));
		if(_wfullpath(abs_path, path, MAX_PATH-1) == NULL)
			wcsncpy_s(abs_path, MAX_PATH, path, MAX_PATH-1);

		CString msg, fmt;
		fmt.LoadString(IDS_ERROR_SCRIPT_MISSING);
		msg.FormatMessage(fmt, m_script, abs_path);

		AfxMessageBox(msg);
	}

	return request_info(bScript?PICTURE_LAYOUT_FILE_NAME_ONLY:PICTURE_LAYOUT_CANCEL_REQUEST);
}

bool __stdcall CFunctionPluginScript::process_picture(const picture_data& _picture_data) 
{ 
	// -name name
	// -file file
	// -dir dir
	// -width PictureWidth
	// -height PictureHeight
	// -i sequence number
	// -n size of files

	// Example:
	// -name c:\picture_folder\picture.jpg
	// -file picture.jpg
	// -dir c:\picture_folder\
	// -width 1024
	// -height 768
	// -i 1
	// -n 4

	const CString PowerShellExe(L"c:\\windows\\system32\\windowspowershell\\v1.0\\powershell.exe ");

//# plugin variables
//# displays a console, use this option for scripts with text output
//#[console=false]
// 
//# noexit keeps the powershell console open, remove this option to have the console closed when processing is done
//#[noexit=false]

	bool console(true);
	bool noexit(false);

	char* consoleTxtANSI = "#[console=";
	char* noexitTxtANSI = "#[noexit=";
	char* trueTxtANSI = "true]";
	WCHAR* consoleTxtUnicode = L"#[console=";
	WCHAR* noexitTxtUnicode = L"#[noexit=";
	WCHAR* trueTxtUnicode = L"true]";
	
	unsigned char Text[512];

	FILE* infile = NULL;
	const errno_t err(_wfopen_s(&infile, m_script, L"rb"));

	if (err == 0)
	{
		const int size(min(512, _filelength(_fileno(infile))));
		fread((char*)Text, sizeof(char), size, infile);

		console = scanVar((char*)Text, consoleTxtANSI, trueTxtANSI, consoleTxtUnicode, trueTxtUnicode, true);
		noexit = scanVar((char*)Text, noexitTxtANSI, trueTxtANSI, noexitTxtUnicode, trueTxtUnicode, false);
		
		fclose(infile);
	}

	CString script(L"-ExecutionPolicy Unrestricted ");

	if(noexit)
		script += L"-noexit ";	// -noexit keeps the powershell console open, remove this option to have the console closed when processing is done
	
	script += L"\".\\" + m_script + L"\" ";

	const int f(_picture_data.m_name.ReverseFind(L'\\')+1);
	const CString file(_picture_data.m_name.Mid(f));
	const CString dir(_picture_data.m_name.Left(f));

	const CString cmd_format(L"-name '%1' -file '%2' -dir '%3' -width %4!d! -height %5!d! -i %6!d! -n %7!d!");
	CString cmd;
	cmd.FormatMessage(cmd_format, 
		_picture_data.m_name, 
		file,
		dir, 
		_picture_data.m_OriginalPictureWidth,
		_picture_data.m_OriginalPictureHeight,
		m_i, m_n
		);

	script += cmd;

	SHELLEXECUTEINFO shInfo;
	memset(&shInfo, 0, sizeof(shInfo));
	shInfo.cbSize = sizeof(shInfo);

	shInfo.lpFile = PowerShellExe;
	shInfo.lpParameters = script;
	shInfo.nShow = console ? SW_SHOWNORMAL : SW_HIDE;
	shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;

	ShellExecuteEx(&shInfo);
	WaitForSingleObject(shInfo.hProcess, INFINITE);

	m_update_info.push_back(update_info(_picture_data.m_name, UPDATE_TYPE_UPDATED));

	m_i++;

	return true;
}
