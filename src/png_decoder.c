#include <png-decoder/png_decoder.h>

#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define __packed		__attribute__((packed))

#define _PNG_TYPE(x)		*((uint32_t*)x)

#define PNG_IHDR_TYPE	_PNG_TYPE("IHDR")
#define PNG_IDAT_TYPE	_PNG_TYPE("IDAT")
#define PNG_IEND_TYPE	_PNG_TYPE("IEND")
#define PNG_SRGB_TYPE	_PNG_TYPE("sRGB")
#define PNG_CHRM_TYPE	_PNG_TYPE("cHRM")
#define PNG_GAMA_TYPE	_PNG_TYPE("gAMA")
#define PNG_BKGD_TYPE	_PNG_TYPE("bKGD")

#define PNG_SIGNATURE	((10L << 56) | (26L << 48) | \
	(10L << 40) | (13L << 32) | (71L << 24) | \
	(78L << 16) | (80L << 8) | (137L))

#define PNG_MAX_BIT_LENGTHS			19 // max number of bit lenghs
#define PNG_MAX_CODE_SYMBOLS		288 // 256 code literals, end code, some length code, etc.

enum _png_block_type {
	PNG_NO_COMPRESSION,
	PNG_FIXED_HUFFMAN_COMPRESSION,
	PNG_DYNAMIC_HUFFMAN_COMPRESSION,
};

struct _png_chunk_header {
	uint32_t length;
	uint32_t type;
};

struct _png_chunk_footer {
	uint32_t crc;
};

struct _png_ihdr_chunk {
	uint32_t width, height;
	uint8_t depth, color, compression, filter, interlace;
} __packed;

struct _png_srgb_chunk {
	uint8_t rendering;
} __packed;

struct _png_chrm_chunk {
	uint32_t white_point[2], red[2], green[2], blue[2];
} __packed;

struct _png_gama_chunk {
	uint32_t gamma;
} __packed;

struct _png_bkgd_chunk {
	uint16_t red, green, blue;
} __packed;

struct _png_zlib_header {
	uint8_t cmf, flg;
} __packed;

struct _png_zlib_block_header {
	bool final;
	enum _png_block_type block_type;
};

struct _png_zlib_block_format {
	uint16_t lit, dist, clen;
};

struct _png_huffman_tree {
};

static uint32_t
_png_be_to_le(uint32_t value)
{
	uint8_t* bytes;

	bytes = (uint8_t*)&value;

	return ((bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]);
}

static void
_png_chunk_read_header(FILE* file, struct _png_chunk_header* header)
{
	fread(header, sizeof(*header), 1, file);
	header->length = _png_be_to_le(header->length);
}

static void
_png_chunk_read_footer(FILE* file, struct _png_chunk_footer* footer)
{
	fread(footer, sizeof(*footer), 1, file);
	footer->crc = _png_be_to_le(footer->crc);
}

static void
_png_verify_signature(FILE* file)
{
	uint64_t signature;

	fread(&signature, sizeof(signature), 1, file);
	if (signature == PNG_SIGNATURE) {
		puts("PNG signature found.");
	} else {
		puts("PNG signature corrupted.");
		exit(-1);
	}
}

static void
_png_ihdr_read_chunk(FILE* file, struct _png_ihdr_chunk* ihdr)
{
	struct _png_chunk_header header;
	struct _png_chunk_footer footer;

	_png_chunk_read_header(file, &header);
	if (header.type == PNG_IHDR_TYPE && header.length == 13) {	
		puts("IHDR signature found.");
	} else {
		puts("IHDR signature corrupted.");
		exit(-1);
	}

	fread(ihdr, sizeof(*ihdr), 1, file);
	ihdr->width = _png_be_to_le(ihdr->width);
	ihdr->height = _png_be_to_le(ihdr->height);

	_png_chunk_read_footer(file, &footer);
}

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

static uint8_t
_png_read_bits(FILE* file, const uint8_t bits)
{
	static uint8_t byte = 0, next_byte = 0, bit_count = 0;

	uint8_t value;

	if (bits < 0 || bits > 8) {
		puts("Wrong bits.");
		exit(-1);
	}

	// TODO: Rough idea of what's supposed to happen. Working is another story...
	if (bit_count >= bits) {
		bit_count -= bits;
		value = (byte & (((1 << bits) - 1) << bit_count)) >> bit_count;
	} else {
		next_byte = fgetc(file);
		value = ((byte & ((1 << bit_count) - 1)) << (bits - bit_count)) | (next_byte & (((1 << (bits - bit_count)) - 1) << (8 - bits - bit_count))) >> (8 - bits - bit_count);
		byte = next_byte;
		bit_count -= bits;
	}

	return value;
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

void
png_load(const char* path, struct png_image* image)
{
	FILE* file;

	struct _png_ihdr_chunk ihdr;
	struct _png_chunk_header chunk;

	file= fopen(path, "rb");
	if (file == NULL) {
		puts("Bad path.");
	}

	_png_verify_signature(file);
	_png_ihdr_read_chunk(file, &ihdr);

	image->width = ihdr.width;
	image->height = ihdr.height;

	for (;;) {
		_png_chunk_read_header(file, &chunk);
		if (chunk.type == PNG_IDAT_TYPE) {
			break;
		}
		fseek(file, chunk.length + 4, SEEK_CUR);
	};

	/* Here begins absolutely, offensive code... */

	/* Starting ZLIB Decoding Implementation */

#if 0
	static const uint8_t code_lengths_order[] = {16, 17, 18, 0, 8, 7, 9, 6, 10,
		5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
#endif

	struct _png_zlib_header zlib;
	struct _png_zlib_block_header block;
	struct _png_zlib_block_format format;

//	struct _png_huffman_tree tree;

	_png_zlib_read_header(file, &zlib);

#if 1
	printf("ZLIB header:\n");
	printf("\tCM: %d\n\tCINFO: %d\n\tFDICT: %d\n\tFLEVEL: %d\n",
		zlib.cmf & ((1 << 4) - 1), zlib.cmf >> 4, zlib.flg & (1 << 5), zlib.flg >> 6);
#endif

	_png_zlib_block_read_header(file, &block);

#if 1
	printf("Block header:\n");
	printf("\tBFINAL: %d BTYPE: %d\n", (int)block.final, (int)block.block_type);
#endif

	_png_zlib_block_read_format(file, &format);

#if 1
	printf("Block format:\n");
	printf("\tHLIT: %d\n\tHDIST: %d\n\tHCLEN: %d\n", format.lit, format.dist, format.clen);
#endif

#if 0
	// Step 1
	uint8_t code_length_code_index[19] = {16, 17, 18, ...},
		code_lengths[19];

	_png_huff_get_code_lengths(code_lengths)
	{
		memset(code_length_count, 0, sizeof(code_length_count));

		for (int i = 0; i < sizeof(code_length_count); i++) {
			code_lengths[code_lengths[i]] = _png_read_bits(3);
		}
	}

	// Step 2
	
	uint8_t max_bits // find the largest code length?

#endif

	fclose(file);
}
