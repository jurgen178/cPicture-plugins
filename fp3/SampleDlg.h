#pragma once

#include "Plugin.h"
#include "resource.h"


enum PICTURE_FORMAT
{
	PICTURE_FORMAT_9 = 0,
	PICTURE_FORMAT_10,

	PICTURE_FORMAT_SIZE,
};


struct picture_order
{
	picture_order(const CString& name = L"", 
					 const PICTURE_FORMAT picture_format = PICTURE_FORMAT_10,
					 const unsigned int number_of_prints = 1)
	  : m_name(name),
		m_picture_format(picture_format),
		m_number_of_prints(number_of_prints)
	{
	};

	CString m_name;
	PICTURE_FORMAT m_picture_format;
	unsigned int m_number_of_prints;
};


// CSampleDlg dialog

class CSampleDlg : public CDialog
{
public:
	CSampleDlg(const vector<picture_data>& _picture_data_list, CWnd* pParent = NULL);   // standard constructor
	virtual ~CSampleDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_SAMPLE3 };

protected:
	HIMAGELIST m_hImageList;
	CListCtrl m_PictureListCtrl;
	CComboBox m_Format;
	CEdit m_Prints;
	CStatic m_StaticTotalPrice;
	CStatic m_StaticNumberOfPictures;

	int picture_price[PICTURE_FORMAT_SIZE];
	const vector<picture_data>& m_picture_data_list;

public:
	vector<picture_order> m_picture_orders;

public:
	void update_total(int& numberOfPictures, int& totalPrice);

protected:
	void update_total();
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMCustomdrawPictureList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonSet();
};
