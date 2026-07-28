#ifndef _OPENGL_DX_ENUMS_
#define _OPENGL_DX_ENUMS_

#include <limits.h>

typedef enum _opengl_renderstate {
	D3DRENDERSTATE_ANTIALIAS = 2,    /* D3DANTIALIASMODE */
	D3DRENDERSTATE_TEXTUREPERSPECTIVE = 4,    /* TRUE for perspective correction */
	D3DRENDERSTATE_ZENABLE = 7,    /* D3DZBUFFERTYPE (or TRUE/FALSE for legacy) */
	D3DRENDERSTATE_FILLMODE = 8,    /* D3DFILL_MODE        */
	D3DRENDERSTATE_SHADEMODE = 9,    /* D3DSHADEMODE */
	D3DRENDERSTATE_LINEPATTERN = 10,   /* D3DLINEPATTERN */
	D3DRENDERSTATE_ZWRITEENABLE = 14,   /* TRUE to enable z writes */
	D3DRENDERSTATE_ALPHATESTENABLE = 15,   /* TRUE to enable alpha tests */
	D3DRENDERSTATE_LASTPIXEL = 16,   /* TRUE for last-pixel on lines */
	D3DRENDERSTATE_SRCBLEND = 19,   /* D3DBLEND */
	D3DRENDERSTATE_DESTBLEND = 20,   /* D3DBLEND */
	D3DRENDERSTATE_CULLMODE = 22,   /* D3DCULL */
	D3DRENDERSTATE_ZFUNC = 23,   /* D3DCMPFUNC */
	D3DRENDERSTATE_ALPHAREF = 24,   /* D3DFIXED */
	D3DRENDERSTATE_ALPHAFUNC = 25,   /* D3DCMPFUNC */
	D3DRENDERSTATE_DITHERENABLE = 26,   /* TRUE to enable dithering */

	D3DRENDERSTATE_ALPHABLENDENABLE = 27,   /* TRUE to enable alpha blending */

	D3DRENDERSTATE_FOGENABLE = 28,   /* TRUE to enable fog blending */
	D3DRENDERSTATE_SPECULARENABLE = 29,   /* TRUE to enable specular */
	D3DRENDERSTATE_ZVISIBLE = 30,   /* TRUE to enable z checking */
	D3DRENDERSTATE_STIPPLEDALPHA = 33,   /* TRUE to enable stippled alpha (RGB device only) */
	D3DRENDERSTATE_FOGCOLOR = 34,   /* D3DCOLOR */
	D3DRENDERSTATE_FOGTABLEMODE = 35,   /* D3DFOGMODE */

	D3DRENDERSTATE_FOGSTART = 36,   /* Fog start (for both vertex and pixel fog) */
	D3DRENDERSTATE_FOGEND = 37,   /* Fog end      */
	D3DRENDERSTATE_FOGDENSITY = 38,   /* Fog density  */

	D3DRENDERSTATE_EDGEANTIALIAS = 40,   /* TRUE to enable edge antialiasing */
	D3DRENDERSTATE_COLORKEYENABLE = 41,   /* TRUE to enable source colorkeyed textures */
	D3DRENDERSTATE_ZBIAS = 47,   /* LONG Z bias */
	D3DRENDERSTATE_RANGEFOGENABLE = 48,   /* Enables range-based fog */

	D3DRENDERSTATE_STENCILENABLE = 52,   /* BOOL enable/disable stenciling */
	D3DRENDERSTATE_STENCILFAIL = 53,   /* D3DSTENCILOP to do if stencil test fails */
	D3DRENDERSTATE_STENCILZFAIL = 54,   /* D3DSTENCILOP to do if stencil test passes and Z test fails */
	D3DRENDERSTATE_STENCILPASS = 55,   /* D3DSTENCILOP to do if both stencil and Z tests pass */
	D3DRENDERSTATE_STENCILFUNC = 56,   /* D3DCMPFUNC fn.  Stencil Test passes if ((ref & mask) stencilfn (stencil & mask)) is true */
	D3DRENDERSTATE_STENCILREF = 57,   /* Reference value used in stencil test */
	D3DRENDERSTATE_STENCILMASK = 58,   /* Mask value used in stencil test */
	D3DRENDERSTATE_STENCILWRITEMASK = 59,   /* Write mask applied to values written to stencil buffer */
	D3DRENDERSTATE_TEXTUREFACTOR = 60,   /* D3DCOLOR used for multi-texture blend */
	D3DRENDERSTATE_WRAP0 = 128,  /* wrap for 1st texture coord. set */
	D3DRENDERSTATE_WRAP1 = 129,  /* wrap for 2nd texture coord. set */
	D3DRENDERSTATE_WRAP2 = 130,  /* wrap for 3rd texture coord. set */
	D3DRENDERSTATE_WRAP3 = 131,  /* wrap for 4th texture coord. set */
	D3DRENDERSTATE_WRAP4 = 132,  /* wrap for 5th texture coord. set */
	D3DRENDERSTATE_WRAP5 = 133,  /* wrap for 6th texture coord. set */
	D3DRENDERSTATE_WRAP6 = 134,  /* wrap for 7th texture coord. set */
	D3DRENDERSTATE_WRAP7 = 135,  /* wrap for 8th texture coord. set */

	D3DRENDERSTATE_CLIPPING = 136,
	D3DRENDERSTATE_LIGHTING = 137,
	D3DRENDERSTATE_EXTENTS = 138,
	D3DRENDERSTATE_AMBIENT = 139,
	D3DRENDERSTATE_FOGVERTEXMODE = 140,
	D3DRENDERSTATE_COLORVERTEX = 141,
	D3DRENDERSTATE_LOCALVIEWER = 142,
	D3DRENDERSTATE_NORMALIZENORMALS = 143,
	D3DRENDERSTATE_COLORKEYBLENDENABLE = 144,
	D3DRENDERSTATE_DIFFUSEMATERIALSOURCE = 145,
	D3DRENDERSTATE_SPECULARMATERIALSOURCE = 146,
	D3DRENDERSTATE_AMBIENTMATERIALSOURCE = 147,
	D3DRENDERSTATE_EMISSIVEMATERIALSOURCE = 148,
	D3DRENDERSTATE_VERTEXBLEND = 151,
	D3DRENDERSTATE_CLIPPLANEENABLE = 152,

	D3DRENDERSTATE_TEXTUREHANDLE = 1,    /* Texture handle for legacy interfaces (Texture,Texture2) */
	D3DRENDERSTATE_TEXTUREADDRESS = 3,    /* D3DTEXTUREADDRESS  */
	D3DRENDERSTATE_WRAPU = 5,    /* TRUE for wrapping in u */
	D3DRENDERSTATE_WRAPV = 6,    /* TRUE for wrapping in v */
	D3DRENDERSTATE_MONOENABLE = 11,   /* TRUE to enable mono rasterization */
	D3DRENDERSTATE_ROP2 = 12,   /* ROP2 */
	D3DRENDERSTATE_PLANEMASK = 13,   /* DWORD physical plane mask */
	D3DRENDERSTATE_TEXTUREMAG = 17,   /* D3DTEXTUREFILTER */
	D3DRENDERSTATE_TEXTUREMIN = 18,   /* D3DTEXTUREFILTER */
	D3DRENDERSTATE_TEXTUREMAPBLEND = 21,   /* D3DTEXTUREBLEND */
	D3DRENDERSTATE_SUBPIXEL = 31,   /* TRUE to enable subpixel correction */
	D3DRENDERSTATE_SUBPIXELX = 32,   /* TRUE to enable correction in X only */
	D3DRENDERSTATE_STIPPLEENABLE = 39,   /* TRUE to enable stippling */

	D3DRENDERSTATE_BORDERCOLOR = 43,   /* Border color for texturing w/border */
	D3DRENDERSTATE_TEXTUREADDRESSU = 44,   /* Texture addressing mode for U coordinate */
	D3DRENDERSTATE_TEXTUREADDRESSV = 45,   /* Texture addressing mode for V coordinate */
	D3DRENDERSTATE_MIPMAPLODBIAS = 46,   /* D3DVALUE Mipmap LOD bias */
	D3DRENDERSTATE_ANISOTROPY = 49,   /* Max. anisotropy. 1 = no anisotropy */

	D3DRENDERSTATE_FLUSHBATCH = 50,   /* Explicit flush for DP batching (DX5 Only) */

	D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT = 51, /* BOOL enable sort-independent transparency */

	D3DRENDERSTATE_STIPPLEPATTERN00 = 64,   /* Stipple pattern 01...  */
	D3DRENDERSTATE_STIPPLEPATTERN01 = 65,
	D3DRENDERSTATE_STIPPLEPATTERN02 = 66,
	D3DRENDERSTATE_STIPPLEPATTERN03 = 67,
	D3DRENDERSTATE_STIPPLEPATTERN04 = 68,
	D3DRENDERSTATE_STIPPLEPATTERN05 = 69,
	D3DRENDERSTATE_STIPPLEPATTERN06 = 70,
	D3DRENDERSTATE_STIPPLEPATTERN07 = 71,
	D3DRENDERSTATE_STIPPLEPATTERN08 = 72,
	D3DRENDERSTATE_STIPPLEPATTERN09 = 73,
	D3DRENDERSTATE_STIPPLEPATTERN10 = 74,
	D3DRENDERSTATE_STIPPLEPATTERN11 = 75,
	D3DRENDERSTATE_STIPPLEPATTERN12 = 76,
	D3DRENDERSTATE_STIPPLEPATTERN13 = 77,
	D3DRENDERSTATE_STIPPLEPATTERN14 = 78,
	D3DRENDERSTATE_STIPPLEPATTERN15 = 79,
	D3DRENDERSTATE_STIPPLEPATTERN16 = 80,
	D3DRENDERSTATE_STIPPLEPATTERN17 = 81,
	D3DRENDERSTATE_STIPPLEPATTERN18 = 82,
	D3DRENDERSTATE_STIPPLEPATTERN19 = 83,
	D3DRENDERSTATE_STIPPLEPATTERN20 = 84,
	D3DRENDERSTATE_STIPPLEPATTERN21 = 85,
	D3DRENDERSTATE_STIPPLEPATTERN22 = 86,
	D3DRENDERSTATE_STIPPLEPATTERN23 = 87,
	D3DRENDERSTATE_STIPPLEPATTERN24 = 88,
	D3DRENDERSTATE_STIPPLEPATTERN25 = 89,
	D3DRENDERSTATE_STIPPLEPATTERN26 = 90,
	D3DRENDERSTATE_STIPPLEPATTERN27 = 91,
	D3DRENDERSTATE_STIPPLEPATTERN28 = 92,
	D3DRENDERSTATE_STIPPLEPATTERN29 = 93,
	D3DRENDERSTATE_STIPPLEPATTERN30 = 94,
	D3DRENDERSTATE_STIPPLEPATTERN31 = 95,

	D3DRENDERSTATE_FOGTABLESTART = 36,   /* Fog table start    */
	D3DRENDERSTATE_FOGTABLEEND = 37,   /* Fog table end      */
	D3DRENDERSTATE_FOGTABLEDENSITY = 38,   /* Fog table density  */

	D3DRENDERSTATE_FORCE_DWORD
} opengl_renderstate;

typedef enum _opengl_transforms {
	D3DTRANSFORMSTATE_WORLD = 1,
	D3DTRANSFORMSTATE_VIEW,
	D3DTRANSFORMSTATE_PROJECTION
} opengl_transforms;

typedef enum _opengl_primitive_type {
	D3DPT_POINTLIST = 1,
	D3DPT_LINELIST = 2,
	D3DPT_LINESTRIP = 3,
	D3DPT_TRIANGLELIST = 4,
	D3DPT_TRIANGLESTRIP = 5,
	D3DPT_TRIANGLEFAN = 6,
	D3DPT_FORCE_DWORD = 0x7fffffff
} opengl_primitive_type;

typedef enum _opengl_blend_opts {
	D3DBLEND_ZERO = 1,
	D3DBLEND_ONE = 2,
	D3DBLEND_SRCCOLOR = 3,
	D3DBLEND_INVSRCCOLOR = 4,
	D3DBLEND_SRCALPHA = 5,
	D3DBLEND_INVSRCALPHA = 6,
	D3DBLEND_DESTALPHA = 7,
	D3DBLEND_INVDESTALPHA = 8,
	D3DBLEND_DESTCOLOR = 9,
	D3DBLEND_INVDESTCOLOR = 10,
	D3DBLEND_SRCALPHASAT = 11,
	D3DBLEND_BOTHSRCALPHA = 12,
	D3DBLEND_BOTHINVSRCALPHA = 13,

	D3DBLEND_FORCE_DWORD = 0x7fffffff
} opengl_blend_opts;

typedef enum _opengl_filtertype {
	
	D3DX_FT_POINT = 0x01,
	D3DX_FT_LINEAR = 0x02,
	D3DX_FT_DEFAULT = ULONG_MAX

} opengl_filtertype;

typedef enum opengl_fillmode {
	D3DFILL_POINT = 1,
	D3DFILL_WIREFRAME = 2,
	D3DFILL_SOLID = 3,

	D3DFILL_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */

} opengl_fillmode;

typedef enum _opengl_tex_mipfilter {
	D3DTFP_NONE = 1,    // mipmapping disabled (use MAG filter)
	D3DTFP_POINT = 2,    // nearest
	D3DTFP_LINEAR = 3,    // linear interpolation
	D3DTFP_FORCE_DWORD = 0x7fffffff,   // force 32-bit size enum
} opengl_tex_mipfilter;

typedef enum _opengl_tex_magfilter {
	D3DTFG_POINT = 1,    // nearest
	D3DTFG_LINEAR = 2,    // linear interpolation
	D3DTFG_FLATCUBIC = 3,    // cubic
	D3DTFG_GAUSSIANCUBIC = 4,   // different cubic kernel
	D3DTFG_ANISOTROPIC = 5,    //

	D3DTFG_FORCE_DWORD = 0x7fffffff,   // force 32-bit size enum
} opengl_tex_magfilter;

typedef enum _opengl_tex_address {
	D3DTADDRESS_WRAP = 1,
	D3DTADDRESS_MIRROR = 2,
	D3DTADDRESS_CLAMP = 3,

	D3DTADDRESS_BORDER = 4,
	D3DTADDRESS_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} opengl_tex_address;

typedef enum _opengl_light_type {
	D3DLIGHT_POINT = 1,
	D3DLIGHT_SPOT = 2,
	D3DLIGHT_DIRECTIONAL = 3,
	D3DLIGHT_PARALLELPOINT = 4,
	D3DLIGHT_GLSPOT = 5,
	D3DLIGHT_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} opengl_light_type;

typedef enum _opengl_texture_operation {
	// Control
	D3DTOP_DISABLE = 1,      // disables stage
	D3DTOP_SELECTARG1 = 2,      // the default
	D3DTOP_SELECTARG2 = 3,

	// Modulate
	D3DTOP_MODULATE = 4,      // multiply args together
	D3DTOP_MODULATE2X = 5,      // multiply and  1 bit
	D3DTOP_MODULATE4X = 6,      // multiply and  2 bits

	// Add
	D3DTOP_ADD = 7,   // add arguments together
	D3DTOP_ADDSIGNED = 8,   // add with -0.5 bias
	D3DTOP_ADDSIGNED2X = 9,   // as above but left  1 bit
	D3DTOP_SUBTRACT = 10,   // Arg1 - Arg2, with no saturation
	D3DTOP_ADDSMOOTH = 11,   // add 2 args, subtract product
	// Arg1 + Arg2 - Arg1*Arg2
	// = Arg1 + (1-Arg1)*Arg2

	// Linear alpha blend: Arg1*(Alpha) + Arg2*(1-Alpha)
	D3DTOP_BLENDDIFFUSEALPHA = 12, // iterated alpha
	D3DTOP_BLENDTEXTUREALPHA = 13, // texture alpha
	D3DTOP_BLENDFACTORALPHA = 14, // alpha from D3DRENDERSTATE_TEXTUREFACTOR
	// Linear alpha blend with pre-multiplied arg1 input: Arg1 + Arg2*(1-Alpha)
	D3DTOP_BLENDTEXTUREALPHAPM = 15, // texture alpha
	D3DTOP_BLENDCURRENTALPHA = 16, // by alpha of current color

	// Specular mapping
	D3DTOP_PREMODULATE = 17,     // modulate with next texture before use
	D3DTOP_MODULATEALPHA_ADDCOLOR = 18,     // Arg1.RGB + Arg1.A*Arg2.RGB
	// COLOROP only
	D3DTOP_MODULATECOLOR_ADDALPHA = 19,     // Arg1.RGB*Arg2.RGB + Arg1.A
	// COLOROP only
	D3DTOP_MODULATEINVALPHA_ADDCOLOR = 20,  // (1-Arg1.A)*Arg2.RGB + Arg1.RGB
	// COLOROP only
	D3DTOP_MODULATEINVCOLOR_ADDALPHA = 21,  // (1-Arg1.RGB)*Arg2.RGB + Arg1.A
	// COLOROP only

	// Bump mapping
	D3DTOP_BUMPENVMAP = 22, // per pixel env map perturbation
	D3DTOP_BUMPENVMAPLUMINANCE = 23, // with luminance channel
	// This can do either diffuse or specular bump mapping with correct input.
	// Performs the function (Arg1.R*Arg2.R + Arg1.G*Arg2.G + Arg1.B*Arg2.B)
	// where each component has been scaled and offset to make it signed.
	// The result is replicated into all four (including alpha) channels.
	// This is a valid COLOROP only.
	D3DTOP_DOTPRODUCT3 = 24,

	D3DTOP_FORCE_DWORD = 0x7fffffff,
} opengl_texture_operation;

typedef enum _opengl_tex_state {
	D3DTSS_COLOROP = 1, /* D3DTEXTUREOP - per-stage blending controls for color channels */
	D3DTSS_COLORARG1 = 2, /* D3DTA_* (texture arg) */
	D3DTSS_COLORARG2 = 3, /* D3DTA_* (texture arg) */
	D3DTSS_ALPHAOP = 4, /* D3DTEXTUREOP - per-stage blending controls for alpha channel */
	D3DTSS_ALPHAARG1 = 5, /* D3DTA_* (texture arg) */
	D3DTSS_ALPHAARG2 = 6, /* D3DTA_* (texture arg) */
	D3DTSS_BUMPENVMAT00 = 7, /* D3DVALUE (bump mapping matrix) */
	D3DTSS_BUMPENVMAT01 = 8, /* D3DVALUE (bump mapping matrix) */
	D3DTSS_BUMPENVMAT10 = 9, /* D3DVALUE (bump mapping matrix) */
	D3DTSS_BUMPENVMAT11 = 10, /* D3DVALUE (bump mapping matrix) */
	D3DTSS_TEXCOORDINDEX = 11, /* identifies which set of texture coordinates index this texture */
	D3DTSS_ADDRESS = 12, /* D3DTEXTUREADDRESS for both coordinates */
	D3DTSS_ADDRESSU = 13, /* D3DTEXTUREADDRESS for U coordinate */
	D3DTSS_ADDRESSV = 14, /* D3DTEXTUREADDRESS for V coordinate */
	D3DTSS_BORDERCOLOR = 15, /* D3DCOLOR */
	D3DTSS_MAGFILTER = 16, /* D3DTEXTUREMAGFILTER filter to use for magnification */
	D3DTSS_MINFILTER = 17, /* D3DTEXTUREMINFILTER filter to use for minification */
	D3DTSS_MIPFILTER = 18, /* D3DTEXTUREMIPFILTER filter to use between mipmaps during minification */
	D3DTSS_MIPMAPLODBIAS = 19, /* D3DVALUE Mipmap LOD bias */
	D3DTSS_MAXMIPLEVEL = 20, /* DWORD 0..(n-1) LOD index of largest map to use (0 == largest) */
	D3DTSS_MAXANISOTROPY = 21, /* DWORD maximum anisotropy */
	D3DTSS_BUMPENVLSCALE = 22, /* D3DVALUE scale for bump map luminance */
	D3DTSS_BUMPENVLOFFSET = 23, /* D3DVALUE offset for bump map luminance */
	D3DTSS_FORCE_DWORD = 0x7fffffff
} opengl_tex_state;

typedef enum _opengl_cmp_ops {
	D3DCMP_NEVER = 1,
	D3DCMP_LESS = 2,
	D3DCMP_EQUAL = 3,
	D3DCMP_LESSEQUAL = 4,
	D3DCMP_GREATER = 5,
	D3DCMP_NOTEQUAL = 6,
	D3DCMP_GREATEREQUAL = 7,
	D3DCMP_ALWAYS = 8,

	D3DCMP_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */

} opengl_cmp_ops;

typedef enum _opengl_surface_format {
	D3DX_SF_UNKNOWN = 0,
	D3DX_SF_R8G8B8 = 1,
	D3DX_SF_A8R8G8B8 = 2,
	D3DX_SF_X8R8G8B8 = 3,
	D3DX_SF_R5G6B5 = 4,
	D3DX_SF_R5G5B5 = 5,
	D3DX_SF_PALETTE4 = 6,
	D3DX_SF_PALETTE8 = 7,
	D3DX_SF_A1R5G5B5 = 8,
	D3DX_SF_X4R4G4B4 = 9,
	D3DX_SF_A4R4G4B4 = 10,
	D3DX_SF_L8 = 11,      // 8 bit luminance-only
	D3DX_SF_A8L8 = 12,      // 16 bit alpha-luminance
	D3DX_SF_U8V8 = 13,      // 16 bit bump map format
	D3DX_SF_U5V5L6 = 14,      // 16 bit bump map format with luminance
	D3DX_SF_U8V8L8 = 15,      // 24 bit bump map format with luminance
	D3DX_SF_UYVY = 16,      // UYVY format (PC98 compliance)
	D3DX_SF_YUY2 = 17,      // YUY2 format (PC98 compliance)
	D3DX_SF_DXT1 = 18,      // S3 texture compression technique 1
	D3DX_SF_DXT3 = 19,      // S3 texture compression technique 3
	D3DX_SF_DXT5 = 20,      // S3 texture compression technique 5
	D3DX_SF_R3G3B2 = 21,      // 8 bit RGB texture format
	D3DX_SF_A8 = 22,      // 8 bit alpha-only
	D3DX_SF_TEXTUREMAX = 23,      // Last texture format

	D3DX_SF_Z16S0 = 256,
	D3DX_SF_Z32S0 = 257,
	D3DX_SF_Z15S1 = 258,
	D3DX_SF_Z24S8 = 259,
	D3DX_SF_S1Z15 = 260,
	D3DX_SF_S8Z24 = 261,
	D3DX_SF_DEPTHMAX = 262,     // Last depth format

	D3DX_SF_FORCEMAX = (DWORD)(-1)
} opengl_surface_format;


#define D3DTA_SELECTMASK        0x0000000f  // mask for arg selector
#define D3DTA_DIFFUSE           0x00000000  // select diffuse color
#define D3DTA_CURRENT           0x00000001  // select result of previous stage
#define D3DTA_TEXTURE           0x00000002  // select texture color
#define D3DTA_TFACTOR           0x00000003  // select RENDERSTATE_TEXTUREFACTOR
#define D3DTA_SPECULAR          0x00000004  // select specular color
#define D3DTA_COMPLEMENT        0x00000010  // take 1.0 - x
#define D3DTA_ALPHAREPLICATE    0x00000020

#define DDPF_ALPHAPIXELS 0x00000001l

#define D3DENUMRET_CANCEL 0
#define D3DENUMRET_OK 1
#define D3DX_TEXTURE_NOMIPMAP  (1 << 8)

#define D3DX_DEFAULT 0
#define DDLOCK_WAIT                             0x00000001L

#define D3DCLEAR_TARGET 0x00000001l
#define D3DCLEAR_ZBUFFER 0x00000002l
#define D3DCLEAR_STENCIL 0x00000004l

#define DDBLT_KEYSRC 0x00008000l
#define DD_OK 0
#define D3D_OK DD_OK

#define D3DFVF_RESERVED0        0x001
#define D3DFVF_POSITION_MASK    0x00E
#define D3DFVF_XYZ              0x002
#define D3DFVF_XYZRHW           0x004

#define D3DFVF_XYZB1            0x006
#define D3DFVF_XYZB2            0x008
#define D3DFVF_XYZB3            0x00a
#define D3DFVF_XYZB4            0x00c
#define D3DFVF_XYZB5            0x00e

#define D3DFVF_NORMAL           0x010
#define D3DFVF_RESERVED1        0x020
#define D3DFVF_DIFFUSE          0x040
#define D3DFVF_SPECULAR         0x080

#define D3DFVF_TEXCOUNT_MASK    0xf00
#define D3DFVF_TEXCOUNT_SHIFT   8
#define D3DFVF_TEX0             0x000
#define D3DFVF_TEX1             0x100
#define D3DFVF_TEX2             0x200
#define D3DFVF_TEX3             0x300
#define D3DFVF_TEX4             0x400
#define D3DFVF_TEX5             0x500
#define D3DFVF_TEX6             0x600
#define D3DFVF_TEX7             0x700
#define D3DFVF_TEX8             0x800

#define D3DFVF_RESERVED2        0xf000  // 4 reserved bits

#define D3DFVF_VERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 )
#define D3DFVF_LVERTEX ( D3DFVF_XYZ | D3DFVF_RESERVED1 | D3DFVF_DIFFUSE | \
                         D3DFVF_SPECULAR | D3DFVF_TEX1 )
#define D3DFVF_TLVERTEX ( D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | \
                          D3DFVF_TEX1 )

#define DDBLT_WAIT 0x01000000l

#define DDCKEY_DESTBLT          0x00000002l
#define DDCKEY_DESTOVERLAY      0x00000004l
#define DDCKEY_SRCBLT           0x00000008l
#define DIRECTINPUT_VERSION         0x0700

#define IID_IDirectInput7 7
#define IID_IDirectInputDevice7 8
#define GUID_SysKeyboard 9
#define GUID_SysMouse 10
#define IID_IDirect3D7 11
#define IID_IDirectDraw7 12
#define IID_IDirect3DHALDevice 13
#define IID_IDirect3DMMXDevice 14
#define IID_IDirect3DRGBDevice 15
#define IID_IDirect3DRampDevice 16

#define DI_OK 0

#define DISCL_EXCLUSIVE     0x00000001
#define DISCL_NONEXCLUSIVE  0x00000002
#define DISCL_FOREGROUND    0x00000004
#define DISCL_BACKGROUND    0x00000008
#define DISCL_NOWINKEY      0x00000010

#define DDPF_ZBUFFER	0x00000400l
#define DDSCAPS_OFFSCREENPLAIN 0x00000040l
#define DDBLT_COLORFILL                         0x00000400l

#define DDSD_CAPS               0x00000001l
#define DDSD_HEIGHT             0x00000002l
#define DDSD_WIDTH              0x00000004l
#define DDSD_PITCH              0x00000008l
#define DDSD_BACKBUFFERCOUNT    0x00000020l
#define DDSD_ZBUFFERBITDEPTH    0x00000040l
#define DDSD_ALPHABITDEPTH      0x00000080l
#define DDSD_LPSURFACE          0x00000800l
#define DDSD_PIXELFORMAT        0x00001000l
#define DDSD_CKDESTOVERLAY      0x00002000l
#define DDSD_CKDESTBLT          0x00004000l
#define DDSD_CKSRCOVERLAY       0x00008000l
#define DDSD_CKSRCBLT           0x00010000l
#define DDSD_MIPMAPCOUNT        0x00020000l
#define DDSD_REFRESHRATE        0x00040000l
#define DDSD_LINEARSIZE         0x00080000l
#define DDSD_TEXTURESTAGE       0x00100000l
#define DDSD_FVF                0x00200000l
#define DDSD_SRCVBHANDLE        0x00400000l
#define DDSD_ALL                0x007ff9eel

#define DDFLIP_WAIT                          0x00000001L

#define DIERR_INPUTLOST                 \
    MAKE_HRESULT(SEVERITY_ERROR, 7, 30)

#define DDERR_GENERIC 0xFF
#define DDERR_INVALIDPARAMS DDERR_GENERIC+1

#define DDSCL_FULLSCREEN                        0x00000001l
#define DDSCL_ALLOWREBOOT                       0x00000002l
#define DDSCL_NOWINDOWCHANGES                   0x00000004l
#define DDSCL_NORMAL                            0x00000008l
#define DDSCL_EXCLUSIVE                         0x00000010l
#define DDSCL_ALLOWMODEX                        0x00000040l
#define DDSCL_SETFOCUSWINDOW                    0x00000080l
#define DDSCL_SETDEVICEWINDOW                   0x00000100l
#define DDSCL_CREATEDEVICEWINDOW                0x00000200l
#define DDSCL_MULTITHREADED                     0x00000400l
#define DDSCL_FPUSETUP                          0x00000800l
#define DDSCL_FPUPRESERVE                       0x00001000l
#define DDSCAPS_PRIMARYSURFACE                  0x00000200l
#define DDSCAPS_FLIP                            0x00000010l
#define DDSCAPS_COMPLEX                         0x00000008l
#define DDSCAPS_3DDEVICE                        0x00002000l
#define DDSCAPS_BACKBUFFER                      0x00000004l
#define DDSCAPS_ZBUFFER                         0x00020000l
#define DDSCAPS_VIDEOMEMORY                     0x00004000l

#define DDERR_EXCEPTION							6000
#define DDERR_INVALIDOBJECT						DDERR_EXCEPTION+1
#define DDERR_INVALIDRECT						DDERR_EXCEPTION+2
#define DDERR_NOBLTHW	DDERR_EXCEPTION+3
#define DDERR_SURFACEBUSY	DDERR_EXCEPTION+4
#define DDERR_SURFACELOST	DDERR_EXCEPTION+5
#define DDERR_UNSUPPORTED	DDERR_EXCEPTION+6
#define DDERR_WASSTILLDRAWING	DDERR_EXCEPTION+7
#define DDERR_BLTFASTCANTCLIP	DDERR_EXCEPTION+8
#define D3DERR_CONFLICTINGTEXTUREFILTER	DDERR_EXCEPTION+9
#define D3DERR_CONFLICTINGTEXTUREPALETTE	DDERR_EXCEPTION+10
#define D3DERR_TOOMANYOPERATIONS	DDERR_EXCEPTION+11
#define D3DERR_UNSUPPORTEDALPHAARG	DDERR_EXCEPTION+12
#define D3DERR_UNSUPPORTEDALPHAOPERATION	DDERR_EXCEPTION+13
#define D3DERR_UNSUPPORTEDTEXTUREFILTER	DDERR_EXCEPTION+14
#define D3DERR_WRONGTEXTUREFORMAT	DDERR_EXCEPTION+15

#define DDSCAPS_TEXTURE                         0x00001000l
#define DDSCAPS_MIPMAP                          0x00400000l
#define DDERR_NOTFOUND                          0x400255

#endif
