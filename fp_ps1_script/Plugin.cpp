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
	if (p)
	{
		p += strlen(TxtANSI);
		return strstr(p, trueTxtANSI) != NULL;
	}
	else
	{
		WCHAR* pw = wcsstr((WCHAR*)Text, TxtUnicode);
		if (pw)
		{
			pw += wcslen(TxtUnicode);
			return wcsstr(pw, trueTxtUnicode) != NULL;
		}
	}

	return def;
}

CString escapeJsonData(CString text)
{
	text.Replace(L"'", L"''");
	text.Replace(L"\\", L"\\\\");
	text.Replace(L"\"", L"\\\\\"\"\"");	// " -> \\"""

	return text;
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

// Each .ps1 file will be set as a plugin.
const int __stdcall GetPluginInit()
{
	Scripts.clear();
	PluginProcArray.clear();

	const CString ScriptMask(L"*.ps1");
	WIN32_FIND_DATA c_file;
	HANDLE hFile;

	if ((hFile = FindFirstFile(ScriptMask, &c_file)) != INVALID_HANDLE_VALUE)
	{
		int i = 0;
		do
		{
			if (!(c_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(c_file.cFileName, L".") && wcscmp(c_file.cFileName, L".."))
			{
				const CString script(c_file.cFileName);
				Scripts.push_back(script);
				PluginProcArray.push_back(GetInstanceList[i++]);
			}
		} while (i < maxscripts && FindNextFile(hFile, &c_file) != 0);

		FindClose(hFile);
	}

	// Anzahl als Negativwert wenn externe Moduldateien (*.bat, *.ps1) verwendet werden.
	return -(int)PluginProcArray.size();
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int k)
{
	if (k < PluginProcArray.size())
		return PluginProcArray[k];
	else
		return NULL;
}

CFunctionPluginPs1Script::CFunctionPluginPs1Script(const CString& script)
	: m_PowerShellExe(L"c:\\windows\\system32\\windowspowershell\\v1.0\\powershell.exe "),
	m_script(script)
{
	// Do not set locale to keep decimal point (LC_NUMERIC) for PowerShell.
	//_wsetlocale(LC_ALL, L".ACP");
}

struct PluginData __stdcall CFunctionPluginPs1Script::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.file_name = m_script;
	pluginData.name.FormatMessage(IDS_SCRIPT_SHORT, m_script);
	pluginData.desc.FormatMessage(IDS_SCRIPT_LONG, m_script.Left(m_script.ReverseFind(L'.')));

	return pluginData;
}


struct request_info __stdcall CFunctionPluginPs1Script::start(HWND hwnd, const vector<const WCHAR*>& file_list)
{
	const bool bScript(CheckFile(m_script));
	if (!bScript)
	{
		const CString path(L".");
		WCHAR abs_path[MAX_PATH];
		memset(abs_path, 0, sizeof(abs_path));
		if (_wfullpath(abs_path, path, MAX_PATH - 1) == NULL)
			wcsncpy_s(abs_path, MAX_PATH, path, MAX_PATH - 1);

		CString msg, fmt;
		fmt.LoadString(IDS_ERROR_SCRIPT_MISSING);
		msg.FormatMessage(fmt, m_script, abs_path);

		AfxMessageBox(msg);
	}

	return request_info(bScript ? PICTURE_LAYOUT_FILE_NAME_ONLY : PICTURE_LAYOUT_CANCEL_REQUEST);
}

bool __stdcall CFunctionPluginPs1Script::process_picture(const picture_data& _picture_data)
{
	picture_data_list.push_back(_picture_data);

	// Signal that the picture could be updated.
	// This info will be submitted in the 'end' event.
	m_update_info.push_back(update_info(_picture_data.m_name, UPDATE_TYPE_UPDATED));

	// Return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_info>& __stdcall CFunctionPluginPs1Script::end()
{
	//# plugin variables
	//# displays a console, use this option for scripts with text output
	//# Do not remove the # on the following line:
	//#[console=false]
	// 
	//# noexit keeps the powershell console open, remove this option to have the console closed when processing is done
	//# Do not remove the # on the following line :
	//#[noexit=false]

	bool console(true);
	bool noexit(false);

	char* consoleTxtANSI = "#[console=";
	char* noexitTxtANSI = "#[noexit=";
	char* trueTxtANSI = "true]";
	WCHAR* consoleTxtUnicode = L"#[console=";
	WCHAR* noexitTxtUnicode = L"#[noexit=";
	WCHAR* trueTxtUnicode = L"true]";

	// Read the first 1024 chars to get the console and noexit flags.
	const int textSize(1024);
	unsigned char Text[textSize];

	FILE* infile = NULL;
	const errno_t err(_wfopen_s(&infile, m_script, L"rb"));

	if (err == 0)
	{
		const int size(min(textSize, _filelength(_fileno(infile))));
		fread((char*)Text, sizeof(char), size, infile);

		console = scanVar((char*)Text, consoleTxtANSI, trueTxtANSI, consoleTxtUnicode, trueTxtUnicode, true);
		noexit = scanVar((char*)Text, noexitTxtANSI, trueTxtANSI, noexitTxtUnicode, trueTxtUnicode, false);

		fclose(infile);
	}

	CString script(L"-ExecutionPolicy Unrestricted ");

	if (noexit)
		script += L"-noexit ";	// -noexit keeps the powershell console open

	script += L"\".\\" + m_script + L" ";

	// Add picture data as json.

	//[
	//	{
	//		"name": "c:\\Bilder\bild1.jpg",
	//		"file": "bild1.jpg",
	//		"dir": "c:\\Bilder\\",
	//		"width": 1200,
	//		"height": 800
	//	},
	//	{
	//		"name": "c:\\Bilder\bild2.jpg",
	//		"file": "bild2.jpg",
	//		"dir": "c:\\Bilder\\",
	//		"width": 1024,
	//		"height": 768
	//	}
	//]

	// Begin of array.
	CString json(L"[");

	for (vector<picture_data>::const_iterator it = picture_data_list.begin(); it != picture_data_list.end(); ++it)
	{
		CString name(it->m_name);
		name.Replace(L"\\", L"\\\\");

		const int f(it->m_name.ReverseFind(L'\\') + 1);
		const CString file(it->m_name.Mid(f));
		CString dir(escapeJsonData(it->m_name.Left(f)));
		dir += L"\\\\";	// Escape the trailing \ of the dir.

		CString aperture;
		if (it->m_fAperture)
			aperture.Format(L"%.1f", it->m_fAperture);
		else
			aperture = L"0";

		CString exiftime;
		exiftime.Format(L"%llu", it->m_exiftime);

		// L"\"\"\"", Escape the quotes in a quoted string.
		CString cmd_format(L"{\"\"\"name\"\"\":\"\"\"%1\"\"\",\"\"\"file\"\"\":\"\"\"%2\"\"\",\"\"\"dir\"\"\":\"\"\"%3\"\"\",\"\"\"width\"\"\":%4!d!,\"\"\"height\"\"\":%5!d!,\"\"\"errormsg\"\"\":\"\"\"%6\"\"\",\"\"\"audio\"\"\":%7,\"\"\"video\"\"\":%8,\"\"\"colorprofile\"\"\":%9,\"\"\"gps\"\"\":\"\"\"%10\"\"\",\"\"\"aperture\"\"\":%11,\"\"\"shutterspeed\"\"\":%12!d!,\"\"\"iso\"\"\":%13!d!,\"\"\"exifdate\"\"\":%14,\"\"\"exifdate_str\"\"\":\"\"\"%15\"\"\",\"\"\"model\"\"\":\"\"\"%16\"\"\",\"\"\"lens\"\"\":\"\"\"%17\"\"\",\"\"\"cdata\"\"\":\"\"\"%18\"\"\"}");

		CString cmd;
		cmd.FormatMessage(cmd_format,
			name,
			file,
			dir,
			it->m_OriginalPictureWidth,
			it->m_OriginalPictureHeight,
			it->m_ErrorMsg,
			it->m_bAudio ? L"true" : L"false",
			it->m_bVideo ? L"true" : L"false",
			it->m_bColorProfile ? L"true" : L"false",
			escapeJsonData(it->m_GPSdata),
			aperture,
			it->m_Shutterspeed,
			it->m_ISO,
			exiftime,
			it->m_ExifDateTime_display,
			escapeJsonData(it->m_Model),
			escapeJsonData(it->m_Lens),
			escapeJsonData(it->m_cdata)
		);

		// No trailing separator for the last array element.
		if (it != picture_data_list.end() - 1)
		{
			cmd += L",";
		}

		json += cmd;
	}

	// End of array.
	json += L"]";

	script += L"-picture_data_json '";
	script += json;
	script += L"'\"";

	SHELLEXECUTEINFO shInfo;
	memset(&shInfo, 0, sizeof(shInfo));
	shInfo.cbSize = sizeof(shInfo);

	shInfo.lpFile = m_PowerShellExe;
	shInfo.lpParameters = script;
	shInfo.nShow = console ? SW_SHOWNORMAL : SW_HIDE;
	shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;

	ShellExecuteEx(&shInfo);
	WaitForSingleObject(shInfo.hProcess, INFINITE);

	return m_update_info;
}
