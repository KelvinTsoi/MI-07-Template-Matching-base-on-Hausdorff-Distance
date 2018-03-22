
// A02Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "A02.h"
#include "A02Dlg.h"
#include "afxdialogex.h"
//#include "DataDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//#define FLAG 1;
#define PI 3.1415926

using namespace cv;
using namespace std;

// 静变量初始化
const Scalar RED = Scalar(0,0,255);
const Scalar PINK = Scalar(230,130,255);
const Scalar BLUE = Scalar(255,0,0);
const Scalar LIGHTBLUE = Scalar(255,255,160);
const Scalar GREEN = Scalar(0,255,0);

const int BGD_KEY = CV_EVENT_FLAG_CTRLKEY;  
const int FGD_KEY = CV_EVENT_FLAG_SHIFTKEY;

static int flag = 0;
static CString CString_fileName;
static string string_fileName;

CA02Dlg IPS;

////////////////////////////////////////Binaryzation///////////////////////////////////////////////////////
static void lhMorpRDilate(const IplImage* src, const IplImage* msk, IplImage* dst, IplConvKernel* se = NULL, int iterations=-1)
{
       assert(src != NULL && msk != NULL && dst != NULL && src != dst );

       if(iterations < 0)
       {
              cvMin(src, msk, dst);
              cvDilate(dst, dst, se);
              cvMin(dst, msk, dst);
              IplImage*  temp1 = cvCreateImage(cvGetSize(src), 8, 1);
              IplImage*  temp2 = cvCreateImage(cvGetSize(src), 8, 1);

              do
              {
                     cvCopy(dst, temp1);
                     cvDilate(dst, dst, se);
                     cvMin(dst, msk, dst);
					 cvCmp(temp1, dst, temp2, CV_CMP_NE );
              }while(cvSum(temp2).val[0] != 0);

              cvReleaseImage(&temp1);
              cvReleaseImage(&temp2);

              return;    
       }
       else if (iterations == 0)
       {
              cvCopy(src, dst);
       }
       else
       {
              cvMin(src, msk, dst);
			  cvDilate(dst, dst, se);
              cvMin(dst, msk, dst);

              for(int i=1; i<iterations; i++)
              {
                     cvDilate(dst, dst, se);
                     cvMin(dst, msk, dst);
              }

       }

}

static void lhMorpROpen(const IplImage* src, IplImage* dst, IplConvKernel* se = NULL, int iterations=1)
{
	assert(src != NULL  && dst != NULL && src != dst );

	IplImage*  temp = cvCreateImage(cvGetSize(src), 8, 1);
	cvErode(src, temp, se, iterations);
	lhMorpRDilate(temp, src, dst, se, 2 );
	cvReleaseImage(&temp);
}

static IplImage* histStretch( IplImage* src, IplImage* dst, int a, int b, int c, int d )
{
	CvMat* src_mat = cvCreateMat( src->height, src->width, CV_32FC1 );		
	cvConvert( src, src_mat );												
	int step = src_mat->step / sizeof( float );
	float* data = src_mat->data.fl;

	CvMat* dst_mat = cvCreateMat( dst->height, dst->width, CV_32FC1 );
	cvConvert( dst, dst_mat );
	int dst_step = dst_mat->step / sizeof( float );
	float* dst_data = dst_mat->data.fl;


	for( int i = 0; i < src->height; i++ )
	{
		for( int j = 0; j < src->width; j++ )
		{
			if( ( data + i * step )[j] < a )
			{
				( dst_data + i * dst_step )[j] =(int) c / a * ( dst_data + i * dst_step )[j];
			}
			else
			{
				if( (data + i * step)[j] > b )
				{
					( dst_data + i * dst_step )[j] = (int)(( 255 - d )/( 255 - b ))*( dst_data + i * dst_step )[j] + d;
				}
				else
				{
					( dst_data + i * dst_step )[j] = (int)( (d-c) * ( ( data + i * step )[j] -a ) / ( b - a ) + c );
				}
			}
		}
	}
	cvConvert( dst_mat, dst );
	return dst;
}

static int Otsu(IplImage* src)  
{  
	int height=src->height;  
	int width=src->width;      

	float histogram[256] = {0};  
	for(int i=0; i < height; i++)
	{  
		unsigned char* p=(unsigned char*)src->imageData + src->widthStep * i;  
		for(int j = 0; j < width; j++) 
		{  
			histogram[*p++]++;  
		}  
	}  
	  
	int size = height * width;  
	for(int i = 0; i < 256; i++)
	{  
		histogram[i] = histogram[i] / size;  
	}  
 
	float avgValue=0;  
	for(int i=0; i < 256; i++)
	{  
		avgValue += i * histogram[i];				
	}   

	int threshold;    
	float maxVariance=0;  
	float w = 0, u = 0;  
	for(int i = 0; i < 256; i++) 
	{  
		w += histogram[i];							
		u += i * histogram[i];						

		float t = avgValue * w - u;  
		float variance = t * t / (w * (1 - w) );  
		if(variance > maxVariance) 
		{  
			maxVariance = variance;  
			threshold = i;  
		}  
	}  

	return threshold;  
} 
////////////////////////////////////////Binaryzation///////////////////////////////////////////////////////

////////////////////////////////////Angular Adjustment/////////////////////////////////////////////////////
static CvSeq* getSeq( IplImage* img_8uc1, int threshold )
{
	IplImage* img_edge = cvCreateImage( cvGetSize(img_8uc1), 8, 1 );

	cvThreshold( img_8uc1, img_edge, threshold, 255, CV_THRESH_BINARY );

	CvMemStorage* storage = cvCreateMemStorage();
	CvSeq* first_contour = NULL;

	int Nc = cvFindContours(
		img_edge,
		storage,
		&first_contour,
		sizeof(CvContour),
		CV_RETR_LIST
	);

	double maxarea = 0;
	double minarea = 100;

	for( ; first_contour != NULL; first_contour = first_contour->h_next )
	{
		if( first_contour->total < 100 ) continue;

		double tmparea=fabs(cvContourArea(first_contour));

		if(tmparea < minarea)	continue;

		if(tmparea > maxarea) maxarea = tmparea;

		return first_contour;
	}
	return NULL;
}

static CvPoint2D32f getPoint( CvSeq* largest_contour )
{

	CvMoments moments;

	double M00, M01, M10;

	cvMoments(largest_contour,&moments);

	M00 = cvGetSpatialMoment(&moments,0,0);
    M10 = cvGetSpatialMoment(&moments,1,0);
    M01 = cvGetSpatialMoment(&moments,0,1);

	double cen_x = M10 / M00; 
    double cen_y = M01 / M00;

	CvPoint2D32f point = cvPoint2D32f( cen_x, cen_y );

	return point;
}

static double calTheta( CvSeq* largest_contour )
{
	CvMoments moments1;
    
	double M00, M01, M10;

    cvMoments(largest_contour,&moments1);

    M00 = cvGetSpatialMoment(&moments1,0,0);
    M10 = cvGetSpatialMoment(&moments1,1,0);
    M01 = cvGetSpatialMoment(&moments1,0,1);

    double posX_Yellow = (int)(M10/M00);
    double posY_Yellow = (int)(M01/M00);

    double theta = 0.5 * atan(
		(2 * cvGetCentralMoment(&moments1, 1, 1)) /
		(cvGetCentralMoment(&moments1, 2, 0) -  cvGetCentralMoment(&moments1, 0, 2))
	);
	
	theta = (theta / PI) * 180;

	return theta;
}

static IplImage* rotateImage(IplImage* src, int angle, bool clockwise)
{
    angle = abs(angle) % 180;
    if (angle > 90)
    {
        angle = 90 - (angle % 90);
    }
    IplImage* dst = NULL;
    int width =
            (double)(src->height * sin(angle * CV_PI / 180.0)) +
            (double)(src->width * cos(angle * CV_PI / 180.0 )) + 1;
    int height =
            (double)(src->height * cos(angle * CV_PI / 180.0)) +
            (double)(src->width * sin(angle * CV_PI / 180.0 )) + 1;
    int tempLength = sqrt(double(src->width * src->width + src->height * src->height)) + 10;
    int tempX = (tempLength + 1) / 2 - src->width / 2;
    int tempY = (tempLength + 1) / 2 - src->height / 2;
    int flag = -1;

    dst = cvCreateImage(cvSize(width, height), src->depth, src->nChannels);
    cvZero(dst);
    IplImage* temp = cvCreateImage(cvSize(tempLength, tempLength), src->depth, src->nChannels);
	cvSet( temp, CV_RGB(255,255,255),NULL );

    cvSetImageROI(temp, cvRect(tempX, tempY, src->width, src->height));
    cvCopy(src, temp, NULL);
    cvResetImageROI(temp);

    if (clockwise)
        flag = 1;

    float m[6];
    int w = temp->width;
    int h = temp->height;
    m[0] = (float) cos(flag * angle * CV_PI / 180.);
    m[1] = (float) sin(flag * angle * CV_PI / 180.);
    m[3] = -m[1];
    m[4] = m[0];

    m[2] = w * 0.5f;
    m[5] = h * 0.5f;

    CvMat M = cvMat(2, 3, CV_32F, m);
    cvGetQuadrangleSubPix(temp, dst, &M);
    cvReleaseImage(&temp);
    return dst;
}
////////////////////////////////////Angular Adjustment/////////////////////////////////////////////////////


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

/*
extern "C" BOOL AFX_EXT_API GetProperty(CTestSheet ** dialog)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    *dialog = new CTestSheet (_T("Test"));
    return TRUE;
}
*/

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CA02Dlg 对话框




CA02Dlg::CA02Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CA02Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CA02Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_edit);
}

BEGIN_MESSAGE_MAP(CA02Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_EDIT1, &CA02Dlg::OnEnChangeEdit1)
	ON_BN_CLICKED(IDC_BUTTON1, &CA02Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CA02Dlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CA02Dlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CA02Dlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CA02Dlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CA02Dlg::OnBnClickedButton6)
END_MESSAGE_MAP()


// CA02Dlg 消息处理程序

BOOL CA02Dlg::OnInitDialog()
{
	//AfxWinInit(::GetModuleHandle(NULL),NULL,::GetCommandLine(),0);
	//afxCurrentInstanceHandle = _AtlBaseModule.GetModuleInstance();
	//afxCurrentResourceHandle = _AtlBaseModule.GetResourceInstance();

	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	/////
	//
	//ASSERT(afxCurrentResourceHandle != NULL);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	//ASSERT(afxCurrentResourceHandle != NULL);
	m_edit.FmtLines(TRUE);
	m_edit.SetWindowText(_T("<GrabCut>\r\nThis program demonstrates GrabCut segmentation -- select an object in a region and then grabcut will attempt to segment it out.\r\n\r\nSelect a rectangular area around the object you want to segment\r\n\r\nHot keys:\r\n\tESC - quit the GrabCut Window\r\n\r\n\tr - restore the original image\r\n\tn - next iteration\r\n\ts - save the result(IMPORTANT)\r\n\r\n\tleft mouse button - set rectangle\r\n\r\n\tCTRL+left mouse button - set GC_BGD pixels\r\n\tSHIFT+left mouse button - set CG_FGD pixels\r\n\r\n\tCTRL+right mouse button - set GC_PR_BGD pixels\r\n\tSHIFT+right mouse button - set CG_PR_FGD pixels\r\n\r\n<Binaryzation>\r\n\This part of program illustraing the OTUS method obtaining the Threshold of Binaryzation and Opening by the Reconstruction in the image.\r\n\r\n<Angular Adjustment>\r\nUse Hu moments to calculate the centre of gravity and the direction angle of inertial principal axis as well as rotate the image to fit into the templates.\r\n\r\n<Recognition>\r\nCalculate the HausDorff Distance between the graphic after image processing and the template library. Then, output the recognized result."));

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CA02Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CA02Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CA02Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

///////////////////////////////////////////GrabCut/////////////////////////////////////////////////////////
static void getBinMask( const Mat& comMask, Mat& binMask )
{
	if( comMask.empty() || comMask.type()!=CV_8UC1 )
		CV_Error( CV_StsBadArg, "comMask is empty or has incorrect type (not CV_8UC1)" );

	if( binMask.empty() || binMask.rows!=comMask.rows || binMask.cols!=comMask.cols )
		binMask.create( comMask.size(), CV_8UC1 );

	binMask = comMask & 1;  
}

void CA02Dlg::reset()
{
	if( !mask.empty() )
		mask.setTo(Scalar::all(GC_BGD));

	bgdPxls.clear(); 
	fgdPxls.clear();
	prBgdPxls.clear(); 
	prFgdPxls.clear();

	isInitialized = false;
	rectState = NOT_SET;    
	lblsState = NOT_SET;
	prLblsState = NOT_SET;
	iterCount = 0;
}

void CA02Dlg::setImageAndWinName( const Mat& _image, const string& _winName  )
{
	if( _image.empty() || _winName.empty() )
		return;
	image = &_image;
	winName = &_winName;
	mask.create( image->size(), CV_8UC1);
	reset();
}

void CA02Dlg::showImage() const
{
	if( image->empty() || winName->empty() )
		return;

	Mat res;
	Mat binMask;

	if( !isInitialized ){
		image->copyTo( res );
	}
	else
	{
		getBinMask( mask, binMask );
		image->copyTo( res, binMask );
	}

	vector<Point>::const_iterator it;

	for( it = bgdPxls.begin(); it != bgdPxls.end(); ++it )  
		circle( res, *it, radius, BLUE, thickness );

	for( it = fgdPxls.begin(); it != fgdPxls.end(); ++it )  
		circle( res, *it, radius, RED, thickness );

	for( it = prBgdPxls.begin(); it != prBgdPxls.end(); ++it )
		circle( res, *it, radius, LIGHTBLUE, thickness );

	for( it = prFgdPxls.begin(); it != prFgdPxls.end(); ++it )
		circle( res, *it, radius, PINK, thickness );

	if( rectState == IN_PROCESS || rectState == SET )
		rectangle( res, Point( rect.x, rect.y ), Point(rect.x + rect.width, rect.y + rect.height ), GREEN, 2);

	imshow( *winName, res );
}

void CA02Dlg::setRectInMask()
{
	//assert( !mask.empty() );

	mask.setTo( GC_BGD );
	rect.x = max(0, rect.x);
	rect.y = max(0, rect.y);
	rect.width = min(rect.width, image->cols-rect.x);
	rect.height = min(rect.height, image->rows-rect.y);

	(mask(rect)).setTo( Scalar(GC_PR_FGD) );
}

void CA02Dlg::setLblsInMask( int flags, Point p, bool isPr )
{
	vector<Point> *bpxls, *fpxls;
	uchar bvalue, fvalue;

	if( !isPr ) 
	{
		bpxls = &bgdPxls;
		fpxls = &fgdPxls;
		bvalue = GC_BGD;    
		fvalue = GC_FGD;    
	}
	else    
	{
		bpxls = &prBgdPxls;
		fpxls = &prFgdPxls;
		bvalue = GC_PR_BGD; 
		fvalue = GC_PR_FGD; 
	}

	if( flags & BGD_KEY )
	{
		bpxls->push_back(p);
		circle( mask, p, radius, bvalue, thickness );
	}

	if( flags & FGD_KEY )
	{
		fpxls->push_back(p);
		circle( mask, p, radius, fvalue, thickness );
	}
}

void CA02Dlg::mouseClick( int event, int x, int y, int flags, void* )
{
	switch( event )
	{
	case CV_EVENT_LBUTTONDOWN: 
		{
			bool isb = (flags & BGD_KEY) != 0,
				isf = (flags & FGD_KEY) != 0;

			if( rectState == NOT_SET && !isb && !isf )
			{
				rectState = IN_PROCESS; 
				rect = Rect( x, y, 1, 1 );
			}

			if ( (isb || isf) && rectState == SET ) 
				lblsState = IN_PROCESS;
		}

		break;

	case CV_EVENT_RBUTTONDOWN: 
		{
			bool isb = (flags & BGD_KEY) != 0,
				isf = (flags & FGD_KEY) != 0;

			if ( (isb || isf) && rectState == SET ) 
				prLblsState = IN_PROCESS;
		}

		break;

	case CV_EVENT_LBUTTONUP:

		if( rectState == IN_PROCESS )
		{
			rect = Rect( Point(rect.x, rect.y), Point(x,y) );  
			rectState = SET;
			setRectInMask();
			assert( bgdPxls.empty() && fgdPxls.empty() && prBgdPxls.empty() && prFgdPxls.empty() );
			showImage();
		}

		if( lblsState == IN_PROCESS )   
		{
			setLblsInMask(flags, Point(x,y), false);    
			lblsState = SET;
			showImage();
		}

		break;

	case CV_EVENT_RBUTTONUP:

		if( prLblsState == IN_PROCESS )
		{
			setLblsInMask(flags, Point(x,y), true); 
			prLblsState = SET;
			showImage();
		}

		break;

	case CV_EVENT_MOUSEMOVE:

		if( rectState == IN_PROCESS )
		{
			rect = Rect( Point(rect.x, rect.y), Point(x,y) );
			assert( bgdPxls.empty() && fgdPxls.empty() && prBgdPxls.empty() && prFgdPxls.empty() );
			showImage();    
		}

		else if( lblsState == IN_PROCESS )
		{
			setLblsInMask(flags, Point(x,y), false);
			showImage();
		}

		else if( prLblsState == IN_PROCESS )
		{
			setLblsInMask(flags, Point(x,y), true);
			showImage();
		}

		break;
	}
}

int CA02Dlg::nextIter()
{
	if( isInitialized )
		grabCut( *image, mask, rect, bgdModel, fgdModel, 1 );
	else
	{
		if( rectState != SET )
			return iterCount;

		if( lblsState == SET || prLblsState == SET )
			grabCut( *image, mask, rect, bgdModel, fgdModel, 1, GC_INIT_WITH_MASK );
		else
			grabCut( *image, mask, rect, bgdModel, fgdModel, 1, GC_INIT_WITH_RECT );

		isInitialized = true;
	}
	iterCount++;

	bgdPxls.clear();
	fgdPxls.clear();
	prBgdPxls.clear();
	prFgdPxls.clear();

	return iterCount;
}

Mat CA02Dlg::getMat(){
	Mat binMask;
	getBinMask( mask, binMask );
	Mat Tmp(binMask.size(), CV_8UC3,cv::Scalar(255,255,255)); 
	image->copyTo(Tmp,binMask);
	return Tmp ;
}

static void on_mouse( int event, int x, int y, int flags, void* param )
{
	IPS.mouseClick( event, x, y, flags, param );
}
///////////////////////////////////////////GrabCut/////////////////////////////////////////////////////////


void CA02Dlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}

void CA02Dlg::DrawPicToHDC( IplImage* img, UINT ID )
{
	CDC *pDC = GetDlgItem(ID)->GetDC();
	HDC hDC = pDC->GetSafeHdc();
	CRect rect;
	GetDlgItem(ID)->GetClientRect(&rect);
	CvvImage cimg;
	cimg.CopyOf(img);
	cimg.DrawToHDC(hDC,&rect);
	ReleaseDC(pDC);
}

void CA02Dlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItem(IDC_EDIT2)->GetWindowText(CString_fileName);
	//MessageBox(CString_fileName);
	const size_t strsize=(CString_fileName.GetLength()+1)*2; 
	char * pstr= new char[strsize];
	size_t sz=0;
	wcstombs_s(&sz,pstr,strsize,CString_fileName,_TRUNCATE);

	string tmp(pstr);
	string_fileName.assign(tmp);

	IplImage* src = cvLoadImage(pstr);

	if(!src){
		MessageBox(_T("Couldn't Load the file! Check the file name one more time!"),_T("ERROR"),MB_ICONHAND);
	}

	DrawPicToHDC(src,IDC_STATIC);
}

void CA02Dlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	Mat img = imread(string_fileName,1);

	if( img.empty() )
    {
		MessageBox(_T("Couldn't read image"));
    }

	const string winName = "GrabCut";
    namedWindow( winName, WINDOW_AUTOSIZE );
	
	setMouseCallback( winName, on_mouse, 0 );

    IPS.setImageAndWinName( img, winName );
    IPS.showImage();

	// Main Loop

	for(;;)
    {
        int c = waitKey(0);
        switch( (char) c )
        {
        case 'r':
            cout << endl;
            IPS.reset();
            IPS.showImage();
            break;
        case 'n':
			{
            int iterCount = IPS.getIterCount();
            int newIterCount = IPS.nextIter();
            if( newIterCount > iterCount )
            {
                IPS.showImage();
            }
            else
				MessageBox(_T("rect must be determined"));
            break;
			}
		case 's':
			{
				Mat Tmp;
				Tmp = IPS.getMat().clone();
				imwrite("GrabResut.jpg",Tmp);
				break;
			}
		case '\x1b':
			{
				flag = 1;
				destroyWindow(winName);
				IplImage* image = cvLoadImage("GrabResut.jpg",-1);
				DrawPicToHDC(image,IDC_STATIC);
				goto LOOPOUT;
				break;
			}
		default:
			break;
        }
LOOPOUT:
		if( flag == 1 )
			break;
    }
	// Main Loop

}

void CA02Dlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	IplImage * src = cvLoadImage("GrabResut.jpg",CV_LOAD_IMAGE_GRAYSCALE); 
	IplImage * gray_img = cvCreateImage(cvGetSize(src), 8, 1);
	gray_img = histStretch( 
				src, 
				gray_img,
				100,
				229,
				50,
				202
				);

	cvSaveImage("Gray_level.jpg",gray_img);

	 IplImage * binary_img = cvCreateImage( cvGetSize(src), 8, 1 );
	 int threshold = Otsu( gray_img );
	 cvThreshold(gray_img, binary_img, threshold, 255, CV_THRESH_BINARY);

	 cvSaveImage("OTUS.jpg",binary_img);

	 IplImage * morphology_img = cvCreateImage( cvGetSize(src),8 ,1 );
	 IplConvKernel* element = cvCreateStructuringElementEx(
												3,
												3,
												0,
												0,
												CV_SHAPE_ELLIPSE,
												NULL
												);
		lhMorpROpen( binary_img, morphology_img, element, 1 );
		cvMorphologyEx( morphology_img, morphology_img, 0, element, CV_MOP_CLOSE, 1 );

		DrawPicToHDC(morphology_img,IDC_STATIC);

		cvSaveImage("binaryzation.jpg",morphology_img);

		cvReleaseImage(&src);
		cvReleaseImage(&gray_img);
		cvReleaseImage(&binary_img);
		cvReleaseImage(&morphology_img);
}

void CA02Dlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	INT_PTR nRes;

	CRecognizeDlg recognizeDlg;

	nRes = recognizeDlg.DoModal();

	 if (IDCANCEL == nRes) 

		 return;

	 UpdateData(TRUE); 

	 UpdateData(FALSE);   

}

void CA02Dlg::OnBnClickedButton5()
{
	// TODO: 在此添加控件通知处理程序代码
	IplImage* rotate_src = cvLoadImage("binaryzation.jpg",CV_LOAD_IMAGE_GRAYSCALE);
	
	if( !rotate_src ) 
		MessageBox(_T("Couldn't Load the Image!"),_T("ERROR"),MB_ICONHAND);

	IplImage* rotate_img = cvCreateImage( cvGetSize(rotate_src), 8, 3 );
	IplImage* rotate_dst = NULL;
	
	CvSeq* contour = getSeq( rotate_src, 128 );

	if( !contour ) MessageBox(_T("Couldn't Calculate the Contour!"),_T("ERROR"),MB_ICONHAND);

	cvCvtColor( rotate_src, rotate_img, CV_GRAY2BGR );

	double theta = calTheta(contour);

	if( theta >= 0 )
		rotate_dst = rotateImage( rotate_img, theta, false );
	else
		rotate_dst = rotateImage( rotate_img, theta, true );

	DrawPicToHDC(rotate_dst,IDC_STATIC);

	cvSaveImage("angular_adjustment.jpg",rotate_dst);

	cvReleaseImage( &rotate_src );
	cvReleaseImage( &rotate_img );
	cvReleaseImage( &rotate_dst );	
}


void CA02Dlg::OnBnClickedButton6()
{
	// TODO: 在此添加控件通知处理程序代码
	MessageBox(_T("Version: 1.0\r\nAuthor: Kevin Tsoi(CAI YUJIAN)\r\nE-mail: kevin_keith@163.com\r\nTEL:+86-13523088931"),_T("About"),MB_ICONEXCLAMATION);
}
