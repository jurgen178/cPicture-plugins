#pragma once

#include "stdafx.h"
#include "resource.h"
#include "Plugin.h"

#include "webp/encode.h"
#include "webp/mux.h"

#include <atomic>
#include <thread>
#include <vector>
using namespace std;

// Custom messages for worker → UI communication
#define WM_ENCODE_PROGRESS  (WM_USER + 100)  // wParam = current frame (1-based), lParam = total
#define WM_ENCODE_DONE      (WM_USER + 101)  // wParam = 1 (success), 0 (error/cancelled)


// CEncodeDlg
// Modal progress dialog that runs WebP animation encoding on a background thread.
// Worker thread posts WM_ENCODE_PROGRESS and WM_ENCODE_DONE back to the dialog.

class CEncodeDlg : public CDialog
{
public:
	CEncodeDlg(
		HWND                        hWndParent,
		const vector<picture_data>& pictureData,
		const AnimSettings&         settings,
		const CString&              outputPath);

	enum { IDD = IDD_ENCODE_PROGRESS };

	bool IsSuccess()   const { return m_success; }
	bool IsCancelled() const { return m_cancelled.load(); }

protected:
	virtual BOOL OnInitDialog();
	afx_msg void OnCancel();
	afx_msg LRESULT OnEncodeProgress(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnEncodeDone(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

private:
	// Encoding inputs (owned by caller, valid for the dialog lifetime)
	const vector<picture_data>& m_pictureData;
	const AnimSettings          m_settings;
	const CString               m_outputPath;

	// State
	std::atomic<bool> m_cancelled;
	bool              m_success;
	std::thread       m_worker;

	// Worker entry point
	void WorkerProc();
};
