#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define __packed		__attribute__((packed))

enum zlib_block_type {
	ZLIB_NO_COMPRESSION,
	ZLIB_FIXED_COMPRESSION,
	ZLIB_DYNAMIC_COMPRESSION,
};

struct zlib_header {
	uint8_t cmf, flg;
} __packed;

struct zlib_block_header {
	bool final;
	enum _png_block_type block_type;
};

struct zlib_block_format {
	uint16_t lit, dist, clen;
};

struct zlib_huffman_node {
};

static void
_zlib_read_header(FILE* file, struct _zlib_header* header)
{
	fread(header, sizeof(*header), 1, file);

	if (header->flg & (1 << 5)) {
		puts("ZLIB DICTID flag present.");
		exit(-1);
	}
}

static void
_zlib_block_read_header(FILE* file, struct _png_zlib_block_header* header)
{
	uint8_t bytes[2];

	bytes[0] = fgetc(file);

	header->final = bytes[0] & 1;
	header->block_type = (bytes[0] & (3 << 1)) >> 1;
}
