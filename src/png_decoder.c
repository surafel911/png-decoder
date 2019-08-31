#include <png-decoder/png_decoder.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <png-decoder/zlib_inflate.h>

#define __packed 		__attribute__((packed))

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

#define PNG_DEBUG

enum png_zlib_block_type {
	PNG_BLOCK_TYPE_NONE,
	PNG_BLOCK_TYPE_FIXED,
	PNG_BLOCK_TYPE_DYNAMIC,
};

struct png_stream_signature {
	uint64_t signature;
} __packed;

struct png_stream_chunk_header {
	uint32_t chunk_length;
	uint32_t chunk_type;
} __packed;

struct png_stream_chunk_footer {
	uint32_t crc;
} __packed;

struct png_stream_ihdr_chunk {
	uint32_t width, height;
	uint8_t depth, color, compression, filter, interlace;
} __packed;

struct png_stream_srgb_chunk {
	uint8_t rendering;
} __packed;

struct png_stream_chrm_chunk {
	uint32_t white_point[2], red[2], green[2], blue[2];
} __packed;

struct png_stream_gama_chunk {
	uint32_t gama;
} __packed;

struct png_stream_bkgd_chunk {
	uint16_t red, green, blue;
} __packed;

static void
_png_fatal_error(const char* message)
{
	puts(message);
	exit(-1);
}

static uint32_t
_png_be_to_le(uint32_t value)
{
	uint8_t* bytes = (uint8_t*)&value;

	return ((bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]);
}

static void
_png_stream_png_signature(FILE* file, 
	struct png_stream_signature* signature)
{
	fread(signature, sizeof(*signature), 1, file);
}

static void
_png_verify_png_signature(struct png_stream_signature* signature)
{
	if (signature->signature == PNG_SIGNATURE) {
		puts("[PNG] PNG signature found.");
	} else {
		_png_fatal_error("[PNG] PNG signature not found.");
	}
}

static void
_png_stream_ihdr_chunk(FILE* file, struct png_stream_ihdr_chunk* chunk)
{
	fread(chunk, sizeof(*chunk), 1, file);
	chunk->width = _png_be_to_le(chunk->width);
	chunk->height = _png_be_to_le(chunk->height);
}

static void
_png_verify_ihdr_header(struct png_stream_chunk_header* header)
{
	if (header->chunk_type == PNG_IHDR_TYPE && header->chunk_length == 13) {	
		puts("\t[PNG] IHDR signature found.");
	} else {
		_png_fatal_error("\t[PNG] IHDR signature not found.");
	}
}

#if 0

static void
_png_stream_zlib_dynamic_huffman_code_lengths_tree_construct(FILE* file,
	struct png_zlib_block_format* block_format,
	struct png_huffman_tree* tree)
{
	unsigned int index, code_length_code_lengths[19];

	_png_stream_zlib_dynamic_code_lengths_code_lengths(file, block_format,
			code_length_code_lengths);

	for (index = 0; index < 19; index++) {
		tree->lengths[index] = code_length_code_lengths[index];
	}

	for (; index < 288; index++) {
		tree->lengths[index] = 0;
	}

	_png_huffman_tree_construct(tree, 288, 7);

#ifdef PNG_DEBUG
	for (int index = 0; index < 288; index++) {
		printf("%u ", tree->symbols[index]);
	}
	puts("");
#endif
}

#if 1

static unsigned int
_png_stream_huffman_decode(FILE* file, struct png_huffman_tree* tree)
{
	return 0;
}

static void
_png_stream_zlib_dynamic_literal_distance_lengths(FILE* file,
	struct png_zlib_block_format* block_format,
	struct png_huffman_tree* code_lengths_tree,
	unsigned int literal_distance_table[PNG_NUM_DEFLATE_SYMBOLS +
		PNG_NUM_DISTANCE_SYMBOLS])
{
	unsigned int value, repeat, symbol, limit = block_format->nlit +
		block_format->ndist;

	for (int index = 0; index < limit;) {
		symbol = _png_stream_huffman_decode(file, code_lengths_tree);

		if (symbol <= 15) {
			literal_distance_table[index++] = symbol;
			continue;
		} else {
			switch (symbol) {
			case 16:
				value = literal_distance_table[index];
				repeat = _png_stream_bits(file, 2) + 3;
				break;
			case 17:
				value = 0;
				repeat = _png_stream_bits(file, 3) + 3;
				break;
			case 18:
				value = 0;
				repeat = _png_stream_bits(file, 7) + 11;
				break;
			default:
				_png_fatal_error("Mega decompression error");
			}

			for (int n = 1; n <= repeat; n++) {
				literal_distance_table[index + n] = value;
			}
			index += repeat;
		}
	}
}

static void
_png_zlib_dynamic_huffman_literals_tree_construct(
	struct png_zlib_block_format* block_format,
	struct png_huffman_tree* literals_tree,
	unsigned int literal_distance_table[PNG_NUM_DEFLATE_SYMBOLS +
		PNG_NUM_DISTANCE_SYMBOLS])
{

}

static void
_png_zlib_dynamic_huffman_distances_tree_construct(
	struct png_zlib_block_format* block_format,
	struct png_huffman_tree* distances_tree,
	unsigned int literal_distance_table[PNG_NUM_DEFLATE_SYMBOLS +
		PNG_NUM_DISTANCE_SYMBOLS])
{
}

static void
_png_stream_zlib_dynamic_inflate_block(FILE* file, uint8_t* stream,
	struct png_huffman_tree* literals_tree,
	struct png_huffman_tree* distances_tree)
{
	unsigned int symbol, distance;

	for (;;) {
		symbol = _png_stream_huffman_decode(file, literals_tree);

		if (symbol == 256) {
			break;
		}

		if (symbol < 256) {
			// write symbol
		} else {
			distance = _png_stream_huffman_decode(file, distances_tree);
			memcpy(stream, stream - distance, distance);
			stream += distance;
		}
	}
}
#endif

static void
_png_stream_zlib_inflate_dynamic_huffman_block(FILE* file, uint8_t* data)
{
	unsigned int literal_distance_table[PNG_NUM_DEFLATE_SYMBOLS + 
		PNG_NUM_DISTANCE_SYMBOLS];

	struct png_huffman_tree code_lengths_tree, literals_tree, distances_tree;
	struct png_zlib_block_format block_format;

	_png_stream_zlib_block_format(file, &block_format);

	_png_stream_zlib_dynamic_huffman_code_lengths_tree_construct(file,
		&block_format, &code_lengths_tree);
#if 1
	_png_stream_zlib_dynamic_literal_distance_lengths(file,
		&block_format, &code_lengths_tree, literal_distance_table);

	_png_zlib_dynamic_huffman_literals_tree_construct(&block_format,
		&literals_tree,	literal_distance_table);
	_png_zlib_dynamic_huffman_distances_tree_construct(&block_format,
		&distances_tree, literal_distance_table);

	_png_stream_zlib_dynamic_inflate_block(file, data, &literals_tree,
		&distances_tree);
#endif
}

static void
_png_stream_zlib_inflate_fixed_huffman_block(FILE* file, uint8_t* data)
{
	_png_fatal_error("[PNG] Fixed huffman inflate not implementated.");
}

static void
_png_stream_zlib_inflate_block(FILE* file, uint8_t* data,
	struct png_zlib_block_header* block_header)
{
	if (block_header->block_type == PNG_BLOCK_TYPE_NONE) {
		_png_stream_zlib_inflate_uncompressed_block(file, data);
	} else {
		if (block_header->block_type == PNG_BLOCK_TYPE_DYNAMIC) {
			_png_stream_zlib_inflate_dynamic_huffman_block(file, data);
		} else {
			_png_stream_zlib_inflate_fixed_huffman_block(file, data);
		}
	}
}

#endif

void
png_stream_chunk_header(FILE* file,
	struct png_stream_chunk_header* header)
{
	fread(header, sizeof(*header), 1, file);
	header->chunk_length = _png_be_to_le(header->chunk_length);
}

void
png_stream_chunk_footer(FILE* file,
	struct png_stream_chunk_footer* footer)
{
	fread(footer, sizeof(*footer), 1, file);
	footer->crc = _png_be_to_le(footer->crc);
}

void
png_check_png_signature(FILE* file)
{
	struct png_stream_signature signature;

	_png_stream_png_signature(file, &signature);
	_png_verify_png_signature(&signature);
}

void
png_check_ihdr_signature(FILE* file, struct png_stream_ihdr_chunk* chunk)
{
	struct png_stream_chunk_header header;
	struct png_stream_chunk_footer footer;

	png_stream_chunk_header(file, &header);

	_png_verify_ihdr_header(&header);
	_png_stream_ihdr_chunk(file, chunk);

	png_stream_chunk_footer(file, &footer);
}

#if 0

void
png_check_zlib_header(FILE* file, struct png_zlib_header* header)
{
	_png_stream_zlib_header(file, header);
	_png_verify_zlib_header(header);
}

void
png_zlib_inflate_block(FILE* file, uint8_t* output)
{
	struct png_zlib_block_header header;

	_png_stream_zlib_block_header(file, &header);
	_png_stream_zlib_inflate_block(file, output, &header);
}

#endif

void
png_load(const char* path, struct png_image* image)
{
	FILE* file;

	struct png_stream_ihdr_chunk ihdr_chunk;
	struct png_stream_chunk_header chunk_header;

	file = fopen(path, "rb");

	png_check_png_signature(file);
	png_check_ihdr_signature(file, &ihdr_chunk);

	png_stream_chunk_header(file, &chunk_header);
	while (chunk_header.chunk_type != PNG_IDAT_TYPE) {
#ifdef PNG_DEBUG
		printf("\t[PNG] %4s chunk.\n", (char*)&chunk_header.chunk_type);
#endif

		fseek(file, chunk_header.chunk_length + 
			sizeof(struct png_stream_chunk_footer), SEEK_CUR);
		png_stream_chunk_header(file, &chunk_header);
	}

#ifdef PNG_DEBUG
	printf("\t[PNG] %4s chunk.\n", (char*)&chunk_header.chunk_type);
#endif

	struct zlib_stream input = {};
	struct zlib_stream output = {};

	while (chunk_header.chunk_type != PNG_IEND_TYPE) {
		input.buffer = realloc(input.buffer, input.size +
			chunk_header.chunk_length);

		fread(input.buffer + input.size, sizeof(*input.buffer),
			chunk_header.chunk_length, file);

		input.size += chunk_header.chunk_length;
		fseek(file, sizeof(struct png_stream_chunk_footer), SEEK_CUR);
		png_stream_chunk_header(file, &chunk_header);
	}

	fclose(file);

	input.cursor = input.buffer;

	output.size = (ihdr_chunk.width * ihdr_chunk.height) * 4;
	output.buffer = malloc(output.size);
	output.cursor = output.buffer;

	zlib_inflate(&input, &output);

	free(input.buffer);

	image->width = ihdr_chunk.width;
	image->height = ihdr_chunk.height;
	image->data = output.buffer;
}
