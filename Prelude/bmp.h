#ifndef _BMP_H_
#define _BMP_H_

#include <stdint.h>

typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint8_t byte;


#pragma pack(1)

typedef struct bmp_header {
	uint16 id;
	uint32 file_size;
	uint32 reserved;
	uint32 data_offset;
	uint32 header_size;
	uint32 width;
	uint32 height;
	uint16 color_planes;
	uint16 bpp;
	uint32 compression;
	uint32 image_size;
	// extra stuff we ignore
} 
#ifdef __linux__
__attribute__((packed))
#endif
bmp_header;

#pragma pack()


uint32* bmp_load(const char * path, int * iw, int * ih);
void bmp_scale(uint32 * pixels, int sw, int sh, int sx, int sy, int fsw, int fsh,
	uint32 *dst, int dw, int dh, int dx, int dy, int fdw, int fdh, int use_color_key, uint32 color_key);
uint32* bmp_load_mm(byte * tmp, bmp_header * header, int * iw, int * ih);

#endif
