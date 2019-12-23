#ifndef PNG_ZLIB_DECODER_H
#define PNG_ZLIB_DECODER_H

#include <stddef.h>
#include <stdint.h>

struct zlib_stream {
	uint8_t* buffer;
	uint8_t* cursor;
	size_t size;
};

void
zlib_inflate(struct zlib_stream* input, struct zlib_stream* output);

#endif // PNG_ZLIB_DECODER_H
