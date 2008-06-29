#pragma once
#include "stdafx.h"

class Texture
{
public:
	Texture(int width, int height);
	~Texture(void);
	void Load(unsigned char* data, GLenum format = GL_BGR);
	void Draw(int x = 0, int y = 0, int w = 0, int h = 0);
private:
	unsigned _NextPow2(unsigned n);
	GLuint _texID;
	int _w, _h;
	float _s, _t;
};
