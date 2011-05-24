/* Graphics_image.cpp
 *
 * Copyright (C) 1992-2011 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * pb 2002/03/07 GPL
 * pb 2007/04/25 better image drawing on the Mac
 * pb 2007/08/03 Quartz
 * pb 2008/01/19 double
 * pb 2009/08/10 image from file
 * fb 2010/02/24 GTK
 * pb 2011/03/17 C++
 */

#include "GraphicsP.h"

#if mac
	#include <time.h>
	#include "sys/macport_on.h"
	#include <QDOffscreen.h>
	static RGBColor theBlackColour = { 0, 0, 0 };
	static void _mac_releaseDataCallback (void *info, const void *data, size_t size) {
		(void) info;
		(void) size;
		Melder_free (data);
	}
#endif

#define wdx(x)  ((x) * my scaleX + my deltaX)
#define wdy(y)  ((y) * my scaleY + my deltaY)

static void screenCellArrayOrImage (I, double **z_float, unsigned char **z_byte,
	long ix1, long ix2, long x1DC, long x2DC,
	long iy1, long iy2, long y1DC, long y2DC,
	double minimum, double maximum,
	long clipx1, long clipx2, long clipy1, long clipy2, int interpolate)
{
	iam (GraphicsScreen);
	/*long t=clock();*/
	long nx = ix2 - ix1 + 1;   /* The number of cells along the horizontal axis. */
	long ny = iy2 - iy1 + 1;   /* The number of cells along the vertical axis. */
	double dx = (double) (x2DC - x1DC) / (double) nx;   /* Horizontal pixels per cell. Positive. */
	double dy = (double) (y2DC - y1DC) / (double) ny;   /* Vertical pixels per cell. Negative. */
	double scale = 255.0 / (maximum - minimum), offset = 255.0 + minimum * scale;
	if (x2DC <= x1DC || y1DC <= y2DC) return;
	if (Melder_debug == 36)
		Melder_casual ("scale %f", scale);
	/* Clip by the intersection of the world window and the outline of the cells. */
	//Melder_casual ("clipy1 %ld clipy2 %ld", clipy1, clipy2);
	if (clipx1 < x1DC) clipx1 = x1DC;
	if (clipx2 > x2DC) clipx2 = x2DC;
	if (clipy1 > y1DC) clipy1 = y1DC;
	if (clipy2 < y2DC) clipy2 = y2DC;
	#if mac
		SetPort (my macPort);
	#endif
	/*
	 * The first decision is whether we are going to use the standard rectangle drawing
	 * (cellArray only), or whether we are going to write into a bitmap.
	 * The standard drawing is best for small numbers of cells,
	 * provided that some cells are larger than a pixel.
	 */
	if (! interpolate && nx * ny < 3000 && (dx > 1.0 || dy < -1.0)) {
		/*unsigned int cellWidth = (unsigned int) dx + 1;*/
		unsigned int cellHeight = (unsigned int) (- (int) dy) + 1;
		long ix, iy;
		long *lefts = NUMlvector (ix1, ix2 + 1);
		#if cairo
			cairo_pattern_t *grey[256];
			int igrey;
			for (igrey=0; igrey<sizeof(grey)/sizeof(*grey); igrey++) {
				double v = igrey / ((double)(sizeof(grey)/sizeof(*grey)) - 1.0);
				grey[igrey] = cairo_pattern_create_rgb(v, v, v);
			}
		#elif win
			int igrey;
			static HBRUSH greyBrush [256];
			RECT rect;
			if (! greyBrush [0])
				for (igrey = 0; igrey <= 255; igrey ++)
					greyBrush [igrey] = CreateSolidBrush (RGB (igrey, igrey, igrey));   /* Once. */
		#elif mac
			int igrey;
			long pixel [256];
			Rect rect;
			for (igrey = 0; igrey <= 255; igrey ++) {
				RGBColor rgb;
				rgb. red = rgb. green = rgb. blue = igrey * 256;
				/*RGBForeColor (& rgb);*/
				pixel [igrey] = Color2Index (& rgb); /*my macWindow -> fgColor;   /* Each time anew. */
			}
		#endif
		if (! lefts) return;
		for (ix = ix1; ix <= ix2 + 1; ix ++)
			lefts [ix] = x1DC + (long) ((ix - ix1) * dx);
		for (iy = iy1; iy <= iy2; iy ++) {
			long bottom = y1DC + (long) ((iy - iy1) * dy), top = bottom - cellHeight;
			if (top > clipy1 || bottom < clipy2) continue;
			if (top < clipy2) top = clipy2;
			if (bottom > clipy1) bottom = clipy1;
			#if win || mac
				rect. bottom = bottom; rect. top = top;
			#endif
			for (ix = ix1; ix <= ix2; ix ++) {
				long left = lefts [ix], right = lefts [ix + 1];
				long value = offset - scale * ( z_float ? z_float [iy] [ix] : z_byte [iy] [ix] );
				if (right < clipx1 || left > clipx2) continue;
				if (left < clipx1) left = clipx1;
				if (right > clipx2) right = clipx2;
				#if cairo
					cairo_set_source(my cr, grey[value <= 0 ? 0 : value >= sizeof(grey)/sizeof(*grey) ? sizeof(grey)/sizeof(*grey) : value]);
					cairo_rectangle(my cr, left, top, right - left, bottom - top);
					cairo_fill(my cr);
				#elif win
					rect. left = left; rect. right = right;
					FillRect (my dc, & rect, greyBrush [value <= 0 ? 0 : value >= 255 ? 255 : value]);
				#elif mac
				{
					RGBColor rgb;
					rgb. red = rgb. green = rgb. blue = (value <= 0 ? 0 : value >= 255 ? 255 : value) * 256;
					RGBForeColor (& rgb);
					/*my macWindow -> fgColor = pixel [value <= 0 ? 0 : value >= 255 ? 255 : value];*/
					rect. left = left; rect. right = right;
					PaintRect (& rect);
				}
				#endif
			}
		}
		NUMlvector_free (lefts, ix1);
		
		#if cairo
			for (igrey=0; igrey<sizeof(grey)/sizeof(*grey); igrey++)
				cairo_pattern_destroy(grey[igrey]);
			cairo_paint(my cr);
		#endif
	} else {
		long xDC, yDC;
		long undersampling = 1;
		/*
		 * Prepare for off-screen bitmap drawing.
		 */
		#if cairo
			long arrayWidth = clipx2 - clipx1;
			long arrayHeight = clipy1 - clipy2;
			if (Melder_debug == 36)
				Melder_casual ("arrayWidth %f, arrayHeight %f", (double) arrayWidth, (double) arrayHeight);
			// We're creating an alpha-only surface here (size is 1/4 compared to RGB24 and ARGB)
			// The grey values are reversed as we let the foreground colour shine through instead of blackening
			cairo_surface_t *sfc = cairo_image_surface_create (/*CAIRO_FORMAT_A8*/ CAIRO_FORMAT_RGB24, arrayWidth, arrayHeight);
			unsigned char *bits = cairo_image_surface_get_data (sfc);
			int scanLineLength = cairo_image_surface_get_stride (sfc);
			unsigned char grey [256];
			if (Melder_debug == 36)
				Melder_casual ("image surface address %ld, bits address %ld, scanLineLength %d, numberOfGreys %d", sfc, bits, scanLineLength, sizeof(grey)/sizeof(*grey));
			for (int igrey = 0; igrey < sizeof (grey) / sizeof (*grey); igrey++)
				grey [igrey] = 255 - (unsigned char) (igrey * 255.0 / (sizeof (grey) / sizeof (*grey) - 1));
		#elif win
			long bitmapWidth = clipx2 - clipx1, bitmapHeight = clipy1 - clipy2;
			int igrey;
			/*
			 * Create a device-independent bitmap, 8 pixels deep, for 256 greys.
			 */
			struct { BITMAPINFOHEADER header; RGBQUAD colours [256]; } bitmapInfo;
			long scanLineLength = (bitmapWidth + 3) & ~3L;
			HBITMAP bitmap;
			unsigned char *bits;
			bitmapInfo. header.biSize = sizeof (BITMAPINFOHEADER);
			bitmapInfo. header.biWidth = scanLineLength;
			bitmapInfo. header.biHeight = bitmapHeight;
			bitmapInfo. header.biPlanes = 1;
			bitmapInfo. header.biBitCount = 8;
			bitmapInfo. header.biCompression = 0;
			bitmapInfo. header.biSizeImage = 0;
			bitmapInfo. header.biXPelsPerMeter = 0;
			bitmapInfo. header.biYPelsPerMeter = 0;
			bitmapInfo. header.biClrUsed = 0;
			bitmapInfo. header.biClrImportant = 0;
			for (igrey = 0; igrey <= 255; igrey ++) {
				bitmapInfo. colours [igrey]. rgbRed = igrey;
				bitmapInfo. colours [igrey]. rgbGreen = igrey;
				bitmapInfo. colours [igrey]. rgbBlue = igrey;
			}
			bitmap = CreateDIBSection (my dc /* ignored */, (CONST BITMAPINFO *) & bitmapInfo,
				DIB_RGB_COLORS, (VOID **) & bits, NULL, 0);
		#elif mac
			/*
			 * QuickDraw requirements.
			 */
			CGrafPtr savePort;
			GDHandle saveDevice;
			GWorldPtr offscreenWorld;
			PixMapHandle offscreenPixMap;
			long offscreenRowBytes;
			unsigned char *offscreenPixels;
			Rect rect, destRect;
			int igrey;
			unsigned long pixel [256];
			/*
			 * Quartz requirements.
			 */
			unsigned char *imageData;
			long bytesPerRow, numberOfRows;
			bool useQuartzForImages = my useQuartz && 1;
			if (useQuartzForImages) {
				bytesPerRow = (clipx2 - clipx1) * 4;
				Melder_assert (bytesPerRow > 0);
				numberOfRows = clipy1 - clipy2;
				Melder_assert (numberOfRows > 0);
				imageData = Melder_malloc_f (unsigned char, bytesPerRow * numberOfRows);
			} else {
				GetGWorld (& savePort, & saveDevice);
				if (my resolution > 150) undersampling = 4;
				SetRect (& rect, 0, 0, (clipx2 - clipx1) / undersampling + 1, (clipy1 - clipy2) / undersampling + 1);
				if (NewGWorld (& offscreenWorld,
					32, /* We're drawing in 32 bit, and copying it to 8, 16, 24, or 32 bit. */
					& rect,
					NULL,   /* BUG: we should use a colour table with 256 shades of grey !!! */
					NULL,
					keepLocal   /* Because we're going to access the pixels directly. */
				) != noErr || offscreenWorld == NULL) {
					static int notified = FALSE;
					if (! notified) Melder_flushError ("(GraphicsScreen::cellArray:) Cannot create offscreen graphics.");
					notified = TRUE;
					goto end;
				}
				SetGWorld (offscreenWorld, /* ignored: */ NULL);
				offscreenPixMap = GetGWorldPixMap (offscreenWorld);
				if (! LockPixels (offscreenPixMap)) {
					static int notified = FALSE;
					if (! notified) Melder_flushError ("(GraphicsScreen::cellArray:) Cannot lock offscreen pixels.");
					notified = TRUE;
					SetGWorld (savePort, saveDevice);
					goto cleanUp;
				}
				offscreenRowBytes = (*offscreenPixMap) -> rowBytes & 0x3FFF;
				//Melder_fatal ("%ld %ld %ld",clipx1,clipx2,offscreenRowBytes);   // 45 393 352
				offscreenPixels = (unsigned char *) GetPixBaseAddr (offscreenPixMap);
				EraseRect (& rect);
				for (igrey = 0; igrey <= 255; igrey ++) {
					RGBColor rgb;
					rgb. red = rgb. green = rgb. blue = igrey * 256;
					/*RGBForeColor (& rgb);*/
					pixel [igrey] = Color2Index (& rgb) /*offscreenWorld -> fgColor*/;
				}
			}
		#endif
		/*
		 * Draw into the bitmap.
		 */
		#if cairo
			#define ROW_START_ADDRESS  (bits + (clipy1 - 1 - yDC) * scanLineLength)
			//#define PUT_PIXEL *pixelAddress ++ = grey [value <= 0 ? 0 : value >= 255 ? 255 : (int) value];
			#define PUT_PIXEL \
				if (1) { \
					unsigned char kar = value <= 0 ? 0 : value >= 255 ? 255 : (int) value; \
					*pixelAddress ++ = kar; \
					*pixelAddress ++ = kar; \
					*pixelAddress ++ = kar; \
					*pixelAddress ++ = 0; \
				}
		#elif win
			#define ROW_START_ADDRESS  (bits + (clipy1 - 1 - yDC) * scanLineLength)
			#define PUT_PIXEL  *pixelAddress ++ = value <= 0 ? 0 : value >= 255 ? 255 : (int) value;
		#elif mac
			#define ROW_START_ADDRESS \
				(useQuartzForImages ? (imageData + (clipy1 - 1 - yDC) * bytesPerRow) : \
				(offscreenPixels + (yDC - clipy2) * offscreenRowBytes / undersampling))
			#define PUT_PIXEL \
				if (useQuartzForImages) { \
					unsigned char kar = value <= 0 ? 0 : value >= 255 ? 255 : (int) value; \
					*pixelAddress ++ = kar; \
					*pixelAddress ++ = kar; \
					*pixelAddress ++ = kar; \
					*pixelAddress ++ = 0; \
				} else { \
					unsigned long pixelValue = pixel [value <= 0 ? 0 : value >= 255 ? 255 : (int) value]; \
					*pixelAddress ++ = pixelValue >> 24; \
					*pixelAddress ++ = (pixelValue & 0xff0000) >> 16; \
					*pixelAddress ++ = (pixelValue & 0xff00) >> 8; \
					*pixelAddress ++ = pixelValue & 0xff; \
				}
		#endif
		if (interpolate) {
			long *ileft = NUMlvector (clipx1, clipx2), *iright = NUMlvector (clipx1, clipx2);
			double *rightWeight = NUMdvector (clipx1, clipx2), *leftWeight = NUMdvector (clipx1, clipx2);
			if (! ileft || ! iright || ! rightWeight || ! leftWeight) goto ready1;
			for (xDC = clipx1; xDC < clipx2; xDC += undersampling) {
				double ix_real = ix1 - 0.5 + ((double) nx * (xDC - x1DC)) / (x2DC - x1DC);
				ileft [xDC] = floor (ix_real), iright [xDC] = ileft [xDC] + 1;
				rightWeight [xDC] = ix_real - ileft [xDC], leftWeight [xDC] = 1.0 - rightWeight [xDC];
				if (ileft [xDC] < ix1) ileft [xDC] = ix1;
				if (iright [xDC] > ix2) iright [xDC] = ix2;
			}
			for (yDC = clipy2; yDC < clipy1; yDC += undersampling) {
				double iy_real = iy2 + 0.5 - ((double) ny * (yDC - y2DC)) / (y1DC - y2DC);
				long itop = ceil (iy_real), ibottom = itop - 1;
				double bottomWeight = itop - iy_real, topWeight = 1.0 - bottomWeight;
				unsigned char *pixelAddress = ROW_START_ADDRESS;
				if (itop > iy2) itop = iy2;
				if (ibottom < iy1) ibottom = iy1;
				if (z_float) {
					double *ztop = z_float [itop], *zbottom = z_float [ibottom];
					for (xDC = clipx1; xDC < clipx2; xDC += undersampling) {
						double interpol =
							rightWeight [xDC] *
								(topWeight * ztop [iright [xDC]] + bottomWeight * zbottom [iright [xDC]]) +
							leftWeight [xDC] *
								(topWeight * ztop [ileft [xDC]] + bottomWeight * zbottom [ileft [xDC]]);
						double value = offset - scale * interpol;
						PUT_PIXEL
					}
				} else {
					unsigned char *ztop = z_byte [itop], *zbottom = z_byte [ibottom];
					for (xDC = clipx1; xDC < clipx2; xDC += undersampling) {
						double interpol =
							rightWeight [xDC] *
								(topWeight * ztop [iright [xDC]] + bottomWeight * zbottom [iright [xDC]]) +
							leftWeight [xDC] *
								(topWeight * ztop [ileft [xDC]] + bottomWeight * zbottom [ileft [xDC]]);
						double value = offset - scale * interpol;
						PUT_PIXEL
					}
				}
			}
			ready1:
			NUMlvector_free (ileft, clipx1);
			NUMlvector_free (iright, clipx1);
			NUMdvector_free (rightWeight, clipx1);
			NUMdvector_free (leftWeight, clipx1);
		} else {
			long *ix = NUMlvector (clipx1, clipx2);
			if (! ix) goto ready2;
			for (xDC = clipx1; xDC < clipx2; xDC += undersampling)
				ix [xDC] = floor (ix1 + (nx * (xDC - x1DC)) / (x2DC - x1DC));
			for (yDC = clipy2; yDC < clipy1; yDC += undersampling) {
				long iy = ceil (iy2 - (ny * (yDC - y2DC)) / (y1DC - y2DC));
				unsigned char *pixelAddress = ROW_START_ADDRESS;
				Melder_assert (iy >= iy1 && iy <= iy2);
				if (z_float) {
					double *ziy = z_float [iy];
					for (xDC = clipx1; xDC < clipx2; xDC += undersampling) {
						double value = offset - scale * ziy [ix [xDC]];
						PUT_PIXEL
					}
				} else {
					unsigned char *ziy = z_byte [iy];
					for (xDC = clipx1; xDC < clipx2; xDC += undersampling) {
						double value = offset - scale * ziy [ix [xDC]];
						PUT_PIXEL
					}
				}
			}
			ready2:
			NUMlvector_free (ix, clipx1);
		}
		/*
		 * Copy the bitmap to the screen.
		 */
		#if cairo
			cairo_matrix_t clip_trans;
			cairo_matrix_init_identity (& clip_trans);
			cairo_matrix_scale (& clip_trans, 1, -1);		// we painted in the reverse y-direction
			cairo_matrix_translate (& clip_trans, - clipx1, - clipy1);
			cairo_pattern_t *bitmap_pattern = cairo_pattern_create_for_surface (sfc);
			if (Melder_debug == 36) Melder_casual ("bitmap pattern %ld", bitmap_pattern);
			if (cairo_status_t status = cairo_pattern_status (bitmap_pattern)) {
				Melder_casual ("bitmap pattern status: %s", cairo_status_to_string (status));
			} else {
				cairo_pattern_set_matrix (bitmap_pattern, & clip_trans);
				cairo_save (my cr);
				cairo_set_source (my cr, bitmap_pattern);
				cairo_paint (my cr);
				cairo_restore (my cr);
			}
			cairo_pattern_destroy (bitmap_pattern);
		#elif win
			SetDIBitsToDevice (my dc, clipx1, clipy2, bitmapWidth, bitmapHeight, 0, 0, 0, bitmapHeight,
				bits, (CONST BITMAPINFO *) & bitmapInfo, DIB_RGB_COLORS);
		#elif mac
			if (useQuartzForImages) {
				CGImageRef image;
				CGColorSpaceRef colourSpace = CGColorSpaceCreateWithName (kCGColorSpaceGenericRGB);   // used to be kCGColorSpaceUserRGB
				Melder_assert (colourSpace != NULL);
				if (1) {
					CGDataProviderRef dataProvider = CGDataProviderCreateWithData (NULL,
						imageData,
						bytesPerRow * numberOfRows,
						_mac_releaseDataCallback   // we need this because we cannot release the image data immediately after drawing,
							// because in PDF files the imageData has to stay available through EndPage
					);
					Melder_assert (dataProvider != NULL);
					image = CGImageCreate (clipx2 - clipx1, numberOfRows,
						8, 32, bytesPerRow, colourSpace, kCGImageAlphaNone, dataProvider, NULL, false, kCGRenderingIntentDefault);
					CGDataProviderRelease (dataProvider);
				} else if (0) {
					Melder_assert (CGBitmapContextCreate != NULL);
					CGContextRef bitmaptest = CGBitmapContextCreate (imageData, 100, 100,
						8, 800, colourSpace, 0);
					Melder_assert (bitmaptest != NULL);
					CGContextRef bitmap = CGBitmapContextCreate (NULL/*imageData*/, clipx2 - clipx1, numberOfRows,
						8, bytesPerRow, colourSpace, kCGImageAlphaLast);
					Melder_assert (bitmap != NULL);
					image = CGBitmapContextCreateImage (bitmap);
					// release bitmap?
				}
				Melder_assert (image != NULL);
				GraphicsQuartz_initDraw (me);
				CGContextDrawImage (my macGraphicsContext, CGRectMake (clipx1, clipy2, clipx2 - clipx1, clipy1 - clipy2), image);
				GraphicsQuartz_exitDraw (me);
				CGColorSpaceRelease (colourSpace);
				CGImageRelease (image);
			} else {
				SetGWorld (savePort, saveDevice);
				/*
				 * Before calling CopyBits, make sure that the foreground colour is black.
				 */
				RGBForeColor (& theBlackColour);
				SetRect (& rect, 0, 0, (clipx2 - clipx1) / undersampling, (clipy1 - clipy2) / undersampling);
				SetRect (& destRect, clipx1, clipy2, clipx2, clipy1);
				/*
				 * According to IM VI:21-19, the first argument to CopyBits should be the PixMap returned from GetGWorldPixMap.
				 * However, the example in Imaging:6-6 violates this, and dereferences the handle directly...
				 */
				CopyBits ((struct BitMap *) *offscreenPixMap,
					(const struct BitMap *) * GetPortPixMap ((CGrafPtr) my macPort),   /* BUG for 1-bit eps preview */
					& rect, & destRect, srcCopy, NULL);
			}
		#endif
		/*
		 * Clean up.
		 */
		#if cairo
			cairo_surface_destroy (sfc);
		#elif win
			DeleteBitmap (bitmap);
		#elif mac
			cleanUp:
			if (useQuartzForImages) {
				//Melder_free (imageData);
			} else {
				UnlockPixels (offscreenPixMap);
				DisposeGWorld (offscreenWorld);
			}
		#endif
	}
	#if win
		end:
		return;
	#elif mac
		end:
		if (my macPort != NULL) {
			RGBForeColor (& theBlackColour);
			SetPort (my macPort);
			RGBForeColor (& theBlackColour);
		}
	#endif
	/*Melder_information2("duration ",Melder_integer(clock()-t));*/
}

#define INTERPOLATE_IN_POSTSCRIPT  TRUE

static void _cellArrayOrImage (I, double **z_float, unsigned char **z_byte,
	long ix1, long ix2, long x1DC, long x2DC,
	long iy1, long iy2, long y1DC, long y2DC, double minimum, double maximum,
	long clipx1, long clipx2, long clipy1, long clipy2, int interpolate)
{
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		#if mac
			if (my drawingArea) GuiMac_clipOn (my drawingArea);
		#endif
		screenCellArrayOrImage (me, z_float, z_byte, ix1, ix2, x1DC, x2DC, iy1, iy2, y1DC, y2DC,
			minimum, maximum, clipx1, clipx2, clipy1, clipy2, interpolate);
		#if mac
			if (my drawingArea) GuiMac_clipOff ();
		#endif
	} else if (my postScript) {
		iam (GraphicsPostscript);
		long interpolateX = 1, interpolateY = 1;
		long ix, iy, nx = ix2 - ix1 + 1, ny = iy2 - iy1 + 1, filling = 0;
		double scale = ( my photocopyable ? 200.1f : 255.1f ) / (maximum - minimum);
		double offset = 255.1f + minimum * scale;
		int minimalGrey = my photocopyable ? 55 : 0;
		my printf (my file, "gsave N %ld %ld M %ld %ld L %ld %ld L %ld %ld L closepath clip\n",
			clipx1, clipy1, clipx2 - clipx1, 0L, 0L, clipy2 - clipy1, clipx1 - clipx2, 0L);
		my printf (my file, "%ld %ld translate %ld %ld scale\n",
			x1DC, y1DC, x2DC - x1DC, y2DC - y1DC);
		if (interpolate) {
			/* Base largest spot size on 106 dpi. */
			#define LARGEST_SPOT_MM  0.24
			double colSize_mm = (double) (x2DC - x1DC) / nx * 25.4 / my resolution;
			double rowSize_mm = (double) (y2DC - y1DC) / ny * 25.4 / my resolution;
			#if INTERPOLATE_IN_POSTSCRIPT
			interpolateX = ceil (colSize_mm / LARGEST_SPOT_MM);
			interpolateY = ceil (rowSize_mm / LARGEST_SPOT_MM);
			#else
			long xDC, yDC;
			long *ileft, *iright;
			double *rightWeight, *leftWeight;
			if (x2DC <= x1DC || y2DC <= y1DC) return;   /* Different from the screen test! */
			/* Clip by the intersection of the world window and the outline of the cells. */
			if (clipx1 < x1DC) clipx1 = x1DC;
			if (clipx2 > x2DC) clipx2 = x2DC;
			if (clipy1 < y1DC) clipy1 = y1DC;   /* Different from the screen version! */
			if (clipy2 > y2DC) clipy2 = y2DC;
			ileft = NUMlvector (clipx1, clipx2);
			iright = NUMlvector (clipx1, clipx2);
			rightWeight = NUMdvector (clipx1, clipx2);
			leftWeight = NUMdvector (clipx1, clipx2);
			if (! ileft || ! iright || ! rightWeight || ! leftWeight) goto ready1;
			/* Allow extra interpolation for PDF. */
			my printf (my file, "/DeviceGray setcolorspace << /ImageType 1 /Width %ld /Height %ld\n"
				"/BitsPerComponent 8 /Decode [0 1] /ImageMatrix [%ld 0 0 %ld 0 0]\n"
				"/DataSource currentfile /ASCIIHexDecode filter /Interpolate true >> image\n",
				clipx2 - clipx1, clipy2 - clipy1, clipx2 - clipx1, clipy2 - clipy1);
			for (xDC = clipx1; xDC < clipx2; xDC ++) {
				double ix_real = ix1 - 0.5 + ((double) nx * (xDC - x1DC)) / (x2DC - x1DC);
				ileft [xDC] = floor (ix_real), iright [xDC] = ileft [xDC] + 1;
				rightWeight [xDC] = ix_real - ileft [xDC], leftWeight [xDC] = 1.0 - rightWeight [xDC];
				if (ileft [xDC] < ix1) ileft [xDC] = ix1;
				if (iright [xDC] > ix2) iright [xDC] = ix2;
			}
			for (yDC = clipy1; yDC < clipy2; yDC ++) {
				double iy_real = iy1 - 0.5 + ((double) ny * (yDC - y1DC)) / (y2DC - y1DC);
				long ibottom = floor (iy_real), itop = ibottom + 1;
				double topWeight = iy_real - ibottom, bottomWeight = 1.0 - topWeight;
				if (ibottom < iy1) ibottom = iy1;
				if (itop > iy2) itop = iy2;
				if (z_float) {
					double *zbottom = z_float [ibottom], *ztop = z_float [itop];
					for (xDC = clipx1; xDC < clipx2; xDC ++) {
						double interpol =
							rightWeight [xDC] *
								(bottomWeight * zbottom [iright [xDC]] + topWeight * ztop [iright [xDC]]) +
							leftWeight [xDC] *
								(bottomWeight * zbottom [ileft [xDC]] + topWeight * ztop [ileft [xDC]]);
						short value = offset - scale * interpol;
						my printf (my file, "%.2x", value <= minimalGrey ? minimalGrey : value >= 255 ? 255 : value);
						if (++ filling == 39) { my printf (my file, "\n"); filling = 0; }
					}
				} else {
					unsigned char *zbottom = z_byte [ibottom], *ztop = z_byte [itop];
					for (xDC = clipx1; xDC < clipx2; xDC ++) {
						double interpol =
							rightWeight [xDC] *
								(bottomWeight * zbottom [iright [xDC]] + topWeight * ztop [iright [xDC]]) +
							leftWeight [xDC] *
								(bottomWeight * zbottom [ileft [xDC]] + topWeight * ztop [ileft [xDC]]);
						short value = offset - scale * interpol;
						my printf (my file, "%.2x", value <= minimalGrey ? minimalGrey : value >= 255 ? 255 : value);
						if (++ filling == 39) { my printf (my file, "\n"); filling = 0; }
					}
				}
			}
			ready1:
			NUMlvector_free (ileft, clipx1);
			NUMlvector_free (iright, clipx1);
			NUMdvector_free (rightWeight, clipx1);
			NUMdvector_free (leftWeight, clipx1);
			#endif
		}
		#if ! INTERPOLATE_IN_POSTSCRIPT
		else
			/* Do not interpolate. */
			my printf (my file, "/picstr %ld string def %ld %ld 8 [%ld 0 0 %ld 0 0]\n"
				"{ currentfile picstr readhexstring pop } image\n",
				nx, nx, ny, nx, ny);
		#endif

		#if INTERPOLATE_IN_POSTSCRIPT
		if (interpolateX <= 1 && interpolateY <= 1) {
			/* Do not interpolate. */
			my printf (my file, "/picstr %ld string def %ld %ld 8 [%ld 0 0 %ld 0 0]\n"
				"{ currentfile picstr readhexstring pop } image\n",
				nx, nx, ny, nx, ny);
		} else if (interpolateX > 1 && interpolateY > 1) {
			/* Interpolate both horizontally and vertically. */
			long nx_new = nx * interpolateX;
			long ny_new = ny * interpolateY;
			/* Interpolation between rows requires us to remember two original rows: */
			my printf (my file, "/lorow %ld string def /hirow %ld string def\n", nx, nx);
			/* New rows (scanlines) are longer: */
			my printf (my file, "/scanline %ld string def\n", nx_new);
			/* The first four arguments to the 'image' command,
			/* namely the new number of columns, the new number of rows, the bit depth, and the matrix: */
			my printf (my file, "%ld %ld 8 [%ld 0 0 %ld 0 0]\n", nx_new, ny_new, nx_new, ny_new);
			/* Since our imageproc is going to output only one scanline at a time, */
			/* the outer loop variable (scanline number) has to be initialized outside the imageproc: */
			my printf (my file, "/irow 0 def\n");
			/* The imageproc starts here. First, we fill one or two original rows if necessary; */
			/* they are read as hexadecimal strings from the current file, i.e. just after the image command. */
			my printf (my file, "{\n"
				/* First test: are we in the first scanline? If so, read two original rows: */
				"irow 0 eq { currentfile lorow readhexstring pop pop lorow hirow copy pop } if\n"
				/* Second test: did we just pass an original data row? */
				/* If so, */
				/*    (1) move that row backwards; */
				/*    (2) read a new one unless we just passed the last original row: */
				"irow %ld mod %ld eq { hirow lorow copy pop\n"
				"irow %ld ne { currentfile hirow readhexstring pop pop } if } if\n",
				interpolateY, interpolateY / 2, ny_new - interpolateY + interpolateY / 2);
			/* Where are we between those two rows? */
			my printf (my file, "/rowphase irow %ld add %ld mod %ld div def\n",
				interpolateY - interpolateY / 2, interpolateY, interpolateY);
			/* Inner loop starts here. It cycles through all new columns: */
			my printf (my file, "0 1 %ld {\n", nx_new - 1);
			/* Get the inner loop variable: */
			my printf (my file, "   /icol exch def\n");
			/* Where are the two original columns? */
			my printf (my file, "   /locol icol %ld sub %ld idiv def\n", interpolateX / 2, interpolateX);
			my printf (my file, "   /hicol icol %ld ge { %ld } { icol %ld add %ld idiv } ifelse def\n",
				nx_new - interpolateX / 2, nx - 1, interpolateX / 2, interpolateX);
			/* Where are we between those two columns? */
			my printf (my file, "   /colphase icol %ld add %ld mod %ld div def\n",
				interpolateX - interpolateX / 2, interpolateX, interpolateX);
			/* Four-point interpolation: */
			my printf (my file,
				"   /plow lorow locol get def\n"
				"   /phigh lorow hicol get def\n"
				"   /qlow hirow locol get def\n"
				"   /qhigh hirow hicol get def\n"
				"   /value\n"
				"      plow phigh plow sub colphase mul add 1 rowphase sub mul\n"
				"      qlow qhigh qlow sub colphase mul add rowphase mul\n"
				"      add def\n"
				"   scanline icol value 0 le { 0 } { value 255 ge { 255 } { value } ifelse } ifelse cvi put\n"
				"} for\n"
				"/irow irow 1 add def scanline } image\n");
		} else if (interpolateX > 1) {
			/* Interpolate horizontally only. */
			long nx_new = nx * interpolateX;
			/* Remember one original row: */
			my printf (my file, "/row %ld string def\n", nx, nx);
			/* New rows (scanlines) are longer: */
			my printf (my file, "/scanline %ld string def\n", nx_new);
			/* The first four arguments to the 'image' command,
			/* namely the new number of columns, the number of rows, the bit depth, and the matrix: */
			my printf (my file, "%ld %ld 8 [%ld 0 0 %ld 0 0]\n", nx_new, ny, nx_new, ny);
			/* The imageproc starts here. We fill one original row. */
			my printf (my file, "{\n"
				"currentfile row readhexstring pop pop\n");
			/* Loop starts here. It cycles through all new columns: */
			my printf (my file, "0 1 %ld {\n", nx_new - 1);
			/* Get the loop variable: */
			my printf (my file, "   /icol exch def\n");
			/* Where are the two original columns? */
			my printf (my file, "   /locol icol %ld sub %ld idiv def\n", interpolateX / 2, interpolateX);
			my printf (my file, "   /hicol icol %ld ge { %ld } { icol %ld add %ld idiv } ifelse def\n",
				nx_new - interpolateX / 2, nx - 1, interpolateX / 2, interpolateX);
			/* Where are we between those two columns? */
			my printf (my file, "   /colphase icol %ld add %ld mod %ld div def\n",
				interpolateX - interpolateX / 2, interpolateX, interpolateX);
			/* Two-point interpolation: */
			my printf (my file,
				"   /plow row locol get def\n"
				"   /phigh row hicol get def\n"
				"   /value plow phigh plow sub colphase mul add def\n"
				"   scanline icol value 0 le { 0 } { value 255 ge { 255 } { value } ifelse } ifelse cvi put\n"
				"} for\n"
				"scanline } image\n");
		} else {
			/* Interpolate vertically only. */
			long ny_new = ny * interpolateY;
			/* Interpolation between rows requires us to remember two original rows: */
			my printf (my file, "/lorow %ld string def /hirow %ld string def\n", nx, nx);
			/* New rows (scanlines) are equally long: */
			my printf (my file, "/scanline %ld string def\n", nx);
			/* The first four arguments to the 'image' command,
			/* namely the number of columns, the new number of rows, the bit depth, and the matrix: */
			my printf (my file, "%ld %ld 8 [%ld 0 0 %ld 0 0]\n", nx, ny_new, nx, ny_new);
			/* Since our imageproc is going to output only one scanline at a time, */
			/* the outer loop variable (scanline number) has to be initialized outside the imageproc: */
			my printf (my file, "/irow 0 def\n");
			/* The imageproc starts here. First, we fill one or two original rows if necessary; */
			/* they are read as hexadecimal strings from the current file, i.e. just after the image command. */
			my printf (my file, "{\n"
				/* First test: are we in the first scanline? If so, read two original rows: */
				"irow 0 eq { currentfile lorow readhexstring pop pop lorow hirow copy pop } if\n"
				/* Second test: did we just pass an original data row? */
				/* If so, */
				/*    (1) move that row backwards; */
				/*    (2) read a new one unless we just passed the last original row: */
				"irow %ld mod %ld eq { hirow lorow copy pop\n"
				"irow %ld ne { currentfile hirow readhexstring pop pop } if } if\n",
				interpolateY, interpolateY / 2, ny_new - interpolateY + interpolateY / 2);
			/* Where are we between those two rows? */
			my printf (my file, "/rowphase irow %ld add %ld mod %ld div def\n",
				interpolateY - interpolateY / 2, interpolateY, interpolateY);
			/* Inner loop starts here. It cycles through all columns: */
			my printf (my file, "0 1 %ld {\n", nx - 1);
			/* Get the inner loop variable: */
			my printf (my file, "   /icol exch def\n");
			/* Two-point interpolation: */
			my printf (my file,
				"   /p lorow icol get def\n"
				"   /q hirow icol get def\n"
				"   /value\n"
				"      p 1 rowphase sub mul\n"
				"      q rowphase mul\n"
				"      add def\n"
				"   scanline icol value 0 le { 0 } { value 255 ge { 255 } { value } ifelse } ifelse cvi put\n"
				"} for\n"
				"/irow irow 1 add def scanline } image\n");
		}
		for (iy = iy1; iy <= iy2; iy ++) for (ix = ix1; ix <= ix2; ix ++) {
			int value = (int) (offset - scale * ( z_float ? z_float [iy] [ix] : z_byte [iy] [ix] ));
			my printf (my file, "%.2x", value <= minimalGrey ? minimalGrey : value >= 255 ? 255 : value);
			if (++ filling == 39) { my printf (my file, "\n"); filling = 0; }
		}
		#endif
		if (filling) my printf (my file, "\n");
		/*if (interpolate && my languageLevel > 1)
			my printf (my file, ">\n");*/
		my printf (my file, "grestore\n");
	}
	_Graphics_setColour (me, my colour);
}

static void cellArrayOrImage (I, double **z_float, unsigned char **z_byte,
	long ix1, long ix2, double x1WC, double x2WC,
	long iy1, long iy2, double y1WC, double y2WC,
	double minimum, double maximum, int interpolate)
{
	iam (Graphics);
	if (ix2 < ix1 || iy2 < iy1 || minimum == maximum) return;
	_cellArrayOrImage (me, z_float, z_byte,
		ix1, ix2, wdx (x1WC), wdx (x2WC),
		iy1, iy2, wdy (y1WC), wdy (y2WC), minimum, maximum,
		wdx (my x1WC), wdx (my x2WC), wdy (my y1WC), wdy (my y2WC), interpolate);
	if (my recording) {
		long nrow = iy2 - iy1 + 1, ncol = ix2 - ix1 + 1, ix, iy;
		op (interpolate ? ( z_float ? IMAGE : IMAGE8 ) :
			 (z_float ? CELL_ARRAY : CELL_ARRAY8 ), 8 + nrow * ncol);
		put (x1WC); put (x2WC); put (y1WC); put (y2WC); put (minimum); put (maximum);
		put (nrow); put (ncol);
		if (z_float) for (iy = iy1; iy <= iy2; iy ++)
			{ double *row = z_float [iy]; for (ix = ix1; ix <= ix2; ix ++) put (row [ix]); }
		else for (iy = iy1; iy <= iy2; iy ++)
			{ unsigned char *row = z_byte [iy]; for (ix = ix1; ix <= ix2; ix ++) put (row [ix]); }
	}
}

void Graphics_cellArray (I, double **z, long ix1, long ix2, double x1WC, double x2WC,
	long iy1, long iy2, double y1WC, double y2WC, double minimum, double maximum)
{ cellArrayOrImage (void_me, z, NULL, ix1, ix2, x1WC, x2WC, iy1, iy2, y1WC, y2WC, minimum, maximum, FALSE); }

void Graphics_cellArray8 (I, unsigned char **z, long ix1, long ix2, double x1WC, double x2WC,
	long iy1, long iy2, double y1WC, double y2WC, unsigned char minimum, unsigned char maximum)
{ cellArrayOrImage (void_me, NULL, z, ix1, ix2, x1WC, x2WC, iy1, iy2, y1WC, y2WC, minimum, maximum, FALSE); }

void Graphics_image (I, double **z, long ix1, long ix2, double x1WC, double x2WC,
	long iy1, long iy2, double y1WC, double y2WC, double minimum, double maximum)
{ cellArrayOrImage (void_me, z, NULL, ix1, ix2, x1WC, x2WC, iy1, iy2, y1WC, y2WC, minimum, maximum, TRUE); }

void Graphics_image8 (I, unsigned char **z, long ix1, long ix2, double x1WC, double x2WC,
	long iy1, long iy2, double y1WC, double y2WC, unsigned char minimum, unsigned char maximum)
{ cellArrayOrImage (void_me, NULL, z, ix1, ix2, x1WC, x2WC, iy1, iy2, y1WC, y2WC, minimum, maximum, TRUE); }

void Graphics_imageFromFile (I, const wchar_t *relativeFileName, double x1, double x2, double y1, double y2) {
	iam (Graphics);
	long x1DC = wdx (x1), x2DC = wdx (x2), y1DC = wdy (y1), y2DC = wdy (y2);
	long width = x2DC - x1DC, height = my yIsZeroAtTheTop ? y1DC - y2DC : y2DC - y1DC;
	if (my screen) {
		iam (GraphicsScreen);
		#if mac
			if (my useQuartz) {
				structMelderFile file;
				if (Melder_relativePathToFile (relativeFileName, & file)) {
					char utf8 [500];
					Melder_wcsTo8bitFileRepresentation_inline (file. path, utf8);
					CFStringRef path = CFStringCreateWithCString (NULL, utf8, kCFStringEncodingUTF8);
					CFURLRef url = CFURLCreateWithFileSystemPath (NULL, path, kCFURLPOSIXPathStyle, false);
					CFRelease (path);
					CGImageSourceRef imageSource = CGImageSourceCreateWithURL (url, NULL);
					//CGDataProviderRef dataProvider = CGDataProviderCreateWithURL (url);
					CFRelease (url);
					if (imageSource != NULL) {
					//if (dataProvider != NULL) {
						CGImageRef image = CGImageSourceCreateImageAtIndex (imageSource, 0, NULL);
						//CGImageRef image = CGImageCreateWithJPEGDataProvider (dataProvider, NULL, true, kCGRenderingIntentDefault);
						CFRelease (imageSource);
						//CGDataProviderRelease (dataProvider);
						if (image != NULL) {
							if (x1 == x2 && y1 == y2) {
								width = CGImageGetWidth (image), x1DC -= width / 2, x2DC = x1DC + width;
								height = CGImageGetHeight (image), y2DC -= height / 2, y1DC = y2DC + height;
							} else if (x1 == x2) {
								width = height * (double) CGImageGetWidth (image) / (double) CGImageGetHeight (image);
								x1DC -= width / 2, x2DC = x1DC + width;
							} else if (y1 == y2) {
								height = width * (double) CGImageGetHeight (image) / (double) CGImageGetWidth (image);
								y2DC -= height / 2, y1DC = y2DC + height;
							}
							GraphicsQuartz_initDraw (me);
							CGContextSaveGState (my macGraphicsContext);
							CGContextTranslateCTM (my macGraphicsContext, 0, y1DC);
							CGContextScaleCTM (my macGraphicsContext, 1.0, -1.0);
							CGContextDrawImage (my macGraphicsContext, CGRectMake (x1DC, 0, width, height), image);
							CGContextRestoreGState (my macGraphicsContext);
							GraphicsQuartz_exitDraw (me);
							CGImageRelease (image);
						}
					}
				}
			}
		#endif
	}
	if (my recording) {
		char *txt_utf8 = Melder_peekWcsToUtf8 (relativeFileName);
		int length = strlen (txt_utf8) / sizeof (double) + 1;
		op (IMAGE_FROM_FILE, 5 + length); put (x1); put (x2); put (y1); put (y2); sput (txt_utf8, length)
	}
}

/* End of file Graphics_image.cpp */
