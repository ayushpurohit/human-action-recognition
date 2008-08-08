#pragma once
#include <cv.h>

class CamShift
{
public:
	CamShift(CvSize frameSize);
	~CamShift(void);
	void Track(IplImage *frame, CvRect &selection, bool calc_hist);
private:
	CvScalar _HSV2RGB( float hue );

	IplImage *_hsv, *_hue, *_mask, *_backproject;
	CvHistogram *_hist;

	int _backproject_mode;
	CvPoint _origin;
	CvRect _track_window;
	CvBox2D _track_box;
	CvConnectedComp _track_comp;
	int _hdims;
	float _hranges_arr[2];
	float* _hranges;
	int _vmin, _vmax, _smin;
};
