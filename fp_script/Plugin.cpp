#include "resource.h"		// main symbols
#include "global.h"
#include "plugin.h"


// Script plugin cpp_script.
// Runs a script for the selected pictures.


static vector<lpfnFunctionGetInstanceProc> PluginProcArray;
static vector<script_info> ScriptsBat;
static vector<script_info> ScriptsPS1;
static vector<script_info> ScriptsPy;


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

#include "GetInstances_bat.h"
#include "GetInstances_ps1.h"
#include "GetInstances_py.h"
#include <functional>


typedef CString(*ScanDescriptionFunc)(char*);

// Template function takes an array by reference and returns its size.
// The size N is deduced at compile time.
template <typename T, std::size_t N>
constexpr std::size_t max_elements(T(&)[N])
{
	return N;
}


// Each .ps1 file will be set as a plugin.
const int __stdcall GetPluginInit()
{
	ScriptsBat.clear();
	ScriptsPS1.clear();
	ScriptsPy.clear();

	PluginProcArray.clear();

	CString desc;
	WIN32_FIND_DATA c_file;
	HANDLE hFile;

	// Script info for batch files.
	auto get_bat_script_info = [&]() -> void
		{
		};

	// For both ps1 and py scripts.
	auto get_ps1_py_script_info = [&](ScanDescriptionFunc scan_description) -> void
		{
			// Read the first 4096 chars to get the description text.
			const int textSize(4096);
			char Text[textSize] = { 0 };
			desc = L"";

			FILE* infile = NULL;
			const errno_t err(_wfopen_s(&infile, c_file.cFileName, L"rb"));

			if (err == 0)
			{
				const int size(min(textSize, _filelength(_fileno(infile))));
				fread(Text, sizeof(char), size, infile);

				desc = scan_description(Text);

				fclose(infile);
			}
		};

	// Script info for ps1 files.
	auto get_ps1_script_info = [&]() -> void
		{
			get_ps1_py_script_info(scanPS1Description);
		};

	// Script info for python files.
	auto get_py_script_info = [&]() -> void
		{
			get_ps1_py_script_info(scanPyDescription);
		};

	auto register_filetyp = [&](
		const CString& script_mask,
		const size_t maxscripts,
		lpfnFunctionGetInstanceProc* getInstanceList,
		vector<script_info>& scripts,
		std::function<void()> get_script_info
		)
		-> void
		{
			if ((hFile = FindFirstFile(script_mask, &c_file)) != INVALID_HANDLE_VALUE)
			{
				int i = 0;

				do
				{
					if (!(c_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(c_file.cFileName, L".") && wcscmp(c_file.cFileName, L".."))
					{
						get_script_info();

						const CString script(c_file.cFileName);
						scripts.push_back(script_info(script, desc));
						PluginProcArray.push_back(getInstanceList[i++]);
					}
				}
				while (i < maxscripts && FindNextFile(hFile, &c_file) != 0);

				FindClose(hFile);
			}
		};


	register_filetyp(L"*.bat", max_elements(GetInstanceBatList), GetInstanceBatList, ScriptsBat, get_bat_script_info);
	register_filetyp(L"*.ps1", max_elements(GetInstancePS1List), GetInstancePS1List, ScriptsPS1, get_ps1_script_info);
	register_filetyp(L"*.py", max_elements(GetInstancePyList), GetInstancePyList, ScriptsPy, get_py_script_info);

	// Anzahl als Negativwert wenn externe Moduldateien (*.bat, *.ps1, *.py) verwendet werden.
	return -static_cast<int>(PluginProcArray.size());
}

lpfnFunctionGetInstanceProc __stdcall GetPluginProc(const int k)
{
	if (k >= 0 && k < PluginProcArray.size())
		return PluginProcArray[k];
	else
		return NULL;
}
