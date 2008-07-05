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

	for(int i=0; i<4; ++i)
		_radius[i] = 30;
}

FaceDetector::~FaceDetector(void)
{
	cvReleaseMemStorage(&_storage);
	cvReleaseImage(&_tmpImg);
	cvReleaseImage(&_gsImg);
}

void FaceDetector::_UpdateLoc()
{
	static int ri = 0;
	static CvPoint lastCenter;
	static double lastRadius;
	static bool firstRun = true;
	static int count = 0;

	_radius[ri++] = (rect.width+rect.height)/4.;
	if(ri>=N_FRAMES_AVG) ri = 0;

	radius = _radius[0];
	for(int i=1; i<N_FRAMES_AVG; ++i)
		radius += _radius[i];
	radius /= (double)N_FRAMES_AVG;

	center.x = rect.x+rect.width/2;
	center.y = rect.y+rect.height/2;

	// assert parameters are reasonable
	if(center.x<0)center.x=0;												// your face must be in view
	if(center.x>_tmpImg->width)center.x=_tmpImg->width;
	if(center.y<0)center.y=0;
	if(center.y>_tmpImg->height)center.y=_tmpImg->height;
	if(radius<1)radius=1;
	if(radius>_tmpImg->width/2) radius=_tmpImg->width/2;
	if(	!firstRun															// if this is not the first frame
		&& count < 3														// and the face detector is not stuck
		&& (_Dist(center.x-lastCenter.x,center.y-lastCenter.y)>radius*2		// and (your face teleported
		|| abs(radius-lastRadius)>lastRadius/1.5)) {						// or your face exploded)
		center = lastCenter;
		radius = lastRadius;
		++count;
	} else count = 0;
	lastCenter = center;
	lastRadius = radius;
	firstRun = false;
}

double FaceDetector::_Dist(double x, double y) {
	return sqrt(x*x+y*y);
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
	_UpdateLoc();
}

void FaceDetector::Draw()
{
	
	glColor3f(1,1,0);
	glLineWidth(2);
	/*
	glBegin(GL_LINE_LOOP);
		glVertex2i(rect.x, rect.y);
		glVertex2i(rect.x + rect.width, rect.y);
		glVertex2i(rect.x + rect.width, rect.y + rect.height);
		glVertex2i(rect.x, rect.y + rect.height);
	glEnd();
	*/

	static const double pi = 3.141592653589793238;

	glBegin(GL_LINE_LOOP);
	for(float i=0; i<2*pi; i+=pi/8)
		glVertex2f(cos(i)*radius+center.x, sin(i)*radius+center.y);
	glEnd();
}