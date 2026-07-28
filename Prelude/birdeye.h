#pragma once

#include "opengl1.h"
#include "../Source/World.h"

typedef unsigned char byte;

class BirdEye {
public:
	int currentLeftMouse[3];
	int currentRightMouse[3];

	float moveMatrix[16];
	int currMouseState;

	int mPressed;

	opengl_vector * eye;
	opengl_vector * at;
	opengl_vector * hup;

	float cameraTime;
	float cameraTimeNull;

	BirdEye();	
	void mouse_motion(int, int);
	int mouse_event(int st,int x, int y);
	

};