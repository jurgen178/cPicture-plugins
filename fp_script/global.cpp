#include "global.h"


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

bool scanBoolVar(const char* Text, const CString& SearchTextTemplate, bool def)
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

#ifdef DEBUG
CString GetLastErrorStr()
{
	DWORD errorCode = GetLastError();
	LPWSTR errorMsg = NULL;
	DWORD formatResult = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		errorMsg,
		0,
		NULL
	);

	CString str;
	if (formatResult)
	{
		str = errorMsg;
		LocalFree(errorMsg);
	}

	return str;
}
#endif
