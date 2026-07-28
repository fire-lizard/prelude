//texture
#ifndef ZSTEXTURE_H
#define ZSTEXTURE_H

#ifdef OPENGL
#include "opengl1.h"
#else
#define D3D_OVERLOADS
#include <ddraw.h>
#include <d3d.h>
#include <d3dx.h>

#endif
#define TEXTURE_NAME_LENGTH	32

class ZSTexture
{
private:
	static BOOL Reduced;
	LPDIRECTDRAWSURFACE7 Surface;
	int Width;
	int Height;
	char filename[TEXTURE_NAME_LENGTH];
	ZSTexture *pNext;
	ZSTexture *pPrev;
	BOOL Used;

public:
	ZSTexture *GetNext() { return pNext; }
	ZSTexture *GetPrev() { return pPrev; }
	BOOL GetUsed() { return Used; }

	void SetNext(ZSTexture *pNewNext) { pNext = pNewNext; }
	void SetPrev(ZSTexture *pNewPrev) { pPrev = pNewPrev; }

	void SetUsed() { Used = TRUE; } 
	void Release() { Used = FALSE; }
	void SetUnused() { Used = FALSE; }

	static void ToggleReduction() { if(Reduced) Reduced = FALSE; else Reduced = TRUE; }
	static void SetReduction(BOOL NewRedux) { Reduced = NewRedux; }
	static BOOL GetReduction() { return Reduced; }
	ZSTexture();

	ZSTexture(const char * filename, LPDIRECT3DDEVICE7 D3DDevice, LPDIRECTDRAW7 DirectDraw, DWORD *Mask, int w, int h);	
	ZSTexture(LPDIRECT3DDEVICE7 D3DDevice, int NewWidth, int NewHeight, DWORD *Mask = NULL);
	ZSTexture(LPDIRECTDRAWSURFACE7 FromSurface, LPDIRECT3DDEVICE7 D3DDevice, int NewWidth, int NewHeight);

	~ZSTexture();

	inline int GetWidth()
	{
		return Width;
	}

	inline int GetHeight()
	{
		return Height;
	}

	inline LPDIRECTDRAWSURFACE7 GetSurface()
	{
		return Surface;
	}

	inline void SetSurface(LPDIRECTDRAWSURFACE7 NewSurface)
	{
		Surface = NewSurface;
	}

	void Load(const char * fname, LPDIRECT3DDEVICE7 D3DDevice, LPDIRECTDRAW7 DirectDraw, DWORD *Mask, int w, int h);

	inline const char *GetName()
	{
		return filename;
	}

	friend class ZSEngine;

};

#endif