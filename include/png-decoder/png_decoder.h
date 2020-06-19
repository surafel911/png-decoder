#ifndef PNG_DECODER_H
#define PNG_DECODER_H

#include <stdint.h>

struct png_image {
	const uint8_t* data;
	uint32_t width, height;
};

void
png_load(const char* path, struct png_image* image);

#endif // PNG_DECODER_H
