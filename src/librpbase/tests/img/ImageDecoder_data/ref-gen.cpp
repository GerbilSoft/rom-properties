#include <QImage>

/**
 * Function to generate the ImageDecoderTest reference image.
 * @param path Pathname.
 * @param alpha If true, use alpha for the lower-right quadrant.
 */
int gen_ref_image(const QString &path, bool alpha)
{
	QImage img(256, 256, QImage::Format_ARGB32);
	// Top Left: R
	// Top Right: G
	// Bottom Left: B
	// Bottom Right: W (+A in lower right quadrant only)
	for (int y = 0; y < 256; y++) {
		for (int x = 0; x < 256; x++) {
			int r = (int)((((double)(255-x) / 255.0) * ((double)(255-y) / 255.0)) * 255);
			int g = (int)((((double)(    x) / 255.0) * ((double)(255-y) / 255.0)) * 255);
			int b = (int)((((double)(255-x) / 255.0) * ((double)(    y) / 255.0)) * 255);

			int a;
			if (!alpha || x < 128 || y < 128) {
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
}
