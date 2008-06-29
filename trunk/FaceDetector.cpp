#include "FaceDetector.h"
#include "stdafx.h"
#include <gl/gl.h>

FaceDetector::FaceDetector(CvSize imgSize, double scale, double scale_factor, int min_neighbours, int flags) 
	: _imgSize(imgSize), _scale(scale), _cs(imgSize), _scale_factor(scale_factor), _min_neighbours(min_neighbours),
	_flags(flags)
{
	_storage = cvCreateMemStorage(0);
	_cascade = (CvHaarClassifierCascade*)cvLoad( 
		"haarcascades/haarcascade_frontalface_alt2.xml", 
		0, 0, 0 );
	if(!_cascade) error("Could not load cascade");
	_tmpImg = cvCreateImage(_imgSize, IPL_DEPTH_8U, 1);
	_gsImg = cvCreateImage(cvSize(_imgSize.width/(int)_scale, _imgSize.height/(int)_scale), IPL_DEPTH_8U, 1);
	rect = cvRect(imgSize.width/2-imgSize.width/10, imgSize.height/2-imgSize.height/8, 
		imgSize.width/5, imgSize.height/4);
}

FaceDetector::~FaceDetector(void)
{
	cvReleaseMemStorage(&_storage);
	cvReleaseImage(&_tmpImg);
	cvReleaseImage(&_gsImg);
}

void FaceDetector::Detect(IplImage *img)
{
	static bool calc_hist = true;
	CvSeq* faces;
	CvRect *r;

	cvCvtColor( img, _tmpImg, CV_BGR2GRAY );
	cvResize( _tmpImg, _gsImg, CV_INTER_LINEAR );
	cvEqualizeHist( _gsImg, _gsImg );
	cvClearMemStorage( _storage );

	faces = cvHaarDetectObjects( _gsImg, _cascade, _storage,
		_scale_factor, _min_neighbours, _flags, cvSize(15/_scale, 15/_scale) );
	
	if(faces->total > 0)
	{
		r = (CvRect*)cvGetSeqElem( faces, 0 );
		rect = cvRect(r->x*(int)_scale, r->y*(int)_scale, r->width*(int)_scale, r->height*(int)_scale);
		calc_hist = true;
	}
	else
	{
		_cs.Track(img, rect, calc_hist);
		calc_hist = false;
	}
}

void FaceDetector::Draw()
{
	glColor3f(1,1,0);
	glLineWidth(2);
	glBegin(GL_LINE_LOOP);
		glVertex2i(rect.x, rect.y);
		glVertex2i(rect.x + rect.width, rect.y);
		glVertex2i(rect.x + rect.width, rect.y + rect.height);
		glVertex2i(rect.x, rect.y + rect.height);
	glEnd();
}