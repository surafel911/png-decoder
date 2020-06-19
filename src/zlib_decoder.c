#include <png-decoder/zlib_decoder.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PNG_MAX_BIT_LENGTHS			19 // max number of bit lenghs
#define PNG_MAX_CODE_SYMBOLS		288 // 256 code literals, end code, some length code, etc.

struct _zlib_header {
	uint8_t cmf, flg;
} __packed;

struct _zlib_block_header {
	bool final;
	enum _png_block_type block_type;
};

struct _zlib_block_format {
	uint16_t lit, dist, clen;
};

struct _zlib_huffman {
	uint16_t* count;
	uint16_t* symbol;
};

static void
_png_zlib_read_header(FILE* file, struct _png_zlib_header* header)
{
	fread(header, sizeof(*header), 1, file);

	if (header->flg & (1 << 5)) {
		puts("ZLIB DICTID flag present.");
		exit(-1);
	}
}

static void
_png_zlib_block_read_header(FILE* file, struct _png_zlib_block_header* header)
{
	uint8_t bytes[2];

	bytes[0] = fgetc(file);

	header->final = bytes[0] & 1;
	header->block_type = (bytes[0] & (3 << 1)) >> 1;
}

/* This code is most likely for BTYPE=2 */
static void
_png_zlib_block_read_format(FILE* file,
	struct _png_zlib_block_format* format)
{
	uint8_t bytes[2];

	fread(&bytes, sizeof(bytes), 1, file);

	format->lit = _png_read_bits(file, 5);
	format->dist = _png_read_bits(file, 5);
	format->clen = _png_read_bits(file, 4);
}

