#include "resource.h"		// main symbols
#include "stdafx.h"
#include "plugin.h"
#include "locale.h"
#include <io.h>

#include <vector>
using namespace std;

// PowerShell script plugin cpp_ps1_script.

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
		// No regex replace available in MFC. Use double group replacement to replace delimiting quotes and inside quotes.

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


CFunctionPluginPs1Script::CFunctionPluginPs1Script(const CString& script)
	//: m_PowerShellExe(L"c:\\windows\\system32\\windowspowershell\\v1.0\\powershell.exe "),
	: m_PowerShellExe(L"pwsh.exe "),
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
		WCHAR abs_path[MAX_PATH] = { 0 };

		if (_wfullpath(abs_path, path, MAX_PATH - 1) == NULL)
			wcsncpy_s(abs_path, MAX_PATH, path, MAX_PATH - 1);

		CString msg;
		msg.FormatMessage(IDS_ERROR_SCRIPT_MISSING, m_script, abs_path);

		AfxMessageBox(msg);
	}

	return request_info(bScript ? PICTURE_REQUEST_INFO_FILE_NAME_ONLY : PICTURE_REQUEST_INFO_CANCEL_REQUEST);
}

bool __stdcall CFunctionPluginPs1Script::process_picture(const picture_data& picture_data)
{
	picture_data_list.push_back(picture_data);

	// Signal that the picture could be updated.
	// This info will be submitted in the 'end' event.
	m_update_info.push_back(update_info(picture_data.m_FileName, UPDATE_TYPE_UPDATED));

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_info>& __stdcall CFunctionPluginPs1Script::end()
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

	// Read the first 1024 chars to get the console and noexit flags.
	const int textSize(1024);
	char Text[textSize] = { 0 };

	FILE* infile = NULL;
	const errno_t err(_wfopen_s(&infile, m_script, L"rb"));

	if (err == 0)
	{
		const int size(min(textSize, _filelength(_fileno(infile))));
		fread(Text, sizeof(char), size, infile);

		console = scanBoolVar(Text, consoleSearchTextTemplate, true);

		// Allow noexit option only when console is displayed.
		if (console)
			noexit = scanBoolVar(Text, noexitSearchTextTemplate, false);

		fclose(infile);
	}

	CString script(L"-ExecutionPolicy Unrestricted ");

	if (noexit)
		script += L"-noexit ";	// -noexit keeps the powershell console open

	script += L"-Command \".\\" + m_script + L" ";

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
		const int f(it->m_FileName.ReverseFind(L'\\') + 1);
		const CString name(it->m_FileName.Mid(f));
		const CString dir(it->m_FileName.Left(f));

		CString cmd;
		cmd.FormatMessage(cmd_format,
			escapeCmdLineJsonData(it->m_FileName),
			escapeCmdLineJsonData(name),
			escapeCmdLineJsonData(dir),
			it->m_OriginalPictureWidth,
			it->m_OriginalPictureHeight,
			escapeCmdLineJsonData(it->m_ErrorMsg),
			escapeCmdLineJsonData(it->m_bAudio),
			escapeCmdLineJsonData(it->m_bVideo),
			escapeCmdLineJsonData(it->m_bColorProfile),
			escapeCmdLineJsonData(it->m_GPSdata),
			escapeCmdLineJsonData(it->m_fAperture),
			it->m_Shutterspeed,
			it->m_ISO,
			escapeCmdLineJsonData(it->m_exiftime),
			it->m_ExifDateTime_display,
			escapeCmdLineJsonData(it->m_Model),
			escapeCmdLineJsonData(it->m_Lens),
			escapeCmdLineJsonData(it->m_cdata)
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
