#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum zlib_block_type {
	ZLIB_NO_COMPRESSION,
	ZLIB_FIXED_COMPRESSION,
	ZLIB_DYNAMIC_COMPRESSION,
};

struct zlib_header {
};

struct zlib_block_header {
};

struct zlib_block_format {
};

struct zlib_huffman_node {
};

static void
_zlib_read_header(FILE* file, struct _zlib_header* header)
{
	
}
