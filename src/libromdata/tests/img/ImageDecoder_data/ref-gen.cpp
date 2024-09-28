#include <QImage>
#include <cassert>
#include <cstdlib>

/**
 * Function to generate the ImageDecoderTest reference image.
 * @param components Number of color components.
 * @param path Pathname.
 * @return 0 on success; non-zero on error.
 */
static int gen_ref_image(int components, const QString &path)
{
	assert(components >= 1);
	assert(components <= 4);
	if (components < 1 || components > 4)
		return EXIT_FAILURE;;

	QImage img(256, 256, QImage::Format_ARGB32);
	// Top Left: R
	// Top Right: G
	// Bottom Left: B
	// Bottom Right: W (+A in lower right quadrant only)
	for (int y = 0; y < 256; y++) {
		for (int x = 0; x < 256; x++) {
			int r = (int)((((double)(255-x) / 255.0) * ((double)(255-y) / 255.0)) * 255);
			int g = (components >= 2 ? (int)((((double)(    x) / 255.0) * ((double)(255-y) / 255.0)) * 255) : 0);
			int b = (components >= 3 ? (int)((((double)(255-x) / 255.0) * ((double)(    y) / 255.0)) * 255) : 0);

			int a;
			if (components < 4 || x < 128 || y < 128) {
				a = 255;
			} else {
				int ax = x-128;
				int ay = y-128;
				a = 255 - ((int)((((double)(ax) / 128.0) * ((double)(ay) / 128.0)) * 128));
			}
			img.setPixel(x, y, qRgba(r, g, b, a));
		}
	}
	img.save(path);
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("Syntax: %s components filename\n", argv[0]);
		return EXIT_FAILURE;
	}

	int components = strtol(argv[1], NULL, 10);
	if (components == 0) {
		printf("Invalid component count: %s\n", argv[1]);
		return EXIT_FAILURE;
	}

	return gen_ref_image(components, QString::fromUtf8(argv[2]));
}
