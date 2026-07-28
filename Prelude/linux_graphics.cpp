#include "linux_aux_wrapper.h"
#include "opengl1.h"
#include "bmp.h"

#ifdef __linux__

int StretchDIBits(HDC hdc, int xDest, int yDest, int DestWidth, int DestHeight, int xSrc, int ySrc, int SrcWidth, int SrcHeight,
	void * lpBits, BITMAPINFO * lpbmi, UINT iUsage, DWORD rop) {

	bmp_header * header = (bmp_header*)lpbmi;
	
	int w, h;
	w = h = 0;
	uint32* data = bmp_load_mm((byte*)lpBits, header - 1, &w, &h);

	opengl_surface * srcs = new opengl_surface();
	srcs->pixels = data;
	srcs->w = w;
	srcs->h = h;

	opengl_surface * dests = (opengl_surface *)hdc;
	RECT dest;
	dest.left = xDest;
	dest.right = xDest + DestWidth;
	dest.top = yDest;
	dest.bottom = yDest + DestHeight;

	RECT src;
	src.left = xSrc;
	src.right = xSrc + SrcWidth;
	src.top = ySrc;
	src.bottom = ySrc + SrcHeight;

	dests->Blt(&dest, srcs, &src, 0, 0);

	return 0;
}

int MessageBox(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
	printf("This should be a message box: %s\n", lpText);
	return 0;
}

HANDLE LoadImage(HINSTANCE hInst, LPCSTR name, UINT type, int cx, int cy, UINT fuLoad) {
	opengl_surface * surface = new opengl_surface();
	surface->loadBitmap((char*)name);
	return (HANDLE)surface;
}

HDC CreateCompatibleDC(HDC h) {
	return new opengl_surface();
}

void * SelectObject(HDC hdc, void* h) {
	opengl_surface * d = (opengl_surface *)hdc;
	opengl_surface * s = (opengl_surface *)h;

	d->w = s->w;
	d->h = s->h;
	d->pixels = s->pixels;
	s->pixels = NULL;
	return h;
}

BOOL DeleteDC(HDC hdc) {
	delete (opengl_surface*)hdc;
	return 1;
}

BOOL DeleteObject(void* ho) {
	opengl_surface *o = (opengl_surface*)ho;
	delete o;
	return 1;
}

int GetObject(HANDLE h, int c, LPVOID pv) {
	return 0;
}

BOOL BitBlt(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop) {
	opengl_surface * dests = (opengl_surface*)hdc;

	opengl_surface * srcs = (opengl_surface*)hdcSrc;

	RECT dest;
	dest.left = x;
	dest.right = x + cx;
	dest.top = y;
	dest.bottom = y + cy;

	if (x1 != 0 || y1 != 0) {
		printf("unimplemented BitBlt\n");
		exit(0);
	}

	RECT src;
	dest.left = x1;
	dest.right = srcs->w;
	dest.top = y1;
	dest.bottom = srcs->h;

	dests->Blt(&dest, srcs, &src, 0, 0);

	return 0;
}

BOOL DestroyWindow(HWND hWnd) {
	return 1;
}



#endif