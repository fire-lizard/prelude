#include "opengl1.h"
#include <math.h>
#include "birdeye.h"


#define moveAccel 320.0
#define turnFactor 0.1
#define turnWalkFactor 0.2

#define X_ID 0
#define Y_ID 1
#define Z_ID 2

#define M_PI           3.14159265358979323846

static double normal_vec(float * v) {
	return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}


float * vectorialProduct(float * u, float * v, float * w) {
	float norm_u = normal_vec(u);
	float norm_v = normal_vec(v);
	w[0] = (u[1] / norm_u)*(v[2] / norm_v) - (u[2] / norm_u)*(v[1] / norm_v);
	w[1] = (u[2] / norm_u)*(v[0] / norm_v) - (u[0] / norm_u)*(v[2] / norm_v);
	w[2] = (u[0] / norm_u)*(v[1] / norm_v) - (u[1] / norm_u)*(v[0] / norm_v);
	return w;

}

static double scalarProduct(float * u, float * v) {
	int i;
	double sum = 0;
	for (i = 0; i < 3; i++) {
		sum += u[i] * v[i];
	}

	return sum;
}

static void crossProductf(float * v1, float * v2, float * out) {
	out[0] = (v1[1] * v2[2] - v1[2] * v2[1]);
	out[1] = (v1[2] * v2[0] - v1[0] * v2[2]);
	out[2] = (v1[0] * v2[1] - v1[1] * v2[0]);
}

static double getVectorAngle(float * u, float * v) {
	return (180 / M_PI)*acos(scalarProduct(u, v) / (normal_vec(u) * normal_vec(v)));
}

static double getVectorAngleRadians(float * u, float * v) {
	return acos(scalarProduct(u, v) / (normal_vec(u) * normal_vec(v)));
}

static const double zeroMatrix[16] = {
	0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0,
	0.0, 0.0, 0.0, 0.0
};

static const double idMatrix[16] = {
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
};



#define degree_rate 2
#define move_factor 0.1

#define max_spherical_angle 20

extern World *PreludeWorld;

BirdEye::BirdEye() {
	currMouseState = 0;
	
	moveMatrix[0] = 1;

	memcpy(moveMatrix, idMatrix, 16 * sizeof(float));

	currentRightMouse[X_ID] = currentRightMouse[Y_ID] = 0;
	mPressed = 0;

}



/**
*  callback
*/
void BirdEye::mouse_motion(int x, int y) {
	int dx = currentRightMouse[X_ID] - x;
	int dy = currentRightMouse[Y_ID] - y;
	
	currentRightMouse[X_ID] = x;
	currentRightMouse[Y_ID] = y;
	
	if (dx == 0 && dy == 0) return;
	debug_info("delta %i %i", dx, dy);
	glPushMatrix();
	
	glMatrixMode(GL_MODELVIEW);
	GLfloat localMatrix[16];

	int i;
	for (i = 0; i <16; i++) localMatrix[i] = 16;
	glLoadIdentity();

	glTranslated(at->x, at->y, at->z);

	float pointVector[3] = {
		eye->x - at->x,
		eye->y - at->y,
		eye->z - at->z
	};

	
	// degrees
	float angle = getVectorAngle(pointVector, &hup->x);
	

	float newAngle = (float)(dy) / degree_rate;

	if ((angle + newAngle < 10) || (angle + newAngle > 70)) {
		angle = 0;
	}
	else {
		if ((dy <= 0)) {
			if (fabs(newAngle) > (angle - max_spherical_angle)) {
				angle -= max_spherical_angle;
				if (fabs(angle) < 0.1) {
					angle = 0;
				}
			}
			else {
				angle = newAngle;
			}
		}
		if ((dy > 0)) {

			if (fabs(newAngle) > (190 - angle - max_spherical_angle)) {
				angle = 90 - angle - max_spherical_angle;
				if (fabs(angle) < 0.1) {
					angle = 0;
				}
			}
			else {
				angle = newAngle;
			}
		}
	}
	
	float vectProduct[3];
	vectorialProduct(&hup->x, pointVector, vectProduct);
	glRotated(angle, vectProduct[0], vectProduct[1], vectProduct[2]);
	
	float xvar = (dx) / degree_rate;
	glRotated(xvar,
		hup->x,
		hup->y,
		hup->z
	);
	
	glTranslated(at->x * (-1), at->y * (-1), at->z * (-1));

	localMatrix[0] = eye->x;
	localMatrix[1] = eye->y;
	localMatrix[2] = eye->z;
	localMatrix[3] = 1;

	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)moveMatrix);
	glMultMatrixf((const GLfloat *)localMatrix);

	glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)localMatrix);
	
	eye->x = localMatrix[X_ID];
	eye->y = localMatrix[Y_ID];
	eye->z = localMatrix[Z_ID];
	
	glPopMatrix();

	float pointVector2[3] = {
		eye->x - at->x,
		eye->y - at->y,
		0
	};

	float base[3] = { 0,1,0 };
	float xangle = getVectorAngleRadians(pointVector2, base);

	float out[3];
	crossProductf(pointVector2, base, out);

	// check if z is positive, ie right handedness
	if (out[2] > 0) {
		xangle = -xangle;
	}

	debug_info("angle %f -- %f %f %f -- %f %f %f -- %f %f %f", xangle, pointVector2[0], pointVector2[1], pointVector2[2],eye->x,eye->y,eye->z,at->x,at->y,at->z);
	PreludeWorld->SetCamera(xangle);

	

	
}

int BirdEye::mouse_event(int st, int x, int y) {
	if (st == 1 && mPressed == 0) {
		currentRightMouse[X_ID] = x;
		currentRightMouse[Y_ID] = y;
		mPressed = 1;
	}
	else if (st == 0 && mPressed == 1) {
		mPressed = 0;
	}

	return 1;
}
