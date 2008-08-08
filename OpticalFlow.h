#pragma once
#include "stdafx.h"
#include "Texture.h"
#define N_FRAMES_SUM 12

class OpticalFlow
{
public:
	OpticalFlow(CvSize frameSize, CvSize finalSize = cvSize(80,60));
	~OpticalFlow(void);
	void Calculate(IplImage *frame, double mspf=41.72);
	void Draw(int x=0, int y=0, int w=0, int h=0);
	void Write(ofstream &fout);
	void Align(CvPoint center, double radius, IplImage *mask = NULL);
	void Normalize(IplImage *image);
	void Smooth(IplImage *image);
	void Split();
	void Finalize();
	double* GetData();
public:
	IplImage *_velx, *_vely, *_velz, *_tmp, *_gray1, *_gray2, *_vis;
	CvSize _finalSize;
	IplImage *_nFlow[N_FRAMES_SUM], *_eFlow[N_FRAMES_SUM], *_sFlow[N_FRAMES_SUM], 
		*_wFlow[N_FRAMES_SUM], *_zFlow[N_FRAMES_SUM];
	IplImage *_nSum, *_eSum, *_sSum, *_wSum, *_zSum;
	double *_data;
	Texture _tex;
	const double _lambda;
	CvTermCriteria _criteria;
};
