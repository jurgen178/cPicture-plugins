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


typedef CString(*ScanDescriptionFunc)(char*, std::wregex&);


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

	// Common lambda for ps1 and py scripts.
	auto get_ps1_py_script_info = [&](ScanDescriptionFunc fp, std::wregex descriptionRegex) -> void
		{
			// Read the first 4096 chars to get the description text.
			const int textSize(4096);
			char Text[textSize] = { 0 };

			FILE* infile = NULL;
			const errno_t err(_wfopen_s(&infile, c_file.cFileName, L"rb"));

			if (err == 0)
			{
				const int size(min(textSize, _filelength(_fileno(infile))));
				fread(Text, sizeof(char), size, infile);

				desc = fp(Text, descriptionRegex);

				fclose(infile);
			}
		};

	// Script info for ps1 files.
	auto get_ps1_script_info = [&]() -> void
		{
			get_ps1_py_script_info(scanPS1Description, GetPS1DescriptionRegex());
		};

	// Script info for python files.
	auto get_py_script_info = [&]() -> void
		{
			get_ps1_py_script_info(scanPyDescription, GetPyDescriptionRegex());
		};

	auto register_filetyp = [&](
		const CString& script_mask,
		const int maxscripts,
		vector<script_info>& scripts,
		lpfnFunctionGetInstanceProc* getInstanceList,
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
				} while (i < maxscripts && FindNextFile(hFile, &c_file) != 0);

				FindClose(hFile);
			}
		};

	const int maxscriptsBat(sizeof(GetInstanceBatList) / sizeof(lpfnFunctionGetInstanceProc));
	register_filetyp(L"*.bat", maxscriptsBat, ScriptsBat, GetInstanceBatList, get_bat_script_info);

	const int maxscriptsPS1(sizeof(GetInstancePS1List) / sizeof(lpfnFunctionGetInstanceProc));
	register_filetyp(L"*.ps1", maxscriptsPS1, ScriptsPS1, GetInstancePS1List, get_ps1_script_info);

	const int maxscriptsPy(sizeof(GetInstancePyList) / sizeof(lpfnFunctionGetInstanceProc));
	register_filetyp(L"*.py", maxscriptsPy, ScriptsPy, GetInstancePyList, get_py_script_info);


	//// Register all *.bat scripts.
	//const CString ScriptBatMask(L"*.bat");
	//if ((hFile = FindFirstFile(ScriptBatMask, &c_file)) != INVALID_HANDLE_VALUE)
	//{
	//	int i = 0;

	//	do
	//	{
	//		if (!(c_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(c_file.cFileName, L".") && wcscmp(c_file.cFileName, L".."))
	//		{
	//			const CString script(c_file.cFileName);
	//			ScriptsBat.push_back(script);
	//			PluginProcArray.push_back(GetInstanceBatList[i++]);
	//		}
	//	}
	//	while (i < maxscripts && FindNextFile(hFile, &c_file) != 0);

	//	FindClose(hFile);
	//}

	//// Register all *.ps1 scripts.
	//const CString ScriptPS1Mask(L"*.ps1");
	//if ((hFile = FindFirstFile(ScriptPS1Mask, &c_file)) != INVALID_HANDLE_VALUE)
	//{
	//	int i = 0;
	//	std::wregex descriptionRegex(GetPS1DescriptionRegex());

	//	do
	//	{
	//		if (!(c_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(c_file.cFileName, L".") && wcscmp(c_file.cFileName, L".."))
	//		{
	//			// Read the first 4096 chars to get the description text.
	//			const int textSize(4096);
	//			char Text[textSize] = { 0 };

	//			FILE* infile = NULL;
	//			const errno_t err(_wfopen_s(&infile, c_file.cFileName, L"rb"));

	//			CString desc;
	//			if (err == 0)
	//			{
	//				const int size(min(textSize, _filelength(_fileno(infile))));
	//				fread(Text, sizeof(char), size, infile);

	//				desc = scanPS1Description(Text, descriptionRegex);

	//				fclose(infile);
	//			}

	//			const CString script(c_file.cFileName);
	//			ScriptsPS1.push_back(script_info(script, desc));
	//			PluginProcArray.push_back(GetInstancePS1List[i++]);
	//		}
	//	}
	//	while (i < maxscripts && FindNextFile(hFile, &c_file) != 0);

	//	FindClose(hFile);
	//}

	//// Register all *.py scripts.
	//const CString ScriptPyMask(L"*.py");
	//if ((hFile = FindFirstFile(ScriptPyMask, &c_file)) != INVALID_HANDLE_VALUE)
	//{
	//	int i = 0;
	//	std::wregex descriptionRegex(GetPyDescriptionRegex());

	//	do
	//	{
	//		if (!(c_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(c_file.cFileName, L".") && wcscmp(c_file.cFileName, L".."))
	//		{
	//			// Read the first 4096 chars to get the description text.
	//			const int textSize(4096);
	//			char Text[textSize] = { 0 };

	//			FILE* infile = NULL;
	//			const errno_t err(_wfopen_s(&infile, c_file.cFileName, L"rb"));

	//			CString desc;
	//			if (err == 0)
	//			{
	//				const int size(min(textSize, _filelength(_fileno(infile))));
	//				fread(Text, sizeof(char), size, infile);

	//				desc = scanPyDescription(Text, descriptionRegex);

	//				fclose(infile);
	//			}

	//			const CString script(c_file.cFileName);
	//			ScriptsPy.push_back(script_info(script, desc));
	//			PluginProcArray.push_back(GetInstancePyList[i++]);
	//		}
	//	}
	//	while (i < maxscripts && FindNextFile(hFile, &c_file) != 0);

	//	FindClose(hFile);
	//}

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
