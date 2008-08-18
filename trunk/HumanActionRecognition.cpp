#include "stdafx.h"
#include "FaceDetector.h"
#include "Texture.h"
#include "OpticalFlow.h"
#include "Classifier.h"
#include "glfont2/glfont2.h"

using namespace glfont;

void GLFWCALL windowResize(int width, int height)
{
	glViewport(0, 0, width, height);
}

void glInit(int width, int height)
{
	glfwInit();
	if(!glfwOpenWindow(width, height, 8, 8, 8, 8, 24, 0, GLFW_WINDOW))
		error("Could not open window");
	glfwEnable(GLFW_STICKY_KEYS);
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
	glfwSetWindowSizeCallback(windowResize);
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
	int frameWidth, frameHeight;
	int frameNum = 0;
	string winner;
	double spf, t3=0;
	const int windowWidth = 640, windowHeight = 480;
	
	
	GLFont font;
	if(!font.Create("glfont2/04b_03.glf"))
		error("Could not create font");
		

	// setup webcam
	if(!vi.setupDevice(device1, 320, 240))
		error("Could not initialize video input");
	frameWidth = vi.getWidth(device1);
	frameHeight = vi.getHeight(device1);
	
	// create images
	frame = cvCreateImage(cvSize(frameWidth, frameHeight), IPL_DEPTH_8U, 3);
	fgMask = cvCreateImage(cvSize(frameWidth, frameHeight), IPL_DEPTH_8U, 1);

	// initialize opengl/glfw
	glInit(windowWidth, windowHeight);

	// initialize face detector & optical flow
	FaceDetector fd(cvSize(frameWidth,frameHeight), 1.5, 1.2, 2, CV_HAAR_DO_CANNY_PRUNING);
	OpticalFlow of(cvSize(frameWidth,frameHeight), cvSize(80,60));

	Classifier classifier("shyp/5ppl-train.xml");

	Texture tex(frameWidth, frameHeight);
	
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
			spf = glfwGetTime() - t3;
			of.Calculate(frame, spf);
			t3 = glfwGetTime();

			of.Align(fd.center, fd.radius);
			of.Split();
			of.Finalize();
			
			if(frameNum >= 15) // wait for background subtraction to settle, and for frames to sum
			{
				cout << classifier.Classify(of.GetData()) << endl;
			}
			
			// draw
			tex.Load((unsigned char*)frame->imageData, GL_BGR);
			tex.Draw(0,0,windowWidth,windowHeight);
			fd.Draw(windowWidth, windowHeight);
			of.Draw(10,10,160,120);
			
			int i = 0;
			for(map<string, double>::const_iterator itr = classifier.votes.begin(); 
					itr != classifier.votes.end(); ++itr, ++i) 
			{
				float w = windowWidth/(float)classifier.votes.size();
				glColor3f(1,0,0);
				glBegin(GL_LINE_LOOP);
					glVertex2f(i*w,windowHeight);
					glVertex2f(i*w,windowHeight-itr->second*80);
					glVertex2f((i+1)*w,windowHeight-itr->second*80);
					glVertex2f((i+1)*w,windowHeight);
				glEnd();

				glColor3f(1,1,1);

				font.Begin();
					font.DrawString(itr->first, i*w, windowHeight);
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

int GetNumFrames(CvCapture *capture)
{
	double posFrames, numFrames;

	posFrames = cvGetCaptureProperty( capture, CV_CAP_PROP_POS_FRAMES );
	cvSetCaptureProperty( capture, CV_CAP_PROP_POS_AVI_RATIO, 1. );			
	numFrames = cvGetCaptureProperty( capture, CV_CAP_PROP_POS_FRAMES );	
	cvSetCaptureProperty( capture, CV_CAP_PROP_POS_FRAMES, posFrames );		

	return (int)numFrames;
}

double GetFrameRate(CvCapture *capture) {
	return cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
}

int GetFrameWidth(CvCapture *capture) {
	return (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH);
}

int GetFrameHeight(CvCapture *capture) {
	return (int)cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT);
}

int WriteData(vector<string> files, ofstream &fout)
{
	CvCapture *capture = 0;
	int numFrames;
	IplImage *frame, *frameCopy;
	int frameWidth, frameHeight;
	const int windowWidth = 640, windowHeight = 480;
	glInit(windowWidth,windowHeight);
	double spf, frameRate;

	for(int i=0; i<(int)files.size(); ++i)
	{
		// get label
		string filename = files[i].substr(files[i].find_last_of("/")+1);
		string label = filename.substr(0,filename.find_first_of("0123456789."));

		// open video and get parameters
		capture = cvCaptureFromFile(files[i].c_str());
		if(!capture) error("Could not open video");
		numFrames = GetNumFrames(capture);
		frameWidth = GetFrameWidth(capture);
		frameHeight = GetFrameHeight(capture);
		frameRate = GetFrameRate(capture);
		spf = 1./frameRate;

		// initialize other objects
		frame = cvQueryFrame(capture);
		Texture tex(frameWidth, frameHeight);
		frameCopy = cvCreateImage(cvGetSize(frame), frame->depth, frame->nChannels);
		FaceDetector fd(cvGetSize(frame));
		OpticalFlow of(cvGetSize(frame),cvSize(40,30));

		cout << "video " << i+1 << " of " << (int)files.size() << " : " << filename << " : " 
			<< frameWidth << "x" << frameHeight << "x" << numFrames << " @ " << frameRate << " FPS" << endl;

		for(int j=0; j<numFrames; ++j)
		{
			// copy and flip the frame (opencv returns it upside-down, and its glitchy unless copied)
			cvFlip(frame, frameCopy);
			
			// find face
			fd.Detect(frameCopy);
			
			// calc optical flow
			of.Calculate(frameCopy, spf);
			of.Align(fd.center, fd.radius);
			of.Split();
			of.Finalize();

			// write data (wait for optical flow to sum)
			if(j>15) {
				fout << label;
				of.Write(fout);
			}

			// draw
			tex.Load((unsigned char*)frameCopy->imageData, GL_BGR);
			tex.Draw(0,0,windowWidth,windowHeight);
			fd.Draw(windowWidth, windowHeight);
			of.Draw(10,10,160,120);
			glfwSwapBuffers();

			// grab the next frame (overwrites old frame)
			cvQueryFrame(capture);
		}

		cvReleaseImage(&frameCopy);
		cvReleaseCapture(&capture);
	}

	glfwTerminate();
	return EXIT_SUCCESS;
}

void WriteTrainingDataSet()
{
	vector<string> files;
	ofstream fout;
	string root;

	root = "D:/Documents and Settings/Mark/My Documents/Visual Studio 2008/Projects/FINAL-DATA/nick/";
	files.push_back(root+"punch-right.avi");
	files.push_back(root+"punch-left.avi");
	files.push_back(root+"wave-right.avi");
	files.push_back(root+"wave-left.avi");
	files.push_back(root+"waves.avi");
	files.push_back(root+"idle.avi");
	files.push_back(root+"chicken.avi");

	root = "D:/Documents and Settings/Mark/My Documents/Visual Studio 2008/Projects/FINAL-DATA/andrew/";
	files.push_back(root+"punch-right.avi");
	files.push_back(root+"punch-left.avi");
	files.push_back(root+"wave-right.avi");
	files.push_back(root+"wave-left.avi");
	files.push_back(root+"waves.avi");
	files.push_back(root+"idle.avi");
	files.push_back(root+"chicken.avi");

	root = "D:/Documents and Settings/Mark/My Documents/Visual Studio 2008/Projects/FINAL-DATA/andy/";
	files.push_back(root+"punch-right.avi");
	files.push_back(root+"punch-left.avi");
	files.push_back(root+"wave-right.avi");
	files.push_back(root+"wave-left.avi");
	files.push_back(root+"waves.avi");
	files.push_back(root+"idle.avi");
	files.push_back(root+"chicken.avi");

	root = "D:/Documents and Settings/Mark/My Documents/Visual Studio 2008/Projects/FINAL-DATA/mohammed/";
	files.push_back(root+"punch-right.avi");
	files.push_back(root+"punch-left.avi");
	files.push_back(root+"wave-right.avi");
	files.push_back(root+"wave-left.avi");
	files.push_back(root+"waves.avi");
	files.push_back(root+"chicken.avi");

	root = "D:/Documents and Settings/Mark/My Documents/Visual Studio 2008/Projects/FINAL-DATA/bronson/";
	files.push_back(root+"punch-right.avi");
	files.push_back(root+"punch-left.avi");
	files.push_back(root+"wave-right.avi");
	files.push_back(root+"wave-left.avi");
	files.push_back(root+"waves.avi");
	files.push_back(root+"idle.avi");
	files.push_back(root+"chicken.avi");

	fout.open("data/train");
	WriteData(files, fout);
	fout.close();
}

void WriteTestDataSet()
{
	vector<string> files;
	ofstream fout;
	string root;

	root = "D:/Documents and Settings/Mark/My Documents/Visual Studio 2008/Projects/DATA/train/mark/";
	files.push_back(root+"punch-right.avi");
	files.push_back(root+"punch-left.avi");
	files.push_back(root+"wave-right.avi");
	files.push_back(root+"wave-left.avi");
	files.push_back(root+"waves.avi");
	files.push_back(root+"idle.avi");
	files.push_back(root+"chicken.avi");

	fout.open("data/mark");
	WriteData(files, fout);
	fout.close();
}

void TrainTest()
{
	system("multiboost -train data/train 25 -shypname shyp/train.xml -verbose 2");
	system("multiboost -test data/test shyp/train.xml");
	getchar();
}

int _tmain(int argc, _TCHAR* argv[])
{
	//WriteTrainingDataSet();
	//WriteTestDataSet();
	//TrainTest();
	//RealTimeTest();	
	system("multiboost -test data/mark shyp/final3.xml");
	getchar();
}