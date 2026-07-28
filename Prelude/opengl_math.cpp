#ifdef OPENGL
#include "opengl1.h"
#include <math.h>
#include <stdio.h>

void debug_info(const char * str, ...);

opengl_matrix * D3DXMatrixIdentity(opengl_matrix * m) {
	m->_11 = 1; m->_12 = 0; m->_13 = 0; m->_14 = 0;
	m->_21 = 0; m->_22 = 1; m->_23 = 0; m->_24 = 0;
	m->_31 = 0; m->_32 = 0; m->_33 = 1; m->_34 = 0;
	m->_41 = 0; m->_42 = 0; m->_43 = 0; m->_44 = 1;
	return m;
}

opengl_matrix * D3DXMatrixRotationZ(opengl_matrix * m, float radians) {
	m->_11 = cos(radians); m->_12 = sin(radians); m->_13 = 0; m->_14 = 0;
	m->_21 = -sin(radians); m->_22 = cos(radians); m->_23 = 0; m->_24 = 0;
	m->_31 = 0; m->_32 = 0; m->_33 = 1; m->_34 = 0;
	m->_41 = 0; m->_42 = 0; m->_43 = 0; m->_44 = 1;

	return m;
}

opengl_matrix * D3DXMatrixRotationY(opengl_matrix * m, float radians) {
	m->_11 = cos(radians); m->_12 = 0; m->_13 = -sin(radians); m->_14 = 0;
	m->_21 = 0; m->_22 = 1; m->_23 = 0; m->_24 = 0;
	m->_31 = sin(radians); m->_32 = 0; m->_33 = cos(radians); m->_34 = 0;
	m->_41 = 0; m->_42 = 0; m->_43 = 0; m->_44 = 1;
	return m;
}

opengl_matrix * D3DXMatrixRotationX(opengl_matrix * m, float radians) {
	m->_11 = 1; m->_12 = 0; m->_13 = 0; m->_14 = 0;
	m->_21 = 0; m->_22 = cos(radians); m->_23 = sin(radians); m->_24 = 0;
	m->_31 = 0; m->_32 = -sin(radians); m->_33 = cos(radians); m->_34 = 0;
	m->_41 = 0; m->_42 = 0; m->_43 = 0; m->_44 = 1;
	return m;
}

opengl_matrix * D3DXMatrixRotationYawPitchRoll(opengl_matrix * pOut, float yaw, float pitch, float roll) {
	opengl_matrix z, y, x, tmp;
	D3DXMatrixRotationZ(&z, roll);
	D3DXMatrixRotationZ(&x, pitch);
	D3DXMatrixRotationZ(&y, yaw);

	D3DXMatrixMultiply(&tmp, &x, &y);
	D3DXMatrixMultiply(pOut, &tmp, &z);
	return pOut;
}

opengl_matrix * D3DXMatrixTranslation(opengl_matrix * m, float x, float y, float z) {
	m->_11 = 1; m->_12 = 0; m->_13 = 0; m->_14 = 0;
	m->_21 = 0; m->_22 = 1; m->_23 = 0; m->_24 = 0;
	m->_31 = 0; m->_32 = 0; m->_33 = 1; m->_34 = 0;
	m->_41 = x; m->_42 = y; m->_43 = z; m->_44 = 1;
	return m;
}

opengl_matrix * D3DXMatrixRotationAxis(opengl_matrix * m, opengl_vector * axis, float radians) {
	D3DXVECTOR3 nv;
	float sangle, cangle, cdiff;

	D3DXVec3Normalize(&nv, axis);
	sangle = sinf(radians);
	cangle = cosf(radians);
	cdiff = 1.0f - cangle;

	m->m[0][0] = cdiff * nv.x * nv.x + cangle;
	m->m[1][0] = cdiff * nv.x * nv.y - sangle * nv.z;
	m->m[2][0] = cdiff * nv.x * nv.z + sangle * nv.y;
	m->m[3][0] = 0.0f;
	m->m[0][1] = cdiff * nv.y * nv.x + sangle * nv.z;
	m->m[1][1] = cdiff * nv.y * nv.y + cangle;
	m->m[2][1] = cdiff * nv.y * nv.z - sangle * nv.x;
	m->m[3][1] = 0.0f;
	m->m[0][2] = cdiff * nv.z * nv.x - sangle * nv.y;
	m->m[1][2] = cdiff * nv.z * nv.y + sangle * nv.x;
	m->m[2][2] = cdiff * nv.z * nv.z + cangle;
	m->m[3][2] = 0.0f;
	m->m[0][3] = 0.0f;
	m->m[1][3] = 0.0f;
	m->m[2][3] = 0.0f;
	m->m[3][3] = 1.0f;

	return m;
}

opengl_matrix * D3DXMatrixMultiply(opengl_matrix * out, opengl_matrix * m1, opengl_matrix * m2) {
	int i, j;
	opengl_matrix tmp;
	opengl_matrix tmp2;
	memcpy(&tmp._11, &m1->_11, 16 * sizeof(float));
	memcpy(&tmp2._11, &m2->_11,16*sizeof(float));
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			out->m[i][j] = tmp.m[i][0] * tmp2.m[0][j] + tmp.m[i][1] * tmp2.m[1][j] + tmp.m[i][2] * tmp2.m[2][j] + tmp.m[i][3] * tmp2.m[3][j];			
		}
	}
	return out;
}

opengl_vector * D3DXVec3Transform(opengl_vector * out, opengl_vector * v, opengl_matrix * m) {
	int i, j;
	float t[4],t2[4];
	t[0] = v->x;
	t[1] = v->y;
	t[2] = v->z;
	t[3] = 1;
	float * mx = &m->_11;
	for (i = 0; i < 4; i++) {
		float s = 0;
		for (j = 0; j < 4; j++) {
			s += t[j] * mx[j*4+i];
		}
		t2[i] = s;
	}
	out->x = t2[0];
	out->y = t2[1];
	out->z = t2[2];
	return out;
}

opengl_matrix * D3DXMatrixInverse(opengl_matrix * out, float * idet, opengl_matrix * mm) {
	float inv[16], det;
	int i;
	float * m = (float*)mm->m;

	inv[0] = m[5] * m[10] * m[15] -
		m[5] * m[11] * m[14] -
		m[9] * m[6] * m[15] +
		m[9] * m[7] * m[14] +
		m[13] * m[6] * m[11] -
		m[13] * m[7] * m[10];

	inv[4] = -m[4] * m[10] * m[15] +
		m[4] * m[11] * m[14] +
		m[8] * m[6] * m[15] -
		m[8] * m[7] * m[14] -
		m[12] * m[6] * m[11] +
		m[12] * m[7] * m[10];

	inv[8] = m[4] * m[9] * m[15] -
		m[4] * m[11] * m[13] -
		m[8] * m[5] * m[15] +
		m[8] * m[7] * m[13] +
		m[12] * m[5] * m[11] -
		m[12] * m[7] * m[9];

	inv[12] = -m[4] * m[9] * m[14] +
		m[4] * m[10] * m[13] +
		m[8] * m[5] * m[14] -
		m[8] * m[6] * m[13] -
		m[12] * m[5] * m[10] +
		m[12] * m[6] * m[9];

	inv[1] = -m[1] * m[10] * m[15] +
		m[1] * m[11] * m[14] +
		m[9] * m[2] * m[15] -
		m[9] * m[3] * m[14] -
		m[13] * m[2] * m[11] +
		m[13] * m[3] * m[10];

	inv[5] = m[0] * m[10] * m[15] -
		m[0] * m[11] * m[14] -
		m[8] * m[2] * m[15] +
		m[8] * m[3] * m[14] +
		m[12] * m[2] * m[11] -
		m[12] * m[3] * m[10];

	inv[9] = -m[0] * m[9] * m[15] +
		m[0] * m[11] * m[13] +
		m[8] * m[1] * m[15] -
		m[8] * m[3] * m[13] -
		m[12] * m[1] * m[11] +
		m[12] * m[3] * m[9];

	inv[13] = m[0] * m[9] * m[14] -
		m[0] * m[10] * m[13] -
		m[8] * m[1] * m[14] +
		m[8] * m[2] * m[13] +
		m[12] * m[1] * m[10] -
		m[12] * m[2] * m[9];

	inv[2] = m[1] * m[6] * m[15] -
		m[1] * m[7] * m[14] -
		m[5] * m[2] * m[15] +
		m[5] * m[3] * m[14] +
		m[13] * m[2] * m[7] -
		m[13] * m[3] * m[6];

	inv[6] = -m[0] * m[6] * m[15] +
		m[0] * m[7] * m[14] +
		m[4] * m[2] * m[15] -
		m[4] * m[3] * m[14] -
		m[12] * m[2] * m[7] +
		m[12] * m[3] * m[6];

	inv[10] = m[0] * m[5] * m[15] -
		m[0] * m[7] * m[13] -
		m[4] * m[1] * m[15] +
		m[4] * m[3] * m[13] +
		m[12] * m[1] * m[7] -
		m[12] * m[3] * m[5];

	inv[14] = -m[0] * m[5] * m[14] +
		m[0] * m[6] * m[13] +
		m[4] * m[1] * m[14] -
		m[4] * m[2] * m[13] -
		m[12] * m[1] * m[6] +
		m[12] * m[2] * m[5];

	inv[3] = -m[1] * m[6] * m[11] +
		m[1] * m[7] * m[10] +
		m[5] * m[2] * m[11] -
		m[5] * m[3] * m[10] -
		m[9] * m[2] * m[7] +
		m[9] * m[3] * m[6];

	inv[7] = m[0] * m[6] * m[11] -
		m[0] * m[7] * m[10] -
		m[4] * m[2] * m[11] +
		m[4] * m[3] * m[10] +
		m[8] * m[2] * m[7] -
		m[8] * m[3] * m[6];

	inv[11] = -m[0] * m[5] * m[11] +
		m[0] * m[7] * m[9] +
		m[4] * m[1] * m[11] -
		m[4] * m[3] * m[9] -
		m[8] * m[1] * m[7] +
		m[8] * m[3] * m[5];

	inv[15] = m[0] * m[5] * m[10] -
		m[0] * m[6] * m[9] -
		m[4] * m[1] * m[10] +
		m[4] * m[2] * m[9] +
		m[8] * m[1] * m[6] -
		m[8] * m[2] * m[5];

	det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	*idet = det;
	if (det == 0) {
		return NULL;
	}

	det = 1.0 / det;

	*idet = det;

	float *invOut = (float*)out->m;
	for (i = 0; i < 16; i++) {
		invOut[i] = inv[i] * det;
	}

	return out;
	
}

opengl_matrix * D3DXMatrixScaling(opengl_matrix * m, float sx, float sy, float sz) {
	m->_11 = sx; m->_12 = 0; m->_13 = 0; m->_14 = 0;
	m->_21 = 0; m->_22 = sy; m->_23 = 0; m->_24 = 0;
	m->_31 = 0; m->_32 = 0; m->_33 = sz; m->_34 = 0;
	m->_41 = 0; m->_42 = 0; m->_43 = 0; m->_44 = 1;
	return m;
}



opengl_matrix * D3DXMatrixOrtho(opengl_matrix * out, float w, float h, float zn, float zf) {
	out->_11 = (2.0 / w);
	out->_12 = 0;
	out->_13 = 0;
	out->_14 = 0;
	out->_21 = 0;
	out->_22 = (2.0 / h);
	out->_23 = 0;
	out->_24 = 0;
	out->_31 = 0;
	out->_32 = 0;
	out->_33 = 1.0/(zn-zf);
	out->_34 = 0;
	out->_41 = 0;
	out->_42 = 0;
	out->_43 = zn/(zn-zf);
	out->_44 = 1;
	
	return out;
}

opengl_matrix * D3DXMatrixLookAt(opengl_matrix * out, const opengl_vector * pEye, const opengl_vector * pAt, const opengl_vector * pUp) {
	opengl_vector v1 = (*pEye) - (*pAt);
	
	opengl_vector z1 = v1.normal();
	
	opengl_vector x1 = CrossProduct(*pUp, z1).normal();
	opengl_vector y1 = CrossProduct(z1, x1);
	
	// inverted xaxis x-y ?
	out->_11 = x1.y;
	out->_12 = y1.x;
	out->_13 = z1.x;
	out->_14 = 0;
	out->_21 = x1.x;
	out->_22 = y1.y;
	out->_23 = z1.y;
	out->_24 = 0;
	out->_31 = x1.z;
	out->_32 = y1.z;
	out->_33 = z1.z;
	out->_34 = 0;
	
	out->_41 = DotProduct(x1,*pEye);
	out->_42 = -DotProduct(y1, *pEye);
	out->_43 = -DotProduct(z1, *pEye);
	out->_44 = 1;

	D3DXVECTOR3 right, upn, vec;

	vec = *pAt - *pEye;
	
	
	D3DXVec3Normalize(&vec, &vec);
	right = CrossProduct(*pUp, vec);
	upn = CrossProduct(vec, right);
	D3DXVec3Normalize(&right, &right);
	D3DXVec3Normalize(&upn, &upn);
	out->m[0][0] = right.x;
	out->m[1][0] = right.y;
	out->m[2][0] = right.z;
	out->m[3][0] = -DotProduct(right, *pEye);
	out->m[0][1] = upn.x;
	out->m[1][1] = upn.y;
	out->m[2][1] = upn.z;
	out->m[3][1] = -DotProduct(upn, *pEye);
	out->m[0][2] = -vec.x;
	out->m[1][2] = -vec.y;
	out->m[2][2] = -vec.z;
	out->m[3][2] = DotProduct(vec, *pEye);
	out->m[0][3] = 0.0f;
	out->m[1][3] = 0.0f;
	out->m[2][3] = 0.0f;
	out->m[3][3] = 1.0f;


	return out;
}

float D3DXVec3Length(const opengl_vector *v1) {
	return sqrt(v1->x*v1->x + v1->y*v1->y + v1->z*v1->z);
}

opengl_vector* D3DXVec3Scale(D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, float s) {
	pOut->x = pV->x*s;
	pOut->y = pV->y*s;
	pOut->z = pV->z*s;

	return pOut;
}

opengl_vector* D3DXVec3Normalize(opengl_vector *out, const opengl_vector *v) {
	float norm = D3DXVec3Length(v);
	out->x = v->x / norm;
	out->y = v->y / norm;
	out->z = v->z / norm;

	return out;
}

opengl_vector Normalize(opengl_vector v) {
	float norm = D3DXVec3Length(&v);
	opengl_vector vx;
	vx.x = v.x / norm;
	vx.y = v.y / norm;
	vx.z = v.z / norm;

	return vx;
}

opengl_vector CrossProduct(opengl_vector v1, opengl_vector v2) {
	opengl_vector out;
	out.x = (v1.y * v2.z - v1.z * v2.y);
	out.y = (v1.z * v2.x - v1.x * v2.z);
	out.z = (v1.x * v2.y - v1.y * v2.x);

	return out;
}

float DotProduct(opengl_vector v1, opengl_vector v2) {
	return v1.x*v2.x + v1.y*v2.y + v1.z * v2.z;
}

float Magnitude(opengl_vector v1) {
	return sqrt(v1.x*v1.x + v1.y*v1.y + v1.z*v1.z);
}


#endif