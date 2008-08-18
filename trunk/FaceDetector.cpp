#include "FaceDetector.h"
#include "stdafx.h"
#include <gl/gl.h>

FaceDetector::FaceDetector(CvSize frameSize, double scale, double scale_factor, int min_neighbours, int flags) 
	: _frameSize(frameSize), _scale(scale), _cs(frameSize), _scale_factor(scale_factor), _min_neighbours(min_neighbours),
	_flags(flags)
{
	_storage = cvCreateMemStorage(0);
	_cascade = (CvHaarClassifierCascade*)cvLoad( 
		"haarcascades/haarcascade_frontalface_alt2.xml", 
		0, 0, 0 );
	if(!_cascade) error("Could not load cascade");
	_tmpImg = cvCreateImage(_frameSize, IPL_DEPTH_8U, 1);
	_gsImg = cvCreateImage(cvSize(_frameSize.width/(int)_scale, _frameSize.height/(int)_scale), IPL_DEPTH_8U, 1);
	rect = cvRect(frameSize.width/2-frameSize.width/10, frameSize.height/2-frameSize.height/8, 
		frameSize.width/5, frameSize.height/4);

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

	// ensure parameters are reasonable
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
		center = lastCenter;												// then don't update loc (prevents short frame glitchs)
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

void FaceDetector::Draw(int windowWidth, int windowHeight)
{
	static const double pi = 3.141592653589793238;
	
	double scale_x = windowWidth/_frameSize.width;
	double scale_y = windowHeight/_frameSize.height;

	glColor3f(1,1,0);
	glLineWidth(2);

	glBegin(GL_LINE_LOOP);
	for(float i=0; i<2*pi; i+=pi/8)
		glVertex2f((cos(i)*radius+center.x)*scale_x, (sin(i)*radius+center.y)*scale_y);
	glEnd();
}