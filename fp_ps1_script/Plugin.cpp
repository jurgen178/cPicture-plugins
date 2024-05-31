#include "resource.h"		// main symbols
#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include <io.h>

#include <regex>
#include <vector>
using namespace std;

// PowerShell script plugin cpp_ps1_script.
// Runs a pswh script for the selected pictures.


static vector<lpfnFunctionGetInstanceProc> PluginProcArray;
static vector<script_info> Scripts;


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

std::wregex GetDescriptionRegex();
CString scanDescription(char* Text, std::wregex&);

const int maxscripts(sizeof(GetInstanceList) / sizeof(lpfnFunctionGetInstanceProc));

// Each .ps1 file will be set as a plugin.
const int __stdcall GetPluginInit()
{
	Scripts.clear();
	PluginProcArray.clear();

	WIN32_FIND_DATA c_file;
	HANDLE hFile;

	// Register all *.ps1 scripts.
	const CString ScriptMask(L"*.ps1");
	if ((hFile = FindFirstFile(ScriptMask, &c_file)) != INVALID_HANDLE_VALUE)
	{
		int i = 0;
		std::wregex descriptionRegex(GetDescriptionRegex());

		do
		{
			if (!(c_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(c_file.cFileName, L".") && wcscmp(c_file.cFileName, L".."))
			{
				// Read the first 4096 chars to get the description text.
				const int textSize(4096);
				char Text[textSize] = { 0 };

				FILE* infile = NULL;
				const errno_t err(_wfopen_s(&infile, c_file.cFileName, L"rb"));

				CString desc;
				if (err == 0)
				{
					const int size(min(textSize, _filelength(_fileno(infile))));
					fread(Text, sizeof(char), size, infile);

					desc = scanDescription(Text, descriptionRegex);

					fclose(infile);
				}

				const CString script(c_file.cFileName);
				Scripts.push_back(script_info(script, desc));
				PluginProcArray.push_back(GetInstanceList[i++]);
			}
		} 
		while (i < maxscripts && FindNextFile(hFile, &c_file) != 0);

		FindClose(hFile);
	}

	// Anzahl als Negativwert wenn externe Moduldateien (*.bat, *.ps1) verwendet werden.
	return -(int)PluginProcArray.size();
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int k)
{
	if (k >= 0 && k < PluginProcArray.size())
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

bool scanBoolVar(char* Text, const CString SearchTextTemplate, bool def)
{
	CString ScanText(Text);

	// Check if it is from a Unicode file (UCS-2, not UTF-8).
	// Only the Byte Order Mask 'ÿþ' and the first char '<' are readable in ANSI: FF FE 3C 00
	if (ScanText.GetLength() <= 3)
	{
		// Reload text as Unicode Text (UCS-2).
		ScanText = (WCHAR*)Text;
	}

	ScanText.Replace(L" ", L"");
	ScanText.Replace(L"\r", L"");

	CString SearchText;

	SearchText.Format(SearchTextTemplate, L"true");
	if (ScanText.Find(SearchText) != -1)
	{
		return true;
	}

	SearchText.Format(SearchTextTemplate, L"false");
	if (ScanText.Find(SearchText) != -1)
	{
		return false;
	}

	return def;
}

std::wregex GetDescriptionRegex()
{
	//<#
	//.DESCRIPTION
	//    Example script to print the picture data.
	//.NOTES
	//    notes
	//#>

	return std::wregex(L"<#.+?[.]DESCRIPTION(?:\\s|\\\\n)+(.+?)(?:\\s|\\\\n)+(?:[.][a-z]{4,}(?:\\s|\\\\n)*|#>)", std::regex::icase);
}

CString scanDescription(char* Text, std::wregex& descriptionRegex)
{
	CString ScanText(Text);

	// Check if it is from a Unicode file (UCS-2, not UTF-8).
	// Only the Byte Order Mask 'ÿþ' and the first char '<' are readable in ANSI: FF FE 3C 00
	if (ScanText.GetLength() <= 3)
	{
		// Reload text as Unicode Text (UCS-2).
		ScanText = (WCHAR*)Text;
	}

	// Simplify the line break.
	ScanText.Replace(L"\r", L"");

	// std:regex multiline
	ScanText.Replace(L"\n", L"\\n");

	//<#
	//.DESCRIPTION
	//    Example script to print the picture data.
	//.NOTES
	//    notes
	//#>

	std::wstring input(ScanText);
	std::wsmatch match;

	if (std::regex_search(input, match, descriptionRegex))
	{
		std::wstring r = match[1];
		CString m(r.c_str());
		m.Replace(L"\\n", L"\n");
		m.Trim(L" \n\t");

		return m;
	}

	return L"";
}

CString escapeCmdLineJsonData(CString text)
{
	// [{"key'1","value"a","key2","value\b\"}]

	text.Replace(L"'", L"''");

	// [{"key''1","value"a","key2","value\b\"}]

	// embedded json data?
	CString textNoSpaces(text);
	textNoSpaces.Replace(L" ", L"");

	if (textNoSpaces.Left(2) == L"[{" && textNoSpaces.Right(2) == L"}]")
	{
		// Use double group replacement to replace delimiting quotes and inside quotes.

		// [{"key''1","value"a","key2","value\b\"}]
		text.Replace(L"\\", L"\\\\\\\\");	// \ -> \\\\ 

		// [{"key''1","value"a","key2","value\\\\b\\\\"}]
		
		// Escape delimiter quotes using a uncommon letter ⱦ.
		text.Replace(L"{\"", L"{\\\\ⱦⱦ");	// " -> \\""
		text.Replace(L":\"", L":\\\\ⱦⱦ");
		text.Replace(L"\":", L"\\\\ⱦⱦ:");
		text.Replace(L",\"", L",\\\\ⱦⱦ");
		text.Replace(L"\",", L"\\\\ⱦⱦ,");
		text.Replace(L"\"}", L"\\\\ⱦⱦ}");

		// [{\\ⱦⱦkey''1\\ⱦⱦ,\\ⱦⱦvalue"a\\ⱦⱦ,\\ⱦⱦkey2\\ⱦⱦ,\\ⱦⱦvalue\\\\b\\\\\\ⱦⱦ}]

		// All remaining quotes are now quotes inside of a quoted string.
		// Now escape " inside quoted string -> \\\\\\""
		text.Replace(L"\"", L"\\\\\\\\\\\\\"\"");	// " -> \\\\\\""

		// [{\\ⱦⱦkey''1\\ⱦⱦ,\\ⱦⱦvalue\\\\\\""a\\ⱦⱦ,\\ⱦⱦkey2\\ⱦⱦ,\\ⱦⱦvalue\\\\b\\\\\\ⱦⱦ}]

		// Replace uncommon letter back to escaped quotes.
		text.Replace(L"{\\\\ⱦⱦ", L"{\\\\\"\"");
		text.Replace(L":\\\\ⱦⱦ", L":\\\\\"\"");
		text.Replace(L"\\\\ⱦⱦ:", L"\\\\\"\":");
		text.Replace(L",\\\\ⱦⱦ", L",\\\\\"\"");
		text.Replace(L"\\\\ⱦⱦ,", L"\\\\\"\",");
		text.Replace(L"\\\\ⱦⱦ}", L"\\\\\"\"}");

		// [{\\""key''1\\"",\\""value\\\\\\""a\\"",\\""key2\\"",\\""value\\\\b\\\\\\""}]

		// trailing \ : "text\"
		// Add double \ escaped \\ to avoid escaping the following quote when text ends with a \ and followed by a ".
		text.Replace(L"\\\\\\\\\\\\\"\",", L"\\\\\\\\\\\\\\\\\\\\\"\",");	// \\\\ \\""  -  \\\\ \\\\ \\"" 
		text.Replace(L"\\\\\\\\\\\\\"\"}", L"\\\\\\\\\\\\\\\\\\\\\"\"}");

		// [{\\""key''1\\"",\\""value\\\\\\""a\\"",\\""key2\\"",\\""value\\\\b\\\\\\\\\\""}]
	}
	else
	{
		// ab"c'1\ 

		text.Replace(L"\\", L"\\\\");	// \ -> \\ 
		text.Replace(L"\"", L"\\\\\"\"");	// " -> ""

		// Add double \ to avoid escaping the following quote when text ends with a \.
		if (text.Right(1) == L"\\")
			text += L"\\\\";

		// ab\\""c''\\1\\\\ 
	}

	return text;
}

CString escapeCmdLineJsonData(bool expr)
{
	return expr ? L"true" : L"false";
}

CString escapeCmdLineJsonData(float value)
{
	CString data;
	if (value)
		data.Format(L"%.1f", value);
	else
		data = L"0";

	return data;
}

CString escapeCmdLineJsonData(__int64 value)
{
	CString data;
	data.Format(L"%llu", value);

	return data;
}


CFunctionPluginScript::CFunctionPluginScript(const script_info script_info)
	//: m_PowerShellExe(L"c:\\windows\\system32\\windowspowershell\\v1.0\\powershell.exe "),
	: m_PowerShellExe(L"pwsh.exe "),
	handle_wnd(NULL),
	m_script_info(script_info)
{
	// Do not set locale to keep decimal point (LC_NUMERIC) for PowerShell.
	//_wsetlocale(LC_ALL, L".ACP");
}

struct PluginData __stdcall CFunctionPluginScript::get_plugin_data()
{
	struct PluginData pluginData;

	// Set plugin info.
	pluginData.file_name = m_script_info.script;
	pluginData.name.FormatMessage(IDS_SCRIPT_NAME, m_script_info.script.Left(m_script_info.script.ReverseFind(L'.')));
	pluginData.desc = m_script_info.desc.IsEmpty() ? pluginData.name : m_script_info.desc;

	return pluginData;
}


enum REQUEST_TYPE __stdcall CFunctionPluginScript::start(HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
{
	handle_wnd = hwnd;

	const bool bScript(CheckFile(m_script_info.script));
	if (!bScript)
	{
		const CString path(L".");
		WCHAR abs_path[MAX_PATH] = { 0 };

		if (_wfullpath(abs_path, path, MAX_PATH - 1) == NULL)
			wcsncpy_s(abs_path, MAX_PATH, path, MAX_PATH - 1);

		CString msg;
		msg.FormatMessage(IDS_ERROR_SCRIPT_MISSING, m_script_info.script, abs_path);

		::MessageBox(handle_wnd, msg, get_plugin_data().name, MB_ICONEXCLAMATION);
	}

	return bScript ? REQUEST_TYPE::REQUEST_TYPE_DATA : REQUEST_TYPE::REQUEST_TYPE_CANCEL;
}

bool __stdcall CFunctionPluginScript::process_picture(const picture_data& picture_data)
{
	// Signal that the picture could be updated.
	// This info will be submitted in the 'end' event.
	update_data_list.push_back(update_data(picture_data.file_name, UPDATE_TYPE::UPDATE_TYPE_UPDATED));

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginScript::end(const vector<picture_data>& picture_data_list)
{
	//# plugin variables
	//
	//# console=true displays a console, use this option for scripts with text output.
	//# Do not remove the # on the following line:
	//#[console=true]
	//
	//# noexit=true keeps the console open, set to 'false' to have the console closed when processing is done.
	//# Do not remove the # on the following line:
	//#[noexit=false]


	bool console(true);
	bool noexit(false);

	// Variable needs to be at the beginning of the line.
	const CString consoleSearchTextTemplate("\n#[console=%s]");
	const CString noexitSearchTextTemplate("\n#[noexit=%s]");

	// Read the first 4096 chars to get the console and noexit variables.
	const int textSize(4096);
	char Text[textSize] = { 0 };

	FILE* infile = NULL;
	const errno_t err(_wfopen_s(&infile, m_script_info.script, L"rb"));

	if (err == 0)
	{
		const int size(min(textSize, _filelength(_fileno(infile))));
		fread(Text, sizeof(char), size, infile);

		console = scanBoolVar(Text, consoleSearchTextTemplate, true);

		// console needs to be set to use noexit option.
		if (console)
			noexit = scanBoolVar(Text, noexitSearchTextTemplate, false);

		fclose(infile);
	}

	CString script(L"-ExecutionPolicy Unrestricted ");

	if (noexit)
		script += L"-noexit ";	// -noexit keeps the powershell console open

	script += L"-Command \".\\" + m_script_info.script + L" ";

	// Add picture data as json.

	//[
	//	{
	//		"name": "C:\\Bilder\\Pike-Place-Market-Kreuzung-360x180.jpg",
	//		"file" : "Pike-Place-Market-Kreuzung-360x180.jpg",
	//		"dir" : "C:\\Bilder\\",
	//		"width" : 1624,
	//		"height" : 812,
	//		"errormsg" : "",
	//		"audio" : false,
	//		"video" : false,
	//		"colorprofile" : true,
	//		"gps" : "N 47° 37' 0,872498\" W 122° 19' 32,021484\"",
	//		"aperture" : 0,
	//		"shutterspeed" : 1250,
	//		"iso" : 100,
	//		"exifdate" : 133553225690000000,
	//		"exifdate_str" : "19.03.2024 11:49:29",
	//		"model" : "[NIKON Z 30] ",
	//		"lens" : ""
	//	},
	//	{
	//		"name": "C:\\Bilder\\DSC_4409.JPG",
	//		"file" : "DSC_4409.JPG",
	//		"dir" : "C:\\Bilder\\",
	//		"width" : 5568,
	//		"height" : 3712,
	//		"errormsg" : "",
	//		"audio" : false,
	//		"video" : false,
	//		"colorprofile" : true,
	//		"gps" : "",
	//		"aperture" : 4.8,
	//		"shutterspeed" : 60,
	//		"iso" : 100,
	//		"exifdate" : 133563690240000000,
	//		"exifdate_str" : "31.03.2024 14:30:24",
	//		"model" : "NIKON Z 30 ",
	//		"lens" : "16-50mm f/3,5-6,3 VR f=30mm/45mm",
	//		"cdata" : "[{\"name\":\"DSC_4409.JPG\",\"dir\":\"C:\\\\Bilder\\\\\",\"model\":\"NIKON Z 30 \",\"settings\":\"f/4,8 ISO 100/21° 16-50mm f/3,5-6,3 VR f=30mm/45mm\",\"contains\":\"XMP, Farbprofil, \",\"gps\":\",\"file_size\":\"9,3 MB (9755433 Bytes)\",\"file_create_date\":\"Sonntag, 31. März 2024 um 14:30:24 Uhr\",\"file_modified_date\":\"Sonntag, 31. März 2024 um 14:56:16 Uhr\",\"exif_date\":\"Sonntag, 31. März 2024 um 14:30:24 Uhr\"}]"
	//	}
	//]


	// Begin of array.
	CString json(L"[");

	for (vector<picture_data>::const_iterator it = picture_data_list.begin(); it != picture_data_list.end(); ++it)
	{
		// \"\" escapes the quotes in a quoted string.
		CString cmd_format(L"{\"\"file\"\":\"\"%1\"\",\"\"name\"\":\"\"%2\"\",\"\"dir\"\":\"\"%3\"\",\"\"width\"\":%4!d!,\"\"height\"\":%5!d!,\"\"errormsg\"\":\"\"%6\"\",\"\"audio\"\":%7,\"\"video\"\":%8,\"\"colorprofile\"\":%9,\"\"gps\"\":\"\"%10\"\",\"\"aperture\"\":%11,\"\"shutterspeed\"\":%12!d!,\"\"iso\"\":%13!d!,\"\"exifdate\"\":%14,\"\"exifdate_str\"\":\"\"%15\"\",\"\"model\"\":\"\"%16\"\",\"\"lens\"\":\"\"%17\"\",\"\"cdata\"\":\"\"%18\"\"}");
		const int f(it->file_name.ReverseFind(L'\\') + 1);
		const CString name(it->file_name.Mid(f));
		const CString dir(it->file_name.Left(f));

		CString cmd;
		cmd.FormatMessage(cmd_format,
			escapeCmdLineJsonData(it->file_name),
			escapeCmdLineJsonData(name),
			escapeCmdLineJsonData(dir),
			it->picture_width,
			it->picture_height,
			escapeCmdLineJsonData(it->error_msg),
			escapeCmdLineJsonData(it->audio),
			escapeCmdLineJsonData(it->video),
			escapeCmdLineJsonData(it->color_profile),
			escapeCmdLineJsonData(it->gps),
			escapeCmdLineJsonData(it->aperture),
			it->shutterspeed,
			it->iso,
			escapeCmdLineJsonData(it->exif_time),
			it->exif_datetime_display,
			escapeCmdLineJsonData(it->model),
			escapeCmdLineJsonData(it->lens),
			escapeCmdLineJsonData(it->cdata)
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

	shInfo.hwnd = handle_wnd;
	shInfo.lpFile = m_PowerShellExe;
	shInfo.lpParameters = script;
	shInfo.nShow = console ? SW_SHOWNORMAL : SW_HIDE;
	//shInfo.fMask = SEE_MASK_NOCLOSEPROCESS;

	ShellExecuteEx(&shInfo);
	WaitForSingleObject(shInfo.hProcess, INFINITE);

	// Return list of pictures that are updated, added or deleted (enum UPDATE_TYPE).
	return update_data_list;
}
