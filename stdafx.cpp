#include "stdafx.h"

void error(const char* msg) {
	cout << msg; getchar();
	exit(EXIT_FAILURE);
}

void Timer(int n)
{
	static double t[10];
	if(n%2==0) t[n/2] = glfwGetTime();
	else printf("%d) %.2fms\n", n/2, (glfwGetTime()-t[n/2])*1000);
}