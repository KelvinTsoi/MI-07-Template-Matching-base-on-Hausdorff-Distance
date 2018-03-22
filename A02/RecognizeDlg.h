#pragma once

#include "CommonHeader.h"
#include "HausDorffComputer.h"

#include <opencv2\opencv.hpp>
#include "afxwin.h"

// CRecognizeDlg 对话框

class CRecognizeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRecognizeDlg)

public:
	CRecognizeDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CRecognizeDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	CEdit r_edit;
};
