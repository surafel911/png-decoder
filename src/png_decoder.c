#include <png-decoder/png_decoder.h>

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define __packed		__attribute__((packed))

#define MAKE_TYPE(a, b, c, d) (((uint32_t)a << 24) | ((uint32_t)b << 16) | \
	((uint32_t)c << 8) | (uint32_t)d)

#define PNG_IHDR_TYPE	MAKE_TYPE('I', 'H', 'D', 'R')
#define PNG_PLTE_TYPE	MAKE_TYPE('P', 'L', 'T', 'E')
#define PNG_IDAT_TYPE	MAKE_TYPE('I', 'D', 'A', 'T')
#define PNG_IEND_TYPE	MAKE_TYPE('I', 'E', 'N', 'D')
#define PNG_GAMA_TYPE	MAKE_TYPE('g', 'A', 'M', 'A')

#define PNG_SIGNATURE	(((uint64_t)10 << 56) | ((uint64_t)26 << 48) | \
	((uint64_t)10 << 40) | ((uint64_t)13 << 32) | ((uint64_t)71 << 24) | \
	((uint64_t)78 << 16) | ((uint64_t)80 << 8) | (uint64_t)137)

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

struct _png_gama_chunk {
	uint32_t gamma;
} __packed;

static uint32_t
_png_get32be(FILE* file)
{
	static uint8_t bytes[4];

	uint32_t result;

	fread(bytes, sizeof(bytes), 1, file);

	result = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

	return result;
}

static void
_png_chunk_read_header(FILE* file, struct _png_chunk_header* header)
{
	header->length = _png_get32be(file);
	header->type = _png_get32be(file);
}

static void
_png_chunk_read_footer(FILE* file, struct _png_chunk_footer* footer)
{
	fread(footer, sizeof(*footer), 1, file);
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
_png_read_ihdr(FILE* file, struct _png_ihdr_chunk* ihdr)
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

	ihdr->width = _png_get32be(file);
	ihdr->height = _png_get32be(file);
	fread(&ihdr->depth, header.length - offsetof(struct _png_ihdr_chunk, depth),
		1, file);

	_png_chunk_read_footer(file, &footer);
}

void
png_load(const char* path, struct png_image* image)
{
	FILE* file;

	struct _png_ihdr_chunk ihdr;
	struct _png_chunk_header header;

	file= fopen(path, "rb");
	if (file == NULL) {
		puts("Bad path.");
	}

	_png_verify_signature(file);
	_png_read_ihdr(file, &ihdr);

	image->width = ihdr.width;
	image->height = ihdr.height;

	do {
		_png_chunk_read_header(file, &header);
		fseek(file, header.length + 4, SEEK_CUR);

		printf("%d\n", header.length);

		switch (header.type) {
		case PNG_PLTE_TYPE:
			puts("PLTE signature found.");
			break;
		case PNG_IDAT_TYPE:
			puts("IDAT signature found.");
			break;
		};
	} while (header.type != PNG_IEND_TYPE);


	fclose(file);
}
