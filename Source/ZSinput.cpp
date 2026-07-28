//*********************************************************************
//*                                                                   *                                                                **
//**************              ZSInput.cpp           *********************
//**                                                                  *                                                               **
//**                                                                  *                                                               **
//*********************************************************************
//*                                                                   *                                                                  *
//*Revision:    Oct 10, 2000			                                  *  
//*Author:      Mat Williams            
//*Purpose:                                                           *
//*		This file provides implementation of ZSWindow class methods
//*
//*********************************************************************
//*Outstanding issues:                                                                                                      *
//*		Need to switch from directx surface as background to zssurface	
//*		No copy contructor or assignment operators defined
//*********************************************************************
//*********************************************************************
//revision 3: found source of memory leak
//revision 4: switched to event notification instead of immediate processing

#include "ZSinput.h"
#include "ZSwindow.h"
#include "ZSutilities.h"
#include "ZSsound.h"
#include "ZSEngine.h"

#include "birdeye.h"

#define MOUSE_CURSOR_WIDTH	32
#define MOUSE_CURSOR_HEIGHT 32

int ZSInputSystem::Init(HWND hWindow, HINSTANCE hInstance, BOOL Windowed)
{
	DIPROPDWORD DIProp;

	DIProp.diph.dwSize = sizeof(DIPROPDWORD);
	DIProp.diph.dwHeaderSize = sizeof(DIPROPHEADER);

	HRESULT hr;
	hr = DirectInputCreateEx(hInstance,
								DIRECTINPUT_VERSION,
								IID_IDirectInput7,
								(void **)&DirectInput,
								NULL);

	if(hr != DI_OK)
	{
		SafeExit("unable to create direct input obejct");
	}

	hr = DirectInput->CreateDeviceEx(GUID_SysKeyboard, IID_IDirectInputDevice7,
        (void**)&KeyBoard, NULL);

	if(hr != DI_OK)
	{
		SafeExit("unable to create direct Keyboard");
	}

	hr = DirectInput->CreateDeviceEx(GUID_SysMouse, IID_IDirectInputDevice7,
        (void**)&Mouse, NULL);

	if(hr != DI_OK)
	{
		SafeExit("unable to create direct Mouse");
	}

//**********************************************************************************
//		Setting the mouse and keyboard to nonexclusive mode eats the hell out of system
//		resources, but is fairly necessary for debugging
//		all tests show that setting to exclusive mode eliminates resource gobbling
//		Resources are returned on safe exit
//
//************************************************************************************

	//set the devices' cooperation levels
	if(Windowed)
	{
		hr = Mouse->SetCooperativeLevel(hWindow, 
			  DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	}
	else
	{
		hr = Mouse->SetCooperativeLevel(hWindow, 
			  DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	}
	if(hr != DI_OK)
	{
		SafeExit("unable to set Mouse co-op");
	}

	if(Windowed)
	{
		hr = KeyBoard->SetCooperativeLevel(hWindow, 
        DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	}
	else
	{
		hr = KeyBoard->SetCooperativeLevel(hWindow, 
			  DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	}

	if(hr != DI_OK)
	{
		SafeExit("unable to set keyboard co-op");
	}

	hr = Mouse->SetDataFormat(&c_dfDIMouse);

	if(hr != DI_OK)
	{
		SafeExit("unable to set mouse format");
	}

	//set the starting cursor position 
	POINT mp;
#ifdef OPENGL
	//get the windows cursor position
	/*GetCursorPos(&mp);

	rMouseScreen.left = mp.x;
	rMouseScreen.top = mp.y;*/

	rMouseScreen.left = 0;
	rMouseScreen.top = 0;
	rMouseScreen.right = 0;
	rMouseScreen.bottom = 0;
#else
	//get the windows cursor position
	GetCursorPos(&mp);

	rMouseScreen.left = mp.x;
	rMouseScreen.top = mp.y;
#endif
	//set resolution to 2
	//grainularity should be finer than this....
	MouseResolution = 2;

	hr = KeyBoard->SetDataFormat(&c_dfDIKeyboard);

	if(hr != DI_OK)
	{
		SafeExit("unable to set keyboard format");
	}

	//set event handling
	InputHandles[0] =	CreateEvent( NULL, FALSE, FALSE, NULL );
	hr = Mouse->SetEventNotification(InputHandles[0]);
	if(hr != DI_OK)
	{
		SafeExit("unable to set Mouse to Event Handling");
	}

	//acquire the mouse
	hr = Mouse->Acquire();
	if(hr != DI_OK)
	{
		SafeExit("unable to set acquire mouse");
	}

	//set event handling
	InputHandles[1] =	CreateEvent( NULL, FALSE, FALSE, NULL );
	hr = KeyBoard->SetEventNotification(InputHandles[1]);
	if(hr != DI_OK)
	{
		SafeExit("unable to set keyboard to event Handling");
	}

	//acquire the keyboard
	hr = KeyBoard->Acquire();
	if(hr != DI_OK)
	{
		SafeExit("unable to acquire keyboard");
	}
	
	return TRUE;
}

void ZSInputSystem::initCameraDevice(D3DVECTOR * eye, D3DVECTOR * at, D3DVECTOR * hup) {
	birdEye = new BirdEye();
	birdEye->eye = eye;
	birdEye->at = at;
	birdEye->hup = hup;
}

void ZSInputSystem::setFreeCameraEnabled(int state) {
	freeCameraEnabled = state;
}

int ZSInputSystem::ShutDown()
{
	if(Mouse) {
		Mouse->SetEventNotification(NULL);
#ifndef OPENGL
		CloseHandle(InputHandles[0]);
#endif
		Mouse->Unacquire();
		Mouse->Release();
		Mouse = NULL;
	}

	if(KeyBoard) {
		KeyBoard->SetEventNotification(NULL);
#ifndef OPENGL
		CloseHandle(InputHandles[1]);
#endif
		KeyBoard->Unacquire();
		KeyBoard->Release();
		KeyBoard = NULL;
	}

	if(DirectInput) {
		DirectInput->Release();
		DirectInput = NULL;
	}
	return FALSE;
}

static ZSWindow * input_focused_window = NULL;
static uint8_t input_key_state[512] = { 0, };
static uint8_t old_key_state[512] = { 0, };

void ZSInput_key_event(GLFWwindow* window, int ikey, int scancode, int action, int mods) {
	
	
	if ((action == GLFW_PRESS) && opengl_dbg_key(ikey)) {
		return;
	}
	
	if (!input_focused_window) {
		return;
	}

	if (input_focused_window->GetState() == WINDOW_STATE_DONE) {
		return;
	}

	int key = opengl_translate_glfw_to_dinput(ikey);

	if ((action == GLFW_RELEASE && key == DIK_F4) && (old_key_state[DIK_F4] & 0x80) && (mods & GLFW_MOD_ALT)) {
		// Might be done in a cleaner way by raising a flag
		exit(0);
	}


	if (action == GLFW_PRESS) {
		input_key_state[key] = 0x80;
		input_focused_window->HandleKeys(input_key_state, old_key_state);
		memcpy(old_key_state, input_key_state, sizeof(input_key_state));
	}
	else if (action == GLFW_REPEAT) {
		input_key_state[key] = 0x0;
		input_focused_window->HandleKeys(input_key_state, old_key_state);
		memcpy(old_key_state, input_key_state, sizeof(input_key_state));
		input_key_state[key] = 0x80;
		input_focused_window->HandleKeys(input_key_state, old_key_state);
		memcpy(old_key_state, input_key_state, sizeof(input_key_state));
	}
	else if (action == GLFW_RELEASE) {
		input_key_state[key] = 0x0;
		input_focused_window->HandleKeys(input_key_state, old_key_state);
		memcpy(old_key_state, input_key_state, sizeof(input_key_state));
	}
	else {
		debug_info("bad key state %i", action);
	}

	if (input_key_state[DIK_ESCAPE] & 0x80 &&
		(input_key_state[DIK_LCONTROL] & 0x80 || input_key_state[DIK_RCONTROL] & 0x80) &&
		(input_key_state[DIK_LSHIFT] & 0x80 || input_key_state[DIK_RSHIFT] & 0x80)) {
		input_focused_window->ReleaseFocus();
		DEBUG_INFO("forcing a focus release from input module\n");
	}
	else {
		if (input_key_state[DIK_ESCAPE] & 0x80 &&
			(input_key_state[DIK_LCONTROL] & 0x80 || input_key_state[DIK_RCONTROL] & 0x80)) {
			SafeExit("hard exit from input module");
		}
	}
}

void ZSInputSystem::Update(ZSWindow *pWin)
{
	DIMOUSESTATE ms;
	HRESULT hr;
	DWORD dwResult;

//	Engine->Sound()->Update();
#ifdef OPENGL
	input_focused_window = pWin;
	opengl_poll();
	input_focused_window = NULL;
#endif
	
	//dwResult = MsgWaitForMultipleObjects(2, InputHandles, FALSE, 0, QS_ALLINPUT); 
	
	dwResult = WaitForSingleObject(InputHandles[0], 0); 	
	
	if(dwResult == WAIT_OBJECT_0)
	{
  // Event 1 has been set. If the event was 
        // created as autoreset, it has also 
        // been reset. 
		//get the mouse state
		//debug_info("get dev state");
		hr = Mouse->GetDeviceState(sizeof(DIMOUSESTATE),&ms);
		
		if(hr != DI_OK)
		{
			//if the mouse was lost attempt to re-acquire it
			if(hr == DIERR_INPUTLOST)
				Mouse->Acquire();
		}
		else
		{
			//update the mouse position from the new mouse state
#ifdef OPENGL			
			//rMouseScreen.left += (MouseState.lX * MouseResolution);
			//rMouseScreen.top += (MouseState.lY * MouseResolution
			//debug_info("mouse r %i %i %i %i\n", rMouseScreen.left, rMouseScreen.top, MouseState.lX, MouseState.lY);
			rMouseScreen.left = ms.lX;
			if (rMouseScreen.left > 800) rMouseScreen.left = 800;
			rMouseScreen.top = ms.lY;
			if (rMouseScreen.top > 600) rMouseScreen.top = 600;
			
			MouseState.lZ *= MouseResolution;
			
			//confirm the position from the input focus
			long left = rMouseScreen.left;
			long top = rMouseScreen.top;
			long lZ = MouseState.lZ;
			pWin->MoveMouse(&left,&top, &lZ);
			//rMouseScreen.left = left;
			//rMouseScreen.top = top;
			MouseState.lZ = lZ;
#else 
			rMouseScreen.left += (MouseState.lX * MouseResolution);
			rMouseScreen.top += (MouseState.lY * MouseResolution);
			MouseState.lZ *= MouseResolution;

			//confirm the position from the input focus
			pWin->MoveMouse(&rMouseScreen.left,&rMouseScreen.top, &MouseState.lZ);
#endif			
			int doubleClick = ms.rgbButtons[3];
			rMouseScreen.right = rMouseScreen.left + MOUSE_CURSOR_WIDTH;
			rMouseScreen.bottom = rMouseScreen.top + MOUSE_CURSOR_HEIGHT;
			
			// Experimental camera device
			if (freeCameraEnabled) {
				if (MouseState.rgbButtons[2] & 0x80) {
					birdEye->mouse_event(1, ms.lX, ms.lY);
					birdEye->mouse_motion(ms.lX, ms.lY);
				}
				else {
					birdEye->mouse_event(0, ms.lX, ms.lY);
				}
			}

			//check button 1
			if(MouseState.rgbButtons[0] &0x80)
			{
				if(ms.rgbButtons[0] & 0x80)
				{
				}
				else
				{
					//see if the left button up has already been handled
					if(!LeftButtonUpHandled)
					{
						LeftButtonUpHandled = TRUE;
						LeftButtonDownHandled = FALSE;

						//if not send a message to the input focus
						pWin->LeftButtonUp(rMouseScreen.left,rMouseScreen.top);
					}
				}
			}
			else
			{
				if(ms.rgbButtons[0] & 0x80)
				{
					//see if the left button down has been handled
					if(!LeftButtonDownHandled)
					{
						LeftButtonDownHandled = TRUE;
						LeftButtonUpHandled = FALSE;
						
						//if not send message
						pWin->LeftButtonDown(rMouseScreen.left,rMouseScreen.top, doubleClick);
					}
				}
				else
				{
				}
			}

			if(MouseState.rgbButtons[1] & 0x80)
			{
				if(ms.rgbButtons[1] & 0x80)
				{
				}
				else
				{
					//see if right button up has been handled
					if(!RightButtonUpHandled)
					{
						RightButtonUpHandled = TRUE;
						RightButtonDownHandled = FALSE;

						//if not send message
						pWin->RightButtonUp(rMouseScreen.left,rMouseScreen.top);
					}
				}
			}
			else
			{
				if(ms.rgbButtons[1] & 0x80)
				{
					//see if right button down has been handled
					if(!RightButtonDownHandled)
					{
						RightButtonDownHandled = TRUE;
						RightButtonUpHandled = FALSE;

						//if not send message
						pWin->RightButtonDown(rMouseScreen.left,rMouseScreen.top);
					}
				}
				else
				{
				}
			}
			MouseState = ms;

		}

		//done with mouse
	}

 
}

void ZSInputSystem::ClearStates()
{
	memset(&MouseState,0,sizeof(MouseState));
	memset(KeyState,0,256);
	RightButtonDownHandled = FALSE;
	RightButtonUpHandled = FALSE;
	LeftButtonUpHandled = FALSE;
	LeftButtonDownHandled = FALSE;
}


ZSInputSystem::ZSInputSystem()
{
	memset(&MouseState,0,sizeof(MouseState));
	memset(KeyState,0,256);
	RightButtonDownHandled = FALSE;
	RightButtonUpHandled = FALSE;
	LeftButtonUpHandled = FALSE;
	LeftButtonDownHandled = FALSE;
	Mouse = NULL;
	DirectInput = NULL;
	KeyBoard = NULL;
	freeCameraEnabled = 0;

}

ZSInputSystem::~ZSInputSystem()
{
	//confirm shutdown
	
	//if mouse or keyboard or input exists we have not shut down
	if(Mouse || KeyBoard || DirectInput)
	{
		ShutDown();
	}

}