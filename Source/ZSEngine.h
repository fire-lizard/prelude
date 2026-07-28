//*********************************************************************
//*                                                                   *                                                                **
//**************              ZSEngine.h           *********************
//**                                                                  *                                                               **
//**                                                                  *                                                               **
//*********************************************************************
//*                                                                   *                                                                  *
//*Revision:    Sept 26, 2000			                                  *  
//*Author:      Mat Williams            
//*Purpose:   Class which encapsulates Graphics, sound and input routines   *
//*	Manages Objects and textures
//*
//*********************************************************************
//*Outstanding issues:                                                   
//				Not fully tested                                          *
//*********************************************************************
//*********************************************************************
#ifndef ZSENGINE_H
#define ZSENGINE_H

#include "defs.h"
#include "ZSinput.h"		//input system
#include "ZSGraphics.h" //graphic system
#include <math.h>
#include <assert.h>
#include <stdio.h>
#include "ZSsound.h"		//sound system

#include "CustomOptions.h"

class ZSModelEx;
class ZSTexture;

#define MAX_MESH	768

class ZSEngine
{
private:
	char RootDirectory[256];	//the starting directory of the application
		
	ZSSoundSystem		 ZSSound;	

	ZSInputSystem		 ZSInput;
	
	ZSGraphicsSystem   ZSGraphics;

	//3d meshes
	int NumMesh;
	ZSModelEx *MeshList[MAX_MESH];


	//Textures
	int NumTextures;
	ZSTexture *TextureList;

	

public:

	int Init(HINSTANCE hInstance); //startup routine
	
	void ReportError(const char *text);	
	void ReportError(HRESULT ddrval);

	inline ZSModelEx* GetMesh(int num) { if (num == -1) return NULL; PTD_ASSERT(num >= 0); PTD_ASSERT(num < MAX_MESH); return MeshList[num]; }
	ZSModelEx *GetMesh(const char *pMeshName);
	int GetMeshNum(const char *pMeshName);
	int GetMeshNum(ZSModelEx *pMesh);
	void SetMesh(int num, ZSModelEx *pNewMesh) { PTD_ASSERT(num >= 0); PTD_ASSERT(num < MAX_MESH); MeshList[num] = pNewMesh; };
	void DeleteMesh(int Num);
	inline int GetNumMesh() { return NumMesh; }	
	
	void LoadMeshes();
	void LoadMeshes(const char *FileName);
	void SaveMeshes(const char *FileName);
	void ImportMeshes();
	void Import(int Num);

	inline ZSTexture *GetTexture(int num) { if (num == -1) return NULL; PTD_ASSERT(num >= 0); PTD_ASSERT(num < NumTextures); return &TextureList[num]; }
	int GetTextureNum(const char *pTextureName);
	int GetTextureNum(ZSTexture *pTexture);
	inline int GetNumTextures() { return NumTextures; }
	

	ZSTexture *GetTexture(const char *pTextureName);
	void LoadTextures();

	inline ZSSoundSystem		 *Sound() { return &ZSSound;	}
	inline ZSInputSystem		 *Input() { return &ZSInput;	}
	inline ZSGraphicsSystem  *Graphics(){ return &ZSGraphics;}

	char *GetRootDirectory() { return RootDirectory; }

	CustomOptions customOptions;

	ZSEngine();
	
	~ZSEngine();

};


//global variable to expose interface to rest of application
extern ZSEngine *Engine;

#endif
