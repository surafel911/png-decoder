#include <png-decoder/png_decoder.h>

int
main(int argc, char* argv[])
{
	struct png_image image;;
	png_load("test/windows.png", &image);
}
