#include "OpticalFlow.h"
#include <GL/glfw.h>
#include <iostream>

OpticalFlow::OpticalFlow(CvSize frameSize, CvSize finalSize) : _tex(finalSize.width, finalSize.height), _lambda(.000005)
{
	_finalSize = finalSize;
	_velx = 0;
	_vely = 0;
	_velz = 0;
	_tmp = cvCreateImage(frameSize, IPL_DEPTH_32F, 1);
	_gray1 = cvCreateImage(frameSize, IPL_DEPTH_8U, 1);
	_gray2 = cvCreateImage(frameSize, IPL_DEPTH_8U, 1);
	_nSum = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
	_sSum = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
	_wSum = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
	_eSum = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
	_zSum = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
	_vis = cvCreateImage(_finalSize, IPL_DEPTH_8U, 3);
	cvZero(_gray1);
	cvZero(_gray2);

	for(int i=0; i<N_FRAMES_SUM; ++i) {
		_nFlow[i] = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
		_eFlow[i] = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
		_sFlow[i] = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
		_wFlow[i] = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
		_zFlow[i] = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
	}

	_data = new double[_finalSize.width * _finalSize.height * 5];
	_criteria = cvTermCriteria(CV_TERMCRIT_ITER | CV_TERMCRIT_EPS, 20, .3);
}

OpticalFlow::~OpticalFlow(void)
{
	cvReleaseImage(&_velx);
	cvReleaseImage(&_vely);
	cvReleaseImage(&_velz);
	cvReleaseImage(&_tmp);
	cvReleaseImage(&_gray1);
	cvReleaseImage(&_gray2);
	cvReleaseImage(&_vis);
	cvReleaseImage(&_nSum);
	cvReleaseImage(&_eSum);
	cvReleaseImage(&_sSum);
	cvReleaseImage(&_wSum);
	cvReleaseImage(&_zSum);

	for(int i=0; i<N_FRAMES_SUM; ++i) {
		cvReleaseImage(&_nFlow[i]);
		cvReleaseImage(&_eFlow[i]);
		cvReleaseImage(&_sFlow[i]);
		cvReleaseImage(&_wFlow[i]);
		cvReleaseImage(&_zFlow[i]);
	}

	delete _data;
}

void OpticalFlow::Calculate(IplImage *frame, double mspf)
{
	// release and re-allocate velocities (necessary because they are resized later)
	cvReleaseImage(&_velx);
	cvReleaseImage(&_vely);
	cvReleaseImage(&_velz);

	_velx = cvCreateImage(cvGetSize(frame), IPL_DEPTH_32F, 1);
	_vely = cvCreateImage(cvGetSize(frame), IPL_DEPTH_32F, 1);
	_velz = cvCreateImage(cvGetSize(frame), IPL_DEPTH_32F, 1);

	// compute optical flow
	cvCvtColor( frame, _gray2, CV_BGR2GRAY );
	cvCalcOpticalFlowHS(_gray1, _gray2, 0, _velx, _vely, _lambda, _criteria);
	//cvCalcOpticalFlowLK(_gray1, _gray2, cvSize(3,3), _velx, _vely);
	cvCvtColor( frame, _gray1, CV_BGR2GRAY );

	// normalize wrt fps and frame size
	if(mspf<=0) mspf = 0.0001;
	double mul = 1000000000./mspf; // big num simply prevents really tiny numbers, but is arbitrary
	cvConvertScale(_velx, _velx, mul/(double)frame->width);
	cvConvertScale(_vely, _vely, mul/(double)frame->height);

	// compute "zero" flow = sqrt(velx^2+vely^2)
	cvMul(_velx, _velx, _velz);
	cvMul(_vely, _vely, _tmp);
	cvAdd(_tmp, _velz, _velz);
	cvPow(_velz, _velz, 0.5);
}

void OpticalFlow::Draw(int x, int y, int w, int h)
{
	for(int y=0; y<_velx->height; ++y)
	{
		for(int x=0; x<_velx->width; ++x)
		{
			double vx = cvGetReal2D(_nSum, y, x); // north = red
			double vy = cvGetReal2D(_eSum, y, x); // east = green
			double vz = cvGetReal2D(_zSum, y, x); // zero = blue
			cvSet2D(_vis, y, x, CV_RGB(vx,vy,vz));
		}
	}
	_tex.Load((unsigned char*)_vis->imageData, GL_BGR);
	_tex.Draw(x,y,w,h);
}

void OpticalFlow::Finalize()
{
	// sum the optical flow over N frames
	// doing it this way avoids having to zero the sums first
	// but means N must be >= 2
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

	// blur the summed optical flows
	Smooth(_nSum);
	Smooth(_eSum);
	Smooth(_sSum);
	Smooth(_wSum);
	Smooth(_zSum);
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

void OpticalFlow::Align(CvPoint center, double radius, IplImage *mask)
{
	IplImage *canvasx, *canvasy, *canvasz;

	// ensure face is within frame
	if(center.x < 0) center.x = 0;
	if(center.x > _velx->width) center.x = _velx->width;
	if(center.y < 0) center.y = 0;
	if(center.y > _velx->height) center.y = _velx->height;

	// canvas must be at least twice the frame size for centering
	// we make it larger depending on face size so that it is scale-invariant
	int canvas_width = max(_velx->width*2, (int)radius*32);
	int canvas_height = max(_velx->height*2, (int)radius*24);

	canvasx = cvCreateImage(cvSize(canvas_width, canvas_height), IPL_DEPTH_32F, 1);
	canvasy = cvCreateImage(cvSize(canvas_width, canvas_height), IPL_DEPTH_32F, 1);
	canvasz = cvCreateImage(cvSize(canvas_width, canvas_height), IPL_DEPTH_32F, 1);

	cvZero(canvasx); 
	cvZero(canvasy); 
	cvZero(canvasz);

	// copy the opical flows to the canvas so that the face is centered
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

	// crop the canvas as reasonable distance around the face
	cvSetImageROI(canvasx, cvRect(canvasx->width/2-(int)radius*12, canvasx->height/2-(int)radius*8, (int)radius*24, (int)radius*18));
	cvSetImageROI(canvasy, cvRect(canvasx->width/2-(int)radius*12, canvasx->height/2-(int)radius*8, (int)radius*24, (int)radius*18));
	cvSetImageROI(canvasz, cvRect(canvasx->width/2-(int)radius*12, canvasx->height/2-(int)radius*8, (int)radius*24, (int)radius*18));

	// shrink the output image to create larger optical flow bins and so that there is less data to process
	_velx = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
	_vely = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);
	_velz = cvCreateImage(_finalSize, IPL_DEPTH_32F, 1);

	cvResize(canvasx, _velx, CV_GAUSSIAN);
	cvResize(canvasy, _vely, CV_GAUSSIAN);
	cvResize(canvasz, _velz, CV_GAUSSIAN);

	cvReleaseImage(&canvasx);
	cvReleaseImage(&canvasy);
	cvReleaseImage(&canvasz);
}

void OpticalFlow::Normalize(IplImage *image)
{
	double norm = cvNorm(image, NULL, CV_L2);
	norm += 0.0005; // avoid division by zero
	cvConvertScale(image, image, 1./norm); // TODO: would taking the log help prevent very small numbers?
}

void OpticalFlow::Smooth(IplImage *image)
{
	int type = CV_GAUSSIAN;
	int size = 5;

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