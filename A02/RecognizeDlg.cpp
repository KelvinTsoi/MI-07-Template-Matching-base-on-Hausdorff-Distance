// RecognizeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "A02.h"
#include "RecognizeDlg.h"
#include "afxdialogex.h"

using namespace cv;


// CRecognizeDlg 对话框

IMPLEMENT_DYNAMIC(CRecognizeDlg, CDialogEx)

CRecognizeDlg::CRecognizeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRecognizeDlg::IDD, pParent)
{

}

CRecognizeDlg::~CRecognizeDlg()
{
}

void CRecognizeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, r_edit);
}


BEGIN_MESSAGE_MAP(CRecognizeDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CRecognizeDlg::OnBnClickedOk)
END_MESSAGE_MAP()



// CRecognizeDlg 消息处理程序


void CRecognizeDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	//CDialogEx::OnOK();

	//CString InitString = _T("Please Wait...\r\n");
	//GetDlgItem(IDC_EDIT1)->SetWindowText(InitString);
	CString InitString;
	InitString.Format(_T("CAUTION:DO NOT STOP OR CLOSE THE PROGRAM\r\nDURING PROCESSING!!!\r\n\r\n"));
	r_edit.ReplaceSel(InitString);
	//r_edit.ReplaceSel(
	InitString.Format(_T("This might take a few minues for the whole process,\r\n"));
	r_edit.ReplaceSel(InitString);
	InitString.Format(_T("Please waiting patiently...\r\n\r\n"));
	r_edit.ReplaceSel(InitString);

	IplImage * img_1 = cvLoadImage("angular_adjustment.jpg");
	HausDorffComputer HDF = HausDorffComputer();

	HDF.SetImageA( img_1 );
	HDF.SetRho( 0.5 );
	HDF.SetBeta( 8.0 );
	HDF.SetImageAScanInterval( 5 );
	HDF.SetImageBScanInterval( 5 );

	Mat image;
	string dir_path="./dir/";
	Directory dir;
	vector<String> fileNames=dir.GetListFiles(dir_path,"*jpg",false);

	int pecentage = 4;
	const int size = RESULTARRAYSIZE;
	double resultArr[size];
	double result = 0;
	double time0 = 0;
	double timeCount = 0;
	for (int i=0;i<fileNames.size();i++)
	{
		string fileName=fileNames[i];
        string fileFullName=dir_path+fileName;
        image=imread(fileFullName,1);

		IplImage imgTmp = image;
		IplImage *img_2 = cvCloneImage(&imgTmp);

		time0 = static_cast<double>(getTickCount());

		HDF.SetImageB( img_2 );
		result = HDF.GetHausDorffValue();
		resultArr[i] = result;
		
		time0 = ( (double)getTickCount() - time0 ) / getTickFrequency();

		InitString.Format(_T("Result: %lf...........%%"),result);
		r_edit.ReplaceSel(InitString);
		InitString.Format(_T("%3d"),pecentage);
		r_edit.ReplaceSel(InitString);
		InitString.Format(_T(" Times: %lfs\r\n"),time0);
		r_edit.ReplaceSel(InitString);
		pecentage += 4;
		timeCount += time0;

		cvReleaseImage( &img_2 );
     }

	InitString.Format(_T("................................%%100\r\n"));
	r_edit.ReplaceSel(InitString);
	InitString.Format(_T("Time Counter: %lfs\r\n"),timeCount);
	r_edit.ReplaceSel(InitString);

	int location = 0;
	double minTmp = resultArr[0];
	for( int i = 1; i < size; i++ )
	{
		if( resultArr[i] < minTmp )
		{
			 minTmp = resultArr[i];
			 location = i;
		}
	}

	if( location>=0 && location <= 8  )	
	{
		InitString.Format(_T("Recognize Result: Silvanidae(锯谷盗)\r\n...Completed"));
		r_edit.ReplaceSel(InitString);
	}
	else if( location>8 && location <= 14 )	
	{
		InitString.Format(_T("Recognize Result: Linnaeus(绿豆象)\r\n...Completed"));
		r_edit.ReplaceSel(InitString);
	}
	else if( location>14 && location < 23 ) 
	{
		InitString.Format(_T("Recognize Result: Rice weevil(米象)\r\n...Completed"));
		r_edit.ReplaceSel(InitString);
	}
	else
	{
		MessageBox(_T("Error!"),_T("ERROR"),MB_ICONHAND);
		InitString.Format(_T("Recognize Result: Error!\r\n"));
		r_edit.ReplaceSel(InitString);
	}
}
