
#include <stdio.h>
#include <stdlib.h>

#include "lodepng.h"

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: mklogo logo.png\n");
		return -1;
	}
	
	unsigned error;
	unsigned char* image;
	unsigned width, height;
	
	error = lodepng_decode32_file(&image, &width, &height, argv[1]);
	if(error) {
		printf("error %u: %s\n", error, lodepng_error_text(error));
		return -2;
	}

	unsigned stride = width * 4;

	// generate header on standard error
	fprintf(stderr, "\n/* This file is automatically generated, DO NOT EDIT! */\n\n"
			"#define GUMBOOT_LOGO_WIDTH %d\n#define GUMBOOT_LOGO_HEIGHT %d\n\nextern unsigned char gumboot_logo_pixels[GUMBOOT_LOGO_WIDTH*GUMBOOT_LOGO_HEIGHT*4];\n",
			width, height);
	unsigned pixels_count = stride*height;

	// generate payload on standard input
	printf("\n/* This file is automatically generated, DO NOT EDIT! */\n\n"
			"unsigned char gumboot_logo_pixels[%d] = {\n",
			pixels_count);

	// start emitting the pixel data
	unsigned i = 0;
	printf("\t\t");
	for(;i<pixels_count-1;i++) {
		printf("0x%02X,", image[i]);
		if (i && (i % stride == 0))
			printf("\n\t\t");
	}
	printf("0x%02X\n", image[pixels_count-1]);

	// finish struct
	printf("\t};\n");

	free(image);

	return 0;
}
