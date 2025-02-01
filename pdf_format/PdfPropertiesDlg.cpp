// PluginPropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "afxdlgs.h"
#include "resource.h"
#include "PdfPropertiesDlg.h"
#include <string>
#include <regex>


// CPdfPropertiesDlg dialog

IMPLEMENT_DYNAMIC(CPdfPropertiesDlg, CDialog)
CPdfPropertiesDlg::CPdfPropertiesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPdfPropertiesDlg::IDD, pParent),
	max_picture_x(8000),
	max_picture_y(8000),
	page_range(L"0-"),
	page_range_from(0),
	page_range_to(-1),
	border_size(25),
	border_color(RGB(255, 216, 0))
{
}

CPdfPropertiesDlg::~CPdfPropertiesDlg()
{
}

void CPdfPropertiesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_COLOR, m_colorStatic);
	DDX_Text(pDX, IDC_EDIT_PDF_MAX_X, max_picture_x);
	DDX_Text(pDX, IDC_EDIT_PDF_MAX_Y, max_picture_y);
	DDX_Text(pDX, IDC_EDIT_PAGE_RANGE, page_range);
	DDX_Text(pDX, IDC_EDIT_BORDER_SIZE, border_size);
}


BEGIN_MESSAGE_MAP(CPdfPropertiesDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_BORDER_COLOR, &CPdfPropertiesDlg::OnClickedButtonBorderColor)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK_PDF, &CPdfPropertiesDlg::OnClickSyslinkPdf)
END_MESSAGE_MAP()


// CPdfPropertiesDlg message handlers

BOOL CPdfPropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	EnableToolTips(TRUE);
	m_ToolTip.Create(this);
	m_ToolTip.SetMaxTipWidth(320);

	CString tooltip;
	tooltip.LoadString(IDS_EDIT_PAGE_RANGE_TOOLTIP);
	m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_PAGE_RANGE), tooltip);

	page_range.Format(L"%d", page_range_from + 1);

	if (page_range_to >= page_range_from)
	{
		CString range;
		range.Format(L"-%d", page_range_to + 1);
		page_range += range;
	}
	else
	if (page_range_to == -1)
	{
		page_range += L"-";
	}

	m_colorStatic.SetColor(border_color);

	UpdateData(false); // write the data

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPdfPropertiesDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_MOUSEMOVE && ::IsWindow(m_ToolTip.m_hWnd))
		m_ToolTip.RelayEvent(pMsg);

	return __super::PreTranslateMessage(pMsg);
}

void CPdfPropertiesDlg::OnOK()
{
	UpdateData(true); // read the data

	std::wstring input(page_range);
	std::wsmatch match;

	static std::wregex& rangeRegex = std::wregex(L"(\\d+)?(-)?(\\d+)?");
	if (std::regex_search(input, match, rangeRegex) && match.size() == 4)
	{
		std::wstring m1 = match[1];
		page_range_from = max(0, _wtoi(m1.c_str()) - 1);

		std::wstring m3 = match[3];
		page_range_to = max(0, _wtoi(m3.c_str()) - 1);

		// 0-
		std::wstring m2 = match[2];
		if (page_range_to == 0 && page_range_from == 0
			||
			page_range_to != 0 && page_range_to < page_range_from
			||
			page_range_to == 0 && m2 == L"-")
		{
			page_range_to = -1;
		}
	}
	else
	{
		page_range_from = 0;
		page_range_to = -1;
	}

	CDialog::OnOK();
}

void CPdfPropertiesDlg::OnClickedButtonBorderColor()
{
	CColorDialog ColorDialog(border_color, CC_ANYCOLOR | CC_FULLOPEN);
	if (ColorDialog.DoModal() == IDOK)
	{
		border_color = ColorDialog.GetColor();
		m_colorStatic.SetColor(border_color);
	}
}

void CPdfPropertiesDlg::OnClickSyslinkPdf(NMHDR* pNMHDR, LRESULT* pResult)
{
	PNMLINK pNMLink = reinterpret_cast<PNMLINK>(pNMHDR);
	LITEM item = pNMLink->item;

	ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);

	*pResult = 0;
}
