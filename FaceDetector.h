#pragma once
#include <cv.h>
#include "CamShift.h"

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
};
