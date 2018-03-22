#include "HausDorffComputer.h"
#include "CommonHeader.h"

HausDorffComputer::HausDorffComputer(void)
{
	this->mImageA=0;
	this->mImageB=0;
	this->mRho=0;
	this->mBeta=0;
}

HausDorffComputer::HausDorffComputer(IplImage * imageA,IplImage * imageB,double rho,double beta,int imageAScanInterval,int imageBScanInterval)
{
	this->mImageA=imageA;
	this->mImageB=imageB;
	this->mRho=rho;
	this->mBeta=beta;
	this->mImageAScanInterval=imageAScanInterval;
	this->mImageBScanInterval=imageBScanInterval;
}

HausDorffComputer::~HausDorffComputer(void)
{
}
	
void HausDorffComputer::SetImageA(IplImage * image)
{
	this->mImageA=image;
}

void HausDorffComputer::SetImageB(IplImage * image)
{
	this->mImageB=image;
}

void HausDorffComputer::SetRho(double rho)
{
	this->mRho=rho;
}

void HausDorffComputer::SetBeta(double beta)
{
	this->mBeta=beta;
}

void HausDorffComputer::SetImageAScanInterval(int interval)
{
	this->mImageAScanInterval=interval;
}

void HausDorffComputer::SetImageBScanInterval(int interval)
{
	this->mImageBScanInterval=interval;
}

double HausDorffComputer::GetHausDorffValue()
{
	int AWPointAmount=0;
	for(int i=0;i<this->mImageA->width;i++)
	{
		for(int j=0;j<this->mImageA->height;j++)
		 {
			if((uchar)(this->mImageA->imageData[j*this->mImageA->width+i])==255)
			{
				AWPointAmount++;
			}
		}
	}

	CvPoint * AWPoints=new CvPoint[AWPointAmount];
	AWPointAmount=0;
	for(int i=0;i<this->mImageA->width;i++)
	{
		for(int j=0;j<this->mImageA->height;j++)
		{
			if((uchar)(this->mImageA->imageData[j*this->mImageA->width+i])==255)
			{
				AWPoints[AWPointAmount].x=i;
				AWPoints[AWPointAmount].y=j;
				AWPointAmount++;
			}
		}
	}
	int BWPointAmount=0;
	for(int i=0;i<this->mImageB->width;i++)
	{
		for(int j=0;j<this->mImageB->height;j++)
		{
			if((uchar)(this->mImageB->imageData[j*this->mImageB->width+i])==255)
			{
				BWPointAmount++;
			}
		}
	}
	CvPoint * BWPoints=new CvPoint[BWPointAmount];
	BWPointAmount=0;
	for(int i=0;i<this->mImageB->width;i++)
	{
		for(int j=0;j<this->mImageB->height;j++)
		{
			if((uchar)(this->mImageB->imageData[j*this->mImageB->width+i])==255)
			{
				BWPoints[BWPointAmount].x=i;
				BWPoints[BWPointAmount].y=j;
				BWPointAmount++;
			}
		}
	}
	int fitPointAmount=0;
	double sumDistance=0;
	#pragma omp parallel for reduction(+:sumDistance,fitPointAmount)
	for(int i=0;i<AWPointAmount;i+=(this->mImageAScanInterval==0?1:this->mImageAScanInterval))
	{
		double minDistance=VERYBIG;
		double tempDistance=0;
		#pragma omp parallel for firstprivate(tempDistance)
		for(int j=0;j<BWPointAmount;j+=(this->mImageBScanInterval==0?1:this->mImageBScanInterval))
		{
			tempDistance=sqrt((double)((AWPoints[i].x-BWPoints[j].x)*(AWPoints[i].x-BWPoints[j].x)+(AWPoints[i].y-BWPoints[j].y)*(AWPoints[i].y-BWPoints[j].y)));
			#pragma omp critical
			minDistance=minDistance<tempDistance?minDistance:tempDistance;
		}
	
		if(minDistance<this->mBeta)
		{
			fitPointAmount++;
			sumDistance+=minDistance;
		}
	}
	if(fitPointAmount==0)
	{
		delete[] AWPoints;
		delete[] BWPoints;
		return VERYBIG
	}

	double distanceAToB=pow( (double)AWPointAmount / (double)fitPointAmount , this->mRho ) * sumDistance / (double)fitPointAmount;
	fitPointAmount=0;


	sumDistance=0;
	#pragma omp parallel for reduction(+:sumDistance,fitPointAmount)
	for(int i=0;i<BWPointAmount;i+=(this->mImageBScanInterval==0?1:this->mImageBScanInterval))
	{
		double minDistance=VERYBIG;
		double tempDistance=0;
		#pragma omp parallel for firstprivate(tempDistance)
		for(int j=0;j<AWPointAmount;j+=(this->mImageAScanInterval==0?1:this->mImageAScanInterval))
		{
			tempDistance=sqrt((double)((BWPoints[i].x-AWPoints[j].x)*(BWPoints[i].x-AWPoints[j].x)+(BWPoints[i].y-AWPoints[j].y)*(BWPoints[i].y-AWPoints[j].y)));
			#pragma omp critical
			minDistance=minDistance<tempDistance?minDistance:tempDistance;
		}

		if(minDistance<this->mBeta)
		{
			fitPointAmount++;
			sumDistance+=minDistance;
		}
	}
	
	if(fitPointAmount==0)
	{
		delete[] AWPoints;
		delete[] BWPoints;
		return VERYBIG
	}
	double distanceBToA=pow((double)AWPointAmount/(double)fitPointAmount,this->mRho)*sumDistance/(double)fitPointAmount;
	return distanceAToB>distanceBToA?distanceAToB:distanceBToA;
}

