// ParameterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ParameterDlg.h"
#include <afxdlgs.h>


CColorEdit::CColorEdit()
  : brush(RGB(0, 0, 0))
{
}

CColorEdit::~CColorEdit()
{
}


BEGIN_MESSAGE_MAP(CColorEdit, CEdit)
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorEdit message handlers



HBRUSH CColorEdit::CtlColor(CDC* pDC, UINT nCtlColor) 
{
	pDC->SetTextColor(RGB(255, 255, 255));
	pDC->SetBkColor(RGB(0, 0, 0));

	return (HBRUSH)brush;
}


// ParameterDlg dialog

ParameterDlg::ParameterDlg(const vector<const WCHAR*>& picture_list, CWnd* pParent /*=NULL*/)
  : CDialog(ParameterDlg::IDD, pParent),
	picture_list(picture_list),
	jpeg_quality(95),
	finished(false)
{
}

ParameterDlg::~ParameterDlg()
{
}

void ParameterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_OUTPUT, output_file);
	DDX_Text(pDX, IDC_EDIT_JPEG_QUALITY, jpeg_quality);
	DDX_Text(pDX, IDC_EDIT_ENFUSE, enfuse_exe_path);
	DDX_Control(pDX, IDC_EDIT_CONSOLE, console);
}


BEGIN_MESSAGE_MAP(ParameterDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &ParameterDlg::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDOK, &ParameterDlg::OnBnClickedOk)
END_MESSAGE_MAP()


BOOL ParameterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	console_font.CreateFont(22, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEVICE_PRECIS, 
				  CLIP_CHARACTER_PRECIS, PROOF_QUALITY, VARIABLE_PITCH | FF_SWISS, L"Consolas");

	console.SetFont(&console_font, FALSE);

	return TRUE;
}

void ParameterDlg::OnBnClickedButtonBrowse()
{
	CString mask;
	mask.LoadString(IDS_FILE_MASK_DESCRIPTION);

	CFileDialog fd(true, L"exe", L"", 
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, 
		mask, NULL);

	if(fd.DoModal() == IDOK)
	{
		if(fd.GetPathName().GetLength())
		{
			enfuse_exe_path = fd.GetPathName();
			UpdateData(false); // write the data
		}
	}
}

void ParameterDlg::OnBnClickedOk()
{
	UpdateData(true); // read the data

	if(!CheckFile(enfuse_exe_path))
	{
		AfxMessageBox(IDS_ENFUSE_MISSING);
		OnBnClickedButtonBrowse();
		//GetDlgItem(IDC_BUTTON_BROWSE)->SetFocus();
	}
	else
	{
		OnOK();
	}
}


// The following function is a shortened variant of Q190351 - HOWTO: Spawn Console Processes with Redirected Standard Handles
// There is no special magic here, and this version doesn't address issues like:
// - redirecting Input handle
// - spawning 16-bits process (well, RTconsole is a 32-bit process anyway so it should solve the problem)
// - command-line limitations (unsafe 1024-char buffer)
// So you might want to use more advanced versions such as the ones you can find on CodeProject
HANDLE ParameterDlg::SpawnAndRedirect(LPCTSTR commandLine, HANDLE *hStdOutputReadPipe, LPCTSTR lpCurrentDirectory)
{
	HANDLE hStdOutputWritePipe, hStdOutput, hStdError;
	CreatePipe(hStdOutputReadPipe, &hStdOutputWritePipe, NULL, 0);	// create a non-inheritable pipe
	DuplicateHandle(GetCurrentProcess(), hStdOutputWritePipe,
								GetCurrentProcess(), &hStdOutput,	// duplicate the "write" end as inheritable stdout
								0, TRUE, DUPLICATE_SAME_ACCESS);
	DuplicateHandle(GetCurrentProcess(), hStdOutput,
								GetCurrentProcess(), &hStdError,	// duplicate stdout as inheritable stderr
								0, TRUE, DUPLICATE_SAME_ACCESS);
	CloseHandle(hStdOutputWritePipe);								// no longer need the non-inheritable "write" end of the pipe

	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFO si = { 0 };

	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);	// (this is bad on a GUI app)
	si.hStdOutput = hStdOutput;
	si.hStdError  = hStdError;
	si.wShowWindow = SW_HIDE;						// IMPORTANT: hide subprocess console window
	
	const int commandLineCopySize(2048);
	WCHAR commandLineCopy[commandLineCopySize];		// CreateProcess requires a modifiable buffer
	wcscpy_s(commandLineCopy, commandLineCopySize, commandLine);
	if (!CreateProcess(	NULL, commandLineCopy, NULL, NULL, TRUE,
						CREATE_NEW_CONSOLE, NULL, lpCurrentDirectory, &si, &pi))
	{
		CloseHandle(hStdOutput);
		CloseHandle(hStdError);
		CloseHandle(*hStdOutputReadPipe);
		*hStdOutputReadPipe = INVALID_HANDLE_VALUE;

		return NULL;
	}

	CloseHandle(pi.hThread);
	CloseHandle(hStdOutput);
	CloseHandle(hStdError);

	return pi.hProcess;
}

void ParameterDlg::Go(LPCTSTR commandLine)
{
	console.SetWindowText(NULL);
	HANDLE hOutput, hProcess;
	hProcess = SpawnAndRedirect(commandLine, &hOutput, NULL);
	if (!hProcess) 
		return;

	//m_Console.SetWindowText(L"Child started ! Receiving output pipe :\r\n");
	//m_Console.SetSel(MAXLONG, MAXLONG);
	//m_Console.RedrawWindow();
	
	CString out;

	BeginWaitCursor();
	CHAR buffer[65];
	DWORD read;
	BOOL bRead(TRUE);

	while (bRead)
	{
		// Eine Zeile lesen.
		while (bRead = ReadFile(hOutput, buffer, 1, &read, NULL))
		{
			buffer[read] = '\0';
			out += CString(buffer); 
			if(*buffer == '\n')
				break;
		}
		 
		const int nSize(console.GetWindowTextLength());
		console.SetSel(nSize, nSize);
		console.ReplaceSel(out);

		out = L"";
	}

	out.LoadString(IDS_CONSOLE_FINSH);

	const int nSize(console.GetWindowTextLength());
	console.SetSel(nSize, nSize);
	console.ReplaceSel(L"\r\n" + out);

	CloseHandle(hOutput);
	CloseHandle(hProcess);
	EndWaitCursor();
}

void ParameterDlg::OnOK()
{
	if(!finished)
	{
		console.ModifyStyle(0, WS_VISIBLE);

		GetDlgItem(IDCANCEL)->MoveWindow(-1, -1, 0, 0);	// Kein EnableWindow(FALSE), sonst geht Abbrechen mit ESC oder [x] nicht mehr

		RemoveCtrl(IDC_STATIC_ENFUSE);
		RemoveCtrl(IDC_BUTTON_BROWSE);
		RemoveCtrl(IDC_EDIT_ENFUSE);
		RemoveCtrl(IDC_STATIC_OUTPUT);
		RemoveCtrl(IDC_EDIT_OUTPUT);
		RemoveCtrl(IDC_STATIC_JPEG_QUALITY);
		RemoveCtrl(IDC_EDIT_JPEG_QUALITY);


		CString cmd;
		cmd.FormatMessage(L"\"%1!s!\" -o \"%2!s!\" --compression=%3!d!", enfuse_exe_path, output_file, jpeg_quality);

		for(vector<const WCHAR*>::const_iterator it = picture_list.begin(); it != picture_list.end(); ++it)
		{
			cmd += L" \"";
			cmd += *it;
			cmd += L"\"";
		}

		Go(cmd);

		finished = true;
	}
	else
	{
		CDialog::OnOK();
	}
}

void ParameterDlg::RemoveCtrl(const DWORD id)
{
	GetDlgItem(id)->EnableWindow(FALSE);
	GetDlgItem(id)->MoveWindow(-1, -1, 0, 0);
}

bool ParameterDlg::CheckFile(const WCHAR* pFile)
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
