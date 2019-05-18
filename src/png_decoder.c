#include <png-decoder/png_decoder.h>

#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

#define HUFF_CODE_LEN_DATA	

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

struct _png_zlib_block {
	uint16_t list, dist, clen;
};

struct _png_context {
	uint8_t* data;
	size_t size;
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

	static const uint8_t hclen_code_lens[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3,
		13, 2, 14, 1, 15};

	struct _png_zlib_header zlib;
	struct _png_zlib_block block;
	
	uint8_t b, bit, remainder, bytes[2],
		hlen_code_len_counts[sizeof(hlen_code_len)],
		hclen_code_len_table[sizeof(hlen_code_lens)];

	fread(&zlib, sizeof(zlib), 1, file);

	printf("\tCM: %d\n\tCINFO: %d\n\tFDICT: %d\n\tFLEVEL: %d\n",
		zlib.cmf & ((1 << 4) - 1), zlib.cmf >> 4, zlib.flg & (1 << 5), zlib.flg >> 6);

	if ((zlib.flg & (1 << 5))) {
		fseek(file, 4, SEEK_CUR);
	}

	b = fgetc(file);
	printf("BFINAL: %d BTYPE: %d\n", b & 1, (b & (3 << 1)) >> 1);

	fread(&bytes, sizeof(bytes), 1, file);

	block.list = ((bytes[1] & ~((1 << 3) - 1)) >> 3) + 257;
	block.dist = ((bytes[1] & ((1 << 3) - 1)) << 5 | (bytes[0] & (3 << 5)) >> 6) + 1;
	block.clen = (bytes[0] & ((1 << 4) - 1)) + 4;

	printf("list: %d dist: %d clen: %d\n", block.list, block.dist,
		block.clen);

	bytes[0] = fgetc(file);
	bytes[1] = fgetc(file);
	for (int i = 0; i < block.clen; i++) {
		if (bit < 5) {
			code_len_count[i] = (bytes[0] & (7 << (5 - bit))) >> (5 - bit);
			bit += 3;
		} else {
			remainder = 7 - bit;
			code_len_count[i] = (bytes[0] & (7 >> (remainder))) | (bytes[1] & (7 << bit) >> bit);
		
			bytes[0] = bytes[1];
			bytes[1] = fgetc(file);
			bit = remainder;
		}
	}

	for (int i = 0; i < block.clen; i++) {
		printf("%d ", code_len_count[i]);
	}

	puts("");

	fclose(file);
}
