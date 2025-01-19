#include "resource.h"		// main symbols
#include "global.h"
#include "plugin.h"


// PowerShell script plugin cpp_script.
// Runs a pswh script for the selected pictures.


CString scanPS1Description(char* Text)
{
	//<#
	//.DESCRIPTION
	//    Example script to print the picture data.
	//.NOTES
	//    notes
	//#>

	static std::wregex& descriptionRegex = std::wregex(L"<#.+?[.]DESCRIPTION(?:\\s|\\\\n)+(.+?)(?:\\s|\\\\n)+(?:[.][a-z]{4,}(?:\\s|\\\\n)*|#>)", std::regex::icase);
	
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

	// std::regex multiline
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


CFunctionPluginPS1Script::CFunctionPluginPS1Script(const script_info script_info)
	//: m_PowerShellExe(L"c:\\windows\\system32\\windowspowershell\\v1.0\\powershell.exe "),
	: m_PowerShellExe(L"pwsh.exe "),
	handle_wnd(NULL),
	m_script_info(script_info)
{
	// Set the locale to "C" to ensure the decimal point is a period.
	_wsetlocale(LC_NUMERIC, L"C");
}

struct plugin_data __stdcall CFunctionPluginPS1Script::get_plugin_data() const
{
	struct plugin_data pluginData;

	// Set plugin info.
	pluginData.file_name = m_script_info.script;
	pluginData.name.FormatMessage(IDS_SCRIPT_PS1_NAME, m_script_info.script.Left(m_script_info.script.ReverseFind(L'.')));
	pluginData.desc = m_script_info.desc.IsEmpty() ? pluginData.name : m_script_info.desc;

	return pluginData;
}

struct arg_count __stdcall CFunctionPluginPS1Script::get_arg_count() const
{
	// At least one picture.
	return arg_count(1, -1);
}

enum REQUEST_TYPE __stdcall CFunctionPluginPS1Script::start(const HWND hwnd, const vector<const WCHAR*>& file_list, vector<request_data_size>& request_data_sizes)
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

bool __stdcall CFunctionPluginPS1Script::process_picture(const picture_data& picture_data)
{
	// Signal that the picture could be updated.
	// This info will be submitted in the 'end' event.
	update_data_list.push_back(update_data(picture_data.file_name, UPDATE_TYPE::UPDATE_TYPE_UPDATED));

	// Return true to load the next picture, return false to stop with this picture and continue to the 'end' event.
	return true;
}

const vector<update_data>& __stdcall CFunctionPluginPS1Script::end(const vector<picture_data>& picture_data_list)
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

	const BOOL ret = ShellExecuteEx(&shInfo);

#ifdef DEBUG
	CString errorMsg = GetLastErrorStr();
#endif

	WaitForSingleObject(shInfo.hProcess, INFINITE);

	// Return list of pictures that are updated, added or deleted (enum UPDATE_TYPE).
	return update_data_list;
}
