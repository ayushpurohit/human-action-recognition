// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "VideoInput.h"
#include <cv.h>
#include <highgui.h>
#include <cvaux.h>
#include <vector>
#include <string>
#include <map>
#include <GL/glfw.h>
#include "GL/glext.h"
//#include <boost/thread/thread.hpp>
//#include <boost/bind.hpp>

using namespace std;
//using namespace boost;

void error(const char* msg);
void Timer(int n);