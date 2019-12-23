#include <png-decoder/png-decoder.h>

#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <png-decoder/zlib-inflate.h>

#define __packed		__attribute__((packed))

#define _PNG_TYPE(x)	*((uint32_t*)x)

#define PNG_IHDR_TYPE	_PNG_TYPE("IHDR")
#define PNG_IDAT_TYPE	_PNG_TYPE("IDAT")
#define PNG_IEND_TYPE	_PNG_TYPE("IEND")
#define PNG_SRGB_TYPE	_PNG_TYPE("sRGB")
#define PNG_CHRM_TYPE	_PNG_TYPE("cHRM")
#define PNG_GAMA_TYPE	_PNG_TYPE("gAMA")
#define PNG_BKGD_TYPE	_PNG_TYPE("bKGD")

#define PNG_MAX_BIT_LENGTHS			19 // max number of bit lenghs
#define PNG_MAX_CODE_SYMBOLS		288 // 256 code literals, end code, some size code, etc.

enum _png_block_type {
	PNG_NO_COMPRESSION,
	PNG_FIXED_HUFFMAN_COMPRESSION,
	PNG_DYNAMIC_HUFFMAN_COMPRESSION,
};

struct _png_chunk_header {
	uint32_t size;
	union {
		uint8_t name[4];
		uint32_t type;
	};
} __packed;

struct _png_chunk_footer {
	uint32_t crc;
} __packed;

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
	header->size = _png_be_to_le(header->size);
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
 	static uint8_t png_signature[] = {137, 80, 78, 71, 13, 10, 26, 10};

	uint64_t signature;

	fread(&signature, sizeof(signature), 1, file);
	if (signature == *(uint64_t*)png_signature) {
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
	if (header.type == PNG_IHDR_TYPE && header.size == 13) {	
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

void
png_load(const char* path, struct png_image* image)
{
	FILE* file;

	struct _png_ihdr_chunk ihdr;
	struct _png_chunk_header header;
	struct zlib_stream input = {}, output = {};

	file = fopen(path, "rb");
	if (file == NULL) {
		puts("Bad path.");
	}

	_png_verify_signature(file);
	_png_ihdr_read_chunk(file, &ihdr);

	image->width = ihdr.width;
	image->height = ihdr.height;

	for (;;) {
		_png_chunk_read_header(file, &header);
		if (header.type == PNG_IDAT_TYPE) {
			break;
		}

		fseek(file, header.size + sizeof(uint32_t), SEEK_CUR);
	}

	for (;;) {
		input.buffer = realloc(input.buffer, input.size + header.size);
		fread(input.buffer + input.size, sizeof(*input.buffer), header.size, file);
		input.size += header.size;

		fseek(file, sizeof(uint32_t), SEEK_CUR);
		_png_chunk_read_header(file, &header);
		if (header.type != PNG_IDAT_TYPE) {
			break;
		}
	}
	input.cursor = input.buffer;

	printf("Input stream size: %zu\n", input.size);

	output.size = (ihdr.width + ihdr.height) * 4;
	output.buffer = malloc(output.size);
	output.cursor = output.buffer;

#if 1
	zlib_inflate(&input, &output);
#endif

	fclose(file);
}
