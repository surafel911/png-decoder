#include <png-decoder/png-decoder.h>

#include <stdio.h>
#include <stdint.h>

int
main(int argc, char* argv[])
{
	struct png_image image;

	if (argc > 1) {
		png_load(argv[1], &image);
	} else {
		png_load("test/gimp.png", &image);
	}
}
