#pragma once
#include <cv.h>
#include "CamShift.h"
#define N_FRAMES_AVG	4

class FaceDetector
{
public:
	FaceDetector(CvSize imgSize, double scale_image = 1.0, 
		double scale_factor = 1.1, int min_neighbours = 3, int flags = 0 );
	~FaceDetector(void);
	void Detect(IplImage *img);
	void Draw();

	CvRect rect;
	CvPoint center;
	double radius;
private:
	CvMemStorage* _storage;
	CvHaarClassifierCascade* _cascade;
	bool _calcHist;
	CvSize _imgSize;
	IplImage *_tmpImg, *_gsImg;
	const double _scale;
	CamShift _cs;
	double _scale_factor;
	int _min_neighbours, _flags;
	double _radius[N_FRAMES_AVG];

	void _UpdateLoc();
	double _Dist(double x, double y);
};
