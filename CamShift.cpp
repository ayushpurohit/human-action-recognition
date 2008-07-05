#include "CamShift.h"

CamShift::CamShift(CvSize imgSize)
{
	_backproject_mode = 0;
	_hdims = 16;
	_hranges_arr[0] = 0;
	_hranges_arr[1] = 180;
	_hranges = _hranges_arr;
	_vmin = 10;
	_vmax = 256;
	_smin = 30;

	_hsv = cvCreateImage( imgSize, IPL_DEPTH_8U, 3 );
	_hue = cvCreateImage( imgSize, IPL_DEPTH_8U, 1 );
	_mask = cvCreateImage( imgSize, IPL_DEPTH_8U, 1 );
	_backproject = cvCreateImage( imgSize, IPL_DEPTH_8U, 1 );
	_hist = cvCreateHist( 1, &_hdims, CV_HIST_ARRAY, &_hranges, 1 );
}

CamShift::~CamShift(void)
{
	cvReleaseImage(&_hsv);
	cvReleaseImage(&_hue);
	cvReleaseImage(&_mask);
	cvReleaseImage(&_backproject);
	cvReleaseHist(&_hist);
}


void CamShift::Track(IplImage *frame, CvRect &selection, bool calc_hist)
{
	int i, bin_w, c;

	cvCvtColor( frame, _hsv, CV_BGR2HSV );

	cvInRangeS( _hsv, cvScalar(0,_smin,MIN(_vmin,_vmax),0),
		cvScalar(180,256,MAX(_vmin,_vmax),0), _mask );
	cvSplit( _hsv, _hue, 0, 0, 0 );

	if(calc_hist)
	{
		float max_val = 0.f;
		cvSetImageROI( _hue, selection );
		cvSetImageROI( _mask, selection );
		cvCalcHist( &_hue, _hist, 0, _mask );
		cvGetMinMaxHistValue( _hist, 0, &max_val, 0, 0 );
		cvConvertScale( _hist->bins, _hist->bins, max_val ? 255. / max_val : 0., 0 );
		cvResetImageROI( _hue );
		cvResetImageROI( _mask );
		_track_window = selection; 
	}

	cvCalcBackProject( &_hue, _backproject, _hist );
	cvAnd( _backproject, _mask, _backproject, 0 );
	cvCamShift( _backproject, _track_window,
		cvTermCriteria( CV_TERMCRIT_EPS | CV_TERMCRIT_ITER, 10, 1 ),
		&_track_comp, &_track_box );
	_track_window = _track_comp.rect;

	if( frame->origin )
		_track_box.angle = -_track_box.angle;

	selection = cvRect(_track_box.center.x-_track_box.size.width/2, _track_box.center.y-_track_box.size.height/2,
		selection.width, selection.height);
}

CvScalar CamShift::_HSV2RGB( float _hue )
{
	int rgb[3], p, sector;
	static const int sector_data[][3] =
	{{0,2,1}, {1,2,0}, {1,0,2}, {2,0,1}, {2,1,0}, {0,1,2}};
	_hue *= 0.033333333333333333333333333333333f;
	sector = cvFloor(_hue);
	p = cvRound(255*(_hue - sector));
	p ^= sector & 1 ? 255 : 0;

	rgb[sector_data[sector][0]] = 255;
	rgb[sector_data[sector][1]] = 0;
	rgb[sector_data[sector][2]] = p;

	return cvScalar(rgb[2], rgb[1], rgb[0], 0);
}