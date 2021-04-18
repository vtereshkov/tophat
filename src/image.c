#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../lib/stb/stb_image.h"

#include "../lib/rawdraw/CNFG.h"

#include "image.h"

#include "misc.h"

image *loadimage(char *path) {
	int w, h, c;

	unsigned char *data = stbi_load(path, &w, &h, &c, 0);

	image *img;
	img = malloc(sizeof(image));
	img->w = w;
	img->h = h;
	img->c = c;

	if (data == NULL) {
		char buff[256];
		sprintf(buff, "could not find image at path %s", path);
		errprint(buff);
		img->rdimg = NULL;
		return img;
	}

	// Faster way, but it doesn't seem to work and jam ends soon. TODO FIXME
	/*if (c == 4) {
		img->rdimg = (unsigned int *)data;

		return img;
	}*/

	rdimg(img, data);
	stbi_image_free(data);

	return img;
}

void flipv(image *img) {

	if (img->rdimg == NULL) {
		errprint("flipv: image is not valid");
		return;
	}

	uint32_t *f;
	f = malloc(sizeof(uint32_t) * img->w * img->h);

	for (int i=0; i < img->w; i++) for (int j=0; j < img->h; j++)
			f[(j + 1) * img->w - i - 1] = img->rdimg[j * img->w + i];

	free(img->rdimg);
	img->rdimg = f;
}

void fliph(image *img) {
	if (img->rdimg == NULL) {
		errprint("fliph: image is not valid");
		return;
	}

	uint32_t *f;
	f = malloc(sizeof(uint32_t) * img->w * img->h);
	for (int i=0; i < img->w; i++) for (int j=0; j < img->h; j++)
			f[(img->h - j - 1) * img->w + i] = img->rdimg[j * img->w + i];

	free(img->rdimg);
	img->rdimg = f;
}

void imgcrop(image *img, int x1, int y1, int x2, int y2) {
	if (img->rdimg == NULL) {
		errprint("crop: image is not valid");
		return;
	}

	uint32_t *n;
	n = malloc(sizeof(uint32_t) * x2-x1 * y2-y1);

	for (int x=0; x < x2-x1; x++) {
		for (int y=0; y < y2-y1; y++) {
			n[y*(x2-x1)+x] = img->rdimg[(y+y1)*img->w+x+x1];
		}
	}
	free(img->rdimg);
	img->w = x2-x1;
	img->h = y2-y1;
	img->rdimg = n;
}

void rdimg(image *img, unsigned char *data) {
	uint32_t *rd;
	rd = malloc(sizeof(int) * img->w * img->h);
	uint32_t current = 0;

	for (int y=0; y < img->h; y += 1) {
		for (int x=0; x < img->w; x += 1) {
			current = 0;
			for (int i=0; i < img->c; i++) {
				current = current << 8;
				current += (uint32_t)data[(y * img->w + x) * img->c + i];
			}
			for (int i=0; i < 4 - img->c; i++) {
				if (current == 1) {
					current = 0x00 | current<<8;
					continue;
				}
				current = 0xff | current<<8;
			}
			rd[(y * img->w + x)] = current;
		}
	}
	img->rdimg = rd;
}
