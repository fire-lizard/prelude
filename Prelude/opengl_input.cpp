#ifdef OPENGL
#include "opengl1.h"
#include <stdio.h>
#include <assert.h>

void debug_info(const char * str, ...);

static opengl_input_device * mouseDevice = NULL;
static opengl_input_device * keyboardDevice = NULL;

static int opengl_input_inited = 0;
static int glfw_dinput_key_map[512];
static int dinput_glinput_key_map[512];

void opengl_input_init() {
	for (size_t i = 0; i < 512; ++i)
		dinput_glinput_key_map[i] = -1;

	dinput_glinput_key_map[DIK_ESCAPE] = GLFW_KEY_ESCAPE;
	dinput_glinput_key_map[DIK_1] = GLFW_KEY_1;
	dinput_glinput_key_map[DIK_2] = GLFW_KEY_2;
	dinput_glinput_key_map[DIK_3] = GLFW_KEY_3;
	dinput_glinput_key_map[DIK_4] = GLFW_KEY_4;
	dinput_glinput_key_map[DIK_5] = GLFW_KEY_5;
	dinput_glinput_key_map[DIK_6] = GLFW_KEY_6;
	dinput_glinput_key_map[DIK_7] = GLFW_KEY_7;
	dinput_glinput_key_map[DIK_8] = GLFW_KEY_8;
	dinput_glinput_key_map[DIK_9] = GLFW_KEY_9;
	dinput_glinput_key_map[DIK_0] = GLFW_KEY_0;
	dinput_glinput_key_map[DIK_MINUS] = GLFW_KEY_MINUS;
	dinput_glinput_key_map[DIK_EQUALS] = GLFW_KEY_EQUAL;
	dinput_glinput_key_map[DIK_BACK] = GLFW_KEY_BACKSPACE;
	dinput_glinput_key_map[DIK_TAB] = GLFW_KEY_TAB;
	dinput_glinput_key_map[DIK_Q] = GLFW_KEY_Q;
	dinput_glinput_key_map[DIK_W] = GLFW_KEY_W;
	dinput_glinput_key_map[DIK_E] = GLFW_KEY_E;
	dinput_glinput_key_map[DIK_R] = GLFW_KEY_R;
	dinput_glinput_key_map[DIK_T] = GLFW_KEY_T;
	dinput_glinput_key_map[DIK_Y] = GLFW_KEY_Y;
	dinput_glinput_key_map[DIK_U] = GLFW_KEY_U;
	dinput_glinput_key_map[DIK_I] = GLFW_KEY_I;
	dinput_glinput_key_map[DIK_O] = GLFW_KEY_O;
	dinput_glinput_key_map[DIK_P] = GLFW_KEY_P;
	dinput_glinput_key_map[DIK_LBRACKET] = GLFW_KEY_LEFT_BRACKET;
	dinput_glinput_key_map[DIK_RBRACKET] = GLFW_KEY_RIGHT_BRACKET;
	dinput_glinput_key_map[DIK_RETURN] = GLFW_KEY_ENTER;
	dinput_glinput_key_map[DIK_LCONTROL] = GLFW_KEY_LEFT_CONTROL;
	dinput_glinput_key_map[DIK_A] = GLFW_KEY_A;
	dinput_glinput_key_map[DIK_S] = GLFW_KEY_S;
	dinput_glinput_key_map[DIK_D] = GLFW_KEY_D;
	dinput_glinput_key_map[DIK_F] = GLFW_KEY_F;
	dinput_glinput_key_map[DIK_G] = GLFW_KEY_G;
	dinput_glinput_key_map[DIK_H] = GLFW_KEY_H;
	dinput_glinput_key_map[DIK_J] = GLFW_KEY_J;
	dinput_glinput_key_map[DIK_K] = GLFW_KEY_K;
	dinput_glinput_key_map[DIK_L] = GLFW_KEY_L;
	dinput_glinput_key_map[DIK_SEMICOLON] = GLFW_KEY_SEMICOLON;
	dinput_glinput_key_map[DIK_APOSTROPHE] = GLFW_KEY_APOSTROPHE;
	dinput_glinput_key_map[DIK_GRAVE] = GLFW_KEY_GRAVE_ACCENT;
	dinput_glinput_key_map[DIK_LSHIFT] = GLFW_KEY_LEFT_SHIFT;
	dinput_glinput_key_map[DIK_BACKSLASH] = GLFW_KEY_BACKSLASH;
	dinput_glinput_key_map[DIK_Z] = GLFW_KEY_Z;
	dinput_glinput_key_map[DIK_X] = GLFW_KEY_X;
	dinput_glinput_key_map[DIK_C] = GLFW_KEY_C;
	dinput_glinput_key_map[DIK_V] = GLFW_KEY_V;
	dinput_glinput_key_map[DIK_B] = GLFW_KEY_B;
	dinput_glinput_key_map[DIK_N] = GLFW_KEY_N;
	dinput_glinput_key_map[DIK_M] = GLFW_KEY_M;
	dinput_glinput_key_map[DIK_COMMA] = GLFW_KEY_COMMA;
	dinput_glinput_key_map[DIK_PERIOD] = GLFW_KEY_PERIOD;
	dinput_glinput_key_map[DIK_SLASH] = GLFW_KEY_SLASH;
	dinput_glinput_key_map[DIK_RSHIFT] = GLFW_KEY_RIGHT_SHIFT;
	dinput_glinput_key_map[DIK_MULTIPLY] = GLFW_KEY_KP_MULTIPLY;
	dinput_glinput_key_map[DIK_LMENU] = GLFW_KEY_LEFT_ALT;
	dinput_glinput_key_map[DIK_SPACE] = GLFW_KEY_SPACE;
	dinput_glinput_key_map[DIK_CAPITAL] = GLFW_KEY_CAPS_LOCK;
	dinput_glinput_key_map[DIK_F1] = GLFW_KEY_F1;
	dinput_glinput_key_map[DIK_F2] = GLFW_KEY_F2;
	dinput_glinput_key_map[DIK_F3] = GLFW_KEY_F3;
	dinput_glinput_key_map[DIK_F4] = GLFW_KEY_F4;
	dinput_glinput_key_map[DIK_F5] = GLFW_KEY_F5;
	dinput_glinput_key_map[DIK_F6] = GLFW_KEY_F6;
	dinput_glinput_key_map[DIK_F7] = GLFW_KEY_F7;
	dinput_glinput_key_map[DIK_F8] = GLFW_KEY_F8;
	dinput_glinput_key_map[DIK_F9] = GLFW_KEY_F9;
	dinput_glinput_key_map[DIK_F10] = GLFW_KEY_F10;
	dinput_glinput_key_map[DIK_NUMLOCK] = GLFW_KEY_NUM_LOCK;
	dinput_glinput_key_map[DIK_SCROLL] = GLFW_KEY_SCROLL_LOCK;
	dinput_glinput_key_map[DIK_NUMPAD7] = GLFW_KEY_KP_7;
	dinput_glinput_key_map[DIK_NUMPAD8] = GLFW_KEY_KP_8;
	dinput_glinput_key_map[DIK_NUMPAD9] = GLFW_KEY_KP_9;
	dinput_glinput_key_map[DIK_SUBTRACT] = GLFW_KEY_KP_SUBTRACT;
	dinput_glinput_key_map[DIK_NUMPAD4] = GLFW_KEY_KP_4;
	dinput_glinput_key_map[DIK_NUMPAD5] = GLFW_KEY_KP_5;
	dinput_glinput_key_map[DIK_NUMPAD6] = GLFW_KEY_KP_6;
	dinput_glinput_key_map[DIK_ADD] = GLFW_KEY_KP_ADD;
	dinput_glinput_key_map[DIK_NUMPAD1] = GLFW_KEY_KP_1;
	dinput_glinput_key_map[DIK_NUMPAD2] = GLFW_KEY_KP_2;
	dinput_glinput_key_map[DIK_NUMPAD3] = GLFW_KEY_KP_3;
	dinput_glinput_key_map[DIK_NUMPAD0] = GLFW_KEY_KP_0;
	dinput_glinput_key_map[DIK_DECIMAL] = GLFW_KEY_KP_DECIMAL;
	dinput_glinput_key_map[DIK_OEM_102] = -1;
	dinput_glinput_key_map[DIK_F11] = GLFW_KEY_F11;
	dinput_glinput_key_map[DIK_F12] = GLFW_KEY_F12;

	dinput_glinput_key_map[DIK_F13] = GLFW_KEY_F13;
	dinput_glinput_key_map[DIK_F14] = GLFW_KEY_F14;
	dinput_glinput_key_map[DIK_F15] = GLFW_KEY_F15;

	
	dinput_glinput_key_map[DIK_NUMPADEQUALS] = GLFW_KEY_KP_EQUAL;

	dinput_glinput_key_map[DIK_NUMPADENTER] = GLFW_KEY_KP_ENTER;
	dinput_glinput_key_map[DIK_RCONTROL] = GLFW_KEY_RIGHT_CONTROL;
	
	dinput_glinput_key_map[DIK_NUMPADCOMMA] = GLFW_KEY_KP_DECIMAL;
	dinput_glinput_key_map[DIK_DIVIDE] = GLFW_KEY_KP_DIVIDE;
	
	dinput_glinput_key_map[DIK_RMENU] = GLFW_KEY_RIGHT_ALT;
	dinput_glinput_key_map[DIK_PAUSE] = GLFW_KEY_PAUSE;
	dinput_glinput_key_map[DIK_HOME] = GLFW_KEY_HOME;
	dinput_glinput_key_map[DIK_UP] = GLFW_KEY_UP;
	dinput_glinput_key_map[DIK_PRIOR] = GLFW_KEY_PAGE_UP;
	dinput_glinput_key_map[DIK_LEFT] = GLFW_KEY_LEFT;
	dinput_glinput_key_map[DIK_RIGHT] = GLFW_KEY_RIGHT;
	dinput_glinput_key_map[DIK_END] = GLFW_KEY_END;
	dinput_glinput_key_map[DIK_DOWN] = GLFW_KEY_DOWN;
	dinput_glinput_key_map[DIK_NEXT] = GLFW_KEY_PAGE_DOWN;
	dinput_glinput_key_map[DIK_INSERT] = GLFW_KEY_INSERT;
	dinput_glinput_key_map[DIK_DELETE] = GLFW_KEY_DELETE;

	// Manage keyboard layouts in a poor way
	// But the game is english-centric, so it will do in the near future :)
	// In fact, text edit should not work with DIKs, but we'll see in the future

	for (int c = 'a'; c <= 'z'; ++c)
	{
		for (int key = 0; key < 512; ++key)
		{
			const char* keyName = glfwGetKeyName(key, 0);
			if (!keyName)
				continue;
			if (keyName[1] != '\0')
				continue;
			if (c != keyName[0])
				continue;
			static const int CHAR_DIKS[26] = {
				DIK_A,
				DIK_B,
				DIK_C,
				DIK_D,
				DIK_E,
				DIK_F,
				DIK_G,
				DIK_H,
				DIK_I,
				DIK_J,
				DIK_K,
				DIK_L,
				DIK_M,
				DIK_N,
				DIK_O,
				DIK_P,
				DIK_Q,
				DIK_R,
				DIK_S,
				DIK_T,
				DIK_U,
				DIK_V,
				DIK_W,
				DIK_X,
				DIK_Y,
				DIK_Z,
			};

			dinput_glinput_key_map[CHAR_DIKS[c - 'a']] = key;
			break;
		}
	}

	int i;
	for (i = 0; i < 512; i++) {
		int remapped_index = dinput_glinput_key_map[i];
		if (remapped_index != -1) {
			assert(remapped_index >= 0);
			assert(remapped_index < 512);
			glfw_dinput_key_map[remapped_index] = i;
		}
	}

	opengl_input_inited = 1;
}

int opengl_translate_glfw_to_dinput(int ikey) {
	return glfw_dinput_key_map[ikey];
}

void opengl_key_event(GLFWwindow* window, int key, int scancode, int action, int mods) {
	
	if ((action == GLFW_PRESS) && opengl_dbg_key(key)) {
		return;
	}
	if (keyboardDevice) {
		keyboardDevice->setKeyEvent(key, scancode, action, mods);
	}
	
}

void opengl_mouse_event(GLFWwindow* window, int button, int action, int mods) {
	
	if (mouseDevice) {
		mouseDevice->setMouseEvent(button, action, mods, -1, -1);
	}
}

void opengl_mouse_cursor_event(GLFWwindow* window, double xpos, double ypos) {

	if (mouseDevice) {
		// The game's windows, hit tests and mouse clamps are all in base
		// resolution pixels; GLFW reports window pixels.
		int bx, by;
		opengl_window_to_base(xpos, ypos, &bx, &by);
		mouseDevice->setMouseEvent(-1,-1,-1, bx, by);
	}
}

void opengl_mouse_wheel_event(GLFWwindow* window, double xoffset, double yoffset) {
	if (mouseDevice) {
		mouseDevice->setScrollEvent(yoffset);
	}
}

int _opengl_input_device::setKeyEvent(int ikey, int scancode, int action, int mods) {
	assert(ikey >= 0);
	assert(ikey < 512);
	key = glfw_dinput_key_map[ikey];
	if (action) {
		key_state[key] = 0x80;
	}
	else {
		key_state[key] = 0;
	}
	setEvent();
	return 0;
}

int _opengl_input_device::setScrollEvent(double var) {
	z = (int)var;
	
	setEvent();
	return 0;
}

int _opengl_input_device::setMouseEvent(int button, int action, int mods, double xpos, double ypos) {
	//debug_info("mouse cursor event %lf %lf %i %i %i %i", xpos, ypos,x,y,lx,ly);
	if (xpos >= 0) {
		x = (int)xpos;
	}
	if (ypos >= 0) {
		y = (int)ypos;
	}
	if (button >= 0) {
		switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT: {
			//debug_info("mouse left %i %i", x, y);
			auto now = std::chrono::high_resolution_clock::now();
			auto diff = now - lastLeftClickEvent;
			button_state[3] = 0;
			if (action == 1) {
				if (diff < std::chrono::milliseconds(DOUBLE_CLICK_TIME_MS)) {
					button_state[3] = 1;
				}
				lastLeftClickEvent = now;
			}
			
			button_state[0] = action ? 0x80 : 0;
			break;
		}
		case GLFW_MOUSE_BUTTON_RIGHT:
			button_state[1] = action ? 0x80 : 0;
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			button_state[2] = action ? 0x80 : 0;
			break;
		}
	}

	return setEvent();
}

int _opengl_input_device::setEvent() {
	if (!eventNotification) {
		return 0;
	}

	SetEvent((HANDLE)eventNotification);

	return 0;
}

int _opengl_input::CreateDeviceEx(int a, int b, LPVOID * out, LPUNKNOWN) {
	if (!opengl_input_inited) {
		opengl_input_init();
	}
	debug_info("opengl_input::CreateDeviceEx %X %X\n", a, b);
	opengl_input_device * dev = new opengl_input_device(a);
	*(opengl_input_device**)out = dev;
	if (a == GUID_SysKeyboard) {
		keyboardDevice = dev;
	}
	else if (a == GUID_SysMouse) {
		mouseDevice = dev;
	}
	return DI_OK;
}

int _opengl_input::Release() {
	return 0;
}

int opengl_input_device::SetCooperativeLevel(HWND, DWORD) {
	return 0;
}

int opengl_input_device::SetDataFormat(opengl_input_data_format *) {
	return 0;
}

int opengl_input_device::SetEventNotification(HANDLE h) {
	eventNotification = h;
	return 0;
}

int opengl_input_device::Acquire() {
	return 0;
}

int opengl_input_device::Unacquire() {
	return 0;
}

int opengl_input_device::Release() {
	return 0;
}

int opengl_input_device::GetDeviceState(DWORD sz, void * st) {
	
	if (type == GUID_SysMouse) {
		DIMOUSESTATE * si = (DIMOUSESTATE*)st;
		si->lX = x-lx;
		si->lY = y-ly;
		lx = x;
		ly = y;
		
		si->lZ = z*2;
		z = 0;

		si->lX = x;
		si->lY = y;
				
		memcpy(&si->rgbButtons, button_state, sizeof(si->rgbButtons));	
		
	}
	else if (type == GUID_SysKeyboard) {
		memcpy(st, key_state, 256);
		//memset(key_state, 0, sizeof(key_state));
	}

	return 0;
}
#endif