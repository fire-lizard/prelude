#ifdef OPENGL
#include "opengl1.h"
#include "bmp.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

void debug_info(const char * str, ...);


int opengl_surface::Blt(LPRECT rect, _opengl_surface * src, LPRECT srcRect, DWORD flags, opengl_aux_dblt * blt_info) {

	int i,j;
	RECT area = { 0, };
	RECT sarea = { 0, };

	opengl_color_key src_color_key = { 0, };
	int use_src_key = 0;
	
	if (flags & DDBLT_KEYSRC) {
		use_src_key = 1;
		assert(src != NULL);
		src_color_key = src->color_key;		
	}
	
	if (use_src_key && src_color_key.dwColorSpaceHighValue != src_color_key.dwColorSpaceLowValue) {
		debug_info("bad range\n");
		exit(0);
	}
	
	if (rect) {
		area = *rect;

	}
	else {
		area.top = 0;
		area.left = 0;
		area.right = w;
		area.bottom = h;
	}

	if (srcRect) {
		sarea = *(srcRect);
	}
	else {
		if (!(flags & DDBLT_COLORFILL)) {
			sarea.top = 0;
			sarea.left = 0;
			sarea.right = src->w;
			sarea.bottom = src->h;
		}
	}	

	if (flags & DDBLT_COLORFILL) {
		i = area.top;

		for (; i < area.bottom; i++) {
			uint32 * row = &pixels[i*w];
			uint32 * l = row + area.left;
			while (l < row + area.right) {
				*l++ = blt_info->dwFillColor;
			}
		}
	}
	else {
		int sw = sarea.right - sarea.left;
		int sh = sarea.bottom - sarea.top;

		int dw = area.right - area.left;
		int dh = area.bottom - area.top;
	
		if ((sw == 0) || (sh == 0))
		{
			// Degenerate source rectangle, do nothing
			return 0;
		}
		int toScale = 0;
		if (sw != dw || dh != sh) {
			toScale = 1;
		}

		if (sarea.bottom > src->h) {
			sarea.bottom = src->h;
		}
		if (sarea.right > src->w) {
			sarea.right = src->w;
		}

		if (sarea.top < 0) {
			sarea.top = 0;
		}
		if (sarea.left <0) {
			sarea.left = 0;
		}

		if (area.bottom > h) {			
			area.bottom = h;
		}
		if (area.right > w) {
			area.right = w;
		}

		if (area.top < 0) {
			area.top = 0;
		}
		if (area.left <0) {
			area.left = 0;
		}

		if (area.right < area.left) {
			return 0;
		}

		sw = sarea.right - sarea.left;
		sh = sarea.bottom - sarea.top;

		dw = area.right - area.left;
		dh = area.bottom - area.top;

		if (toScale) {
			bmp_scale(src->pixels, sarea.right - sarea.left, sarea.bottom - sarea.top, sarea.left, sarea.top, src->w, src->h,
				pixels, area.right - area.left, area.bottom - area.top, area.left, area.top,w,h, use_src_key, src_color_key.dwColorSpaceLowValue);
		}
		else {
			i = area.top;
			j = sarea.top;
			
			
			//debug_info("dim %i %i %i %i, %i %i", w, h, src->w, src->h,i,area.bottom);
			for (; i < area.bottom; i++) {
				uint32 * row = &pixels[i*w];
				uint32 * l = row + area.left;
				uint32 * s = &src->pixels[j*src->w] + sarea.left;
				while (l < (row + area.right)) {
					if (use_src_key && src_color_key.dwColorSpaceLowValue == *s) {
						l++;
						s++;
					}
					else {
						*l++ = *s++;
					}
				}
				j++;
			}
		}
	}

	
	return 0;
}


int _opengl_surface::Release() {
	refcount--;

	if (watermark != 0x11223344) {
		debug_info("WARNING bad surface %X", watermark);
		return -1;
	}
	
	if (refcount == 0) delete this;
	return 0;
}

int _opengl_surface::Restore() {
	return 0;
}

int _opengl_surface::AddRef() {
	
	refcount++;
	return 0;
}

int _opengl_surface::Lock(LPRECT r, opengl_surface_desc2 * s, DWORD a, HANDLE b) {
	s->ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
	s->ddpfPixelFormat.dwRBitMask = 0xFF;
	s->ddpfPixelFormat.dwGBitMask = 0xFF00;
	s->ddpfPixelFormat.dwBBitMask = 0xFF0000;
	s->ddpfPixelFormat.dwRGBBitCount = 32;
	assert(pixels != NULL);
	s->lpSurface = pixels;
	s->lPitch = w*4;

	s->dwHeight = h;
	s->dwWidth = w;
	return 0;
}

int _opengl_surface::Unlock(void *) {
	return 0;
}

int _opengl_surface::SetColorKey(DWORD flags, opengl_color_key *c) {
	color_key = *c;
	return 0;
}

int _opengl_surface::ReleaseDC(HDC) {
	return 0;
}

int _opengl_surface::GetDC(HDC * out) {
	*(opengl_surface**)out = this;
	return 0;
}

int _opengl_surface::Flip(opengl_draw * draw, DWORD flags) {
	uint32 * p = bbuffer->pixels;
	bbuffer->pixels = pixels;
	pixels = p;
	char dbg[4196];
	dbg[0] = 0;
	int i, j;
	
	flush();
	
	return 0;
}

int _opengl_surface::GetPixelFormat(opengl_pixelformat * f) {
	f->dwRGBBitCount = 32;
	return 0;
}

int _opengl_surface::GetAttachedSurface(opengl_ddscaps * caps, _opengl_surface ** out) {
	if (draw == NULL) {
		return DDERR_NOTFOUND;
	}
	(*out) = new opengl_surface(this, draw);
	(*out)->buildSurface(draw->w, draw->h);

	if (caps->dwCaps & DDSCAPS_BACKBUFFER) {
		opengl_set_backbuffer(*out);
		bbuffer = *out;
		(*out)->bbuffer = this;
	}
	return 0;
}

int _opengl_surface::AddAttachedSurface(_opengl_surface *) {
	return 0;
}

int _opengl_draw::CreateSurface(opengl_surface_desc2 * d, opengl_surface ** out, void *) {
	opengl_surface * s = new opengl_surface(NULL, this);
	s->desc = d;
	*out = s;
	int iw = w;
	int ih = h;

	if (d->dwFlags & DDSD_HEIGHT) {
		ih = d->dwHeight;
	}

	if (d->dwFlags & DDSD_WIDTH) {
		iw = d->dwWidth;
	}

	s->buildSurface(iw, ih);
	if (d->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
		opengl_set_primary_surface(s);
	}
	return 0;
}

int _opengl_draw::CreateClipper(DWORD, opengl_clipper **, void *) {
	return 0;
}

int _opengl_draw::QueryInterface(DWORD iid, void * out) {
	switch (iid) {
	case IID_IDirect3D7:
		*(void**)out = new opengl_3d();
		return 0;
	}
	*(void**)out = new opengl_draw();
	return 0;
}

int _opengl_draw::SetCooperativeLevel(HWND wd, DWORD flags) {

	return 0;
}

int _opengl_draw::SetDisplayMode(DWORD iw, DWORD ih, DWORD ibpp, DWORD irr, DWORD flags) {
	w = iw;
	h = ih;
	bpp = ibpp;
	debug_info("draw display mode %i %i %i", iw, ih, ibpp);
	return 0;
}

int _opengl_draw::Release() {
	return 0;
}

/////////////////////////////////////////////////////////////////////////////

_opengl_surface::_opengl_surface(struct _opengl_surface * iref, opengl_draw * idraw) {
	hasTexture = 0;
	bbuffer = iref;
	draw = idraw;
	pixels = NULL;
	clipper = NULL;
	mipmaps_size = 0;
	w = draw->w;
	h = draw->h;
	memset(&color_key, 0, sizeof(color_key));
	watermark = 0x11223344;
	x = y = 0;
	refcount = 1;
	filename[0] = 0;
	
}

void opengl_surface::buildSurface(int iw, int ih) {
	w = iw;
	h = ih;
	
	pixels = new uint32[w*h];
	glGenTextures(1, &tex);
	hasTexture = 1;
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	

	glBindTexture(GL_TEXTURE_2D, 0);
}

int _opengl_surface::bltBitmap(const char * fname, int width, int height) {	
	delete[] pixels;
	pixels = bmp_load(fname, &w, &h);
	assert(pixels != NULL);
	strcpy(filename, fname);
	return 0;
	//this->Blt(&rect,)
}

int _opengl_surface::loadBitmap(const char * fname) {
	delete[] pixels;
	pixels = bmp_load(fname, &w, &h);
	assert(pixels != NULL);
	strcpy(filename, fname);
	return 0;
}

int _opengl_surface::createMipMaps(int number) {
	mipmaps_size = number;

	return 0;
}

void _opengl_surface::flush() {

	if (hasTexture) {
		glBindTexture(GL_TEXTURE_2D, tex);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	flush_surface(this);
}



#endif