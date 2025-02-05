// PluginPropertiesDlg.cpp : implementation file
//

#include "stdafx.h"
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
	page_range(L"1-"),
	page_range_from(0),
	page_range_to(-1),
	border_size(25),
	scaling(100),
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
	DDX_Control(pDX, IDC_COMBO_SCALING, m_comboScaling);
}


BEGIN_MESSAGE_MAP(CPdfPropertiesDlg, CDialog)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK_PDF, &CPdfPropertiesDlg::OnClickSyslinkPdf)
	ON_EN_KILLFOCUS(IDC_EDIT_PDF_MAX_X, &CPdfPropertiesDlg::OnEnKillfocusEditControlMaxX)
	ON_EN_KILLFOCUS(IDC_EDIT_PDF_MAX_Y, &CPdfPropertiesDlg::OnEnKillfocusEditControlMaxY)
	ON_EN_KILLFOCUS(IDC_EDIT_BORDER_SIZE, &CPdfPropertiesDlg::OnEnKillfocusEditControlBorderSize)
	ON_STN_CLICKED(IDC_STATIC_BORDER_COLOR, &CPdfPropertiesDlg::OnStnClickedBorderColorStaticText)
END_MESSAGE_MAP()


// CPdfPropertiesDlg message handlers

BOOL CPdfPropertiesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_BorderColorStaticText.SubclassDlgItem(IDC_STATIC_BORDER_COLOR, this);

	EnableToolTips(TRUE);
	m_ToolTip.Create(this);
	m_ToolTip.SetMaxTipWidth(320);

	CString tooltip;
	tooltip.LoadString(IDS_EDIT_PDF_MAX_X_TOOLTIP);
	m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_PDF_MAX_X), tooltip);

	tooltip.LoadString(IDS_EDIT_PDF_MAX_Y_TOOLTIP);
	m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_PDF_MAX_Y), tooltip);

	tooltip.LoadString(IDS_EDIT_PAGE_RANGE_TOOLTIP);
	m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_PAGE_RANGE), tooltip);

	tooltip.LoadString(IDS_EDIT_BORDER_SIZE_TOOLTIP);
	m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_BORDER_SIZE), tooltip);

	tooltip.LoadString(IDS_STATIC_COLOR_TOOLTIP);
	m_ToolTip.AddTool(GetDlgItem(IDC_STATIC_COLOR), tooltip);
	
	tooltip.LoadString(IDS_STATIC_BORDER_COLOR_TEXT_TOOLTIP);
	m_ToolTip.AddTool(GetDlgItem(IDC_STATIC_BORDER_COLOR), tooltip);

	page_range.Format(L"%d", page_range_from + 1);

	if (page_range_to > page_range_from)
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

	UpdateData(false); // write the data

	m_colorStatic.SetColor(border_color);

	// Add specific scaling values to the ComboBox.
	constexpr int values[] = { 10, 25, 50, 75, 100, 125, 150, 200, 400, 1000 };
	for (const int value : values)
	{
		CString strValue;
		strValue.Format(_T("%d"), value);
		m_comboScaling.AddString(strValue);
	}

	CString strScaling;
	strScaling.Format(_T("%d"), scaling);
	m_comboScaling.SetWindowText(strScaling);

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

	border_color = m_colorStatic.GetColor();

	std::wstring input(page_range);
	std::wsmatch match;

	static std::wregex& rangeRegex = std::wregex(L"\\s*(\\d+)?\\s*(-)?\\s*(\\d+)?");
	if (std::regex_search(input, match, rangeRegex) && match.size() == 4)
	{
		std::wstring m1 = match[1];
		page_range_from = max(0, _wtoi(m1.c_str()) - 1);

		std::wstring m2 = match[2];

		std::wstring m3 = match[3];
		page_range_to = m2.empty() ? 0 : _wtoi(m3.c_str()) - 1;

		// validate the range
		if (m1.empty() && m2.empty() && m3.empty()
			||
			page_range_to == 0 && page_range_from == 0 && m2.empty() && !m3.empty()
			||
			page_range_to != 0 && page_range_to < page_range_from && m2 == L"-"
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

	// Get the selected value from the ComboBox.
	CString strValue;
	m_comboScaling.GetWindowText(strValue);
	if (strValue.IsEmpty())
	{
		scaling = 100;
	}
	else
	{
		scaling = _wtoi(strValue);
	}

	CDialog::OnOK();
}

void CPdfPropertiesDlg::OnClickSyslinkPdf(NMHDR* pNMHDR, LRESULT* pResult)
{
	const PNMLINK pNMLink = reinterpret_cast<PNMLINK>(pNMHDR);
	const LITEM item = pNMLink->item;

	ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);

	*pResult = 0;
}

void CPdfPropertiesDlg::OnStnClickedBorderColorStaticText()
{
	border_color = RGB(255, 216, 0);
	m_colorStatic.SetColor(border_color);
}

void CPdfPropertiesDlg::SetControl(const int id, CString default) const
{
	CString strValue;
	GetDlgItem(id)->GetWindowText(strValue);

	if (strValue.IsEmpty())
	{
		GetDlgItem(id)->SetWindowText(default); // Set default value
	}
}

void CPdfPropertiesDlg::OnEnKillfocusEditControlMaxX()
{
	SetControl(IDC_EDIT_PDF_MAX_X, _T("8000"));
}

void CPdfPropertiesDlg::OnEnKillfocusEditControlMaxY()
{
	SetControl(IDC_EDIT_PDF_MAX_Y, _T("8000"));
}

void CPdfPropertiesDlg::OnEnKillfocusEditControlBorderSize()
{
	SetControl(IDC_EDIT_BORDER_SIZE, _T("25"));
}
