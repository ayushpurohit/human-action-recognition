#include "OpticalFlow.h"
#include <GL/glfw.h>
#include <iostream>

OpticalFlow::OpticalFlow(CvSize imgSize, CvSize outImageSize)
{
	_outImageSize = outImageSize;
	_velx = 0;
	_vely = 0;
	_velz = 0;
	_tmp = cvCreateImage(imgSize, IPL_DEPTH_32F, 1);
	_gray1 = cvCreateImage(imgSize, IPL_DEPTH_8U, 1);
	_gray2 = cvCreateImage(imgSize, IPL_DEPTH_8U, 1);
	cvZero(_gray1);
	cvZero(_gray2);

	for(int i=0; i<N_FRAMES_SUM; ++i) 
	{
		_nFlow[i] = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);
		_eFlow[i] = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);
		_sFlow[i] = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);
		_wFlow[i] = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);
		_zFlow[i] = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);
	}

	_nSum = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);
	_sSum = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);
	_wSum = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);
	_eSum = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);
	_zSum = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);

	_data = new double[_outImageSize.width * _outImageSize.height * 5];
}

OpticalFlow::~OpticalFlow(void)
{
	cvReleaseImage(&_velx);
	cvReleaseImage(&_vely);
	cvReleaseImage(&_velz);
	cvReleaseImage(&_tmp);
	cvReleaseImage(&_gray1);
	cvReleaseImage(&_gray2);

	for(int i=0; i<N_FRAMES_SUM; ++i) 
	{
		cvReleaseImage(&_nFlow[i]);
		cvReleaseImage(&_eFlow[i]);
		cvReleaseImage(&_sFlow[i]);
		cvReleaseImage(&_wFlow[i]);
		cvReleaseImage(&_zFlow[i]);
	}

	cvReleaseImage(&_nSum);
	cvReleaseImage(&_eSum);
	cvReleaseImage(&_sSum);
	cvReleaseImage(&_wSum);
	cvReleaseImage(&_zSum);

	delete _data;
}

void OpticalFlow::Calculate(IplImage *frame)
{
	cvReleaseImage(&_velx);
	cvReleaseImage(&_vely);
	cvReleaseImage(&_velz);

	_velx = cvCreateImage(cvGetSize(frame), IPL_DEPTH_32F, 1);
	_vely = cvCreateImage(cvGetSize(frame), IPL_DEPTH_32F, 1);
	_velz = cvCreateImage(cvGetSize(frame), IPL_DEPTH_32F, 1);

	cvCvtColor( frame, _gray2, CV_BGR2GRAY );
	cvCalcOpticalFlowLK(_gray1, _gray2, cvSize(3,3), _velx, _vely);
	cvCvtColor( frame, _gray1, CV_BGR2GRAY );

	Smooth(_velx);
	Smooth(_vely);

	cvPow(_velx, _velz, 2);
	cvPow(_vely, _tmp, 2);
	cvAdd(_tmp, _velz, _velz);
	cvPow(_velz, _velz, 0.5);
}

void OpticalFlow::Draw()
{
	/*
	double minx = 65000, miny = 65000, maxz = -65000;

	for(int y=0; y<_velx->height; ++y)
		for(int x=0; x<_velx->width; ++x)
		{
			double vx = cvGetReal2D(_velx, y, x);
			double vy = cvGetReal2D(_vely, y, x);
			double vz = cvGetReal2D(_velz, y, x);
			if(vx < minx) minx = vx;
			if(vy < miny) miny = vy;
			if(vz > maxz) maxz = vz;
		}

	if(minx<0) minx = -minx;
	else minx = 0.5;
	if(miny<0) miny = -miny;
	else miny = 0.5;
	*/

	glBegin(GL_POINTS);
		for(int y=0; y<_velx->height; ++y)
			for(int x=0; x<_velx->width; ++x)
			{
				float vx = (float)cvGetReal2D(_velx, y, x);
				float vy = (float)cvGetReal2D(_vely, y, x);
				float vz = (float)cvGetReal2D(_velz, y, x);
				glColor3f((vx+250)/(500),(vy+250)/(500),(vz+250)/(500));
				glVertex2i(x,y);
			}
	glEnd();
}

void OpticalFlow::Finalize()
{
	cvAdd(_nFlow[0], _nFlow[1], _nSum);
	cvAdd(_eFlow[0], _eFlow[1], _eSum);
	cvAdd(_sFlow[0], _sFlow[1], _sSum);
	cvAdd(_wFlow[0], _wFlow[1], _wSum);
	cvAdd(_zFlow[0], _zFlow[1], _zSum);

	for(int i=2; i<N_FRAMES_SUM; ++i)
	{
		cvAdd(_nFlow[i], _nSum, _nSum);
		cvAdd(_eFlow[i], _eSum, _eSum);
		cvAdd(_sFlow[i], _sSum, _sSum);
		cvAdd(_wFlow[i], _wSum, _wSum);
		cvAdd(_zFlow[i], _zSum, _zSum);
	}

	/*
	Smooth(_nSum);
	Smooth(_eSum);
	Smooth(_sSum);
	Smooth(_wSum);
	Smooth(_zSum);
	*/

	/*
	Normalize(_nSum);
	Normalize(_eSum);
	Normalize(_sSum);
	Normalize(_wSum);
	Normalize(_zSum);
	*/

	EqualizeHistogram(_nSum);
	EqualizeHistogram(_eSum);
	EqualizeHistogram(_sSum);
	EqualizeHistogram(_wSum);
	EqualizeHistogram(_zSum);
}

void OpticalFlow::Write(ofstream &fout)
{
	for(int y=0; y<_velx->height; ++y)
		for(int x=0; x<_velx->width; ++x)
		{
			fout << " " << cvGetReal2D(_nSum, y, x) 
				 << " " << cvGetReal2D(_eSum, y, x)
				 << " " << cvGetReal2D(_sSum, y, x)
				 << " " << cvGetReal2D(_wSum, y, x)
				 << " " << cvGetReal2D(_zSum, y, x);
		}
	fout << endl;
}

double* OpticalFlow::GetData()
{
	for(int y=0; y<_velx->height; ++y)
		for(int x=0; x<_velx->width; ++x)
		{
			_data[_velx->width*y*5 + x*5 + 0] = cvGetReal2D(_nSum, y, x);
			_data[_velx->width*y*5 + x*5 + 1] = cvGetReal2D(_eSum, y, x);
			_data[_velx->width*y*5 + x*5 + 2] = cvGetReal2D(_sSum, y, x);
			_data[_velx->width*y*5 + x*5 + 3] = cvGetReal2D(_wSum, y, x);
			_data[_velx->width*y*5 + x*5 + 4] = cvGetReal2D(_zSum, y, x);
		}
	return _data;
}

void OpticalFlow::Align(CvRect face, IplImage *mask)
{
	IplImage *canvasx, *canvasy, *canvasz;

	CvPoint center = cvPoint(face.x + face.width/2, face.y + face.height/2);
	if(center.x < 0) center.x = 0;
	if(center.x > _velx->width) center.x = _velx->width;
	if(center.y < 0) center.y = 0;
	if(center.y > _velx->height) center.y = _velx->height;

	int radius = (face.width+face.height)/2;

	int canvas_width = max(_velx->width*2, radius * 16);
	int canvas_height = max(_velx->height*2, radius * 12);

	canvasx = cvCreateImage(cvSize(canvas_width, canvas_height), IPL_DEPTH_32F, 1);
	canvasy = cvCreateImage(cvSize(canvas_width, canvas_height), IPL_DEPTH_32F, 1);
	canvasz = cvCreateImage(cvSize(canvas_width, canvas_height), IPL_DEPTH_32F, 1);

	cvZero(canvasx); 
	cvZero(canvasy); 
	cvZero(canvasz);

	cvSetImageROI(canvasx, cvRect(canvasx->width/2-center.x, canvasx->height/2-center.y,
		_velx->width, _velx->height));
	cvSetImageROI(canvasy, cvRect(canvasx->width/2-center.x, canvasx->height/2-center.y,
		_velx->width, _velx->height));
	cvSetImageROI(canvasz, cvRect(canvasx->width/2-center.x, canvasx->height/2-center.y,
		_velx->width, _velx->height));

	cvCopy(_velx, canvasx, mask);
	cvCopy(_vely, canvasy, mask);
	cvCopy(_velz, canvasz, mask);

	cvReleaseImage(&_velx);
	cvReleaseImage(&_vely);
	cvReleaseImage(&_velz);

	cvSetImageROI(canvasx, cvRect(canvasx->width/2-radius*6, canvasx->height/2-radius*4.5, radius*12, radius*9));
	cvSetImageROI(canvasy, cvRect(canvasx->width/2-radius*6, canvasx->height/2-radius*4.5, radius*12, radius*9));
	cvSetImageROI(canvasz, cvRect(canvasx->width/2-radius*6, canvasx->height/2-radius*4.5, radius*12, radius*9));

	_velx = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);
	_vely = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);
	_velz = cvCreateImage(_outImageSize, IPL_DEPTH_32F, 1);

	cvResize(canvasx, _velx, CV_GAUSSIAN);
	cvResize(canvasy, _vely, CV_GAUSSIAN);
	cvResize(canvasz, _velz, CV_GAUSSIAN);

	cvReleaseImage(&canvasx);
	cvReleaseImage(&canvasy);
	cvReleaseImage(&canvasz);
}

void OpticalFlow::Normalize(IplImage *image)
{
	const double eps = 0.00000001;

	double norm = cvNorm(image, NULL, CV_L2);
	if(norm < eps) norm = eps;

	for(int row=0; row<image->height; ++row)
		for(int col=0; col<image->width; ++col)
			cvSetReal2D(image, row, col, cvGetReal2D(image, row, col) / norm);
}

void OpticalFlow::EqualizeHistogram(IplImage *image)
{
	double min, max;
	cvMinMaxLoc(image, &min, &max);
	for(int i=0; i<image->width*image->height; ++i)
	{
		image->imageData[i] = (image->imageData[i]-min)/max*10000;
	}


	/*
	IplImage *u8 = cvCreateImage(cvGetSize(image), IPL_DEPTH_8U, 1);
	double min, max;
	const double eps = 0.000001;
	cvMinMaxLoc(image, &min, &max);
	if(max<eps) max = eps;
	for(int i=0; i<image->width*image->height; ++i)
		u8->imageData[i] = (image->imageData[i]-min)/max*255;
	cvEqualizeHist(u8, u8);
	for(int i=0; i<image->width*image->height; ++i)
		image->imageData[i] = u8->imageData[i];
	cvReleaseImage(&u8);
	*/
}

void OpticalFlow::Smooth(IplImage *image)
{
	int type = CV_GAUSSIAN;
	int size = 3;

	cvSmooth(image, image, type, size, size);
}

void OpticalFlow::Split()
{
	static int i = 0;

	cvThreshold(_velx, _eFlow[i], 0, 0, CV_THRESH_TOZERO);
	cvThreshold(_velx, _wFlow[i], 0, 0, CV_THRESH_TOZERO_INV);
	cvAbs(_wFlow[i], _wFlow[i]);
	cvThreshold(_vely, _nFlow[i], 0, 0, CV_THRESH_TOZERO);
	cvThreshold(_vely, _sFlow[i], 0, 0, CV_THRESH_TOZERO_INV);
	cvAbs(_sFlow[i], _sFlow[i]);
	cvCopy(_velz, _zFlow[i]);

	if(++i >= N_FRAMES_SUM) i = 0;
}