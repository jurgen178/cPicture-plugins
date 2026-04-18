#include "global.h"
#include "resource.h"


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

CString escapeCmdLineJsonDataPS1Str(CString text)
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
		text.Replace(L"\\", L"\\\\\\\\");	// \ -> \\\\ (4 Backslashes)

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
		// ab"c'1\  (1 Backslash) 

		text.Replace(L"\\", L"\\\\");	// \ -> \\  (2 Backslashes)
		text.Replace(L"\"", L"\\\\\"\"");	// " -> ""

		// Add double \ to avoid escaping the following quote when text ends with a \.
		if (text.Right(1) == L"\\")
			text += L"\\\\";

		// ab\\""c''\\1\\\\  (2 Backslashes) 
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

CString escapeCmdLineJsonDataPyStr(CString text)
{
	// embedded json data?
	CString textNoSpaces(text);
	textNoSpaces.Replace(L" ", L"");

	if (textNoSpaces.Left(2) == L"[{" && textNoSpaces.Right(2) == L"}]")
	{
		// Use double group replacement to replace delimiting quotes and inside quotes.

		// [{"key''1\":"value\"a","key2" : "value\b\\"}]
		text.Replace(L"\\", L"\\\\");	// \ -> \\

		// Escape delimiter quotes using a uncommon letter ⱦ.
		text.Replace(L"{\"", L"{\\\\ⱦⱦ");
		text.Replace(L":\"", L":\\\\ⱦⱦ");
		text.Replace(L"\":", L"\\\\ⱦⱦ:");
		text.Replace(L",\"", L",\\\\ⱦⱦ");
		text.Replace(L"\",", L"\\\\ⱦⱦ,");
		text.Replace(L"\"}", L"\\\\ⱦⱦ}");

		// [{\\ⱦⱦkey''1\\ⱦⱦ,\\ⱦⱦvalue"a\\ⱦⱦ,\\ⱦⱦkey2\\ⱦⱦ,\\ⱦⱦvalue\\\\b\\\\\\ⱦⱦ}]

		// All remaining quotes are now quotes inside of a quoted string.
		// Now escape " inside quoted string -> \"
		text.Replace(L"\"", L"\\\"");	// " -> \"

		// Replace uncommon letter back to escaped quotes.
		text.Replace(L"{\\\\ⱦⱦ", L"{\"");
		text.Replace(L":\\\\ⱦⱦ", L":\"");
		text.Replace(L"\\\\ⱦⱦ:", L"\":");
		text.Replace(L",\\\\ⱦⱦ", L",\"");
		text.Replace(L"\\\\ⱦⱦ,", L"\",");
		text.Replace(L"\\\\ⱦⱦ}", L"\"}");
	}
	else
	{
		// ab"c'1\  (1 Backslash) 

		text.Replace(L"\\", L"\\\\");	// \ -> \\  (2 Backslashes)

		text.Replace(L"\"", L"\\\"");	// " -> \"

		// Add double \ to avoid escaping the following quote when text ends with a \.
		if (text.Right(1) == L"\\")
			text += L"\\\\";

		// ab\\""c''\\1\\\\  (2 Backslashes) 
	}

	return text;
}

bool toUTF8(const CString& data, unsigned char*& utf8Buffer, int& utf8Length)
{
	// Determine the required buffer size for the UTF-8 string.
	const int bufferSize = WideCharToMultiByte(CP_UTF8, 0, data, -1, NULL, 0, NULL, NULL);
	if (bufferSize)
	{
		// Allocate the buffer for the UTF-8 string.
		utf8Buffer = new unsigned char[bufferSize];
		memset(utf8Buffer, 0, bufferSize);

		// Convert the CString to a UTF-8 encoded buffer.
		utf8Length = WideCharToMultiByte(CP_UTF8, 0, data, -1, reinterpret_cast<char*>(utf8Buffer), bufferSize, NULL, NULL);
		if (utf8Length)
		{
			return true;
		}
	}

	return false;
}

CString toBase64(const unsigned char* data, const int len)
{
	CString base64;
	base64.Preallocate(4 * len / 10);

	const int size3 = len - len % 3;
	const char base64map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	for (int i = 0; i < size3; i += 3)
	{
		const unsigned char c1 = data[i] >> 2;
		const unsigned char c2 = ((data[i] & 0x03) << 4) | (data[i + 1] >> 4);
		const unsigned char c3 = ((data[i + 1] & 0x0F) << 2) | (data[i + 2] >> 6);
		const unsigned char c4 = data[i + 2] & 0x3F;

		base64 += base64map[c1];
		base64 += base64map[c2];
		base64 += base64map[c3];
		base64 += base64map[c4];
	}

	const int last_block_size = len % 3;
	// Base64 tail blocks are zero-padded; do not read beyond the provided input buffer.

	if (last_block_size == 1)
	{
		const unsigned char c1 = data[size3] >> 2;
		const unsigned char c2 = (data[size3] & 0x03) << 4;

		base64 += base64map[c1];
		base64 += base64map[c2];

		base64 += L"==";

		return base64;
	}
	else
	if (last_block_size == 2)
	{
		const unsigned char c1 = data[size3] >> 2;
		const unsigned char c2 = ((data[size3] & 0x03) << 4) | (data[size3 + 1] >> 4);
		const unsigned char c3 = (data[size3 + 1] & 0x0F) << 2;

		base64 += base64map[c1];
		base64 += base64map[c2];
		base64 += base64map[c3];

		base64 += L"=";

		return base64;
	}

	return base64;
}

CString toBase64(const CString& data)
{
	unsigned char* utf8Buffer = NULL;
	int utf8Length = 0;
	if(toUTF8(data, utf8Buffer, utf8Length))
	{
		const CString base64 = toBase64(utf8Buffer, utf8Length);
		
		delete[] utf8Buffer;
		return base64;
	}

	return L"";
}

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

namespace
{
	CString LoadResourceString(const UINT resourceId)
	{
		CString text;
		text.LoadString(resourceId);
		return text;
	}

	void AppendDetailSection(CString& text, const UINT labelId, const CString& value)
	{
		text += LoadResourceString(labelId);
		text += value;
		text += L"\r\n\r\n";
	}

	class CErrorDetailsDialog : public CDialog
	{
	public:
		CErrorDetailsDialog(const CString& title, const CString& text, CWnd* parentWnd)
			: CDialog(IDD_ERROR_DETAILS_DIALOG, parentWnd),
			m_title(title),
			m_text(text)
		{
		}

	protected:
		virtual BOOL OnInitDialog() override
		{
			CDialog::OnInitDialog();

			SetWindowText(m_title);
			SetDlgItemText(IDC_ERROR_DETAILS_EDIT, m_text);
			SetDlgItemText(IDC_ERROR_DETAILS_COPY, LoadResourceString(IDS_ERROR_DETAILS_COPY));
			SetDlgItemText(IDOK, LoadResourceString(IDS_ERROR_DETAILS_OK));

			if (CWnd* editControl = GetDlgItem(IDC_ERROR_DETAILS_EDIT))
			{
				editControl->SetFocus();
				return FALSE;
			}

			return TRUE;
		}

		afx_msg void OnSize(UINT nType, int cx, int cy)
		{
			CDialog::OnSize(nType, cx, cy);

			if (!::IsWindow(m_hWnd))
				return;

			const int margin = 10;
			const int buttonWidth = 120;
			const int buttonHeight = 28;
			const int buttonTop = cy - margin - buttonHeight;
			const int editHeight = max(60, buttonTop - margin);

			if (CWnd* editControl = GetDlgItem(IDC_ERROR_DETAILS_EDIT))
				editControl->MoveWindow(margin, margin, max(50, cx - (2 * margin)), editHeight);

			if (CWnd* copyButton = GetDlgItem(IDC_ERROR_DETAILS_COPY))
				copyButton->MoveWindow(max(margin, cx - margin - (2 * buttonWidth) - 8), buttonTop, buttonWidth, buttonHeight);

			if (CWnd* okButton = GetDlgItem(IDOK))
				okButton->MoveWindow(max(margin, cx - margin - buttonWidth), buttonTop, buttonWidth, buttonHeight);
		}

		afx_msg void OnCopyDetails()
		{
			if (CWnd* editControl = GetDlgItem(IDC_ERROR_DETAILS_EDIT))
			{
				editControl->SetFocus();
				editControl->SendMessage(EM_SETSEL, 0, -1);
				editControl->SendMessage(WM_COPY, 0, 0);
			}
		}

		DECLARE_MESSAGE_MAP()

	private:
		CString m_title;
		CString m_text;
	};

	BEGIN_MESSAGE_MAP(CErrorDetailsDialog, CDialog)
		ON_WM_SIZE()
		ON_BN_CLICKED(IDC_ERROR_DETAILS_COPY, &CErrorDetailsDialog::OnCopyDetails)
	END_MESSAGE_MAP()
}

void ShowShellExecuteErrorMessage(HWND hwnd, const CString& title, const CString& executable, const CString& parameters, const CString& scriptFile)
{
	const DWORD errorCode = GetLastError();
	const CString errorMsg = GetLastErrorStr();

	WCHAR currentDirectory[MAX_PATH] = { 0 };
	if (::GetCurrentDirectory(MAX_PATH, currentDirectory) == 0)
		wcsncpy_s(currentDirectory, MAX_PATH, LoadResourceString(IDS_ERROR_DETAILS_UNKNOWN), _TRUNCATE);

	CString msg;
	msg += LoadResourceString(IDS_ERROR_SCRIPT_START_FAILED);
	msg += L"\r\n\r\n";
	if (!scriptFile.IsEmpty())
		AppendDetailSection(msg, IDS_ERROR_DETAILS_SCRIPT_LABEL, scriptFile);
	AppendDetailSection(msg, IDS_ERROR_DETAILS_FILE_LABEL, executable);
	AppendDetailSection(msg, IDS_ERROR_DETAILS_PARAMS_LABEL, parameters);
	AppendDetailSection(msg, IDS_ERROR_DETAILS_WORKDIR_LABEL, currentDirectory);
	msg += LoadResourceString(IDS_ERROR_DETAILS_LASTERROR_LABEL);
	CString errorCodeText;
	errorCodeText.Format(L"%lu", errorCode);
	msg += errorCodeText;
	msg += L"\r\n\r\n";
	msg += LoadResourceString(IDS_ERROR_DETAILS_MESSAGE_LABEL);
	msg += errorMsg.IsEmpty() ? LoadResourceString(IDS_ERROR_DETAILS_NO_SYSTEM_MESSAGE) : errorMsg;

	CWnd* parentWnd = hwnd != NULL ? CWnd::FromHandle(hwnd) : NULL;
	CErrorDetailsDialog dialog(title, msg, parentWnd);
	dialog.DoModal();
}
