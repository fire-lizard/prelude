#ifdef OPENGL
#include "opengl1.h"
#include <math.h>
#include <stdio.h>
#include <GL/glew.h>
#include "bmp.h"
#include <assert.h>

#ifdef __linux__
#include <X11/extensions/Xinerama.h>
#endif

typedef struct _opengl_general_desc {
	opengl_surface * primary;
	opengl_surface * back_buffer;

	GLFWwindow* window;
	int nativeResX = 0;
	int nativeResY = 0;

	int baseResX = 0;
	int baseResY = 0;
	float baseResRatio = 0;
	float baseResVar = 0;

	// Resolution the player asked for, which is not always the one they get:
	// a window as big as the desktop is shrunk to fit the work area. Rendering
	// follows nativeRes* (what the window really is), the options screen shows
	// and cycles from this.
	int reqResX = 0;
	int reqResY = 0;

	GLuint renderBuffer;
	GLuint pixelsFrameBuffer;
} opengl_general_desc;

static opengl_general_desc opengl_local_context = { 0, };
///////////////////////////////////////////////
/// DEBUG functions
///////////////////////////////////////////////
static bool debug_mode_on = false;
static int debug_dump_transform = 0;
static int debug_dump_frame_st = 0;
static int debug_dump_frame = 0;
static FILE * dbg_file = NULL;

void debug_info(const char * str, ...);

int opengl_dbg_key(int key) {
	if (key == GLFW_KEY_GRAVE_ACCENT)
	{
		debug_mode_on ^= true;
		return 1;
	}
	if (debug_mode_on)
	{
		if (key == GLFW_KEY_ESCAPE) {
			opengl_exit();
		}
		else if (key == GLFW_KEY_F1) {
			debug_dump_transform = 1;
			return 1;
		}
		else if (key == GLFW_KEY_F2) {
			debug_dump_frame_st = 1;
			return 1;
		}
		else if (key == GLFW_KEY_F3) {
			opengl_mouse_cursor_event(NULL, 629, 409);

			return 1;
		}
	}
	return 0;
}

static void sprint_matrix(char * out, int lim, opengl_matrix * m) {
	memset(out, 0, lim);
	float * mm;
	mm = &m->_11;
	int i,j;
	for (j = 0; j < 4; j++) {
		for (i = 0; i < 4; i++) {
			sprintf(out + strlen(out), "%f, ", mm[i+(j*4)]);
		}
		sprintf(out + strlen(out), "\n");
	}
}

static void dbg_dump_transform(opengl_device * dev, opengl_matrix * d3dtranslate) {
	FILE * fp = fopen("dump", "w");
	char tmp[1024];
	float * mm;
	int i;

	sprint_matrix(tmp, 1024, &dev->model);
	fwrite(tmp, strlen(tmp), 1, fp);
	sprintf(tmp, "\n");
	fwrite(tmp, strlen(tmp), 1, fp);

	sprint_matrix(tmp, 1024, &dev->world);
	fwrite(tmp, strlen(tmp),1, fp);
	sprintf(tmp, "\n");
	fwrite(tmp, strlen(tmp), 1, fp);

	sprint_matrix(tmp, 1024, &dev->projection);
	fwrite(tmp, strlen(tmp), 1, fp);
	sprintf(tmp, "\n");
	fwrite(tmp, strlen(tmp), 1, fp);

	if (d3dtranslate) {
		sprint_matrix(tmp, 1024, d3dtranslate);
		fwrite(tmp, strlen(tmp), 1, fp);
		sprintf(tmp, "\n");
		fwrite(tmp, strlen(tmp), 1, fp);
	}

	fclose(fp);


}

static uint32 flip_tmp_buffer[8192];

void flip_surface_pixels(uint32 * pixels, int w, int h) {
	int j;
	for (j = 0; j < h/2; j++) {
		uint32 * t = (pixels + j*w);
		memcpy(flip_tmp_buffer, t, w * sizeof(uint32));
		uint32 * t2 = (pixels + ((h-j)-1)*w);
		memcpy(t, t2, w * sizeof(uint32));
		memcpy(t2, flip_tmp_buffer, w * sizeof(uint32));
	}
}


///////////////////////////////////////////////
/// Directx functions
///////////////////////////////////////////////

opengl_input_data_format c_dfDIMouse = { 0, };
opengl_input_data_format c_dfDIKeyboard = { 0, };

int D3DXLoadTextureFromSurface(opengl_device * pd3dDevice, opengl_surface * pTexture, DWORD mipMapLevel, opengl_surface * pSurfaceSrc, RECT * pSrcRect, RECT * pDestRect, opengl_filtertype filterType) {
	if (pDestRect) {
		debug_info("unsupported D3DXLoadTextureFromSurface");
		exit(0);
	}

	if (pSurfaceSrc == opengl_local_context.back_buffer) {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glReadBuffer(GL_BACK);
		
		int xp = (opengl_local_context.nativeResX - ((float)opengl_local_context.baseResX*opengl_local_context.baseResVar)) / 2;
		uint32 * tmp = new uint32[(opengl_local_context.nativeResX-xp*2) *opengl_local_context.nativeResY];
		glReadPixels(xp, 0, opengl_local_context.nativeResX-xp*2,opengl_local_context.nativeResY, GL_RGBA, GL_UNSIGNED_BYTE, tmp);

		//{FILE * fpx = fopen("debug.dump", "wb"); fwrite(tmp, (opengl_local_context.nativeResX - xp * 2)* opengl_local_context.nativeResY, 4, fpx); fclose(fpx); exit(0); }


		bmp_scale(tmp, opengl_local_context.nativeResX - xp * 2, opengl_local_context.nativeResY, 0, 0, opengl_local_context.nativeResX - xp * 2,
			opengl_local_context.nativeResY,
			pSurfaceSrc->pixels, pSurfaceSrc->w, pSurfaceSrc->h, 0, 0, pSurfaceSrc->w, pSurfaceSrc->h, 0, 0);
		
		flip_surface_pixels(pSurfaceSrc->pixels, pSurfaceSrc->w, pSurfaceSrc->h);		
		
		delete[] tmp;
	}

	if (pTexture->w != pSurfaceSrc->w || pTexture->h != pSurfaceSrc->h) {
		if (pSrcRect) {
			
			int rw = pSrcRect->right - pSrcRect->left;
			int rh = pSrcRect->bottom - pSrcRect->top;
			if (rw != pTexture->w || rh != pTexture->h) {
				debug_info("load tex from surface %i %i, %i %i", pTexture->w, pTexture->h, rw,rh);
				exit(0);
			}
			int j;
			
			for (j = 0; j < rh; j++) {
				uint32 *t = pSurfaceSrc->pixels + (j*pSurfaceSrc->w);
				uint32 * t2 = pTexture->pixels + (j*pTexture->w);				
				memcpy(t2, t, sizeof(uint32)*rw);	
			}
		}
		else {
			debug_info("load tex from surface %i %i, %i %i", pTexture->w, pTexture->h, pSurfaceSrc->w, pSurfaceSrc->w);
			exit(0);
		}
	}
	else {
		memcpy(pTexture->pixels, pSurfaceSrc->pixels, pTexture->w* pTexture->h*sizeof(uint32));
		
	}
	return 0;
}


int opengl_clipper::SetClipList(RGNDATA* data, DWORD) {
	debug_info("SetClipList %i",data->rdh.dwSize);
	return 0;
}

int opengl_clipper::SetHWnd(DWORD, HWND) {
	debug_info("SetHWnd");
	printf("Unsupported windowed mode\n");
	exit(0);
}

int opengl_clipper::Release() {
	return 0;
}

int _opengl_3d::EnumZBufferFormats(DWORD a, HRESULT (__stdcall *cb)(opengl_pixelformat *lpDDPixFmt, void *lpContext), void * b) {
	return 0;
}

int _opengl_3d::CreateDevice(DWORD, opengl_surface * surface, opengl_device ** out) {	
	opengl_device * dev = new opengl_device();
	dev->state = 0;
	glGenVertexArrays(1, &dev->va[0]);
	glBindVertexArray(dev->va[0]);

	glGenBuffers(1, &dev->vb[0]);
	glBindBuffer(GL_ARRAY_BUFFER, dev->vb[0]);

	glEnableVertexAttribArray(0);	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, 0);	

	//glEnableVertexAttribArray(2);
	//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 32, (GLvoid*)(24));	

	glBindVertexArray(0);

	glBindVertexArray(dev->va[1]);

	glGenBuffers(1, &dev->vb[1]);
	glBindBuffer(GL_ARRAY_BUFFER, dev->vb[1]);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, 0);

	//glEnableVertexAttribArray(2);
	//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 32, (GLvoid*)(24));

	glBindVertexArray(0);

	dev->projection.identity();
	dev->model.identity();
	dev->world.identity();
	dev->bound_textures_size = 0;
	dev->useTexture = -1;

	dev->surface = surface;

	glGenTextures(128, &dev->bound_textures[0]);
	
	(*out) = dev;
	return 0;
}

int _opengl_3d::Release() {
	return 0;
}

int _opengl_device::SetRenderState(opengl_renderstate state, DWORD st) {
	
	switch (state) {
		case D3DRENDERSTATE_CULLMODE: {
			if (st == D3DCULL_NONE) {
				states.cullmode = 0;
			}
			else if(st == D3DCULL_CCW) {
				states.cullmode = 1;
			}
			break;
		}
		case D3DRENDERSTATE_FILLMODE: {
			if (st == D3DFILL_SOLID) {
				states.polygon = GL_FILL;
			}
			else {
				states.polygon = GL_LINE;
			}
			break;
		}
		case D3DRENDERSTATE_ALPHATESTENABLE: {
			states.alphatest = st;
			break;
		}
		case D3DRENDERSTATE_ALPHAFUNC: {
			GLuint f = 0;
			if (st == D3DCMP_GREATER) {
				f = GL_GREATER;
			}
			else if (st == D3DCMP_GREATEREQUAL) {
				f = GL_GEQUAL;
			}
			states.alphafunc = f;
			break;
		}
		case D3DRENDERSTATE_ALPHAREF: {
			states.alpharef = st;
			states.alpharef /= 0xFF;
			break;
		}
		case D3DRENDERSTATE_ALPHABLENDENABLE:		
			states.blend = st;
			break;
		case D3DRENDERSTATE_LIGHTING:
			//st = 0;
			states.lighting = st;
			break;
		case D3DRENDERSTATE_ZENABLE: {
			states.depth = st;
			break;
		}
		case D3DRENDERSTATE_SRCBLEND:
		case D3DRENDERSTATE_DESTBLEND: {
			GLuint func = GL_SRC_ALPHA;
			if (st == D3DBLEND_INVSRCALPHA) {
				func = GL_ONE_MINUS_SRC_ALPHA;
			}
			else if (st == D3DBLEND_SRCALPHA) {
				func = GL_SRC_ALPHA;
			}
			else if (st == D3DBLEND_DESTCOLOR) {
				func = GL_DST_COLOR;
			}
			else if (st == D3DBLEND_ZERO) {
				func = GL_ZERO;
			}
			else if (st == D3DBLEND_ONE) {
				func = GL_ONE;
			}
			else {
				debug_info("missing blend function");
				exit(0);
			}
			if (state == D3DRENDERSTATE_DESTBLEND) {
				states.dstblend = func;
			}
			else  {
				states.srcblend = func;
			}
			break;
		}
		case D3DRENDERSTATE_ZWRITEENABLE: {
			states.zwrite = st;
			break;
		}
		case D3DRENDERSTATE_COLORKEYENABLE: {
			// TODO Review
			break;
		}
		case D3DRENDERSTATE_AMBIENT: {
			// TODO Review
			break;
		}
		default:
			assert(false);
			break;
	}
	return 0;
}

int _opengl_device::SetMaterial(opengl_material * material) {
	
	states.color = *material;
	
	return 0;
}

int _opengl_device::SetTransform(opengl_transforms tr, opengl_matrix * m) {
	//debug_info("SetTransform %X %i",this,tr);
	if (tr == D3DTRANSFORMSTATE_PROJECTION) {
		projection = *m;
	}
	else if (tr == D3DTRANSFORMSTATE_VIEW) {
		model = *m;
		//debug_info("View transform");
		//char tmp[1024] = { 0, }; sprint_matrix(tmp, 1024, &model); debug_info(tmp);
	}	
	else if (tr == D3DTRANSFORMSTATE_WORLD) {
		world = *m;
	}
	
	return 0;
}

void _opengl_device::buildVPMatrix(opengl_matrix * m) {
	m->_11 = 2.0 / local_vp.dwWidth; m->_12 = 0; m->_13 = 0; m->_14 = 0;
	m->_21 = 0; m->_22 = -2.0 / local_vp.dwHeight; m->_23 = 0; m->_24 = 0;
	m->_31 = 0; m->_32 = 0; m->_33 = 1; m->_34 = 0;
	m->_41 = -1; m->_42 = 1; m->_43 = 0; m->_44 = 1;
}

int opengl_device_states::gl_set_states() {
	if (blend) {
		glEnable(GL_BLEND);
		glBlendFunc(srcblend, dstblend);
	}
	else {
		glDisable(GL_BLEND);
	}

	if (depth) {
		glEnable(GL_DEPTH_TEST);
	}
	else {
		glDisable(GL_DEPTH_TEST);
	}

	if (alphatest) {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(alphafunc, alpharef);
	}
	else {
		glDisable(GL_ALPHA_TEST);
	}
	
	if (lighting) {
		glEnable(GL_LIGHTING);
		//glEnable(GL_NORMALIZE);
		//glEnable(GL_COLOR_MATERIAL);
		//glColor4f(0, 0, 1, 1);

		glDisable(GL_COLOR_MATERIAL);
		
		glMaterialfv(GL_FRONT, GL_AMBIENT, (float*)&color.ambient.dvR);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, (float*)&color.dcvDiffuse.dvR);
		glMaterialfv(GL_FRONT, GL_SPECULAR, (float*)&color.dcvSpecular.dvR);
		//glMaterialfv(GL_FRONT, GL_EMISSION, (float*)&color.dcvEmissive.dvR);			

		int i;
		for (i = 0; i < 8; i++) {
			if (light_enable[i]) {
				
				opengl_light * light = &lights[i];

				//light->dcvDiffuse.dvA = 1; light->dcvAmbient.dvA = 1; light->dcvSpecular.dvA = 1;

				glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, &light->dcvDiffuse.dvR);
				glLightfv(GL_LIGHT0 + i, GL_AMBIENT, &light->dcvAmbient.dvR);				
				glLightfv(GL_LIGHT0 + i, GL_SPECULAR, &light->dcvSpecular.dvR);


				// TODO I fixed specular to black here to stop weird flickering issues,
				// but this might warrant further investigation
				float fixed_specular[4] = { 0,0,0,1 };				
				glLightfv(GL_LIGHT0 + i, GL_SPECULAR, fixed_specular);
				
				float dir[4];				

				if (light->dltType == D3DLIGHT_DIRECTIONAL) {
					dir[0] = light->dvDirection.x;
					dir[1] = light->dvDirection.y;
					dir[2] = light->dvDirection.z;
					dir[3] = 0;
				}
				else if (light->dltType == D3DLIGHT_POINT) {
					dir[0] = light->dvPosition.x;
					dir[1] = light->dvPosition.y;
					dir[2] = light->dvPosition.z;
					dir[3] = 1;
					glLightf(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, 0.01f);					
					
				}
				else {
					debug_info("unimplemented light");
					exit(0);
				}
				
				float modelView[16], projection[16];
				glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
				glGetFloatv(GL_PROJECTION_MATRIX, projection);
				glMatrixMode(GL_PROJECTION);
				glLoadIdentity();

				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				
				glLightfv(GL_LIGHT0 + i, GL_POSITION, dir);
				glEnable(GL_LIGHT0 + i);

				glMatrixMode(GL_PROJECTION);
				glLoadMatrixf(projection);
				glMatrixMode(GL_MODELVIEW);
				glLoadMatrixf(modelView);
				
			}
			else {
				glDisable(GL_LIGHT0 + i);
			}
		}
	}
	else {
		
		glDisable(GL_LIGHTING);
	//	glDisable(GL_NORMALIZE);
	}
	

	if (cullmode == 0) {
		glDisable(GL_CULL_FACE);
	}
	else if (cullmode == 1) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		glFrontFace(GL_CCW);
	}

	glPolygonMode(GL_FRONT_AND_BACK, polygon);
	

	if (zwrite) {
		glDepthMask(GL_TRUE);
	}
	else {
		glDepthMask(GL_FALSE);
	}

	return 0;
}

void _opengl_device::prepareDraw(DWORD vtype) {

	if (vtype & D3DFVF_XYZRHW) {
		state = 0;
		// transforms dont apply, still need to do d3d normalization
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		opengl_matrix m;
		buildVPMatrix(&m);
		glLoadMatrixf(&m.m[0][0]);

	}
	else if ((vtype & D3DFVF_XYZ)) {
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glLoadMatrixf(&projection.m[0][0]);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glLoadMatrixf(&model.m[0][0]);
		glMultMatrixf(&world.m[0][0]);		

		state = 1;
#ifdef OGLDEBUG
		if (debug_dump_transform) {
			debug_dump_transform = 0;
			dbg_dump_transform(this, NULL);
		}
#endif
	}

	if (useTexture >= 0) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, bound_textures[0]);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, states.tex_mag_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, states.tex_min_filter);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glColor4f(1, 1, 1, 1);

	}
	else {
		glDisable(GL_TEXTURE_2D);
		
		glColor4f(states.color.diffuse.r, states.color.diffuse.g, states.color.diffuse.b, states.color.diffuse.a);
	}
	
	states.gl_set_states();
}

GLuint _opengl_device::mapPrimitiveType(opengl_primitive_type type) {
	GLuint gl_primitive = 0;
	if (type == D3DPT_TRIANGLESTRIP) {
		gl_primitive = GL_TRIANGLE_STRIP;
	}
	else if (type == D3DPT_TRIANGLELIST) {
		gl_primitive = GL_TRIANGLES;
	}
	return gl_primitive;
}

int _opengl_device::DrawPrimitive(opengl_primitive_type type, DWORD vtype, void * verts, DWORD n, DWORD flags) {
	GLuint gl_primitive = mapPrimitiveType(type);
		
	prepareDraw(vtype);

	glDepthFunc(GL_LEQUAL);
	//glDepthFunc(GL_ALWAYS);

#ifdef OGLDEBUG
	if (debug_dump_frame) {
		float mm[16];
		glGetFloatv(GL_PROJECTION_MATRIX, mm);
		fwrite(mm, 4, 16, dbg_file);
		glGetFloatv(GL_MODELVIEW_MATRIX, mm);
		fwrite(mm, 4, 16, dbg_file);
		fwrite(&useTexture, 1, 4, dbg_file);
		debug_info("dbg tex %i", useTexture);
		if (useTexture >= 0) {
			opengl_surface * s = tex_stages[0];
			fwrite(&s->w, 1, 4, dbg_file);
			fwrite(&s->h, 1, 4, dbg_file);
			fwrite(s->pixels, 4, s->w*s->h, dbg_file);
		}
		fwrite(&type, 1, 4, dbg_file);
		fwrite(&vtype, 1, 4, dbg_file);
		fwrite(&n, 1, 4, dbg_file);
		debug_info("dbg n %i %i", sizeof(opengl_vertex), n);
		fwrite(verts, sizeof(opengl_vertex), n, dbg_file);

		debug_info("dbg off %i %X", n * sizeof(opengl_vertex), ftell(dbg_file));
	}
#endif
		
	switch (vtype) {
	case D3DFVF_VERTEX: {
		opengl_vertex * lv = (opengl_vertex*)verts;
		
		glBindVertexArray(va[0]);
		glBindBuffer(GL_ARRAY_BUFFER, vb[0]);				
		
		glBufferData(GL_ARRAY_BUFFER, n * sizeof(opengl_vertex), verts, GL_DYNAMIC_DRAW);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, (void*)0);
		glTexCoordPointer(2, GL_FLOAT, 32, (void*)24);
		glDisableClientState(GL_COLOR_ARRAY);

		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT,32,(void*)12);
		glDrawArrays(gl_primitive, 0, n);
		glBindVertexArray(0);

		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
		break;
	}
	case D3DFVF_LVERTEX: {
		opengl_lvertex * lv = (opengl_lvertex*)verts;
		glBindVertexArray(va[0]);
		glBindBuffer(GL_ARRAY_BUFFER, vb[0]);

		glBufferData(GL_ARRAY_BUFFER, n * sizeof(opengl_lvertex), verts, GL_DYNAMIC_DRAW);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, (void*)0);
		glTexCoordPointer(2, GL_FLOAT, 32, (void*)24);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, 32, (void*)16);
		glDrawArrays(gl_primitive, 0, n);
		glBindVertexArray(0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		break;
	}
	case D3DFVF_TLVERTEX: {
		glBindVertexArray(va[1]);
		glBindBuffer(GL_ARRAY_BUFFER, vb[1]);
		opengl_tlvertex * vv = (opengl_tlvertex*)verts;
		glBufferData(GL_ARRAY_BUFFER, n * sizeof(opengl_tlvertex), verts, GL_DYNAMIC_DRAW);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, (void*)0);
		glTexCoordPointer(2, GL_FLOAT, 32, (void*)24);
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_UNSIGNED_BYTE, 32, (void*)16);
		glDrawArrays(gl_primitive, 0, n);
		glBindVertexArray(0);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	
		break;
	}
	default:
		debug_info("unimplemented draw primitive %i",vtype);
		exit(0);
	}
	
	glDisable(GL_TEXTURE_2D);
	return 0;
}

int _opengl_device::DrawIndexedPrimitiveStrided(opengl_primitive_type d3dptPrimitiveType, DWORD dwVertexTypeDesc,
	opengl_primitive_strided_data * lpVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD dwIndexCount, DWORD dwFlags) {
	
	if (dwIndexCount > tmp_vertex_buffer.size()) {
		tmp_vertex_buffer.resize(dwIndexCount);
	}
	
	opengl_vertex* data = tmp_vertex_buffer.data();
	unsigned int i;
	for (i = 0; i < dwIndexCount; i++) {
		DWORD idx = lpwIndices[i];
		opengl_vertex * v = (opengl_vertex*)(((byte*)lpVertexArray->position.lpvData) + (lpVertexArray->position.dwStride*idx));
		memcpy(&data[i].dvX, &v->dvX, 3 * sizeof(float));
		
		float * normal = (float*)(((byte*)lpVertexArray->normal.lpvData) + (lpVertexArray->normal.dwStride*idx));
		memcpy(&data[i].dvNX, normal, 3 * sizeof(float));
		
		float * tex = (float*)(((byte*)lpVertexArray->textureCoords[0].lpvData) + (lpVertexArray->textureCoords[0].dwStride*idx));
		memcpy(&data[i].tu, tex, 2* sizeof(float));			

	}	
	
	DrawPrimitive(d3dptPrimitiveType, dwVertexTypeDesc, data, dwIndexCount, 0);
	
	return 0;
}

int _opengl_device::DrawIndexedPrimitive(opengl_primitive_type type, DWORD vtype, LPVOID verts,
	DWORD dwVertexCount, LPWORD lpwIndices, DWORD  dwIndexCount, DWORD  dwFlags) {
	
	opengl_vertex * lverts = (opengl_vertex*)verts;

	if (dwIndexCount > tmp_vertex_buffer.size())
		tmp_vertex_buffer.resize(dwIndexCount);
	opengl_vertex * data = tmp_vertex_buffer.data();
	unsigned int i;
	for (i = 0; i < dwIndexCount; i++) {
		data[i] = lverts[lpwIndices[i]];
	}
	DrawPrimitive(type, vtype, data, dwIndexCount, 0);

	return 0;
}

int _opengl_device::BeginScene() {

#ifdef OGLDEBUG
	if (debug_dump_frame_st) {
		debug_dump_frame_st = 0;
		debug_dump_frame = 1;
		dbg_file = fopen("frame_dump", "wb");
	}
#endif
	int xp = (opengl_local_context.nativeResX - ((float)opengl_local_context.baseResX*opengl_local_context.baseResVar)) / 2;
	int y = opengl_local_context.baseResY - local_vp.dwHeight;
	glViewport(xp, (int)(y * opengl_local_context.baseResVar), opengl_local_context.nativeResX - xp * 2, (int)(local_vp.dwHeight * opengl_local_context.baseResVar));


	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	glMatrixMode(GL_PROJECTION);

	glLoadIdentity();
	glOrtho(0, 0, 800, 500, -100, 100);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	return 0;
}

int _opengl_device::EndScene() {
	//glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, opengl_local_context.primary->pixels);
#ifdef OGLDEBUG
	if (dbg_file) {
		fclose(dbg_file);
		dbg_file = NULL;
	}
	debug_dump_frame = 0;
#endif
	return 0;
}

int _opengl_device::GetTransform(opengl_transforms mode, opengl_matrix * m) {
	if (mode == D3DTRANSFORMSTATE_PROJECTION) {
		*m = projection;
	}
	else if (mode == D3DTRANSFORMSTATE_VIEW) {
		*m = model;	
	}

	else if (mode == D3DTRANSFORMSTATE_WORLD) {
		*m = world;
	}
	else {
		debug_info("bad transform %i", mode);
		exit(0);
	}
	return 0;
}

int _opengl_device::Clear(DWORD Count, const opengl_rect * pRects, DWORD Flags, opengl_color Color, float Z, DWORD Stencil) {	
	memset(surface->pixels, 0, local_vp.dwHeight * surface->w * 4);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	return 0;
}

int _opengl_device::SetTextureStageState(DWORD Stage, opengl_tex_state Type, DWORD Value) {
	if (Type == D3DTSS_MAGFILTER) {
		if (Value == D3DTFG_POINT) {
			states.tex_mag_filter = GL_NEAREST;
		}
		else if (Value == D3DTFG_LINEAR) {
			states.tex_mag_filter = GL_LINEAR;
		}
		else {
			states.tex_mag_filter = GL_NEAREST;
		}

	}
	else if (Type == D3DTSS_MINFILTER) {
		if (Value == D3DTFG_POINT) {
			states.tex_min_filter = GL_NEAREST;
		}
		else if (Value == D3DTFG_LINEAR) {
			states.tex_min_filter = GL_LINEAR;
		}
		else {
			states.tex_min_filter = GL_NEAREST;
		}
	}
	return 0;
}



int _opengl_device::SetLight(DWORD id, opengl_light * light) {
	debug_info("set light");
	
	assert(id < 8);
	states.lights[id] = *light;

	return 0;
}

int _opengl_device::LightEnable(DWORD i, DWORD st) {
	assert(i < 8);
	if (st) {
		states.light_enable[i] = 1;		
	}
	else {
		states.light_enable[i] = 0;		
	}
	return 0;
}

int _opengl_device::GetLightEnable(DWORD i, BOOL *ist) {
	*ist = states.light_enable[i];
	return 0;
}

int _opengl_device::ValidateDevice(LPDWORD) {
	return 0;
}

int _opengl_device::SetTexture(DWORD Stage, opengl_surface *surface) {
	GLuint tex;

	if (!surface) {
		useTexture = -1;
		return 0;
	}

	useTexture = 1;
	if (Stage > 100) {
		debug_info("SetTexture %i", Stage);
		exit(0);
	}

	if (bound_textures_size == 0) {
		tex = bound_textures[bound_textures_size];

		glBindTexture(GL_TEXTURE_2D, tex);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		
		bound_textures_size++;
	}
	else {
		tex = bound_textures[0];
		glBindTexture(GL_TEXTURE_2D, tex);
	}
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);

	tex_stages[Stage] = surface;
	return 0;
}

int _opengl_device::GetCaps(opengl_3d_device_desc * d) {
	memset(d, 0, sizeof(opengl_3d_device_desc));
	return 0;
}

int _opengl_device::SetViewport(opengl_viewport * vp) {
	local_vp = *vp;
	debug_info("d3d viewport %i %i, %i %i", vp->dwHeight, vp->dwWidth,vp->dwX,vp->dwY);
	return 0;
}

int _opengl_device::Release() {
	return 0;
}


int D3DXCheckTextureRequirements(opengl_device* pd3dDevice, LPDWORD pFlags, LPDWORD pWidth, LPDWORD pHeight, opengl_surface_format* pPixelFormat) {
	return 0;
}

int D3DXCreateTextureFromFile(opengl_device* pd3dDevice, LPDWORD pFlags, LPDWORD pWidth, LPDWORD pHeight, opengl_surface_format* pPixelFormat,
	opengl_draw_palette* pDDPal, opengl_surface** ppDDSurf, LPDWORD pNumMipMaps, LPCSTR pSrcName, opengl_filtertype filterType) {

	debug_info("D3DXCreateTextureFromFile %s %i %i", pSrcName,*pWidth,*pHeight);
		
	int w = 0;
	int h = 0;
	int i, j;
	unsigned char r, g, b,a;
	opengl_surface * surface = new opengl_surface();
	uint32 * tmp = bmp_load(pSrcName,&w,&h);
	assert(tmp != NULL);
	surface->buildSurface(w, h);
	delete[] surface->pixels;
	surface->pixels = tmp;
	strcpy(surface->filename, pSrcName);

	
	if (*pNumMipMaps > 1) {
		surface->createMipMaps(*pNumMipMaps - 1);
	}
	*ppDDSurf = surface;

	debug_info("BMP loaded with size %i, %i", w, h);
	return 0;
}

int D3DXCreateTexture(LPDIRECT3DDEVICE7 pd3dDevice, LPDWORD pFlags, LPDWORD pWidth, LPDWORD pHeight, opengl_surface_format* pPixelFormat, opengl_draw_palette* pDDPal, opengl_surface** ppDDSurf, LPDWORD pNumMipMaps) {
	opengl_surface * s = new opengl_surface();
	s->buildSurface(*pWidth, *pHeight);
	*pNumMipMaps = 1;
	*ppDDSurf = s;
	return 0;
}

int D3DXInitialize() {
	debug_info("D3DXInitialize");
	return 0;
}

int D3DXUninitialize() {
	debug_info("D3DXUninitialize");
	opengl_exit();
	return 0;
}

int DirectInputCreateEx(HINSTANCE hinst, DWORD dwVersion, int riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter) {
	debug_info("DirectInputCreateEx called for guid %i\n",riidltf);
	return 0;
}

int DirectDrawCreateEx(DWORD * lpGuid, LPVOID  *lplpDD, DWORD iid, IUnknown FAR *pUnkOuter) {
	*lplpDD = new opengl_draw();
	return 0;
}

void D3DXGetErrorString(HRESULT hr, DWORD   strLength, LPSTR   pStr) {
	strcpy(pStr, "unimplemented");
}

//////////////////////////////////////////////////////////////////////////////////////////
/// opengl util
//////////////////////////////////////////////////////////////////////////////////////////


static void glfw_err_callback(int code, const char * desc) {
	debug_info("glfw error %i %s", code, desc);
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == 256) {
		glfwDestroyWindow(window);

		glfwTerminate();
		exit(0);
	}
}

void opengl_get_native_res(int * x, int * y) {
	*x = opengl_local_context.nativeResX;
	*y = opengl_local_context.nativeResY;
}

uint32 mpixels[1920 * 1080];
inline void opengl_do_redraw() {
#ifdef OPENGL_RENDER_TO_BUFFER
	//glReadPixels(0, 0, 1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, mpixels);
	//glBindFramebuffer(GL_FRAMEBUFFER, opengl_local_context.pixelsFrameBuffer);
	//glDrawPixels(1920, 1080, GL_RGBA, GL_UNSIGNED_BYTE, mpixels);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
	//debug_info("new frame");
	//glfwPollEvents();
	glfwSwapBuffers(opengl_local_context.window);

}

void ZSInput_key_event(GLFWwindow* window, int key, int scancode, int action, int mods);

#define OPENGL_WINDOW_TITLE	"Prelude to Darkness"

// The game always renders a frame at the base resolution from gui.ini and this
// layer scales that frame into the window, so the window size is a free choice:
// it only feeds the viewport maths. BeginScene, flush_surface and the mouse
// mapping all recompute from these three fields every time they run, so
// changing resolution needs nothing more than this.
static void opengl_sync_window_size(void) {
	int fbw = 0, fbh = 0;

	glfwGetFramebufferSize(opengl_local_context.window, &fbw, &fbh);
	if (fbw <= 0 || fbh <= 0 || opengl_local_context.baseResY <= 0) {
		return;
	}

	opengl_local_context.nativeResX = fbw;
	opengl_local_context.nativeResY = fbh;
	opengl_local_context.baseResVar = (float)fbh / (float)opengl_local_context.baseResY;

	int xp = (fbw - ((float)opengl_local_context.baseResX * opengl_local_context.baseResVar)) / 2;
	glViewport(xp, 0, fbw - xp * 2, fbh);
	debug_info("window %ix%i, base %ix%i, scale %f, pillarbox %i", fbw, fbh,
		opengl_local_context.baseResX, opengl_local_context.baseResY,
		opengl_local_context.baseResVar, xp);
}

// Size the window to `w`x`h` and centre it, shrinking it if the frame and title
// bar would not fit the monitor work area - picking a resolution at or above
// the desktop's then gives the largest window that stays fully reachable
// instead of one hanging off the bottom of the screen. glfwSetWindowPos places
// the client area, hence the frame terms.
static void opengl_place_window(int w, int h) {
	int ax = 0, ay = 0, aw = 0, ah = 0;
	int fl = 0, ft = 0, fr = 0, fb = 0;

	glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), &ax, &ay, &aw, &ah);
	glfwGetWindowFrameSize(opengl_local_context.window, &fl, &ft, &fr, &fb);

	if (aw > 0 && w > aw - fl - fr) w = aw - fl - fr;
	if (ah > 0 && h > ah - ft - fb) h = ah - ft - fb;

	glfwSetWindowSize(opengl_local_context.window, w, h);
	glfwSetWindowPos(opengl_local_context.window,
		ax + fl + (aw - (w + fl + fr)) / 2,
		ay + ft + (ah - (h + ft + fb)) / 2);
}

static void opengl_close_callback(GLFWwindow * window) {
	opengl_exit();
}

void opengl_set_resolution(int w, int h) {
	if (!opengl_local_context.window) {
		return;
	}

	opengl_local_context.reqResX = w;
	opengl_local_context.reqResY = h;

	opengl_place_window(w, h);
	opengl_sync_window_size();
}

void opengl_get_resolution(int * w, int * h) {
	*w = opengl_local_context.reqResX;
	*h = opengl_local_context.reqResY;
}

// Window pixel -> game pixel. The frame is drawn at baseResVar scale with `xp`
// pillarbox columns either side, so undo exactly that; the game's GUI, mouse
// clamps and hit tests are all in base-resolution coordinates.
// ponytail: assumes window coords are framebuffer pixels, true unless GLFW is
// asked to scale to monitor DPI - divide by the fb/window ratio if that changes.
void opengl_window_to_base(double wx, double wy, int * bx, int * by) {
	float s = opengl_local_context.baseResVar;

	if (s <= 0.0f) {
		*bx = (int)wx;
		*by = (int)wy;
		return;
	}

	int xp = (opengl_local_context.nativeResX - ((float)opengl_local_context.baseResX * s)) / 2;

	*bx = (int)((wx - xp) / s);
	*by = (int)(wy / s);

	if (*bx < 0) *bx = 0;
	if (*by < 0) *by = 0;
}

void * opengl_create_window(int w, int h, int windowed, int winW, int winH) {
	glfwSetErrorCallback(glfw_err_callback);
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	// Size comes from the in-game options, not from dragging the window edge -
	// one code path for the resolution instead of two.
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	if (windowed) {
		opengl_local_context.reqResX = winW;
		opengl_local_context.reqResY = winH;
		opengl_local_context.window = glfwCreateWindow(winW, winH, OPENGL_WINDOW_TITLE, NULL, NULL);
	}
	else {
		opengl_local_context.reqResX = mode->width;
		opengl_local_context.reqResY = mode->height;
		glfwWindowHint(GLFW_DECORATED, GL_FALSE);
		opengl_local_context.window = glfwCreateWindow(mode->width, mode->height,
			OPENGL_WINDOW_TITLE, glfwGetPrimaryMonitor(), NULL);
	}

	if (!opengl_local_context.window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	opengl_local_context.baseResY = h;
	opengl_local_context.baseResX = w;
	opengl_local_context.baseResRatio = (float)w / (float)h;


	//glfwSetKeyCallback(opengl_local_context.window, opengl_key_event);
	glfwSetKeyCallback(opengl_local_context.window, ZSInput_key_event);

	glfwSetMouseButtonCallback(opengl_local_context.window, opengl_mouse_event);
	glfwSetCursorPosCallback(opengl_local_context.window, opengl_mouse_cursor_event);
	glfwSetScrollCallback(opengl_local_context.window, opengl_mouse_wheel_event);
	// A decorated window has a close button; without this it does nothing.
	glfwSetWindowCloseCallback(opengl_local_context.window, opengl_close_callback);

	glfwMakeContextCurrent(opengl_local_context.window);
	glewInit();

	glfwSwapInterval(1);

	if (windowed) {
		opengl_place_window(winW, winH);
	}
	opengl_sync_window_size();

#ifdef _WIN32
	void * wd = (void*)glfwGetWin32Window(opengl_local_context.window);
#elif __linux__
	void * wd = (void*)glfwGetX11Window(opengl_local_context.window);
#endif
	debug_info("Window handler %X\n", wd);


#ifdef OPENGL_RENDER_TO_BUFFER
	/*glGenRenderbuffers(1, &opengl_local_context.renderBuffer);
	glGenFramebuffers(1, &opengl_local_context.pixelsFrameBuffer);

	glBindRenderbuffer(GL_RENDERBUFFER, opengl_local_context.renderBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, 1920, 1080);

	glBindFramebuffer(GL_FRAMEBUFFER, opengl_local_context.pixelsFrameBuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, opengl_local_context.renderBuffer);
	*/
#endif
	
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	glfwPollEvents();
	opengl_do_redraw();	
	
	return wd;
}



void opengl_set_primary_surface(opengl_surface * surface) {
	opengl_local_context.primary = surface;
}

void opengl_set_backbuffer(opengl_surface * bbuffer) {
	opengl_local_context.back_buffer = bbuffer;
}

void opengl_swap_primary_surface(opengl_surface * s1, opengl_surface * s2) {
	if (opengl_local_context.primary == s1) {
		opengl_local_context.primary = s2;
	}
	else if (opengl_local_context.primary == s2) {
		opengl_local_context.primary = s1;
	}
	else {
		debug_info("bad swap");
		exit(0);
	}
}

void aspectSurfaceBounds(opengl_surface * surface, int * w, int * h) {

	float nx = (float)surface->w *opengl_local_context.baseResVar;
	(*w) = (int)nx;
	(*h) = (float)surface->h * opengl_local_context.baseResVar;
	
	
	//debug_info("reaspect to %i %i ", *w, *h);
}

/// Actually apply the surface to the window, if it is the primary
void flush_surface(opengl_surface * surface) {
	if (surface != opengl_local_context.primary) {
		return;
	}
	
	//debug_info("draw from %X", surface->pixels);
	int x=0, y=0, w, h;
	
	aspectSurfaceBounds(surface,&w, &h);
	
	glMatrixMode(GL_PROJECTION);	
	glLoadIdentity();
	
	int xp = (opengl_local_context.nativeResX - ((float)opengl_local_context.baseResX*opengl_local_context.baseResVar)) / 2;

	glOrtho(0.0, opengl_local_context.nativeResX - xp * 2, 0, opengl_local_context.nativeResY, -3.0, 3.0);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	
	glViewport(xp, 0, opengl_local_context.nativeResX - xp * 2, opengl_local_context.nativeResY);

	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glUseProgram(0);

	glEnable(GL_TEXTURE_2D);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, surface->tex);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
	
	glColor4f(1, 1, 1, 1);	
	
	glBegin(GL_QUADS);
	glTexCoord2f(0, 1);
	glVertex3f(x, y, 3);
	glTexCoord2f(1, 1);
	glVertex3f(x+w, y, 3);
	glTexCoord2f(1, 0);
	glVertex3f(x+w, y+h, 3);
	glTexCoord2f(0, 0);
	glVertex3f(x, y+h, 3);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
	
	opengl_do_redraw();

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
		
}

void opengl_exit() {
	glfwDestroyWindow(opengl_local_context.window);
	glfwTerminate();
	exit(0);
}

void opengl_poll() {
	glfwPollEvents();
}

// The game draws its own cursor, so the system one is hidden - but only over
// the client area. Win32's ShowCursor(FALSE) would also blank it over the
// title bar, which a windowed game still needs you to be able to grab.
void opengl_show_cursor(int show) {
	if (!opengl_local_context.window) {
		return;
	}
	glfwSetInputMode(opengl_local_context.window, GLFW_CURSOR,
		show ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}

#ifdef __linux__
void ShowCursor(BOOL bShow) {
	if (!bShow) {
		glfwSetInputMode(opengl_local_context.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}
	else {
		glfwSetInputMode(opengl_local_context.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

BOOL GetWindowRect(HWND hWnd, LPRECT lpRect) {
	// FIXME to implement
	return 1;
}

BOOL GetCursorPos(LPPOINT lpPoint) {
	return 1;
}

#endif

int opengl_light::equal(opengl_light * l2) {
	return memcmp(this, l2, sizeof(opengl_light));
	
}
#endif
