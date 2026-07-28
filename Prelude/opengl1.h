#ifndef _OPENGL1_H_
#define _OPENGL1_H_

#ifdef _WIN32
#include <Windows.h>
#include <Unknwnbase.h>
#endif

#ifdef __linux__ 
#include "linux_aux_wrapper.h"
#endif

#include <GL/glew.h>
#include <math.h>

#include "opengl_dinput.h"
#include "opengl_dx_enums.h"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#elif __linux__ 
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#endif
#ifdef __linux__
// some hack to avoid clashes with X11 Region
#define Region Region_X11
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#ifdef __linux__
#undef Region
#endif
#include <vector>
#include <unordered_map>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <chrono>

typedef struct _opengl_draw opengl_draw;
typedef struct _opengl_device opengl_device;

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint8_t byte;

typedef struct _opengl_color_value {
	union {
		float r;
		float dvR;
	};
	union {
		float g;
		float dvG;
	};
	union {
		float b;
		float dvB;
	};
	union {
		float a;
		float dvA;
	};
} opengl_color_value;
static_assert(sizeof(_opengl_color_value) == 16, "");

typedef struct _opengl_pixelformat {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwFourCC;
	union {
		DWORD   dwRGBBitCount;          // how many bits per pixel
		DWORD   dwYUVBitCount;          // how many bits per pixel
		DWORD   dwZBufferBitDepth;      // how many total bits/pixel in z buffer (including any stencil bits)
		DWORD   dwAlphaBitDepth;        // how many bits for alpha channels
		DWORD   dwLuminanceBitCount;    // how many bits per pixel
		DWORD   dwBumpBitCount;         // how many bits per "buxel", total
	};
	union {
		DWORD   dwRBitMask;             // mask for red bit
		DWORD   dwYBitMask;             // mask for Y bits
		DWORD   dwStencilBitDepth;      // how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is total Z-only bits)
		DWORD   dwLuminanceBitMask;     // mask for luminance bits
		DWORD   dwBumpDuBitMask;        // mask for bump map U delta bits
	};
	union {
		DWORD   dwGBitMask;             // mask for green bits
		DWORD   dwUBitMask;             // mask for U bits
		DWORD   dwZBitMask;             // mask for Z bits
		DWORD   dwBumpDvBitMask;        // mask for bump map V delta bits
	};
	union {
		DWORD   dwBBitMask;             // mask for blue bits
		DWORD   dwVBitMask;             // mask for V bits
		DWORD   dwStencilBitMask;       // mask for stencil bits
		DWORD   dwBumpLuminanceBitMask; // mask for luminance in bump map
	};
	union {
		DWORD   dwRGBAlphaBitMask;      // mask for alpha channel
		DWORD   dwYUVAlphaBitMask;      // mask for alpha channel
		DWORD   dwLuminanceAlphaBitMask;// mask for alpha channel
		DWORD   dwRGBZBitMask;          // mask for Z channel
		DWORD   dwYUVZBitMask;          // mask for Z channel
	};
} opengl_pixelformat;

typedef struct _opengl_color_key {
	DWORD       dwColorSpaceLowValue;   // low boundary of color space that is to
										// be treated as Color Key, inclusive
	DWORD       dwColorSpaceHighValue;  // high boundary of color space that is
										// to be treated as Color Key, inclusive
} opengl_color_key;




typedef struct _opengl_ddscaps {
	DWORD       dwCaps;         // capabilities of surface wanted
	DWORD       dwCaps2;
	DWORD       dwCaps3;
	DWORD       dwCaps4;
} opengl_ddscaps, DDSCAPS2;

typedef struct _opengl_surface_desc2 {
	DWORD               dwSize;                 // size of the DDSURFACEDESC structure
	DWORD               dwFlags;                // determines what fields are valid
	DWORD               dwHeight;               // height of surface to be created
	DWORD               dwWidth;
	union {
		LONG            lPitch;                 // distance to start of next line (return value only)
		DWORD           dwLinearSize;           // Formless late-allocated optimized surface size
	};
	DWORD               dwBackBufferCount;      // number of back buffers requested
	union {
		DWORD           dwMipMapCount;          // number of mip-map levels requestde
												// dwZBufferBitDepth removed, use ddpfPixelFormat one instead
		DWORD           dwRefreshRate;          // refresh rate (used when display mode is described)
		DWORD           dwSrcVBHandle;          // The source used in VB::Optimize
	};
	DWORD               dwAlphaBitDepth;        // depth of alpha buffer requested
	DWORD               dwReserved;             // reserved
	LPVOID              lpSurface;              // pointer to the associated surface memory
	union {
		opengl_color_key      ddckCKDestOverlay;      // color key for destination overlay use
		DWORD           dwEmptyFaceColor;       // Physical color for empty cubemap faces
	} ;
	opengl_color_key          ddckCKDestBlt;          // color key for destination blt use
	opengl_color_key          ddckCKSrcOverlay;       // color key for source overlay use
	opengl_color_key          ddckCKSrcBlt;           // color key for source blt use
	union {
		opengl_pixelformat   ddpfPixelFormat;        // pixel format description of the surface
		DWORD           dwFVF;                  // vertex format description of vertex buffers
	} ;
	opengl_ddscaps            ddsCaps;
	DWORD               dwTextureStage;         // stage in multitexture cascade
} opengl_surface_desc2;


typedef struct _opengl_clipper {
	int SetClipList(RGNDATA*, DWORD);
	int SetHWnd(DWORD, HWND);
	int Release();
} opengl_clipper;

typedef struct _opengl_aux_dblt opengl_aux_dblt;

typedef struct _opengl_surface {
	int hasTexture = 0;
	DWORD watermark;
	GLuint tex;
	opengl_clipper* clipper;
	opengl_surface_desc2 * desc;
	struct _opengl_surface * bbuffer;
	opengl_draw * draw;
	int w, h;
	int x, y;
	int refcount;
	int mipmaps_size;
	uint32 * pixels;
	opengl_color_key color_key;

	char filename[256];

	int Blt (LPRECT unnamedParam1, struct _opengl_surface * unnamedParam2, LPRECT unnamedParam3, DWORD unnamedParam4, opengl_aux_dblt * unnamedParam5);
	void flush();
	int Release();
	int Restore();
	int AddRef();
	int Lock(LPRECT, opengl_surface_desc2*, DWORD, HANDLE);
	int Unlock(void*);
	int SetColorKey(DWORD, opengl_color_key*);
	int ReleaseDC(HDC);
	int GetDC(HDC*);
	int Flip(opengl_draw*,DWORD);
	int SetClipper(opengl_clipper* iclipper) {
		clipper = iclipper;
		return 0;
	}
	int GetPixelFormat(opengl_pixelformat*);
	int GetAttachedSurface(opengl_ddscaps*, struct _opengl_surface**);
	int AddAttachedSurface(struct _opengl_surface *);

	int createMipMaps(int number);
	
	_opengl_surface() {
		pixels = NULL;
		draw = NULL;
		clipper = NULL;
		mipmaps_size = 0;
		memset(&color_key, 0, sizeof(color_key));
		watermark = 0x11223344;
		x = y = 0;
		refcount = 1;
		filename[0] = 0;
		h = w = 0;
		hasTexture = 0;
	}

	void buildSurface(int w, int h);

	_opengl_surface(struct _opengl_surface * iref, opengl_draw * idraw);
	~_opengl_surface() {
		hasTexture = 0;
		h = w = 0;
		if (pixels) delete[] pixels;
	}

	////////////////////////////////////////////////////
	/// utils

	int bltBitmap(const char * fname, int width, int height);

	int loadBitmap(const char *);
} opengl_surface;

typedef struct _opengl_aux_dblt {
	DWORD      dwSize;
	DWORD      dwDDFX;
	DWORD      dwROP;
	DWORD      dwDDROP;
	DWORD      dwRotationAngle;
	DWORD      dwZBufferOpCode;
	DWORD      dwZBufferLow;
	DWORD      dwZBufferHigh;
	DWORD      dwZBufferBaseDest;
	DWORD      dwZDestConstBitDepth;
	union {
		DWORD   dwZDestConst;                   // Constant to use as Z buffer for dest
		opengl_surface * lpDDSZBufferDest;   // Surface to use as Z buffer for dest
	};
	DWORD       dwZSrcConstBitDepth;            // Bit depth used to specify Z constant for source
	union {
		DWORD   dwZSrcConst;                    // Constant to use as Z buffer for src
		opengl_surface * lpDDSZBufferSrc;    // Surface to use as Z buffer for src
	};
	DWORD       dwAlphaEdgeBlendBitDepth;       // Bit depth used to specify constant for alpha edge blend
	DWORD       dwAlphaEdgeBlend;               // Alpha for edge blending
	DWORD       dwReserved;
	DWORD       dwAlphaDestConstBitDepth;       // Bit depth used to specify alpha constant for destination
	union {
		DWORD   dwAlphaDestConst;               // Constant to use as Alpha Channel
		opengl_surface * lpDDSAlphaDest;     // Surface to use as Alpha Channel
	};
	DWORD       dwAlphaSrcConstBitDepth;        // Bit depth used to specify alpha constant for source
	union {
		DWORD   dwAlphaSrcConst;                // Constant to use as Alpha Channel
		opengl_surface * lpDDSAlphaSrc;      // Surface to use as Alpha Channel
	};
	union {
		DWORD   dwFillColor;                    // color in RGB or Palettized
		DWORD   dwFillDepth;                    // depth value for z-buffer
		DWORD   dwFillPixel;                    // pixel value for RGBA or RGBZ
		opengl_surface * lpDDSPattern;       // Surface to use as pattern
	};

	opengl_color_key ddckDestColorkey;
	opengl_color_key ddckSrcColorkey;

} opengl_aux_dblt;


typedef struct _opengl_rect {

} opengl_rect;


typedef struct _opengl_draw {
	int w, h, bpp;
	int CreateSurface(opengl_surface_desc2 *, opengl_surface**, void*);
	int CreateClipper(DWORD, opengl_clipper** ,void*);
	int QueryInterface(DWORD, void*);
	int SetCooperativeLevel(HWND, DWORD);
	int SetDisplayMode(DWORD, DWORD, DWORD, DWORD, DWORD);
	int Release();
} opengl_draw;

typedef struct _opengl_3d {
	int EnumZBufferFormats(DWORD, HRESULT (__stdcall *cb)(opengl_pixelformat* lpDDPixFmt, void* lpContext), void*);
	int CreateDevice(DWORD,opengl_surface*,opengl_device**);
	int Release();
	
} opengl_3d;

typedef struct _opengl_matrix {
	union {
		struct {
			float _11; float _12; float _13; float _14;
			float _21; float _22; float _23; float _24;
			float _31; float _32; float _33; float _34;
			float _41; float _42; float _43; float _44;
		};
		float m[4][4];
	};
	float& operator()(int iRow, int iColumn) { return m[iRow][iColumn]; }

	float * asFArray() {
		return (float*)&_11;
	}

	void identity() {
		memset(&m[0][0], 0, sizeof(m));
		_11 = _22 = _33 = _44 = 1;
	}

	float * transpose(float * tr) {
		tr[0] = _11; tr[1] = _21; tr[2] = _31; tr[3] = _41;
		tr[4] = _12; tr[5] = _22; tr[6] = _32; tr[7] = _42;
		tr[8] = _13; tr[9] = _23; tr[10] = _33; tr[11] = _43;
		tr[12] = _14; tr[13] = _24; tr[14] = _34; tr[15] = _44;
		return tr;
	}
} opengl_matrix;

typedef struct _opengl_color {
public:
	DWORD color;
	_opengl_color() {

	}
	_opengl_color(DWORD cl) {
		color = cl;
	}
	_opengl_color(float ir, float ig, float ib) {
		DWORD ri = (DWORD)(ir * 255);
		DWORD gi = (DWORD)(ig * 255);
		DWORD bi = (DWORD)(ib * 255);
		DWORD ai = 0xFF;
		color = (ri << 0) + (gi << 8) + (bi << 16) + (ai << 24);
	}

	_opengl_color(int ir, int ig, int ib, int ia) {
		color = (ir << 0) + (ig << 8) + (ib << 16) + (ia << 24);
	}

	_opengl_color(DWORD ir, DWORD ig, DWORD ib, DWORD ia) {
		color = (ir << 0) + (ig << 8) + (ib << 16) + (ia << 24);
	}

	_opengl_color(float ir, float ig, float ib, float ia) {
		DWORD ri = (DWORD)(ir * 255);
		DWORD gi = (DWORD)(ig * 255);
		DWORD bi = (DWORD)(ib * 255);
		DWORD ai = (DWORD)(ia * 255);
		color = (ri << 0) + (gi << 8) + (bi << 16) + (ai << 24);
	}

	DWORD red() {
		return color & 0xFF;
	}

	DWORD green() {
		return (color>>8) & 0xFF;
	}

	DWORD blue() {
		return (color>>16) & 0xFF;
	}

	DWORD alpha() {
		return (color>>24) & 0xFF;
	}

	DWORD dword() {
		return color;
	}
} opengl_color;

typedef struct _opengl_material {
	union {
		opengl_color_value   diffuse;        /* Diffuse color RGBA */
		opengl_color_value   dcvDiffuse;
	};
	union {
		opengl_color_value   ambient;        /* Ambient color RGB */
		opengl_color_value   dcvAmbient;
	};
	union {
		opengl_color_value   specular;       /* Specular 'shininess' */
		opengl_color_value   dcvSpecular;
	};
	union {
		opengl_color_value   emissive;       /* Emissive color RGB */
		opengl_color_value   dcvEmissive;
	};
	union {
		float        power;          /* Sharpness if specular highlight */
		float        dvPower;
	};

	_opengl_material() {

	}

	char * str(char * data) {
		sprintf(data, "{%f %f %f %f,%f %f %f %f,%f %f %f %f,%f %f %f %f -- %f}",
			diffuse.r, diffuse.g, diffuse.b, diffuse.a,
			ambient.r, ambient.g, ambient.b, ambient.a,
			specular.r, specular.g, specular.b, specular.a,
			emissive.r, emissive.g, emissive.b, emissive.a,
			power);
		return data;
	}
	
} opengl_material;



typedef struct _opengl_vertex {
	union {
		float     x;             /* Homogeneous coordinates */
		float     dvX;
	};
	union {
		float     y;
		float     dvY;
	};
	union {
		float     z;
		float     dvZ;
	};
	union {
		float     nx;            /* Normal */
		float     dvNX;
	};
	union {
		float     ny;
		float     dvNY;
	};
	union {
		float     nz;
		float     dvNZ;
	};
	union {
		float     tu;            /* Texture coordinates */
		float     dvTU;
	};
	union {
		float     tv;
		float     dvTV;
	};	
	_opengl_vertex() {

	}
} opengl_vertex;

typedef struct _opengl_tlvertex {
	_opengl_tlvertex() {

	}
	union {
		float sx;
		float dvSX;
	};
	union {
		float sy;
		float dvSY;
	};
	union {
		float sz;
		float dvSZ;
	};
	union {
		float rhw;
		float dvRHW;
	};
	union {
		opengl_color color;
		opengl_color dcColor;
	};
	union {
		opengl_color specular;
		opengl_color dcSpecular;
	};
	union {
		float tu;
		float dvTU;
	};
	union {
		float tv;
		float dvTV;
	};
} opengl_tlvertex;

typedef struct _opengl_lvertex {
	union {
		float     x;             /* Homogeneous coordinates */
		float     dvX;
	};
	union {
		float     y;
		float     dvY;
	};
	union {
		float     z;
		float     dvZ;
	};
	DWORD            dwReserved;
	union {
		opengl_color     color;         /* Vertex color */
		opengl_color     dcColor;
	};
	union {
		opengl_color     specular;      /* Specular component of vertex */
		opengl_color     dcSpecular;
	};
	union {
		float     tu;            /* Texture coordinates */
		float     dvTU;
	};
	union {
		float     tv;
		float     dvTV;
	};
	_opengl_lvertex() {

	}
} opengl_lvertex;

typedef struct _opengl_vector {
	union {
		float x;
		float dvX;
	};
	union {
		float y;
		float dvY;
	};
	union {
		float z;
		float dvZ;
	};

	_opengl_vector(float a, float b, float c) {
		x = a;
		y = b;
		z = c;
	}
	_opengl_vector() {
		x = y = z = 0;
	}

	_opengl_vector& operator += (const _opengl_vector& v) {
		x += v.x;
		y += v.y;
		z += v.z;

		return *this;
	}

	_opengl_vector& operator -= (const _opengl_vector& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;

		return *this;
	}

	_opengl_vector& operator *= (const float& f) {
		x *= f;
		y *= f;
		z *= f;

		return *this;
	}

	_opengl_vector normal() {
		float s1 = sqrt((x*x) + (y*y) + (z*z));
		return _opengl_vector(x/s1,y/s1,z/s1);
	}

	friend int operator == (const struct _opengl_vector& v1, const struct _opengl_vector& v2) {
		return ((v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z));
	}

	friend struct _opengl_vector operator - (const struct _opengl_vector& v1, const struct _opengl_vector& v2) {
		return _opengl_vector(v1.x-v2.x, v1.y-v2.y, v1.z-v2.z);
	}

	friend struct _opengl_vector operator + (const struct _opengl_vector& v1, const struct _opengl_vector& v2) {
		return _opengl_vector(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
	}

	friend struct _opengl_vector operator * (const struct _opengl_vector& v1, const float& r) {
		return _opengl_vector(v1.x*r, v1.y*r, v1.z*r);
	}

} opengl_vector;


typedef struct _opengl_stride_aux{
	void * lpvData;
	DWORD dwStride;
} opengl_stride_aux;

typedef struct _opengl_primitive_strided_data {
	opengl_stride_aux position;
	opengl_stride_aux normal;
	opengl_stride_aux diffuse;
	opengl_stride_aux specular;
	opengl_stride_aux textureCoords[8];
} opengl_primitive_strided_data;

#pragma pack(1)
typedef struct opengl_light {
	opengl_light_type    dltType;            /* Type of light source */
	opengl_color_value   dcvDiffuse;         /* Diffuse color of light */
	opengl_color_value   dcvSpecular;        /* Specular color of light */
	opengl_color_value   dcvAmbient;         /* Ambient color of light */
	opengl_vector       dvPosition;         /* Position in world space */
	opengl_vector       dvDirection;        /* Direction in world space */
	float        dvRange;            /* Cutoff range */
	float        dvFalloff;          /* Falloff */
	float        dvAttenuation0;     /* Constant attenuation */
	float        dvAttenuation1;     /* Linear attenuation */
	float        dvAttenuation2;     /* Quadratic attenuation */
	float        dvTheta;            /* Inner angle of spotlight cone */
	float        dvPhi;

	int equal(opengl_light * l2);

	char * str(char * data) {
		sprintf(data, "{%i -- %f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f -- %f,%f,%f,%f,%f,%f -- %f,%f,%f,%f,%f,%f,%f}", dltType,
			dcvDiffuse.r, dcvDiffuse.g, dcvDiffuse.b, dcvDiffuse.a,
			dcvSpecular.r, dcvSpecular.g, dcvSpecular.b, dcvSpecular.a,
			dcvAmbient.r, dcvAmbient.g, dcvAmbient.b, dcvAmbient.a,
			dvPosition.x, dvPosition.y, dvPosition.z,
			dvDirection.x, dvDirection.y, dvDirection.z,
			dvRange, dvFalloff, dvAttenuation0, dvAttenuation1, dvAttenuation2, dvTheta, dvPhi);
		return data;
	}

} opengl_light;

#pragma pack()

typedef struct _opengl_propheader {
	DWORD   dwSize;
	DWORD   dwHeaderSize;
	DWORD   dwObj;
	DWORD   dwHow;
} opengl_propheader;

typedef struct _opengl_propdword {
	opengl_propheader diph;
	DWORD   dwData;
} opengl_propdword;

typedef struct _opengl_3d_device_desc {
	DWORD            dwDevCaps;              /* Capabilities of device */
											 //D3DPRIMCAPS      dpcLineCaps;
											 //D3DPRIMCAPS      dpcTriCaps;
	DWORD            dwDeviceRenderBitDepth; /* One of DDBB_8, 16, etc.. */
	DWORD            dwDeviceZBufferBitDepth;/* One of DDBD_16, 32, etc.. */

	DWORD       dwMinTextureWidth, dwMinTextureHeight;
	DWORD       dwMaxTextureWidth, dwMaxTextureHeight;

	DWORD       dwMaxTextureRepeat;
	DWORD       dwMaxTextureAspectRatio;
	DWORD       dwMaxAnisotropy;

	float    dvGuardBandLeft;
	float    dvGuardBandTop;
	float    dvGuardBandRight;
	float    dvGuardBandBottom;

	float    dvExtentsAdjust;
	DWORD       dwStencilCaps;

	DWORD       dwFVFCaps;
	DWORD       dwTextureOpCaps;
	WORD        wMaxTextureBlendStages;
	WORD        wMaxSimultaneousTextures;

	DWORD       dwMaxActiveLights;
	float    dvMaxVertexW;
	DWORD        deviceGUID;

	WORD        wMaxUserClipPlanes;
	WORD        wMaxVertexBlendMatrices;

	DWORD       dwVertexProcessingCaps;

	DWORD       dwReserved1;
	DWORD       dwReserved2;
	DWORD       dwReserved3;
	DWORD       dwReserved4;
} opengl_3d_device_desc;

typedef struct _opengl_viewport {
	DWORD       dwX;
	DWORD       dwY;            /* Viewport Top left */
	DWORD       dwWidth;
	DWORD       dwHeight;       /* Viewport Dimensions */
	float    dvMinZ;         /* Min/max of clip Volume */
	float    dvMaxZ;
} opengl_viewport;

typedef struct _opengl_device_states {
	GLuint srcblend, dstblend;
	GLuint polygon;
	int alphatest;
	float alpharef;
	GLuint alphafunc;
	int blend;
	int lighting;
	int depth;
	int cullmode;
	opengl_material color;
	opengl_light lights[8];
	int light_enable[8];
	int zwrite;
	
	// textures

	int tex_mag_filter;
	int tex_min_filter;

	void dump(char * data) {
		char tmp[2048];
		sprintf(data, "srcblend: %i\ndstblend: %i\npolygon: %i\nalphatest: %i\nalpharef: %f\nalphafunc: %i\nblend: %i\nlighting: %i\ndepth: %i\ncullmode: %i\ncolor: %s\nlights: %s\n %s\n %s\n %s\n %s\n %s\n %s\n %s\nlight_enable: %i %i %i %i %i %i %i %i\nzwrite: %i\ntex_mag_filter: %i\ntex_min_filter: %i\n",
			srcblend,dstblend,polygon,alphatest,alpharef,alphafunc,blend,lighting,depth,cullmode,color.str(tmp),
			lights[0].str(tmp), lights[1].str(tmp), lights[2].str(tmp), lights[3].str(tmp), 
			lights[4].str(tmp), lights[5].str(tmp), lights[6].str(tmp), lights[7].str(tmp),
			light_enable[0], light_enable[1], light_enable[2], light_enable[3], 
			light_enable[4], light_enable[5], light_enable[6], light_enable[7],
			zwrite, tex_mag_filter, tex_min_filter);
	}

	_opengl_device_states() {
		polygon = GL_LINE;
		blend = 1;
		zwrite = 1;
		cullmode = 0;
		depth = 1;
		lighting = 0;
		//srcblend = GL_SRC_ALPHA;
		//dstblend = GL_ONE_MINUS_SRC_ALPHA;

		srcblend = GL_ONE;
		dstblend = GL_ZERO;
		alphatest = 0;
		alpharef = 0;
		alphafunc = 0;
		color.dcvDiffuse.a = color.dcvDiffuse.r = color.dcvDiffuse.g = color.dcvDiffuse.b = 1;
		memset(light_enable, 0, sizeof(light_enable));
		tex_mag_filter = tex_min_filter = GL_NEAREST;
	}
	int gl_set_states();
} opengl_device_states;

typedef struct _opengl_device {
	int useTexture;
	opengl_surface* tex_stages[100];
	opengl_viewport local_vp;
	opengl_matrix projection;
	opengl_matrix model;
	opengl_matrix world;
	GLuint va[3];
	GLuint vb[3];
	opengl_surface * surface;

	opengl_device_states states;
	int state;

	GLuint bound_textures[128];
	int bound_textures_size;
	std::unordered_map<opengl_surface*, int> texmap;
	
	std::vector<opengl_vertex> tmp_vertex_buffer;

	

	int SetRenderState(opengl_renderstate state, DWORD st);
	int SetMaterial(opengl_material * material);
	int SetTransform(opengl_transforms tr, opengl_matrix * m);
	void buildVPMatrix(opengl_matrix * m);
	void prepareDraw(DWORD vtype);
	GLuint mapPrimitiveType(opengl_primitive_type type);
	int DrawPrimitive(opengl_primitive_type type, DWORD vtype, void *, DWORD, DWORD);
	int BeginScene();
	int EndScene();
	int GetTransform(opengl_transforms mode, opengl_matrix * m);
	int Clear(DWORD Count, const opengl_rect *pRects,DWORD Flags, opengl_color Color, float Z, DWORD Stencil);
	int SetTextureStageState(DWORD Stage, opengl_tex_state Type, DWORD Value);
	int DrawIndexedPrimitiveStrided(opengl_primitive_type d3dptPrimitiveType, DWORD  dwVertexTypeDesc, opengl_primitive_strided_data* lpVertexArray, DWORD dwVertexCount, LPWORD lpwIndices, DWORD  dwIndexCount, DWORD  dwFlags);
	int DrawIndexedPrimitive(opengl_primitive_type unnamedParam1, DWORD, LPVOID, DWORD, LPWORD, DWORD, DWORD);
	int SetLight(DWORD, opengl_light *);
	int LightEnable(DWORD, DWORD);
	int GetLightEnable(DWORD, BOOL*);
	int ValidateDevice(LPDWORD);
	int SetTexture(DWORD, opengl_surface*);
	int GetCaps(opengl_3d_device_desc*);
	int SetViewport(opengl_viewport*);
	int Release();
} opengl_device;



typedef struct _opengl_input_data_format {
	DWORD   dwSize;
	DWORD   dwObjSize;
	DWORD   dwFlags;
	DWORD   dwDataSize;
	DWORD   dwNumObjs;
	
} opengl_input_data_format;


typedef struct _opengl_input {

	
	int CreateDeviceEx(int, int, LPVOID *, LPUNKNOWN);
	int Release();
} opengl_input;

typedef struct _opengl_key_event_queue {
	struct _opengl_key_event_queue * next;
	int key;
	int action;
} opengl_key_event_queue;

typedef struct _opengl_input_device {
	int x, y,z;
	int lx, ly, lz;
	unsigned char button_state[5];
	void * eventNotification;
	int key;
	unsigned char key_state[512];

	int type;
	int SetCooperativeLevel(HWND, DWORD);
	int SetDataFormat(opengl_input_data_format *);
	int SetEventNotification(HANDLE);
	int Acquire();
	int Unacquire();
	int Release();
	int GetDeviceState(DWORD, void*);
	int setEvent();
	int setKeyEvent(int key, int scancode, int action, int mods);

	int setScrollEvent(double var);

	int setMouseEvent(int button, int action, int mods, double xpos, double ypos);
	std::chrono::time_point<std::chrono::high_resolution_clock> lastLeftClickEvent;
	int leftState;

	_opengl_input_device(int itype) {
		type = itype;
		eventNotification = NULL;
		memset(button_state, 0, sizeof(button_state));
		z = x = y = 0;
		lz = lx = ly = 0;
		memset(key_state, 0, sizeof(key_state));
		leftState = 0;
	}
} opengl_input_device;

typedef struct opengl_mouse_state {
	LONG lX;
	LONG lY;
	LONG lZ;
	BYTE rgbButtons[5];
} opengl_mouse_state; 

typedef struct _opengl_draw_palette {

} opengl_draw_palette;



// directx map
typedef opengl_vector D3DVECTOR, D3DXVECTOR3, D3DXVECTOR4, _D3DVECTOR;
typedef opengl_color D3DCOLOR;
typedef opengl_color_value D3DCOLORVALUE;
typedef opengl_vertex D3DVERTEX;
typedef opengl_tlvertex D3DTLVERTEX;
typedef opengl_blend_opts D3DBLEND;
typedef opengl_material D3DMATERIAL7;
typedef opengl_texture_operation D3DTEXTUREOP;
typedef opengl_lvertex D3DLVERTEX;
typedef opengl_matrix D3DMATRIX, D3DXMATRIX;
typedef opengl_device *LPDIRECT3DDEVICE7;
typedef opengl_surface *LPDIRECTDRAWSURFACE7;
typedef opengl_draw *LPDIRECTDRAW7, *LPDIRECTDRAW;
typedef opengl_clipper *LPDIRECTDRAWCLIPPER;
typedef opengl_pixelformat * LPDDPIXELFORMAT, DDPIXELFORMAT;
typedef opengl_mouse_state  DIMOUSESTATE, *LPDIMOUSESTATE;
typedef opengl_surface_format D3DX_SURFACEFORMAT;
typedef opengl_propdword DIPROPDWORD;
typedef opengl_propheader DIPROPHEADER;
typedef opengl_3d *LPDIRECT3D7;
typedef opengl_draw_palette *LPDIRECTDRAWPALETTE;
typedef opengl_renderstate D3DRENDERSTATETYPE;

typedef opengl_input * LPDIRECTINPUT7;
typedef opengl_input_device *LPDIRECTINPUTDEVICE7;

typedef opengl_primitive_strided_data D3DDRAWPRIMITIVESTRIDEDDATA;

typedef opengl_light D3DLIGHT7;
typedef opengl_aux_dblt DDBLTFX;
typedef opengl_rect D3DRECT;
typedef opengl_viewport D3DVIEWPORT7;

typedef opengl_surface_desc2 DDSURFACEDESC2;
typedef opengl_color_key DDCOLORKEY;

typedef opengl_3d_device_desc D3DDEVICEDESC7;

typedef float D3DVALUE;


#define OPENGLRGB(r,g,b) opengl_color(r,g,b)
#define D3DRGB(r,g,b) opengl_color(r,g,b)
#define RGB_MAKE(r,g,b) D3DRGB(r,g,b)
#define OPENGLRGBINT(i) opengl_color(i)
#define D3DRGBA(r,g,b,a) opengl_color(r,g,b,a)
#define OPENGLRGBA(r,g,b,a) opengl_color(r,g,b,a)

#define RGBA_GETRED(c) (c & 0xFF)
#define RGBA_GETGREEN(c) ((c >> 8) & 0xFF)
#define RGBA_GETBLUE(c) ((c >> 16) & 0xFF)
#define RGBA_GETALPHA(c) ((c >> 24) & 0xFF)
#define RGBA_MAKE(r,g,b,a) opengl_color(r,g,b,a).dword()

typedef enum _opengl_misc {
	D3DCULL_NONE = 1000,
	D3DCULL_CCW
} opengl_misc;

// directx functions
opengl_matrix* D3DXMatrixIdentity(opengl_matrix * m);
opengl_matrix* D3DXMatrixRotationZ(opengl_matrix* m, float radians);
opengl_matrix* D3DXMatrixRotationY(opengl_matrix* m, float radians);
opengl_matrix* D3DXMatrixRotationX(opengl_matrix* m, float radians);
opengl_matrix* D3DXMatrixRotationYawPitchRoll(opengl_matrix *pOut, float yaw, float pitch, float roll);
opengl_matrix* D3DXMatrixTranslation(opengl_matrix *m, float x, float y, float z);
opengl_matrix* D3DXMatrixRotationAxis(opengl_matrix * m, opengl_vector * axis, float radians);

opengl_matrix* D3DXMatrixMultiply(opengl_matrix * out, opengl_matrix * m1, opengl_matrix * m2);

opengl_vector* D3DXVec3Transform(opengl_vector * out, opengl_vector * v, opengl_matrix * m);
opengl_matrix* D3DXMatrixInverse(opengl_matrix * out, float * det, opengl_matrix *m);

opengl_matrix* D3DXMatrixScaling(opengl_matrix *out, float sx, float sy, float sz);
opengl_matrix* D3DXMatrixOrtho(opengl_matrix *out, float w, float h, float zn, float zf);
opengl_matrix* D3DXMatrixLookAt(opengl_matrix *out, const opengl_vector *pEye, const opengl_vector *pAt,const opengl_vector *pUp);

opengl_vector* D3DXVec3Normalize(opengl_vector *out, const opengl_vector *v);
float D3DXVec3Length(const opengl_vector *pV);
opengl_vector* D3DXVec3Scale(D3DXVECTOR3 *pOut, const D3DXVECTOR3 *pV, float s);

int D3DXLoadTextureFromSurface(opengl_device* pd3dDevice,opengl_surface* pTexture,DWORD mipMapLevel,
	opengl_surface* pSurfaceSrc,RECT* pSrcRect,RECT* pDestRect,opengl_filtertype filterType);

opengl_vector Normalize(opengl_vector v);
opengl_vector CrossProduct(opengl_vector v1, opengl_vector v2);
float DotProduct(opengl_vector v1, opengl_vector v2);
float Magnitude(opengl_vector v1);
//float acos(opengl_vector v);

int D3DXCheckTextureRequirements(opengl_device* pd3dDevice,	LPDWORD pFlags,	LPDWORD pWidth,	LPDWORD pHeight, opengl_surface_format* pPixelFormat);
int D3DXCreateTextureFromFile(opengl_device* pd3dDevice, LPDWORD pFlags, LPDWORD pWidth, LPDWORD pHeight, opengl_surface_format* pPixelFormat, 
	opengl_draw_palette* pDDPal, opengl_surface** ppDDSurf, LPDWORD pNumMipMaps, LPCSTR pSrcName, opengl_filtertype filterType);

int D3DXCreateTexture(LPDIRECT3DDEVICE7 pd3dDevice, LPDWORD pFlags, LPDWORD pWidth, LPDWORD pHeight, opengl_surface_format* pPixelFormat, opengl_draw_palette* pDDPal, opengl_surface** ppDDSurf, LPDWORD pNumMipMaps);
int D3DXInitialize();
int D3DXUninitialize();

int DirectInputCreateEx(HINSTANCE hinst, DWORD dwVersion, int riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);

int DirectDrawCreateEx(DWORD * lpGuid, LPVOID  *lplpDD, DWORD iid, IUnknown FAR *pUnkOuter);

void D3DXGetErrorString(HRESULT hr,	DWORD   strLength,	LPSTR   pStr);

// directx externs
extern opengl_input_data_format c_dfDIMouse;
extern opengl_input_data_format c_dfDIKeyboard;

// opengl
void * opengl_create_window(int baseW, int baseH, int windowed, int winW, int winH);
void opengl_get_native_res(int * x, int * y);
// Window (output) resolution - independent of the base resolution the game
// renders at, which stays fixed because the GUI is laid out in its pixels.
void opengl_set_resolution(int w, int h);
void opengl_get_resolution(int * w, int * h);
void opengl_window_to_base(double wx, double wy, int * bx, int * by);
void opengl_show_cursor(int show);
void flush_surface(opengl_surface * surface);
void opengl_set_primary_surface(opengl_surface * surface);
void opengl_set_backbuffer(opengl_surface * bbuffer);
void opengl_swap_primary_surface(opengl_surface * s1, opengl_surface * s2);
void opengl_key_event(GLFWwindow* window, int key, int scancode, int action, int mods);
void opengl_mouse_event(GLFWwindow* window, int button, int action, int mods);
void opengl_exit();
void opengl_mouse_cursor_event(GLFWwindow* window, double xpos, double ypos);
void opengl_mouse_wheel_event(GLFWwindow* window, double xoffset, double yoffset);
void opengl_poll();
int opengl_dbg_key(int key);
int opengl_translate_glfw_to_dinput(int ikey);

// misc

// time in ms for a click to be considered 'double click'
#define DOUBLE_CLICK_TIME_MS 500
#endif