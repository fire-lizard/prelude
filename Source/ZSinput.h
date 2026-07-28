//*********************************************************************
//*                                                                   *                                                                **
//**************              ZSInput.h           *********************
//**                                                                  *                                                               **
//**                                                                  *                                                               **
//*********************************************************************
//*                                                                   *                                                                  *
//*Revision:    Oct 7, 2000			                                  *  
//*Author:      Mat Williams            
//*Purpose:                                                           *
//*	provide an os independent input manager
//*
//*********************************************************************
//*Outstanding issues:                                                                                                      *
//		
//
//*********************************************************************
//*********************************************************************
#ifndef ZSINPUT_H
#define ZSINPUT_H

#ifdef OPENGL
#include "opengl1.h"
#else
#include <dinput.h>
#endif

class ZSWindow;
class BirdEye;

class ZSInputSystem
{
private:
	
	BirdEye * birdEye;
	//directx interface
	LPDIRECTINPUT7	DirectInput;
	
	//mouse device
	LPDIRECTINPUTDEVICE7 Mouse; 
	
	//mouse state
	DIMOUSESTATE MouseState;
	
	//Booleans to maintain what messages have been handled
	BOOL LeftButtonDownHandled;
	BOOL LeftButtonUpHandled;
	BOOL RightButtonDownHandled;
	BOOL RightButtonUpHandled;

	//the mouse screen rectangle.
	RECT rMouseScreen;
	
	//the resolution
	int MouseResolution;

	//keyboard device
	LPDIRECTINPUTDEVICE7 KeyBoard; 
	
	//previous key state
	BYTE KeyState[256];
	
	//new key state
	BYTE NewKeys[256];

	//handles for event notification
	HANDLE InputHandles[2];

	int freeCameraEnabled;

public:

	// Inits the free camera
	void initCameraDevice(D3DVECTOR * eye, D3DVECTOR * at, D3DVECTOR * hup);

	//
	void setFreeCameraEnabled(int state);

	//Update a window with the current input information
	void Update(ZSWindow *pWin);

	//Clear all current states
	void ClearStates();

	//initialize input system
	int Init(HWND hWindow, HINSTANCE hInstance, BOOL Windowed);
	
	//close input system
	int ShutDown();

	//constructor
	ZSInputSystem();

	//destructor
	~ZSInputSystem();

	//access the mouse location
	RECT *GetMouseRect() { return &rMouseScreen; }
};


#endif