#include "resource.h"		// main symbols
#include "global.h"
#include "plugin.h"


// Script plugin cpp_script.
// Runs a script for the selected pictures.


static vector<lpfnFunctionGetInstanceProc> PluginProcArray;
static vector<CString> ScriptsBat;
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


const int maxscripts(sizeof(GetInstanceBatList) / sizeof(lpfnFunctionGetInstanceProc));

// Each .ps1 file will be set as a plugin.
const int __stdcall GetPluginInit()
{
	ScriptsBat.clear();
	ScriptsPS1.clear();
	ScriptsPy.clear();
	PluginProcArray.clear();

	WIN32_FIND_DATA c_file;
	HANDLE hFile;

	// Register all *.bat scripts.
	const CString ScriptBatMask(L"*.bat");
	if ((hFile = FindFirstFile(ScriptBatMask, &c_file)) != INVALID_HANDLE_VALUE)
	{
		int sequence = 0;

		do
		{
			if (!(c_file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && wcscmp(c_file.cFileName, L".") && wcscmp(c_file.cFileName, L".."))
			{
				const CString script(c_file.cFileName);
				ScriptsBat.push_back(script);
				PluginProcArray.push_back(GetInstanceBatList[sequence++]);
			}
		}
		while (sequence < maxscripts && FindNextFile(hFile, &c_file) != 0);

		FindClose(hFile);
	}

	// Register all *.ps1 scripts.
	const CString ScriptPS1Mask(L"*.ps1");
	if ((hFile = FindFirstFile(ScriptPS1Mask, &c_file)) != INVALID_HANDLE_VALUE)
	{
		int i = 0;
		std::wregex descriptionRegex(GetPS1DescriptionRegex());

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

					desc = scanPS1Description(Text, descriptionRegex);

					fclose(infile);
				}

				const CString script(c_file.cFileName);
				ScriptsPS1.push_back(script_info(script, desc));
				PluginProcArray.push_back(GetInstancePS1List[i++]);
			}
		}
		while (i < maxscripts && FindNextFile(hFile, &c_file) != 0);

		FindClose(hFile);
	}

	// Register all *.py scripts.
	const CString ScriptPyMask(L"*.py");
	if ((hFile = FindFirstFile(ScriptPyMask, &c_file)) != INVALID_HANDLE_VALUE)
	{
		int i = 0;
		std::wregex descriptionRegex(GetPyDescriptionRegex());

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

					desc = scanPyDescription(Text, descriptionRegex);

					fclose(infile);
				}

				const CString script(c_file.cFileName);
				ScriptsPy.push_back(script_info(script, desc));
				PluginProcArray.push_back(GetInstancePyList[i++]);
			}
		}
		while (i < maxscripts && FindNextFile(hFile, &c_file) != 0);

		FindClose(hFile);
	}

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
