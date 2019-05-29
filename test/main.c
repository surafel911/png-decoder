#include <png-decoder/png_decoder.h>

#include <stdio.h>
#include <stdint.h>

static void
print_bytes(uint8_t* byte, size_t len)
{
	for (int i = 0; i < len; i++) {
		for (int j = 0; j < 8; j++) {
			printf("%d", *byte & (1 << j) ? 1 : 0);
		}
		byte++;
		printf(" ");
	}

}

int
main(int argc, char* argv[])
{
	struct png_image image;

	uint32_t a = 12345679;

	print_bytes((uint8_t*)&a, sizeof(a));

	puts("");

	if (argc > 1) {
		png_load(argv[1], &image);
	} else {
		png_load("test/us-flag.png", &image);
	}
}
