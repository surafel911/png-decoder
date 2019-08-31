#ifndef ZLIB_INFLATE_H
#define ZLIB_INFLATE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct zlib_stream {
	uint8_t* buffer;
	uint8_t* cursor;
	size_t size;
};

bool
zlib_inflate(struct zlib_stream* input, struct zlib_stream* output);

#endif // ZLIB_INFLATE_H
