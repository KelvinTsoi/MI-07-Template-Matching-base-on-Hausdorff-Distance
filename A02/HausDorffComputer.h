#include "CommonHeader.h"

#pragma once
 
class HausDorffComputer
{
	public:
		HausDorffComputer(void);
		HausDorffComputer(IplImage * imageA,IplImage * imageB,double rho,double beta,int imageAScanInterval=0,int imageBScanInterval=0);
		~HausDorffComputer(void);
		void SetImageA(IplImage * image);
		void SetImageB(IplImage * image);
		void SetRho(double rho);
		void SetBeta(double beta);
		void SetImageAScanInterval(int interval);
		void SetImageBScanInterval(int interval);
		double GetHausDorffValue();
	private:
		IplImage * mImageA;
	    IplImage * mImageB;
	    double mRho;
	    double mBeta;
	    int mImageAScanInterval;
	    int mImageBScanInterval;
};

