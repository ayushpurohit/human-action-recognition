/* TODO: 
 *	- Multithread FaceDetector, OpticalFlow, Background Subtraction (does not help??)
 *	- Experiment with normalization of optical flow on and off and histogram equalization
 *  - ** Experiment with setting ROI within small region around previous frame for face detection (and camshift?)
 *	- Experiment with different values of gParams.n_gauss and smooth sizes on the foreground mask
 *  - Smooth _velx and _vely just before creating _velz
 *	*** If it's normalized... there can be no large values to differentiate idle from....chicken or something ---
 *		Normalize based on frame rate or something??? And RESOLUTION
 *	- Draw optical flow to texture and stretch?
 */
#include "stdafx.h"
#include "FaceDetector.h"
#include "Texture.h"
#include "OpticalFlow.h"
#include "Classifier.h"
#include "glfont2/glfont2.h"

using namespace glfont;

void glInit(int width, int height)
{
	glfwInit();
	if(!glfwOpenWindow(width, height, 8, 8, 8, 8, 24, 0, GLFW_WINDOW))
		error("Could not open window");
	glfwEnable( GLFW_STICKY_KEYS );
	glfwSetWindowTitle("Human Action Recognition");
	glViewport(0, 0, width, height);
	glClearColor(.5, .5, .5, 1);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, width, height, 0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void DrawIplImage(IplImage *image)
{
	cvFlip(image, image);

	GLenum format;
	if(image->nChannels > 1) format = GL_BGR;
	else format = GL_LUMINANCE;

	glDrawPixels(image->width, image->height, format, GL_UNSIGNED_BYTE, image->imageData);
}

int RealTimeTest()
{
	VideoInput vi;
	const int device1 = 0;
	IplImage *frame, *fgMask;
	char windowTitle[128];
	double t0 = 0, t1 = 0;
	int nFrames = 0;
	int width, height;
	int frameNum = 0;
	CvBGStatModel *bg_model;
	CvGaussBGStatModelParams gParams;
	string winner;

	// setup webcam
	if(!vi.setupDevice(device1, 320, 240))
		error("Could not initialize video input");
	width = vi.getWidth(device1);
	height = vi.getHeight(device1);
	
	// create images
	frame = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
	fgMask = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);

	// initialize opengl/glfw
	glInit(width, height);

	// initialize face detector & optical flow
	FaceDetector fd(cvSize(width,height), 1.5, 1.2, 2, CV_HAAR_DO_CANNY_PRUNING);
	OpticalFlow of(cvSize(width,height), cvSize(80,60));

	// initialize background subtraction
	gParams.win_size = 2;
	gParams.n_gauss = 5;
	gParams.bg_threshold = 0.7;
	gParams.std_threshold = 2.5;
	gParams.minArea = 15;
	gParams.weight_init = 0.05;
	gParams.variance_init = 30;
	
	// create bg stat model (for background subtraction)
	vi.getPixels(device1, (unsigned char*)frame->imageData, false, true);
	bg_model = cvCreateGaussianBGModel(frame, &gParams);

	Classifier classifier("shyp/bowl-5c-80x60.xml");

	GLFont font;
	if(!font.Create("glfont2/04b_03.glf",1))
		error("Could not create font");
	
	while(!glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam(GLFW_OPENED))
	{
		if(vi.isFrameNew(device1))
		{
			// compute fps
			t1 = glfwGetTime();
			if(t1 - t0 > 1) {
				sprintf_s(windowTitle, "FPS: %.2f", (double)nFrames/(t1-t0));
				glfwSetWindowTitle(windowTitle);
				nFrames = 0;
				t0 = glfwGetTime();
			}
			++nFrames;

			// get frame from cam
			vi.getPixels(device1, (unsigned char*)frame->imageData, false, true);
			
			
			// detect & track face
			fd.Detect(frame);
			

			// compute optical flow
			of.Calculate(frame);

			// update foreground mask
			//cvUpdateBGStatModel(frame, bg_model);
			//cvSmooth(bg_model->foreground, fgMask, CV_BLUR, 5, 5);

			of.Align(fd.center, fd.radius, NULL);
			of.Split();
			
			if(frameNum >= 15) // wait for background subtraction to settle, and for frames to sum
			{
				of.Finalize();
				cout << classifier.Classify(of.GetData()) << endl;
			}
			
			// draw
			glClear(GL_COLOR_BUFFER_BIT);
			DrawIplImage(frame);
			fd.Draw();
			
			int i = 0;
			for(map<string, double>::const_iterator itr = classifier.votes.begin(); 
					itr != classifier.votes.end(); ++itr, ++i) 
			{
				float w = width/(float)classifier.votes.size();
				glColor3f(1,0,0);
				glBegin(GL_LINE_LOOP);
					glVertex2f(i*w,height);
					glVertex2f(i*w,height-itr->second*50);
					glVertex2f((i+1)*w,height-itr->second*50);
					glVertex2f((i+1)*w,height);
				glEnd();

				glColor3f(1,1,1);

				font.Begin();
					font.DrawString(itr->first, i*w, height);
				font.End();
			}

			glfwSwapBuffers();

			++frameNum;
		}
	}

	// clean up
	glfwTerminate();
	cvReleaseImage(&frame);
	vi.stopDevice(device1);

	return EXIT_SUCCESS;
}

int WriteData(vector<string> files, ofstream &fout)
{
	CvCapture *capture = 0;
	IplImage *frame, *fgMask;
	int numFrames;
	int width = 320, height = 240;
	CvBGStatModel *bg_model;
	CvGaussBGStatModelParams gParams;
	string label;
	double t0 = 0, t1;
	int fpsFrames;
	char windowTitle[80];
	
	// create images
	frame = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
	fgMask = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);

	// initialize face detector & optical flow
	FaceDetector fd(cvSize(width,height), 1.0, 1.1, 3, 0);
	OpticalFlow of(cvSize(width,height), cvSize(80,60));

	// initialize background subtraction
	gParams.win_size = 2;			// default: 2
	gParams.n_gauss = 5;			// default: 5
	gParams.bg_threshold = 0.7;		// default: 0.7
	gParams.std_threshold = 7.5;	// default: 2.5
	gParams.minArea = 15;			// default: 15
	gParams.weight_init = 0.05;		// default: 0.05
	gParams.variance_init = 30;		// default: 30

	// initialize opengl/glfw
	Texture tex(320,240);
	glInit(80, 60);

	for(int i=0; i<(int)files.size(); ++i)
	{
		string filename = files[i].substr(files[i].find_last_of("/")+1);
		label = filename.substr(0,filename.find_first_of("0123456789."));

		cout << "Processing video " << i+1 << " of " << files.size() << " (" << filename << ")" << endl;
		capture = cvCaptureFromFile(files[i].c_str());

		cvSetCaptureProperty( capture, CV_CAP_PROP_POS_AVI_RATIO, 1. );
		numFrames = (int) cvGetCaptureProperty( capture, CV_CAP_PROP_POS_FRAMES );
		cvSetCaptureProperty( capture, CV_CAP_PROP_POS_FRAMES, 0. );

		frame = cvQueryFrame(capture);
		cvFlip(frame);
		bg_model = cvCreateGaussianBGModel(frame, &gParams);

		for(int j=1; j<numFrames; ++j)
		{
			if(j>3)
			{
				if(glfwGetKey('N')==GLFW_PRESS) break;
				if(glfwGetKey('B')==GLFW_PRESS && i>0) {
					i-=2; break;
				}
			}

			// compute fps
			t1 = glfwGetTime();
			if(t1 - t0 > 1) {
				sprintf_s(windowTitle, "%s - FPS: %.2f", label.c_str(), (double)fpsFrames/(t1-t0));
				glfwSetWindowTitle(windowTitle);
				fpsFrames = 0;
				t0 = glfwGetTime();
			}
			++fpsFrames;
			
			// grab the next frame
			frame = cvQueryFrame(capture);
			cvFlip(frame);

			// detect & track face
			fd.Detect(frame);

			// create foreground mask
			cvUpdateBGStatModel(frame, bg_model); 
			cvSmooth(bg_model->foreground, fgMask, CV_BLUR, 3, 3);

			// compute optical flow
			of.Calculate(frame);
			of.Align(fd.center, fd.radius, fgMask);
			of.Split();

			of.Finalize();
			
			if(j >= 20) // wait for background subtraction to settle, and for frames to sum
			{				
				fout << label;
				of.Write(fout);

				cout << "  Writing frame " << j+1 << " of " << numFrames << endl;
			}
			else {
				cout << "  Processing frame " << j+1 << " of " << numFrames << endl;
			}
			
			glClear(GL_COLOR_BUFFER_BIT);
			of.Draw();
			
			//tex.Draw(0,0,80,60);
			//DrawIplImage(frame);
			//fd.Draw();
			glfwSwapBuffers();
		}

		cvReleaseBGStatModel(&bg_model);
		cvReleaseCapture(&capture);
	}

	glfwTerminate();
	cvReleaseImage(&fgMask);

	return EXIT_SUCCESS;
}

void Train()
{
	vector<string> files;
	ofstream fout;
	string root;
	
	// train data
	fout.open("data/temp");
	root = "C:/Documents and Settings/Mark/My Documents/Visual Studio 2008/Projects/DATA/train/bowl/";
	files.push_back(root+"wave02.avi");
	files.push_back(root+"wave-left02.avi");
	files.push_back(root+"punch-right02.avi");
	files.push_back("C:/Documents and Settings/Mark/My Documents/Visual Studio 2008/Projects/DATA/test/mohammed/idle02.avi");
	files.push_back("C:/Documents and Settings/Mark/My Documents/Visual Studio 2008/Projects/DATA/train/brown/idle01.avi");
	files.push_back(root+"chicken02.avi");
	files.push_back(root+"clap02.avi");
	files.push_back(root+"clap03.avi");
	files.push_back(root+"punch-left02.avi");
	files.push_back(root+"punch-left03.avi");
	files.push_back(root+"punch-right03.avi");

	files.push_back(root+"wave-right02.avi");
	WriteData(files, fout);
	fout.close();

	/*
	// test data
	fout.open("eqhist_pj");
	files.clear();
	root = "C:/Documents and Settings/Mark/My Documents/Visual Studio 2008/Projects/DATA2/";
	files.push_back(root+"clap02.avi");
	files.push_back(root+"punch-left02.avi");
	files.push_back(root+"punch-right01.avi");
	files.push_back(root+"chicken02.avi");
	files.push_back(root+"wave02.avi");
	files.push_back(root+"wave-left02.avi");
	files.push_back(root+"wave-right02.avi");
	WriteData(files, fout);
	fout.close();
	*/
}

int _tmain(int argc, _TCHAR* argv[])
{
	//Train();
	RealTimeTest();	
}