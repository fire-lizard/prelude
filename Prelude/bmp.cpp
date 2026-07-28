#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if _WIN32
#include <direct.h>
#endif
#include <assert.h>
#include <errno.h>

#include "bmp.h"

#include "linux_aux_wrapper.h"

void debug_info(const char*, ...);

/**
 * Loads a bitmap, returns a single array with all pixels in 32 bits (ABRG) format.
 * This is just a wrapper for files, @see bmp_load_mm for the actual processing.
 * @param file path File path
 * @param iw pointer to file width, if the provided width is larger than file width,
 * iw will be used. Contains final width after processing
 * @param ih height @see iw
 * @return single array with all pixels in 32 bits (ABRG) format. Array is allocated in
 * this function, and must be deleted elsewhere
 */
uint32* bmp_load(const char * path, int * iw, int * ih) {
	//debug_info("bmp_load %s", path);
	
	FILE * fp = fopen(path, "rb");	

	if (!fp) {
		debug_info("bad file on bmp %s %i", path,errno);
		char tmp[1024];
		getcwd(tmp, 1024);
		debug_info("directory %s", tmp);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	byte * tmp = (byte*)malloc(size);
	int rt = fread(tmp, 1, size, fp);
	
	if (rt != size) {
		debug_info("bad fread");
		exit(0);
	}

	bmp_header * header = (bmp_header*)tmp;
	assert(header->data_offset < (uint32)size);
	uint32 * rtx = bmp_load_mm(tmp + header->data_offset, header, iw, ih);
	free(tmp);

	fclose(fp);

	return rtx;
}

/**
 * Process a bitmap, returns a single array with all pixels in 32 bits (ABRG) format.
 * Some methods/compressions may not be supported
 * 
 * @param tmp byte array pointing to the bitmap raw pixel data
 * @param header must be point to the bitmap header, and contain the info header also
 * @param iw pointer to file width, if the provided width is larger than file width,
 * iw will be used. Contains final width after processing
 * @param ih height @see iw
 * @return single array with all pixels in 32 bits (ABRG) format. Array is allocated in
 * this function, and must be deleted elsewhere. Returns NULL in case of error
 */
uint32* bmp_load_mm(byte * tmp, bmp_header * header, int * iw, int * ih) {
	int i, j;
	uint32 * data;
	
	if (header->compression || header->header_size != 0x28 ||
		(header->bpp != 32 && header->bpp != 16 && header->bpp != 8 && header->bpp != 1 
			&& header->bpp != 24 && header->bpp != 4)) {
		debug_info("unsupported bmp %i", header->bpp);
		
		return NULL;
	}

	int w = header->width;
	int h = header->height;
	if (*iw > w) {
		w = *iw;
	}
	if (*ih > h) {
		h = *ih;
	}
	*ih = h;
	*iw = w;
	data = new uint32[w*h];
	memset(data, 0, w*h * sizeof(uint32));
	uint32 * t2;

	if (header->bpp == 32) {		
		t2 = data;
		for (j = h - 1; j >= 0; j--) {
			byte * tt = (tmp ) + j * w * 4;
			memcpy(t2, tt, 4 * w);
			t2 +=  w;
		}

		int i;
		t2 = data;
		for (i = 0; i < w*h; i++) {
			(*t2) = (((*t2) & 0xFF) << 16) + (((*t2) & 0xFF0000) >> 16) + ((*t2) & 0xFF00) + 0xFF000000;
			
			t2++;
		}
		
		return data;
	}

	uint32 * color_table = (uint32*)((byte*)header + 14 + header->header_size);

	if (w == 1) {
		data = new uint32[1];
		data[0] = 0xFFFFFFFF;
		
		return data;
	}

	
	if (header->bpp == 1) {
		if (w % 8) {
			debug_info("unsupported bmp 1bpp");
			exit(0);
		}
		j = h - 1;
		byte * t = tmp;

		t = t + (j*(w / 8));
		uint32 * t2 = data;
		int col = 0;
		int leap = 7;
		byte val = *t;
		while (j >= 0) {

			int bit = (val >> leap) & 1;
			*t2++ = (color_table[bit]) | 0xFF000000;
			leap--;
			
			if (leap == -1) {
				t++;
				if (col + 1< w)
					val = *t;

				leap = 7;
			}
			col++;
			if (col == w) {
				col = 0;
				j--;
				t = tmp + (j*(w / 8));
				if (j >= 0)
					val = *t;
				leap = 7;
			}
		}

		return data;
	}

	if (header->bpp == 4) {
		byte * t = tmp;		

		if (w % 8) {
			debug_info("unsupported bmp 1bpp %i",w);
			exit(0);
		}
		j = h - 1;		

		t = t + (j*(w / 8));
		uint32 * t2 = data;
		int col = 0;
		int leap = 0;
		byte val = *t;
		while (j >= 0) {
			int bit = (val >> leap) & 0x0F;
			*t2 = (color_table[bit]) | 0xFF000000;
			leap+=4;
			if (leap == 8) {
				t++;
				val = *t;
				leap = 0;
			}
			col++;
			if (col == w) {
				col = 0;
				j--;
				t = tmp + (j*(w / 8));
			}
		}
		
		return data;
	}
		
	int stride = header->bpp / 8;
	int bytes_per_row = stride*w;
	int pad = (bytes_per_row % 4);

	if (pad) pad = 4 - pad;

	byte * t;
	t2 = data;
	byte * end = tmp + (w*h*stride);
	int col = 0;
	j = h - 1;
	t = (tmp)+(j*(w*stride + pad));

	while (j >= 0) {
		
		uint32 val = 0xFF000000;
		if (stride == 2) {
			debug_info("unsupported stride");
			exit(0);
		}
		else if (stride == 1) {
			byte v = *t;
			uint32 c = color_table[v];
			val = ((c & 0xFF) << 16) + ((c & 0xFF0000) >> 16) + (c & 0xFF00) + 0xFF000000;
		}
		else if (stride == 3) {
			val = ((*t) << 16) + ((*(t + 1)) << 8) + ((*(t + 2))) + 0xFF000000;			
		}

		*t2++ = val;
		t += stride;
		col++;

		if (col == w) {
			if (pad) {
				for (i = 0; i < pad; i++) {
					t += stride;
				}
			}
			j--;
			t = (tmp) + (j*(w*stride+pad));
			
			col = 0;
		}
	}
	

	
	return data;
}

/**
 * Stretches a raw 32 bits array on one axis. Despite the name it is using
 * nearest pixel for stretching.
 *
 */

void interpolate_axis_increase(uint32 * pixels, int sw, int sh, int sx, int sy, int fsw, int fsh,
	uint32 *dst, int dw, int dh, int dx, int dy, int fdw, int fdh, int axis,
	int use_color_key, uint32 color_key) {
	int i, j = 0;
	int l;

	int sz_axis = (axis == 1 ? dw : dh);
	int sz_axis2 = (axis == 1 ? dh : dw);
	float ratio = (axis == 1 ? (float)sw / (float)dw : (float)sh / (float)dh);

	int loc = -1;

	int * disps = new int[axis == 1 ? dw : dh];
	int cc = 0;
	int k = -1;
	
	for (i = 0; i < sz_axis; i++) {

		int ref = (int)(i*ratio);
		if (ref != loc) {

			if (k >= 0 && cc) {
				disps[k++] = cc;
			}
			else if (k < 0) {
				k++;
			}
			cc = 0;

			for (j = 0; j < sz_axis2; j++) {
				if (axis == 1) {
					uint32 fp = pixels[((sy + j)*fsw) + (sx + ref)];
					if (!use_color_key || fp != color_key) {
						dst[((dy + j)*fdw) + (dx + i)] = fp;
					}
				}
				else {
					uint32 fp = pixels[((sy + ref)*fsw) + (sx + j)];
					if (!use_color_key || fp != color_key) {
						dst[((dy + i)*fdw) + (dx + j)] = fp;
					}
				}
			}
			loc = ref;
		}
		else {
			cc++;
		}
	}

	int klim = k;
	k = 0;
	loc = -1;
	cc = 0;
	for (i = 0; i < sz_axis; i++) {
		int ref = (int)(i*ratio);

		if (ref == loc) {
			cc++;
			int ref1 = i - 1;
			
			int ref2 = 0;

			for (j = 0; j < sz_axis2; j++) {

				uint32 f1, f2;

				if (axis == 1) {
					f1 = dst[((dy + j)*fdw) + (dx + ref1)];
				}
				else {
					f1 = dst[((dy + ref1)*fdw) + (dx + j)];
				}


				if (k >= klim) {
					//clamp

					for (l = i; l < sz_axis; l++) {
						if (!use_color_key || f1 != color_key) {
							if (axis == 1) {
								dst[((dy + j)*fdw) + (dx + l)] = f1;
							}
							else {
								dst[((dy + l)*fdw) + (dx + j)] = f1;
							}
						}
					}

				}
				else {
					ref2 = i + disps[k];
					if (axis == 1) {
						f2 = dst[((dy + j)*fdw) + (dx + ref2)];
					}
					else {
						f2 = dst[((dy + ref2)*fdw) + (dx + j)];
					}
					int i1ref = (int)(ref1*ratio);
					int i2ref = (int)(ref2*ratio);
					for (l = i; l < ref2; l++) {
						uint32 pval = (uint32)f1;
						if (!use_color_key || pval != color_key) {
							if (axis == 1) {
								dst[((dy + j)*fdw) + (dx + l)] = pval;
							}
							else {
								dst[((dy + l)*fdw) + (dx + j)] = pval;
							}
						}
					}

				}

			}
			if (k >= klim) {
				i = sz_axis;
			}
			else {
				i = ref2 - 1;
			}
		}
		else {
			if (cc) {
				cc = 0;
				k++;
			}

			loc = ref;
		}
	}

	delete[] disps;
}

/**
* Compresses a raw 32 bits pixel array on one axis. Despite the name it is using
* nearest pixel for compressing.
*
*/
void interpolate_axis_decrease(uint32 * pixels, int sw, int sh, int sx, int sy, int fsw, int fsh,
	uint32 *dst, int dw, int dh, int dx, int dy, int fdw, int fdh, int axis, int use_color_key, uint32 color_key) {
	int i, j = 0;
	int l;

	int sz_axis = (axis == 1 ? dw : dh);
	int sz_axis2 = (axis == 1 ? dh : dw);
	float ratio = (axis == 1 ? (float)sw / (float)dw : (float)sh / (float)dh);

	int loc = -1;

	int * disps = new int[axis == 1 ? dw : dh];
	int cc = 0;
	int k = -1;

	for (i = 0; i < sz_axis; i++) {
		int ref = (int)(i*ratio);
		int refn = (int)((i + 1)*ratio) - 1;


		for (j = 0; j < sz_axis2; j++) {
			if (ref == refn) {

				uint32 f1 = axis == 1 ? pixels[((sy + j)*fsw) + (sx + ref)] : pixels[((sy + ref)*fsw) + (sx + j)];
				if (!use_color_key || color_key != f1) {
					if (axis == 1) {
						dst[((dy + j)*fdw) + (dx + i)] = f1;
					}
					else {
						dst[((dy + i)*fdw) + (dx + j)] = f1;
					}
				}
			}
			else {
				uint32 f1, f2;
				if (axis == 1) {
					f1 = pixels[((sy + j)*fsw) + (sx + ref)];
					f2 = pixels[((sy + j)*fsw) + (sx + refn)];
				}
				else {
					f1 = pixels[((sy + ref)*fsw) + (sx + j)];
					f2 = pixels[((sy + refn)*fsw) + (sx + j)];
				}
				/*
				uint32 r = (f1 & 0xFF) + (f2 & 0xFF) / 2;
				uint32 g = ((f1 >> 8) & 0xFF) + ((f2 >> 8) & 0xFF) / 2;
				uint32 b = ((f1 >> 16) & 0xFF) + ((f2 >> 16) & 0xFF) / 2;
				uint32 res = (r+(g<<8)+(b<<16)) + 0xFF000000;
				*/
				uint32 res = f1;

				if (!use_color_key || color_key != res) {
					if (axis == 1) {
						dst[((dy + j)*fdw) + (dx + i)] = res;
					}
					else {
						dst[((dy + i)*fdw) + (dx + j)] = res;
					}
				}
			}
		}
	}
	delete[] disps;
}

/**
 * Scales a 32 bits pixels array 
 *
 */
void bmp_scale(uint32 * pixels, int sw, int sh, int sx, int sy, int fsw, int fsh,
	uint32 *dst, int dw, int dh, int dx, int dy, int fdw, int fdh, int use_color_key, uint32 color_key) {

	assert(sw > 0);
	assert(sh > 0);
	assert(dw > 0);
	assert(dh > 0);

	int x, y;
	int xrsz, yrsz;
	if (sw < dw) {
		xrsz = 1;
	}
	else if (sw == dw) {
		xrsz = 0;
	}
	else {
		xrsz = -1;
	}

	if (sh < dh) {
		yrsz = 1;
	}
	else if (sh == dh) {
		yrsz = 0;
	}
	else {
		yrsz = -1;
	}

	if (xrsz == 0 && yrsz == 0) {
		return;
	}
	else if (xrsz > 0 && yrsz < 0) {
		uint32 * tmp = new uint32[dw*sh];
		interpolate_axis_increase(pixels, sw, sh, sx, sy, fsw, fsh, tmp, dw, sh, 0, 0, dw, sh,1, 0,0);
		interpolate_axis_decrease(tmp, dw, sh, 0, 0, dw, sh, dst, dw, dh, dx, dy, fdw, fdh, 2,use_color_key,color_key);
		delete[] tmp;
	}
	else if (xrsz > 0 && yrsz == 0) {
		interpolate_axis_increase(pixels, sw, sh, sx, sy, fsw, fsh, dst, dw, dh, dx, dy, fdw, fdh, 1, use_color_key, color_key);
	}
	else if (xrsz < 0 && yrsz == 0) {
		interpolate_axis_decrease(pixels, sw, sh, sx, sy, fsw, fsh, dst, dw, dh, dx, dy, fdw, fdh, 1, use_color_key, color_key);
	}
	else if (xrsz < 0 && yrsz > 0) {
		uint32 * tmp = new uint32[sw*dh];
		interpolate_axis_increase(pixels, sw, sh, sx, sy, fsw, fsh, tmp, sw, dh, 0, 0, sw, dh, 2,0,0);
		interpolate_axis_decrease(tmp, sw, dh, 0, 0, sw, dh, dst, dw, dh, dx, dy, fdw, fdh, 1, use_color_key, color_key);
		delete[] tmp;
	}
	else if (xrsz == 0 && yrsz > 0) {
		interpolate_axis_increase(pixels, sw, sh, sx, sy, fsw, fsh, dst, dw, dh, dx, dy, fdw, fdh, 2, use_color_key, color_key);
	}
	else if (xrsz == 0 && yrsz < 0) {
		interpolate_axis_decrease(pixels, sw, sh, sx, sy, fsw, fsh, dst, dw, dh, dx, dy, fdw, fdh, 2, use_color_key, color_key);
	}
	else if (xrsz > 0 && yrsz > 0) {
		uint32 * tmp = new uint32[dw*sh];
		interpolate_axis_increase(pixels, sw, sh, sx, sy, fsw, fsh, tmp, dw, sh, 0, 0, dw, sh, 1, 0, 0);

		interpolate_axis_increase(tmp, dw, sh, 0, 0, dw, sh, dst, dw, dh, dx, dy, fdw, fdh, 2, use_color_key, color_key);
		delete[] tmp;

	}
	else if (xrsz < 0 && yrsz < 0) {
		uint32 * tmp = new uint32[dw*sh];
		interpolate_axis_decrease(pixels, sw, sh, sx, sy, fsw, fsh, tmp, dw, sh, 0, 0, dw, sh, 1, 0, 0);
		interpolate_axis_decrease(tmp, dw, sh, 0, 0, dw, sh, dst, dw, dh, dx, dy, fdw, fdh, 2, use_color_key, color_key);
		delete[] tmp;
	}
}