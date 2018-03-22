
// A02Dlg.h : 头文件
//

#pragma once
#include <cv.h>
#include <highgui.h>
#include "CvvImage.h"
#include "afxwin.h"

#include <opencv2\highgui\highgui.hpp>
#include <opencv2\imgproc\imgproc.hpp>

#include <Windows.h>
#include <iostream>
#include <string>
#include <math.h>

#include "RecognizeDlg.h"

using namespace std;
using namespace cv;



// CA02Dlg 对话框
class CA02Dlg : public CDialogEx
{
// 构造
public:
	CA02Dlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_A02_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	// 私有函数定义

	void setRectInMask();
	void setLblsInMask( int flags, Point p, bool isPr );

	const string* winName;
	const Mat* image;
	Mat mask;
	Mat bgdModel, fgdModel;

	uchar rectState, lblsState, prLblsState;
	bool isInitialized;

	Rect rect;
	vector<Point> fgdPxls, bgdPxls, prFgdPxls, prBgdPxls;
	int iterCount;

public:
	afx_msg void OnEnChangeEdit1();
	CEdit m_edit;

	// 按键消息函数

	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();

	void DrawPicToHDC( IplImage* img, UINT ID );

	// 共有函数定义

	enum{ NOT_SET = 0, IN_PROCESS = 1, SET = 2 };
	static const int radius = 2;
	static const int thickness = -1;

	void reset();
	void setImageAndWinName( const Mat& _image, const string& _winName );
	void showImage() const;
	void mouseClick( int event, int x, int y, int flags, void* param );
	int nextIter();
	int getIterCount() const { return iterCount; }

	Mat getMat();


	afx_msg void OnBnClickedButton6();
};
