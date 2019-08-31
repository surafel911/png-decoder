#include <png-decoder/zlib_inflate.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define ZLIB_MAX_BITS					15
#define ZLIB_MAX_SYMBOLS				288
#define ZLIB_NUM_DEFLATE_SYMBOLS		288
#define ZLIB_NUM_DISTANCE_SYMBOLS		32
#define ZLIB_NUM_CODE_LENGTH_CODES		19

#define ZLIB_ARRAY_SIZE(x)				(sizeof(x)\sizeof(*x))

#define ZLIB_ZERO_INT_ARRAY(x) 			\
	for (int i = 0; i < ZLIB_ARRAY_SIZE(x); i++) {	\
		x[i] = 0;	\
	}

enum _zlib_block_type {
	ZLIB_BLOCK_TYPE_NONE,
	ZLIB_BLOCK_TYPE_FIXED,
	ZLIB_BLOCK_TYPE_DYNAMIC,
};

struct _zlib_header {
	bool fdict;
	unsigned char compression_method, compression_info,
		flevel;
};

struct _zlib_block_header {
	bool block_final;
	enum _zlib_block_type block_type;
};

struct _zlib_block_format {
	unsigned short nlen;
	unsigned char ndist, ncodes;
};

struct _zlib_stream_uncompressed_header {
	uint32_t length, nlength;
};

struct _zlib_huffman_tree {
	unsigned int ncodes, nbits, lengths[ZLIB_MAX_SYMBOLS],
		symbols[ZLIB_MAX_SYMBOLS];
};

static void
_zlib_fatal_error(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	exit(-1);
}

static uint8_t
_zlib_stream_getc(struct zlib_stream* stream)
{
	return *(stream->cursor++);
}

static uint8_t
_zlib_stream_get_bits(const uint8_t byte, const uint8_t start_bit,
	const uint8_t end_bit)
{
	return byte & (((1 << (end_bit - 1)) - 1) << (start_bit - 1));
}

static uint8_t
_zlib_stream_read_bit(struct zlib_stream* stream)
{
	static uint8_t* cursor = NULL;
	static uint8_t bit, byte, bits_left;

	if (bits_left == 0 || cursor != stream->cursor) {
		byte = _zlib_stream_getc(stream);
		cursor = stream->cursor;
		bits_left = 8;
	}

	bit = byte & 1;

	byte >>= 1;
	bits_left--;

	return bit;
}

static uint8_t
_zlib_stream_read_bits(struct zlib_stream* stream, uint8_t bits)
{
	uint8_t value;

	value = 0;
	for (uint8_t bit = 0; bit < bits; bit++) {
		value |= _zlib_stream_read_bit(stream) << bit;
	}

	return value;
}

static size_t
_zlib_stream_read(uint8_t* ptr, const size_t bytes,
	struct zlib_stream* stream)
{
	if (stream->cursor - (stream->buffer + stream->size) < bytes) {
		return 0;
	}

	memcpy(ptr, stream->cursor, bytes);
	stream->cursor += bytes;

	return bytes;
}

static size_t
_zlib_stream_copy(struct zlib_stream* input, struct zlib_stream* output,
	const size_t bytes)
{
	if (input->cursor - (input->buffer + input->size) < bytes) {
		return 0;
	}

	memcpy(output->cursor, input->cursor, bytes);
	input->cursor += bytes;
	output->cursor += bytes;

	return bytes;
}

static void
_zlib_stream_read_header(struct zlib_stream* stream,
	struct _zlib_header* header)
{
	uint8_t compression, flags;

	compression = _zlib_stream_getc(stream);
	flags = _zlib_stream_getc(stream);

	header->compression_method = _zlib_stream_get_bits(compression, 5, 1);
	header->compression_info = compression >> 4;
	header->fdict = _zlib_stream_get_bits(flags, 6, 1);
	header->flevel = flags >> 6;
}

static void
_zlib_stream_verify_header(struct _zlib_header* header)
{
	// TODO: Fix this error lmao
	//_zlib_fatal_error("[ZLIB] That shit is broken dawg.");
}

static void
_zlib_stream_read_block_header(struct zlib_stream* stream,
	struct _zlib_block_header* block_header)
{
	block_header->block_final = _zlib_stream_read_bit(stream);
	block_header->block_type = _zlib_stream_read_bits(stream, 2);

#ifdef ZLIB_DEBUG
	printf("\t[ZLIB] Block Final: %d\n\t[ZLIB] Block Type: %d\n",
		header->block_final, header->block_type);
#endif
}

static void
_zlib_stream_read_block_format(struct zlib_stream* stream,
	struct _zlib_block_format* block_format)
{
	block_format->nlen = _zlib_stream_read_bits(stream, 5) + 257;
	block_format->ndist = _zlib_stream_read_bits(stream, 5) + 1;
	block_format->ncodes = _zlib_stream_read_bits(stream, 4) + 4;

#ifdef ZLIB_DEBUG
	printf("\t[ZLIB] Block NLIT: %d\n\t[ZLIB] Block NDIST: %d.\n\t[ZLIB] \
	Block NCODES: %d\n", format->nlen, format->ndist, format->ncodes);
#endif
}

static void
_zlib_stream_inflate_uncompressed_block(struct zlib_stream* input,
	struct zlib_stream* output)
{
	struct _zlib_stream_uncompressed_header uncompressed_header;

	_zlib_stream_read((uint8_t*)&uncompressed_header,
		sizeof(uncompressed_header), input);

	if (uncompressed_header.nlength != ~uncompressed_header.length) {
		_zlib_fatal_error("[ZLIB] Error in uncompressed block.");
	}

	_zlib_stream_copy(input, output, uncompressed_header.length);
}

static void
_zlib_stream_read_code_length_code_lengths(struct zlib_stream* stream,
	struct _zlib_block_format* block_format,
	unsigned int code_length_code_lengths[ZLIB_NUM_CODE_LENGTH_CODES])
{
	static unsigned int code_length_code_lengths_order[19] = {
		16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
	};

	int index = 0;

	for (; index < block_format->ncodes; index++) {
		code_length_code_lengths[code_length_code_lengths_order[index]] = 
			_zlib_stream_read_bits(stream, 3);
	}

	for (; index < ZLIB_NUM_CODE_LENGTH_CODES; index++) {
		code_length_code_lengths[code_length_code_lengths_order[index]] = 0;
	}
}

#if 1
static void
print_bits(void* ptr, size_t bytes)
{
	char* n = ptr;

	for (int i = bytes; i >= 0; i--) {
		for (int j = 7; j >= 0; j--) {
			printf("%u", (n[i] >> j) & 1);
		}
	}
	puts("");
}
#endif

static void
_zlib_huffman_tree_construct(unsigned int* table,
	unsigned int* lengths, const unsigned int codes, unsigned int bits)
{
	unsigned int index, code_length_counts[ZLIB_MAX_SYMBOLS],
		next_code[ZLIB_MAX_SYMBOLS];


	for (index = 0; index < codes; index++) {
		next_code[index] = 0;
		code_length_counts[index] = 0;
	}

	for (index = 0; index < codes; index++) {
		code_length_counts[lengths[index]]++;
	}

	code_length_counts[0] = 0;

	for (index = 1; index <= ZLIB_MAX_BITS; index++) {
		next_code[index] = (next_code[index - 1] +
			code_length_counts[index - 1]) << 1;
	}

	for (index = 0; index < codes; index++) {
		if (lengths[index] != 0) {
			table[index] = next_code[lengths[index]]++;
		} else {
			table[index] = 0;
		}
		print_bits(&table[index], sizeof(*table));
	}
}

bool
zlib_inflate(struct zlib_stream* input, struct zlib_stream* output)
{
	unsigned int table[ZLIB_MAX_SYMBOLS],
		code_length_code_lengths[ZLIB_NUM_CODE_LENGTH_CODES];

	struct _zlib_header header;
	struct _zlib_block_header block_header;
	struct _zlib_block_format block_format;

	_zlib_stream_read_header(input, &header);
	printf("%ld\n", input->cursor - input->buffer);
	_zlib_stream_verify_header(&header);
	_zlib_stream_read_block_header(input, &block_header);
	_zlib_stream_read_block_format(input, &block_format);

	puts("[ZLIB] Block header:\n");
	printf("\t[ZLIB] Final: %d\n", block_header.block_final);
	printf("\t[ZLIB] Type: %d\n", block_header.block_type);

	switch (block_header.block_type) {
	case ZLIB_BLOCK_TYPE_NONE:
		_zlib_stream_inflate_uncompressed_block(input, output);
		break;
	case ZLIB_BLOCK_TYPE_FIXED:
		break;
	case ZLIB_BLOCK_TYPE_DYNAMIC:
		puts("[ZLIB] Dynamic huffman block.");
		_zlib_stream_read_code_length_code_lengths(input, &block_format,
			code_length_code_lengths);

		_zlib_huffman_tree_construct(table, code_length_code_lengths, 19, 7);
		break;
	default:
		_zlib_fatal_error("[ZLIB] Unknown block type.\n");
	}

	return true;
}
