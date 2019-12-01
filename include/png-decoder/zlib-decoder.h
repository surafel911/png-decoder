#ifndef PNG_ZLIB_DECODER_H
#define PNG_ZLIB_DECODER_H

#include <stdint.h>

struct zlib_stream {
	uint8_t* buffer;
	uint32_t cursor, length;
};

void
zlib_decode(struct zlib_stream* input, struct zlib_stream* output);

#endif // PNG_ZLIB_DECODER_H
