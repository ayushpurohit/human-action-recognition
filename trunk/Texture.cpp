#include "Texture.h"

Texture::Texture(int w, int h) : _w(w), _h(h)
{
	int tw = _NextPow2(w);
	int th = _NextPow2(h);

	_s = (float)w/tw;
	_t = (float)h/th;

	glGenTextures(1, &_texID);

	glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _texID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glDisable(GL_TEXTURE_2D);
}

Texture::~Texture(void)
{
	glDeleteTextures(1, &_texID);
}

void Texture::Load(unsigned char* data, GLenum format)
{
	glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _texID);
 		glTexSubImage2D(GL_TEXTURE_2D,0,0,0,_w,_h,format,GL_UNSIGNED_BYTE,data);	
	glDisable(GL_TEXTURE_2D);
}

void Texture::Draw(int x, int y, int w, int h)
{
	if(w == 0) w = _w;
	if(h == 0) h = _h;

	glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _texID);
		int px0 = x;
		int py0 = y;
		int px1 = x+w;
		int py1 = y+h;
		glBegin(GL_QUADS);
			glTexCoord2f(0,0);		glVertex2i(px0, py0);
			glTexCoord2f(_s,0);		glVertex2i(px1, py0);
			glTexCoord2f(_s,_t);	glVertex2i(px1, py1);
			glTexCoord2f(0,_t);		glVertex2i(px0, py1);
		glEnd();
	glDisable(GL_TEXTURE_2D);
}

unsigned Texture::_NextPow2(unsigned n)
{
	const int mantissaMask = (1<<23) - 1;

	(*(float*)&n) = (float) n;
	n = n + mantissaMask & ~mantissaMask;
	n = (unsigned) (*(float*)&n);

	return n;
} 