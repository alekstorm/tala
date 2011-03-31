/* Graphics_linesAndAreas.cpp
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
 * pb 2004/01/13 the same high-resolution line width formula for all platforms
 * pb 2004/01/29 removed path size restriction for polygons
 * pb 2004/02/11 ORDER_DC: ellipses and rectangles handle reversed axes better
 * pb 2005/07/31 better arrowheads
 * pb 2007/01/06 made ORDER_DC compatible with PostScript
 * pb 2007/03/14 arrowSize
 * pb 2007/08/01 reintroduced yIsZeroAtTheTop
 * pb 2008/01/19 double
 * sdk 2008/03/24 cairo
 * pb 2009/07/10 fillArea Quartz
 * pb 2009/07/27 quartz
 * pb 2009/09/04 FUNCTIONS_ARE_CLIPPED (compiler flag)
 * fb 2010/02/24 cairo
 * fb 2010/03/01 cairo
 * pb 2010/05/13 support for XOR mode via GDK
 * pb 2011/03/17 C++
 */

#include "GraphicsP.h"

/* Normally on, because e.g. the intensity contour in the Sound window should not run through the play buttons: */
#define FUNCTIONS_ARE_CLIPPED  1

#define POSTSCRIPT_MAXPATH  1000
#define LINE_WIDTH_IN_PIXELS(me)  ( my resolution > 192 ? my lineWidth * (my resolution / 192.0) : my lineWidth )
#define ORDER_DC  { long temp; if (x1DC > x2DC) temp = x1DC, x1DC = x2DC, x2DC = temp; \
	if (my yIsZeroAtTheTop == (y2DC > y1DC)) temp = y1DC, y1DC = y2DC, y2DC = temp; }

static void psPrepareLine (GraphicsPostscript me) {
	double lineWidth_pixels = LINE_WIDTH_IN_PIXELS (me);
	if (my lineType == Graphics_DOTTED)
		my printf (my file, "[%ld %ld] 0 setdash\n", (long) (my resolution / 100), (long) (my resolution / 75 + lineWidth_pixels));
	else if (my lineType == Graphics_DASHED)
		my printf (my file, "[%ld %ld] 0 setdash\n", (long) (my resolution / 25), (long) (my resolution / 50 + lineWidth_pixels));
	if (my lineWidth != 1.0)
		my printf (my file, "%g setlinewidth\n", lineWidth_pixels);
}
static void psRevertLine (GraphicsPostscript me) {
	if (my lineType != Graphics_DRAWN)
		my printf (my file, "[] 0 setdash\n");
	if (my lineWidth != 1.0)
		my printf (my file, "%g setlinewidth\n", my resolution > 192 ? my resolution / 192.0 : 1.0);   /* 0.375 point */
}

#if cairo
	static void gdkPrepareLine (GraphicsScreen me) {
		gdk_gc_set_line_attributes (my gc, my lineWidth,
			my lineType >= Graphics_DOTTED ? GDK_LINE_ON_OFF_DASH : GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
	}
	static void gdkRevertLine (GraphicsScreen me) {
		if (my lineType >= Graphics_DOTTED) {
			gdk_gc_set_line_attributes (my gc, my lineWidth, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
		}
	}
	static void cairoPrepareLine (GraphicsScreen me) {
		if (my cr == NULL) return;
		double dotted_line [] = { 2, 2 };
		double dashed_line [] = { 6, 2 };
		double dashed_dotted_line [] = { 6, 2, 2, 2 };
		cairo_save (my cr);
		switch (my lineType) {
			case Graphics_DOTTED:
				cairo_set_dash (my cr, dotted_line, 2, 1.0);
				break;
			case Graphics_DASHED:
				cairo_set_dash (my cr, dashed_line, 2, 1.0);
				break;
			case Graphics_DASHED_DOTTED:
				cairo_set_dash (my cr, dashed_dotted_line, 4, 1.0);
				break;
		}
		cairo_set_line_width (my cr, my lineWidth);
	}
	static void cairoRevertLine (GraphicsScreen me) {
		if (my cr == NULL) return;
		if (my lineType >= Graphics_DOTTED) {
			cairo_set_dash (my cr, NULL, 0, 0);
		}
		cairo_restore (my cr);
	}
#elif win
	#define MY_BRUSH  SelectPen (my dc, GetStockPen (NULL_PEN)), SelectBrush (my dc, my brush);
	#define DEFAULT  SelectPen (my dc, GetStockPen (BLACK_PEN)), SelectBrush (my dc, GetStockBrush (NULL_BRUSH));
	static void winPrepareLine (GraphicsScreen me) {
		HPEN newPen;
		int lineWidth_pixels = LINE_WIDTH_IN_PIXELS (me) + 0.5;
		if (! lineWidth_pixels) lineWidth_pixels = 1;
		my fatNonSolid = my lineType != Graphics_DRAWN && my lineWidth > 1;
		if (Melder_debug == 10) {
			LOGBRUSH brush;
			brush. lbStyle = BS_SOLID;
			brush. lbColor = my foregroundColour;
			brush. lbHatch = my lineType == Graphics_DRAWN ? 0 : my lineType == Graphics_DOTTED ? PS_DOT : my lineType == Graphics_DASHED ? PS_DASH : PS_DASHDOT;
			newPen = ExtCreatePen (PS_GEOMETRIC, lineWidth_pixels, & brush, 0, NULL);
		} else {
			/*newPen = CreatePen (my lineType == Graphics_DRAWN ? PS_SOLID :
				my lineType == Graphics_DOTTED ? PS_DOT : my lineType == Graphics_DASHED ? PS_DASH : PS_DASHDOT,
				my fatNonSolid ? 1 : lineWidth_pixels, my foregroundColour);*/
			LOGPEN pen;
			pen. lopnStyle = my lineType == Graphics_DRAWN ? PS_SOLID : my lineType == Graphics_DOTTED ? PS_DOT : my lineType == Graphics_DASHED ? PS_DASH : PS_DASHDOT;
			pen. lopnWidth. x = my fatNonSolid ? 1 : lineWidth_pixels;
			pen. lopnWidth. y = 0;
			pen. lopnColor = my foregroundColour | 0x02000000;
			newPen = CreatePenIndirect (& pen);
		}
		SelectPen (my dc, newPen);
		DeletePen (my pen);
		my pen = newPen;
	}
#elif mac
	static void quartzPrepareLine (GraphicsScreen me) {
		if (my duringXor) {
			CGContextSetBlendMode (my macGraphicsContext, kCGBlendModeDifference);
			CGContextSetAllowsAntialiasing (my macGraphicsContext, false);
			CGContextSetRGBStrokeColor (my macGraphicsContext, 1.0, 1.0, 1.0, 1.0);
		} else {
			CGContextSetRGBStrokeColor (my macGraphicsContext, my macColour.red / 65536.0, my macColour.green / 65536.0, my macColour.blue / 65536.0, 1.0);
		}
		double lineWidth_pixels = LINE_WIDTH_IN_PIXELS (me);
		CGContextSetLineWidth (my macGraphicsContext, lineWidth_pixels);
		float lengths [4];
		if (my lineType == Graphics_DOTTED)
			lengths [0] = my resolution > 192 ? my resolution / 100.0 : 2,
			lengths [1] = my resolution > 192 ? my resolution / 75.0 + lineWidth_pixels : 2;
		if (my lineType == Graphics_DASHED)
			lengths [0] = my resolution > 192 ? my resolution / 25 : 6,
			lengths [1] = my resolution > 192 ? my resolution / 50.0 + lineWidth_pixels : 2;
		if (my lineType == Graphics_DASHED_DOTTED)
			lengths [0] = my resolution > 192 ? my resolution / 25 : 6,
			lengths [1] = my resolution > 192 ? my resolution / 50.0 + lineWidth_pixels : 2;
			lengths [2] = my resolution > 192 ? my resolution / 100.0 : 2;
			lengths [3] = my resolution > 192 ? my resolution / 50.0 + lineWidth_pixels : 2;
		CGContextSetLineDash (my macGraphicsContext, 0.0, my lineType == Graphics_DRAWN ? NULL : lengths, my lineType == 0 ? 0 : my lineType == Graphics_DASHED_DOTTED ? 4 : 2);
	}
	static void quartzRevertLine (GraphicsScreen me) {
		if (my duringXor) {
			CGContextSetBlendMode (my macGraphicsContext, kCGBlendModeNormal);
			CGContextSetAllowsAntialiasing (my macGraphicsContext, true);
		}
	}
	static void quartzPrepareFill (GraphicsScreen me) {
		CGContextSetAlpha (my macGraphicsContext, 1.0);
		CGContextSetBlendMode (my macGraphicsContext, kCGBlendModeNormal);
		CGContextSetRGBFillColor (my macGraphicsContext, my macColour.red / 65536.0, my macColour.green / 65536.0, my macColour.blue / 65536.0, 1.0);
	}
	static RGBColor theBlackColour = { 0, 0, 0 };
	static void quickdrawPrepareLine (GraphicsScreen me) {
		MacintoshPattern pattern;
		long lineWidth_pixels = LINE_WIDTH_IN_PIXELS (me) + 0.5;
		SetPort (my macPort);
		if (my lineType == Graphics_DOTTED) PenPat (GetQDGlobalsLightGray (& pattern));
		if (my lineType == Graphics_DASHED) PenPat (GetQDGlobalsDarkGray (& pattern));
		if (my lineType == Graphics_DASHED_DOTTED) PenPat (GetQDGlobalsGray (& pattern));
		if (! lineWidth_pixels) lineWidth_pixels = 1;
		PenSize (lineWidth_pixels, lineWidth_pixels);
		if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0)
			RGBForeColor (& my macColour);
	}
	static void quickdrawRevertLine (GraphicsScreen me) {
		MacintoshPattern pattern;
		if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0)
			RGBForeColor (& theBlackColour);
		if (my lineType != Graphics_DRAWN)
			PenPat (GetQDGlobalsBlack (& pattern));
		PenSize (1, 1);
	}
#endif

/* First level. */

static void polyline (I, long numberOfPoints, long *xyDC) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		#if cairo
			if (my duringXor) {
				gdkPrepareLine (me);
				for (long i = 0; i < numberOfPoints - 1; i ++) {
					gdk_draw_line (my window, my gc, xyDC [i + i], xyDC [i + i + 1], xyDC [i + i + 2], xyDC [i + i + 3]);
				}
				gdkRevertLine (me);
			} else {
				if (my cr == NULL) return;
				cairoPrepareLine (me);
				// cairo_new_path (my cr); // move_to() automatically creates a new path
				cairo_move_to (my cr, (double) xyDC [0], (double) xyDC [1]);
				for (long i = 1; i < numberOfPoints; i ++) {
					cairo_line_to (my cr, (double) xyDC [i + i], (double) xyDC [i + i + 1]);
				}
				// cairo_close_path (my cr);
				cairo_stroke (my cr);
				cairoRevertLine (me);
			}
		#elif win
			static POINT *points;
			winPrepareLine (me);
			if (points == NULL) {
				points = (POINT *) malloc (sizeof (POINT) * 15001L);
			}
			if (points) {
				long nleft = numberOfPoints, i;
				while (nleft > 15000) {
					for (i = 0; i < 15001; i ++)
						points [i]. x = *xyDC, xyDC ++, points [i]. y = *xyDC, xyDC ++;
					Polyline (my dc, points, 15001);
					if (my fatNonSolid) {
						for (i = 0; i < 15001; i ++) points [i]. x -= 1;
						Polyline (my dc, points, 15001);
						for (i = 0; i < 15001; i ++) points [i]. x += 1, points [i]. y -= 1;
						Polyline (my dc, points, 15001);
					}
					nleft -= 15000;
					xyDC -= 2;
				}
				for (i = 0; i < nleft; i ++)
					points [i]. x = *xyDC, xyDC ++, points [i]. y = *xyDC, xyDC ++;
				Polyline (my dc, points, nleft);
				if (my fatNonSolid) {
					for (i = 0; i < nleft; i ++) points [i]. x -= 1;
					Polyline (my dc, points, nleft);
					for (i = 0; i < nleft; i ++) points [i]. x += 1, points [i]. y -= 1;
					Polyline (my dc, points, nleft);
				}
			}
			DEFAULT
		#elif mac
			if (my useQuartz) {
				GraphicsQuartz_initDraw (me);
				quartzPrepareLine (me);
				CGContextBeginPath (my macGraphicsContext);
				CGContextMoveToPoint (my macGraphicsContext, xyDC [0], xyDC [1]);
				for (long i = 1; i < numberOfPoints; i ++) {
					CGContextAddLineToPoint (my macGraphicsContext, xyDC [i + i], xyDC [i + i + 1]);
				}
				CGContextStrokePath (my macGraphicsContext);
				quartzRevertLine (me);
				GraphicsQuartz_exitDraw (me);
			} else {   // QuickDraw
				int halfLine = ceil (0.5 * my lineWidth);
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				quickdrawPrepareLine (me);
				MoveTo (xyDC [0] - halfLine, xyDC [1] - halfLine);
				for (long i = 1; i < numberOfPoints; i ++)
					LineTo (xyDC [i + i] - halfLine, xyDC [i + i + 1] - halfLine);
				quickdrawRevertLine (me);
				if (my drawingArea) GuiMac_clipOff ();
			}
		#endif
	} else if (my postScript) {
		iam (GraphicsPostscript);
		long nn = numberOfPoints + numberOfPoints, i, ipath;
		psPrepareLine (me);
		my printf (my file, "N %ld %ld moveto\n", xyDC [0], xyDC [1]);
		for (i = 2, ipath = 1; i < nn; i += 2) {
			long dx = xyDC [i] - xyDC [i - 2], dy = xyDC [i + 1] - xyDC [i - 1];
			if (dx == 1 && i < nn - 20 && xyDC [i + 2] - xyDC [i] == 1 &&
				 xyDC [i + 4] - xyDC [i + 2] == 1 && xyDC [i + 6] - xyDC [i + 4] == 1 &&
				 xyDC [i + 8] - xyDC [i + 6] == 1 && xyDC [i + 10] - xyDC [i + 8] == 1 &&
				 xyDC [i + 12] - xyDC [i + 10] == 1 && xyDC [i + 14] - xyDC [i + 12] == 1 &&
				 xyDC [i + 16] - xyDC [i + 14] == 1 && xyDC [i + 18] - xyDC [i + 16] == 1)
			{
				my printf (my file, "%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld F\n",
					xyDC [i + 19] - xyDC [i + 17], xyDC [i + 17] - xyDC [i + 15],
					xyDC [i + 15] - xyDC [i + 13], xyDC [i + 13] - xyDC [i + 11],
					xyDC [i + 11] - xyDC [i + 9], xyDC [i + 9] - xyDC [i + 7],
					xyDC [i + 7] - xyDC [i + 5], xyDC [i + 5] - xyDC [i + 3],
					xyDC [i + 3] - xyDC [i + 1], dy);
				ipath += 9;
				i += 18;
			} else if (dx != 0 || dy != 0 || i < 4) {
				my printf (my file, "%ld %ld L\n", dx, dy);
			}
			if (++ ipath >= POSTSCRIPT_MAXPATH && i != nn - 2) {
				my printf (my file, "currentpoint stroke moveto\n");
				ipath = 1;
			}
		}
		my printf (my file, "stroke\n");
		psRevertLine (me);
	}
}

static void fillArea (I, long numberOfPoints, long *xyDC) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		#if cairo
			if (my cr == NULL) return;
			// cairo_new_path (my cr); // move_to() automatically creates a new path
			cairo_move_to (my cr, xyDC [0], xyDC [1]);
			for (long i = 1; i < numberOfPoints; i ++)
				cairo_line_to (my cr, xyDC [i + i], xyDC [i + i + 1]);
			cairo_close_path (my cr);
			cairo_fill (my cr);
		#elif win
			MY_BRUSH
			BeginPath (my dc);
			MoveToEx (my dc, xyDC [0], xyDC [1], NULL);
			for (long i = 1; i < numberOfPoints; i ++)
				LineTo (my dc, xyDC [i + i], xyDC [i + i + 1]);
			EndPath (my dc);
			FillPath (my dc);
			DEFAULT
		#elif mac
			if (my useQuartz) {
				GraphicsQuartz_initDraw (me);
				quartzPrepareFill (me);
				CGContextBeginPath (my macGraphicsContext);
				CGContextMoveToPoint (my macGraphicsContext, xyDC [0], xyDC [1]);
				for (long i = 1; i < numberOfPoints; i ++) {
					CGContextAddLineToPoint (my macGraphicsContext, xyDC [i + i], xyDC [i + i + 1]);
				}
				CGContextFillPath (my macGraphicsContext);
				GraphicsQuartz_exitDraw (me);
			} else {   // QuickDraw
				PolyHandle macpolygon;
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				SetPort (my macPort);
				if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0) RGBForeColor (& my macColour);
				macpolygon = OpenPoly ();
				MoveTo (xyDC [0], xyDC [1]);
				for (long i = 1; i < numberOfPoints; i ++)
					LineTo (xyDC [i + i], xyDC [i + i + 1]);
				ClosePoly ();
				PaintPoly (macpolygon);
				KillPoly (macpolygon);
				if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0) RGBForeColor (& theBlackColour);
				if (my drawingArea) GuiMac_clipOff ();
			}
		#endif
	} else if (my postScript) {
		iam (GraphicsPostscript);
		long nn = numberOfPoints + numberOfPoints, i;
		my printf (my file, "N %ld %ld M\n", xyDC [0], xyDC [1]);
		/*
		 * Very old (?) printers have path size restrictions.
		 * That's no reason to truncate the path on newer printers.
		 */
		#if 0
			if (numberOfPoints > POSTSCRIPT_MAXPATH) {
				Melder_warning1 (L"GraphicsPostscript::fillArea: path truncated.");
				numberOfPoints = POSTSCRIPT_MAXPATH, nn = numberOfPoints + numberOfPoints;
			}
		#endif
		for (i = 2; i < nn; i += 2) {
			my printf (my file, "%ld %ld L\n",
				xyDC [i] - xyDC [i - 2], xyDC [i + 1] - xyDC [i - 1]);
		}
		my printf (my file, "closepath fill\n");
	}
}

/* Second level. */

static void rectangle (I, long x1DC, long x2DC, long y1DC, long y2DC) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		ORDER_DC
		{
		#if cairo
			if (my cr == NULL) return;
			int width = x2DC - x1DC, height = y1DC - y2DC;
			if (width <= 0 || height <= 0) return;
			cairoPrepareLine (me);
			cairo_rectangle (my cr, x1DC, y2DC, width, height);
			cairo_stroke(my cr);
			cairoRevertLine (me);
		#elif win
			winPrepareLine (me);
			Rectangle (my dc, x1DC, y2DC, x2DC + 1, y1DC + 1);
			DEFAULT
		#elif mac
			if (my useQuartz) {
				GraphicsQuartz_initDraw (me);
				quartzPrepareLine (me);
				CGContextStrokeRect (my macGraphicsContext, CGRectMake (x1DC - 0.5, y2DC - 0.5, x2DC - x1DC, y1DC - y2DC));
				quartzRevertLine (me);
				GraphicsQuartz_exitDraw (me);
			} else {   // QuickDraw
				Rect rect;
				double halfLine = 0.5 * my lineWidth;
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				SetRect (& rect, x1DC - halfLine, y2DC - halfLine, x2DC + halfLine, y1DC + halfLine);
				quickdrawPrepareLine (me);
				FrameRect (& rect);
				quickdrawRevertLine (me);
				if (my drawingArea) GuiMac_clipOff ();
			}
		#else
			long xyDC [10];
			xyDC [0] = x1DC;	xyDC [1] = y1DC;
			xyDC [2] = x2DC;	xyDC [3] = y1DC;
			xyDC [4] = x2DC;	xyDC [5] = y2DC;
			xyDC [6] = x1DC;	xyDC [7] = y2DC;
			xyDC [8] = x1DC;	xyDC [9] = y1DC;
			polyline (me, 5, & xyDC [0]);
		#endif
		}
	} else if (my postScript) {
		iam (GraphicsPostscript);
		psPrepareLine (me);
		my printf (my file, "N %ld %ld M %ld %ld lineto %ld %ld lineto %ld %ld lineto closepath stroke\n",
			x1DC, y1DC, x2DC, y1DC, x2DC, y2DC, x1DC, y2DC);
		psRevertLine (me);
	}
}

void _Graphics_fillRectangle (I, long x1DC, long x2DC, long y1DC, long y2DC) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		ORDER_DC
		{
		#if cairo
			if (my cr == NULL) return;	
			int width = x2DC - x1DC + 1, height = y1DC - y2DC + 1;
			if (width <= 0 || height <= 0) return;
			cairo_rectangle (my cr, x1DC, y2DC, width, height);
			cairo_fill (my cr);
		#elif win
			RECT rect;
			rect. left = x1DC, rect. right = x2DC, rect. top = y2DC, rect. bottom = y1DC;   /* Superfluous? */
			MY_BRUSH
			Rectangle (my dc, x1DC, y2DC, x2DC + 1, y1DC + 1);
			DEFAULT
		#elif mac
			if (my useQuartz) {
				GraphicsQuartz_initDraw (me);
				quartzPrepareFill (me);
				CGContextFillRect (my macGraphicsContext, CGRectMake (x1DC, y2DC, x2DC - x1DC, y1DC - y2DC));
				GraphicsQuartz_exitDraw (me);
			} else {   // QuickDraw
				Rect rect;
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				SetRect (& rect, x1DC, y2DC, x2DC, y1DC);
				SetPort (my macPort);
				if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0) RGBForeColor (& my macColour);
				PaintRect (& rect);
				if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0) RGBForeColor (& theBlackColour);
				if (my drawingArea) GuiMac_clipOff ();
			}
		#else
			long xyDC [10];
			xyDC [0] = x1DC;	xyDC [1] = y1DC;
			xyDC [2] = x2DC;	xyDC [3] = y1DC;
			xyDC [4] = x2DC;	xyDC [5] = y2DC;
			xyDC [6] = x1DC;	xyDC [7] = y2DC;
			xyDC [8] = x1DC;	xyDC [9] = y1DC;
			fillArea (me, 5, & xyDC [0]);
		#endif
		}
	} else if (my postScript) {
		iam (GraphicsPostscript);
		my printf (my file,
			"N %ld %ld M %ld %ld lineto %ld %ld lineto %ld %ld lineto closepath fill\n",
			x1DC, y1DC, x2DC, y1DC, x2DC, y2DC, x1DC, y2DC);
	}
}

static void circle (I, double xDC, double yDC, double rDC) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		#if cairo
			if (my duringXor) {
				gdkPrepareLine (me);
				gdk_draw_arc (my window, my gc, FALSE, xDC - rDC, yDC - rDC, rDC + rDC, rDC + rDC, 0, 360 * 64);
				gdkRevertLine (me);
			} else {
				if (my cr == NULL) return;
				cairoPrepareLine (me);
				cairo_new_path(my cr);
				cairo_arc (my cr, xDC, yDC, rDC, 0.0, 2 * M_PI);
				cairo_stroke(my cr);
				cairoRevertLine (me);
			}
		#elif win
			winPrepareLine (me);
			Ellipse (my dc, xDC - rDC, yDC - rDC, xDC + rDC + 1, yDC + rDC + 1);
			DEFAULT
		#elif mac
			if (my useQuartz) {
				GraphicsQuartz_initDraw (me);
				quartzPrepareLine (me);
				CGContextBeginPath (my macGraphicsContext);
				CGContextAddArc (my macGraphicsContext, xDC, yDC, rDC, 0.0, NUM2pi, 0);
				CGContextStrokePath (my macGraphicsContext);
				quartzRevertLine (me);
				GraphicsQuartz_exitDraw (me);
			} else {   // QuickDraw
				Rect rect;
				double halfLine = 0.5 * my lineWidth;
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				SetRect (& rect, xDC - rDC - halfLine, yDC - rDC - halfLine, xDC + rDC + halfLine, yDC + rDC + halfLine);
				quickdrawPrepareLine (me);
				FrameOval (& rect);
				quickdrawRevertLine (me);
				if (my drawingArea) GuiMac_clipOff ();
			}
		#endif
	} else if (my postScript) {
		iam (GraphicsPostscript);
		psPrepareLine (me);
		my printf (my file, "N %ld %ld %ld C\n", (long) xDC, (long) yDC, (long) rDC);
		psRevertLine (me);
	}
}

static void ellipse (I, long x1DC, long x2DC, long y1DC, long y2DC) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		ORDER_DC
		{
		#if cairo
			if (my cr == NULL) return;
			cairoPrepareLine (me);
			cairo_new_path(my cr);
			cairo_save(my cr);
			cairo_translate(my cr, 0.5 * (x2DC + x1DC), 0.5 * (y2DC + y1DC));
			cairo_scale(my cr, 0.5 * (x2DC - x1DC), 0.5 * (y2DC - y1DC));
			cairo_arc(my cr, 0.0, 0.0, 1.0, 0.0, 2 * M_PI);
			cairo_restore(my cr);
			cairo_stroke(my cr);
			cairoRevertLine (me);
		#elif win
			winPrepareLine (me);
			Ellipse (my dc, x1DC, y2DC, x2DC + 1, y1DC + 1);
			DEFAULT
		#elif mac
			if (my useQuartz) {
				GraphicsQuartz_initDraw (me);
				quartzPrepareLine (me);
				CGContextBeginPath (my macGraphicsContext);
				CGContextSaveGState (my macGraphicsContext);
				CGContextTranslateCTM (my macGraphicsContext, 0.5 * (x2DC + x1DC), 0.5 * (y2DC + y1DC));
				CGContextScaleCTM (my macGraphicsContext, 0.5 * (x2DC - x1DC), 0.5 * (y2DC - y1DC));
				CGContextAddArc (my macGraphicsContext, 0.0, 0.0, 1.0, 0.0, NUM2pi, 0);
				CGContextScaleCTM (my macGraphicsContext, 2.0 / (x2DC - x1DC), 2.0 / (y2DC - y1DC));
				CGContextStrokePath (my macGraphicsContext);
				CGContextRestoreGState (my macGraphicsContext);
				quartzRevertLine (me);
				GraphicsQuartz_exitDraw (me);
			} else {   // QuickDraw
				Rect rect;
				double halfLine = 0.5 * my lineWidth;
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				SetRect (& rect, x1DC - halfLine, y2DC - halfLine, x2DC + halfLine, y1DC + halfLine);
				quickdrawPrepareLine (me);
				FrameOval (& rect);
				quickdrawRevertLine (me);
				if (my drawingArea) GuiMac_clipOff ();
			}
		#endif
		}
	} else if (my postScript) {
		iam (GraphicsPostscript);
		if (x1DC != x2DC && y1DC != y2DC) {   /* Prevent division by zero. */
			psPrepareLine (me);
			/* To draw an ellipse, we will have to 'translate' and 'scale' and draw a circle. */
			/* However, we have to scale back before the actual 'stroke', */
			/* because we want the normal line thickness; */
			/* So we cannot use 'gsave' and 'grestore', which clear the path (Cookbook 3). */
			my printf (my file, "gsave %f %f translate %f %f scale N 0 0 1 0 360 arc\n"
				" %f %f scale stroke grestore\n",
				0.5 * (x2DC + x1DC), 0.5 * (y2DC + y1DC), 0.5 * (x2DC - x1DC), 0.5 * (y2DC - y1DC),
				2.0 / (x2DC - x1DC), 2.0 / (y2DC - y1DC));
			psRevertLine (me);
		}
	}
}

static void arc (I, long xDC, long yDC, long rDC, double fromAngle, double toAngle) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		#if cairo
			if (my cr == NULL) return;
			cairoPrepareLine (me);
			cairo_new_path(my cr);
			cairo_arc (my cr, xDC, yDC, rDC, -toAngle * (M_PI / 180.0), -fromAngle * (M_PI / 180.0));
			cairo_stroke(my cr);
			cairoRevertLine (me);
		#elif win
			int arcAngle = (int) toAngle - (int) fromAngle;
			POINT pt;
			if (arcAngle < 0.0) arcAngle += 360;
			winPrepareLine (me);
			MoveToEx (my dc, xDC + rDC * cos (NUMpi / 180 * fromAngle), yDC - rDC * sin (NUMpi / 180 * fromAngle), & pt);
			AngleArc (my dc, xDC, yDC, rDC, fromAngle, arcAngle);
			DEFAULT
		#elif mac
			if (my useQuartz) {
				GraphicsQuartz_initDraw (me);
				quartzPrepareLine (me);
				CGContextBeginPath (my macGraphicsContext);
				CGContextAddArc (my macGraphicsContext, xDC, yDC, rDC, NUM2pi - NUMpi / 180 * toAngle, NUM2pi - NUMpi / 180 * fromAngle, 0);
				CGContextStrokePath (my macGraphicsContext);
				quartzRevertLine (me);
				GraphicsQuartz_exitDraw (me);
			} else {   // QuickDraw
				Rect rect;
				double halfLine = 0.5 * my lineWidth;
				int startAngle = 90 - (int) fromAngle;
				int arcAngle = (int) fromAngle - (int) toAngle;
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				if (arcAngle > 0) arcAngle -= 360;
				SetRect (& rect, xDC - rDC - halfLine, yDC - rDC - halfLine, xDC + rDC + halfLine, yDC + rDC + halfLine);
				quickdrawPrepareLine (me);
				FrameArc (& rect, startAngle, arcAngle);
				quickdrawRevertLine (me);
				if (my drawingArea) GuiMac_clipOff ();
			}
		#endif
	} else if (my postScript) {
		iam (GraphicsPostscript);
		psPrepareLine (me);
		my printf (my file, "N %ld %ld %ld %f %f arc stroke\n", xDC, yDC, rDC, fromAngle, toAngle);
		psRevertLine (me);
	}
}

/* Third level. */

static void fillCircle (I, long xDC, long yDC, long rDC) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		#if cairo
			if (my cr == NULL) return;
			cairo_new_path(my cr);
			cairo_arc (my cr, xDC, yDC, rDC, 0, 2 * M_PI);
			cairo_fill (my cr);
		#elif win
			MY_BRUSH
			/*
			 * NT cannot fill circles that span less than five pixels...
			 */
			Ellipse (my dc, xDC - rDC - 1, yDC - rDC - 1, xDC + rDC + 1, yDC + rDC + 1);
			DEFAULT
		#elif mac
			if (my useQuartz) {
				GraphicsQuartz_initDraw (me);
				quartzPrepareFill (me);
				CGContextBeginPath (my macGraphicsContext);
				CGContextAddArc (my macGraphicsContext, xDC, yDC, rDC, 0.0, NUM2pi, 0);
				CGContextFillPath (my macGraphicsContext);
				GraphicsQuartz_exitDraw (me);
			} else {   // QuickDraw
				Rect rect;
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				SetRect (& rect, xDC - rDC, yDC - rDC, xDC + rDC, yDC + rDC);
				SetPort (my macPort);
				if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0) RGBForeColor (& my macColour);
				PaintOval (& rect);
				if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0) RGBForeColor (& theBlackColour);
				if (my drawingArea) GuiMac_clipOff ();
			}
		#else
			circle (me, xDC, yDC, rDC);
		#endif
	} else if (my postScript) {
		iam (GraphicsPostscript);
		my printf (my file, "N %ld %ld %ld FC\n", xDC, yDC, rDC);
	}
}

static void fillEllipse (I, long x1DC, long x2DC, long y1DC, long y2DC) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		ORDER_DC
		{
		#if cairo
			if (my cr == NULL) return;	
			cairo_new_path(my cr);
			cairo_save(my cr);
			cairo_translate(my cr, 0.5 * (x2DC + x1DC), 0.5 * (y2DC + y1DC));
			cairo_scale(my cr, 0.5 * (x2DC - x1DC), 0.5 * (y2DC - y1DC));
			cairo_arc(my cr, 0.0, 0.0, 1.0, 0.0, 2 * M_PI);
			cairo_restore(my cr);
			cairo_fill (my cr);
		#elif win
			MY_BRUSH
			Ellipse (my dc, x1DC, y2DC, x2DC + 1, y1DC + 1);
			DEFAULT
		#elif mac
			if (my useQuartz) {
				GraphicsQuartz_initDraw (me);
				quartzPrepareFill (me);
				CGContextBeginPath (my macGraphicsContext);
				CGContextSaveGState (my macGraphicsContext);
				CGContextTranslateCTM (my macGraphicsContext, 0.5 * (x2DC + x1DC), 0.5 * (y2DC + y1DC));
				CGContextScaleCTM (my macGraphicsContext, 0.5 * (x2DC - x1DC), 0.5 * (y2DC - y1DC));
				CGContextAddArc (my macGraphicsContext, 0.0, 0.0, 1.0, 0.0, NUM2pi, 0);
				CGContextScaleCTM (my macGraphicsContext, 2.0 / (x2DC - x1DC), 2.0 / (y2DC - y1DC));
				CGContextFillPath (my macGraphicsContext);
				CGContextRestoreGState (my macGraphicsContext);
				GraphicsQuartz_exitDraw (me);
			} else {   // QuickDraw
				Rect rect;
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				SetRect (& rect, x1DC, y2DC, x2DC, y1DC);
				SetPort (my macPort);
				if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0) RGBForeColor (& my macColour);
				PaintOval (& rect);
				if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0) RGBForeColor (& theBlackColour);
				if (my drawingArea) GuiMac_clipOff ();
			}
		#else
			ellipse (me, x1DC, x2DC, y1DC, y2DC);
		#endif
		}
	} else if (my postScript) {
		iam (GraphicsPostscript);
		my printf (my file, "gsave %ld %ld translate %ld %ld scale N 0 0 1 FC grestore\n",
			(x2DC + x1DC) / 2, (y2DC + y1DC) / 2, (x2DC - x1DC) / 2, (y2DC - y1DC) / 2);
	}
}

static void fillArc (I, long xDC, long yDC, long rDC, double fromAngle, double toAngle) {
	iam (Graphics);
	arc (me, xDC, yDC, rDC, fromAngle, toAngle);
	// TODO: shouldn't there be a cairo entry here with cairo_arc fill?
}

static void button (I, long x1DC, long x2DC, long y1DC, long y2DC) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		ORDER_DC
		{
		#if cairo
			if (x2DC <= x1DC || y1DC <= y2DC) return;
			
			cairo_save (my cr);
			if (my drawingArea && 0) {
				// clip to drawing area
				int w, h;
				gdk_drawable_get_size (my window, & w, & h);
				cairo_rectangle (my cr, 0, 0, w, h);
				cairo_clip (my cr);
			}
			cairo_set_line_width (my cr, 1.0);
			double left = x1DC - 0.5, right = x2DC - 0.5, top = y2DC + 0.5, bottom = y1DC + 0.5;
			double width = right - left, height = bottom - top;
			cairo_set_source_rgb (my cr, 0.1, 0.1, 0.1);   // dark grey
			cairo_rectangle (my cr, left, top, width, height);
			cairo_stroke (my cr);
			
			left ++, right --, top ++, bottom --, width -= 2, height -= 2;
			if (width > 0 && height > 0) {
				cairo_set_source_rgb (my cr, 0.3, 0.3, 0.3);
				cairo_move_to (my cr, left + 1, bottom);
				cairo_line_to (my cr, right, bottom);
				cairo_line_to (my cr, right, top + 1);
				cairo_stroke (my cr);
				
				cairo_set_source_rgb (my cr, 1.0, 1.0, 1.0);
				cairo_move_to (my cr, left, bottom);
				cairo_line_to (my cr, left, top);
				cairo_line_to (my cr, right, top);
				cairo_stroke (my cr);
				left += 0.5, right -= 0.5, top += 0.5, bottom -= 0.5, width -= 1, height -= 1;
				if (width > 0 && height > 0) {
					cairo_set_source_rgb (my cr, 0.65, 0.65, 0.65);
					cairo_rectangle (my cr, left, top, width, height);
					cairo_fill (my cr);
				}
			}
			cairo_restore (my cr);
		#elif win
			RECT rect;
			rect. left = x1DC, rect. right = x2DC, rect. top = y2DC, rect. bottom = y1DC;
			DrawEdge (my dc, & rect, EDGE_RAISED, BF_RECT);
			SelectPen (my dc, GetStockPen (NULL_PEN));
			SelectBrush (my dc, GetStockBrush (LTGRAY_BRUSH));
			Rectangle (my dc, x1DC + 1, y2DC + 1, x2DC - 1, y1DC - 1);
			SelectPen (my dc, GetStockPen (BLACK_PEN));
			SelectBrush (my dc, GetStockBrush (NULL_BRUSH));
		#elif mac
			int width, height;
			Rect rect;
			RGBColor rgb;
			width = x2DC - x1DC, height = y1DC - y2DC;
			if (width <= 0 || height <= 0) return;
			if (my drawingArea) GuiMac_clipOn (my drawingArea);
			SetPort (my macPort);
			/* Dark grey. */
			rgb. red = rgb. green = rgb. blue = (int) (unsigned int) (0.1 * 65535.0); RGBForeColor (& rgb);
			SetRect (& rect, x1DC - 1, y2DC - 1, x2DC, y1DC);
			FrameRect (& rect);
			if (width > 2 && height > 2) {
				rgb. red = rgb. green = rgb. blue = (int) (unsigned int) (0.3 * 65535.0); RGBForeColor (& rgb);
				MoveTo (x1DC, y1DC - 2); LineTo (x2DC - 2, y1DC - 2);
				MoveTo (x2DC - 2, y1DC - 2); LineTo (x2DC - 2, y2DC);
				rgb. red = rgb. green = rgb. blue = (int) (unsigned int) (1.0 * 65535.0); RGBForeColor (& rgb);
				MoveTo (x1DC, y1DC - 2); LineTo (x1DC, y2DC);
				MoveTo (x1DC, y2DC); LineTo (x2DC - 2, y2DC);
				if (width > 4 && height > 4) {
					rgb. red = rgb. green = rgb. blue = (int) (unsigned int) (0.65 * 65535.0); RGBForeColor (& rgb);
					SetRect (& rect, x1DC + 1, y2DC + 1, x2DC - 2, y1DC - 2);
					PaintRect (& rect);
				}
			}
			RGBForeColor (& theBlackColour);
			if (my drawingArea) GuiMac_clipOff ();
		#endif
		}
	} else if (my postScript) {
		iam (GraphicsPostscript);
		rectangle (me, x1DC, x2DC, y1DC, y2DC);
	}
}

static void roundedRectangle (I, long x1DC, long x2DC, long y1DC, long y2DC, long r) {
	iam (Graphics);
	long dy = my yIsZeroAtTheTop ? - r : r, xyDC [4];
	ORDER_DC
	#if win
		if (my screen) {
			iam (GraphicsScreen);
			winPrepareLine (me);
			RoundRect (my dc, x1DC, y2DC, x2DC + 1, y1DC + 1, r + r, r + r);
			DEFAULT
			return;
		}
	#endif
	xyDC [0] = x1DC + r;
	xyDC [1] = y1DC;
	xyDC [2] = x2DC - r;
	xyDC [3] = y1DC;
	polyline (me, 2, xyDC);
	arc (me, x2DC - r, y1DC + dy, r, -90, 0);
	xyDC [0] = x2DC;
	xyDC [1] = y1DC + dy;
	xyDC [2] = x2DC;
	xyDC [3] = y2DC - dy;
	polyline (me, 2, xyDC);
	arc (me, x2DC - r, y2DC - dy, r, 0, 90);
	xyDC [0] = x2DC - r;
	xyDC [1] = y2DC;
	xyDC [2] = x1DC + r;
	xyDC [3] = y2DC;
	polyline (me, 2, xyDC);
	arc (me, x1DC + r, y2DC - dy, r, 90, 180);
	xyDC [0] = x1DC;
	xyDC [1] = y2DC - dy;
	xyDC [2] = x1DC;
	xyDC [3] = y1DC + dy;
	polyline (me, 2, xyDC);
	arc (me, x1DC + r, y1DC + dy, r, 180, 270);
}

/* Fourth level. */

static void fillRoundedRectangle (I, long x1DC, long x2DC, long y1DC, long y2DC, long r) {
	iam (Graphics);
	long dy = my yIsZeroAtTheTop ? - r : r;
	ORDER_DC
	fillCircle (me, x2DC - r, y1DC + dy, r);
	fillCircle (me, x2DC - r, y2DC - dy, r);
	fillCircle (me, x1DC + r, y2DC - dy, r);
	fillCircle (me, x1DC + r, y1DC + dy, r);
	_Graphics_fillRectangle (me, x1DC, x2DC, y1DC + dy, y2DC - dy);
	_Graphics_fillRectangle (me, x1DC + r, x2DC - r, y1DC, y2DC);
}

/* Fifth level. */

#define wdx(x)  ((x) * my scaleX + my deltaX)
#define wdy(y)  ((y) * my scaleY + my deltaY)

void Graphics_polyline (I, long numberOfPoints, double *xWC, double *yWC) {	/* Base 0. */
	iam (Graphics);
	if (numberOfPoints == 0) return;
	long *xyDC = Melder_malloc_e (long, 2 * numberOfPoints);
	if (xyDC == NULL) { Melder_clearError (); return; }
	for (long i = 0; i < numberOfPoints; i ++) {
		xyDC [i + i] = wdx (xWC [i]);
		xyDC [i + i + 1] = wdy (yWC [i]);
	}
	polyline (me, numberOfPoints, xyDC);
	Melder_free (xyDC);
	if (my recording) {
		op (POLYLINE, 1 + 2 * numberOfPoints);
		put (numberOfPoints);
		mput (numberOfPoints, & xWC [0])
		mput (numberOfPoints, & yWC [0])
	}
}

void Graphics_line (I, double x1WC, double y1WC, double x2WC, double y2WC) {
	iam (Graphics);
	long xyDC [4];
	xyDC [0] = wdx (x1WC);
	xyDC [1] = wdy (y1WC);
	xyDC [2] = wdx (x2WC);
	xyDC [3] = wdy (y2WC);
	polyline (me, 2, xyDC);
	if (my recording) { op (LINE, 4); put (x1WC); put (y1WC); put (x2WC); put (y2WC); }
}

void Graphics_fillArea (I, long numberOfPoints, double *xWC, double *yWC) {
	iam (Graphics);
	long *xyDC = Melder_malloc_e (long, 2 * numberOfPoints);
	if (xyDC == NULL) { Melder_clearError (); return; }
	for (long i = 0; i < numberOfPoints; i ++) {
		xyDC [i + i] = wdx (xWC [i]);
		xyDC [i + i + 1] = wdy (yWC [i]);
	}
	fillArea (me, numberOfPoints, xyDC);
	Melder_free (xyDC);
	if (my recording) {
		op (FILL_AREA, 1 + 2 * numberOfPoints);
		put (numberOfPoints);
		mput (numberOfPoints, & xWC [0])
		mput (numberOfPoints, & yWC [0])
	}
}

static void initLine (I) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		#if cairo
			if (my cr == NULL) return;
			cairoPrepareLine (me);
		#elif win
			winPrepareLine (me);
		#elif mac
			if (my useQuartz) {
				quartzPrepareLine (me);
			} else {   // QuickDraw
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				quickdrawPrepareLine (me);
			}
		#endif
	} else {
		iam (GraphicsPostscript);
		psPrepareLine (me);
	}
}

static void exitLine (I) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		#if cairo
			if (my cr == NULL) return;
			cairoRevertLine (me);
		#elif win
			DEFAULT
		#elif mac
			if (my useQuartz) {
				quartzRevertLine (me);
			} else {   // QuickDraw
				quickdrawRevertLine (me);
				if (my drawingArea) GuiMac_clipOff ();
			}
		#endif
	} else {
		iam (GraphicsPostscript);
		psRevertLine (me);
	}
}

static void polysegment (I, long numberOfPoints, long *xyDC) {
	iam (Graphics);
	initLine (me);
	if (my screen) {
		iam (GraphicsScreen);
		#if cairo
			if (my cr == NULL) return;
			long i;
			int halfLine = ceil (0.5 * my lineWidth);
			for (i = 0; i < numberOfPoints; i ++) {
				cairo_move_to (my cr, xyDC [i + i] - halfLine, xyDC [i + i + 1] - halfLine);
				cairo_line_to (my cr, xyDC [i + i] - halfLine, xyDC [i + i + 1] - halfLine);
			}
			cairo_stroke(my cr);
		#elif win
			long i;
			int halfLine = 0 /*ceil (0.5 * my lineWidth)*/;
			for (i = 0; i < numberOfPoints; i += 2) {
				long xfrom = xyDC [i + i] - halfLine, yfrom = xyDC [i + i + 1] - halfLine;
				long xto = xyDC [i + i + 2] - halfLine, yto = xyDC [i + i + 3] - halfLine;
				MoveToEx (my dc, xfrom, yfrom, NULL);
				LineTo (my dc, xto, yto);
				if (my fatNonSolid) {
					MoveToEx (my dc, xfrom - 1, yfrom, NULL);
					LineTo (my dc, xto - 1, yto);
					MoveToEx (my dc, xfrom, yfrom - 1, NULL);
					LineTo (my dc, xto, yto - 1);
				}
			}
		#elif mac
			if (my useQuartz) {
			} else {   // QuickDraw
				long i;
				int halfLine = ceil (0.5 * my lineWidth);
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				quickdrawPrepareLine (me);
				for (i = 0; i < numberOfPoints; i ++) {
					MoveTo (xyDC [i + i] - halfLine, xyDC [i + i + 1] - halfLine);
					i ++;
					LineTo (xyDC [i + i] - halfLine, xyDC [i + i + 1] - halfLine);
				}
			}
		#endif
	} else if (my postScript) {
		iam (GraphicsPostscript);
		long nn = numberOfPoints + numberOfPoints, i, ipath;
		my printf (my file, "N\n");
		for (i = 0, ipath = 1; i < nn; i += 4) {
			long dx = xyDC [i + 2] - xyDC [i], dy = xyDC [i + 3] - xyDC [i + 1];
			my printf (my file, "%ld %ld M %ld %ld L\n", xyDC [i], xyDC [i + 1], dx, dy);
			if (++ ipath >= POSTSCRIPT_MAXPATH / 2 && i != nn - 4) {
				my printf (my file, "currentpoint stroke moveto\n");
				ipath = 1;
			}
		}
		my printf (my file, "stroke\n");
	}
	exitLine (me);
}

#if 0
#define MACRO_Graphics_function(TYPE) \
	long x1DC, x2DC; \
	long clipy1 = wdy (my y1WC), clipy2 = wdy (my y2WC); \
	double dx, offsetX, translation, scale; \
	long i, n = ix2 - ix1 + 1; \
 \
	if (ix2 <= ix1 || my scaleX == 0.0) return; \
 \
	dx = (x2WC - x1WC) / (n - 1); \
	offsetX = x1WC - ix1 * dx; \
	/* xDC = wdx (offsetX + i * dx) */ \
	translation = wdx (offsetX); \
	scale = dx * my scaleX; \
	x1DC = translation + ix1 * scale; \
	x2DC = translation + ix2 * scale; \
	if (n > (x2DC - x1DC + 1) * 2) {  /* Optimize: draw one vertical line for each device x coordinate. */ \
		long numberOfPixels = x2DC - x1DC + 1, k = 0; \
		long numberOfPointsActuallyDrawn = numberOfPixels * 2; \
		long *xyDC, previousMinDC = -32768, previousMaxDC = 32767; \
		if (numberOfPointsActuallyDrawn < 1) return; \
		xyDC = (long *) Melder_malloc (2 * numberOfPointsActuallyDrawn * sizeof (long)); \
		for (i = 0; i < numberOfPixels; i ++) { \
			long j, jmin = ix1 + i / scale, jmax = ix1 + (i + 1) / scale; \
			TYPE minWC, maxWC; \
			long minDC, maxDC; \
			if (jmin > ix2) jmin = ix2; \
			if (jmax > ix2) jmax = ix2; \
			minWC = yWC [STAGGER (jmin)], maxWC = minWC; \
			for (j = jmin + 1; j < jmax; j ++) {   /* No overlap. */ \
				TYPE value = yWC [STAGGER (j)]; \
				if (value > maxWC) maxWC = value; \
				else if (value < minWC) minWC = value; \
			} \
			if (my yIsZeroAtTheTop) { \
				minDC = wdy (maxWC); \
				maxDC = wdy (minWC); \
				if (minDC > clipy1) minDC = clipy1; \
				if (maxDC > clipy1) maxDC = clipy1; \
				if (maxDC < clipy2) maxDC = clipy2; \
				if (minDC < clipy2) minDC = clipy2; \
			} else { \
				minDC = wdy (minWC); \
				maxDC = wdy (maxWC); \
				if (minDC < clipy1) minDC = clipy1; \
				if (maxDC < clipy1) maxDC = clipy1; \
				if (maxDC > clipy2) maxDC = clipy2; \
				if (minDC > clipy2) minDC = clipy2; \
			} \
			if (minDC > previousMaxDC) minDC = previousMaxDC + 1; \
			if (maxDC < previousMinDC) maxDC = previousMinDC - 1; \
			xyDC [k ++] = x1DC + i; \
			xyDC [k ++] = minDC; \
			xyDC [k ++] = x1DC + i; \
			xyDC [k ++] = maxDC; \
			previousMinDC = minDC; \
			previousMaxDC = maxDC; \
		} \
		if (k > 1) polysegment (me, k / 2, xyDC); \
		Melder_free (xyDC); \
	} else {  /* Normal. */  \
		long *xyDC = (long *) Melder_malloc (2 * n * sizeof (long)); \
		for (i = 0; i < n; i ++) { \
			long ix = ix1 + i; \
			long value = wdy (yWC [STAGGER (ix)]); \
			xyDC [i + i] = translation + ix * scale; \
			if (my yIsZeroAtTheTop) { \
				if (value > clipy1) value = clipy1; \
				if (value < clipy2) value = clipy2; \
			} else { \
				if (value < clipy1) value = clipy1; \
				if (value > clipy2) value = clipy2; \
			} \
			xyDC [i + i + 1] = value; \
		} \
		polyline (me, n, xyDC); \
		Melder_free (xyDC); \
	}
#endif

#define MACRO_Graphics_function(TYPE) \
	long x1DC, x2DC; \
	long clipy1 = wdy (my y1WC), clipy2 = wdy (my y2WC); \
	double dx, offsetX, translation, scale; \
	long i, n = ix2 - ix1 + 1; \
 \
	if (ix2 <= ix1 || my scaleX == 0.0) return; \
 \
	dx = (x2WC - x1WC) / (n - 1); \
	offsetX = x1WC - ix1 * dx; \
	/* xDC = wdx (offsetX + i * dx) */ \
	translation = wdx (offsetX); \
	scale = dx * my scaleX; \
	x1DC = translation + ix1 * scale; \
	x2DC = translation + ix2 * scale; \
	if (n > (x2DC - x1DC + 1) * 2) {  /* Optimize: draw one vertical line for each device x coordinate. */ \
		long numberOfPixels = x2DC - x1DC + 1, k = 0; \
		long numberOfPointsActuallyDrawn = numberOfPixels * 2; \
		long *xyDC; \
		TYPE lastMini; \
		if (numberOfPointsActuallyDrawn < 1) return; \
		xyDC = Melder_malloc_f (long, 2 * numberOfPointsActuallyDrawn); \
		for (i = 0; i < numberOfPixels; i ++) { \
			long j, jmin = ix1 + i / scale, jmax = ix1 + (i + 1) / scale; \
			TYPE mini, maxi; \
			long minDC, maxDC; \
			if (jmin > ix2) jmin = ix2; \
			if (jmax > ix2) jmax = ix2; \
			mini = yWC [STAGGER (jmin)], maxi = mini; \
			for (j = jmin + 1; j <= jmax; j ++) {   /* One point overlap. */ \
				TYPE value = yWC [STAGGER (j)]; \
				if (value > maxi) maxi = value; \
				else if (value < mini) mini = value; \
			} \
			minDC = wdy (mini); \
			maxDC = wdy (maxi); \
			if (my yIsZeroAtTheTop) { \
				if (minDC > clipy1) minDC = clipy1; \
				if (maxDC > clipy1) maxDC = clipy1; \
				if (maxDC < clipy2) maxDC = clipy2; \
				if (minDC < clipy2) minDC = clipy2; \
			} else { \
				if (minDC < clipy1) minDC = clipy1; \
				if (maxDC < clipy1) maxDC = clipy1; \
				if (maxDC > clipy2) maxDC = clipy2; \
				if (minDC > clipy2) minDC = clipy2; \
			} \
			if (i == 0) { \
				if (yWC [STAGGER (jmin)] < yWC [STAGGER (jmax)]) { \
					xyDC [k ++] = x1DC; \
					xyDC [k ++] = minDC; \
					xyDC [k ++] = x1DC; \
					xyDC [k ++] = maxDC; \
				} else { \
					xyDC [k ++] = x1DC; \
					xyDC [k ++] = maxDC; \
					xyDC [k ++] = x1DC; \
					xyDC [k ++] = minDC; \
				} \
			} else if (minDC == xyDC [k - 1]) { \
				xyDC [k ++] = x1DC + i; \
				xyDC [k ++] = maxDC; \
			} else if (maxDC == xyDC [k - 1]) { \
				xyDC [k ++] = x1DC + i; \
				xyDC [k ++] = minDC; \
			} else if (mini > lastMini) { \
				xyDC [k ++] = x1DC + i; \
				xyDC [k ++] = minDC; \
				xyDC [k ++] = x1DC + i; \
				xyDC [k ++] = maxDC; \
			} else { \
				xyDC [k ++] = x1DC + i; \
				xyDC [k ++] = maxDC; \
				xyDC [k ++] = x1DC + i; \
				xyDC [k ++] = minDC; \
			} \
			lastMini = mini; \
		} \
		if (k > 1) polyline (me, k / 2, xyDC); \
		Melder_free (xyDC); \
	} else {  /* Normal. */  \
		long *xyDC = Melder_malloc_f (long, 2 * n); \
		for (i = 0; i < n; i ++) { \
			long ix = ix1 + i; \
			long value = wdy (yWC [STAGGER (ix)]); \
			xyDC [i + i] = translation + ix * scale; \
			if (my yIsZeroAtTheTop) { \
				if (FUNCTIONS_ARE_CLIPPED && value > clipy1) value = clipy1; \
				if (FUNCTIONS_ARE_CLIPPED && value < clipy2) value = clipy2; \
			} else { \
				if (FUNCTIONS_ARE_CLIPPED && value < clipy1) value = clipy1; \
				if (FUNCTIONS_ARE_CLIPPED && value > clipy2) value = clipy2; \
			} \
			xyDC [i + i + 1] = value; \
		} \
		polyline (me, n, xyDC); \
		Melder_free (xyDC); \
	}

void Graphics_function (I, double yWC [], long ix1, long ix2, double x1WC, double x2WC) {
	iam (Graphics);
	#define STAGGER(i)  (i)
	MACRO_Graphics_function (double)
	#undef STAGGER
	if (my recording) { op (FUNCTION, 3 + n); put (n); put (x1WC); put (x2WC); mput (n, & yWC [ix1]) }
}

void Graphics_function16 (I, int16_t yWC [], int stagger, long ix1, long ix2, double x1WC, double x2WC) {
	iam (Graphics);
	if (stagger == 1) {
		#define STAGGER(i)  ((i) + (i))
		MACRO_Graphics_function (int16_t)
		#undef STAGGER
	} else if (stagger > 1) {
		#define STAGGER(i)  ((stagger + 1) * (i))
		MACRO_Graphics_function (int16_t)
		#undef STAGGER
	} else {
		#define STAGGER(i)  (i)
		MACRO_Graphics_function (int16_t)
		#undef STAGGER
	}
}

void Graphics_rectangle (I, double x1WC, double x2WC, double y1WC, double y2WC) {
	iam (Graphics);
	rectangle (me, wdx (x1WC), wdx (x2WC), wdy (y1WC), wdy (y2WC));
	if (my recording) { op (RECTANGLE, 4); put (x1WC); put (x2WC); put (y1WC); put (y2WC); }
}

void Graphics_fillRectangle (I, double x1WC, double x2WC, double y1WC, double y2WC) {
	iam (Graphics);
	_Graphics_fillRectangle (me, wdx (x1WC), wdx (x2WC), wdy (y1WC), wdy (y2WC));
	if (my recording) { op (FILL_RECTANGLE, 4); put (x1WC); put (x2WC); put (y1WC); put (y2WC); }
}

void Graphics_roundedRectangle (I, double x1WC, double x2WC, double y1WC, double y2WC, double r_mm) {
	iam (Graphics);
	roundedRectangle (me, wdx (x1WC), wdx (x2WC), wdy (y1WC), wdy (y2WC), ceil (r_mm * my resolution / 25.4));
	if (my recording) { op (ROUNDED_RECTANGLE, 5); put (x1WC); put (x2WC); put (y1WC); put (y2WC); put (r_mm); }
}

void Graphics_fillRoundedRectangle (I, double x1WC, double x2WC, double y1WC, double y2WC, double r_mm) {
	iam (Graphics);
	fillRoundedRectangle (me, wdx (x1WC), wdx (x2WC), wdy (y1WC), wdy (y2WC), ceil (r_mm * my resolution / 25.4));
	if (my recording) { op (FILL_ROUNDED_RECTANGLE, 5); put (x1WC); put (x2WC); put (y1WC); put (y2WC); put (r_mm); }
}

void Graphics_button (I, double x1WC, double x2WC, double y1WC, double y2WC) {
	iam (Graphics);
	button (me, wdx (x1WC), wdx (x2WC), wdy (y1WC), wdy (y2WC));
	if (my recording) { op (BUTTON, 4); put (x1WC); put (x2WC); put (y1WC); put (y2WC); }
}

void Graphics_innerRectangle (I, double x1WC, double x2WC, double y1WC, double y2WC) {
	iam (Graphics);
	int dy = my yIsZeroAtTheTop ? -1 : 1;
	rectangle (me, wdx (x1WC) + 1, wdx (x2WC) - 1, wdy (y1WC) + dy, wdy (y2WC) - dy);
	if (my recording) { op (INNER_RECTANGLE, 4); put (x1WC); put (x2WC); put (y1WC); put (y2WC); }
}

void Graphics_circle (I, double xWC, double yWC, double rWC) {
	iam (Graphics);
	circle (me, wdx (xWC), wdy (yWC), my scaleX * rWC);
	if (my recording) { op (CIRCLE, 3); put (xWC); put (yWC); put (rWC); }
}

void Graphics_circle_mm (I, double xWC, double yWC, double diameter) {
	iam (Graphics);
	circle (me, wdx (xWC), wdy (yWC), 0.5 * diameter * my resolution / 25.4);
	if (my recording) { op (CIRCLE_MM, 3); put (xWC); put (yWC); put (diameter); }
}

void Graphics_fillCircle (I, double xWC, double yWC, double rWC) {
	iam (Graphics);
	fillCircle (me, wdx (xWC), wdy (yWC), ceil (my scaleX * rWC));
	if (my recording) { op (FILL_CIRCLE, 3); put (xWC); put (yWC); put (rWC); }
}

void Graphics_fillCircle_mm (I, double xWC, double yWC, double diameter) {
	iam (Graphics);
	fillCircle (me, wdx (xWC), wdy (yWC), ceil (0.5 * diameter * my resolution / 25.4));
	if (my recording) { op (FILL_CIRCLE_MM, 3); put (xWC); put (yWC); put (diameter); }
}

void Graphics_rectangle_mm (I, double xWC, double yWC, double horSide, double vertSide) {
	iam (Graphics);
	long xDC = wdx (xWC), yDC = wdy (yWC);
	long halfHorSide = ceil (0.5 * horSide * my resolution / 25.4);
	long halfVertSide = ceil (0.5 * vertSide * my resolution / 25.4);
	if (my yIsZeroAtTheTop) {
		rectangle (me, xDC - halfHorSide, xDC + halfHorSide, yDC + halfVertSide, yDC - halfVertSide);
	} else {
		rectangle (me, xDC - halfHorSide, xDC + halfHorSide, yDC - halfVertSide, yDC + halfVertSide);
	}
	if (my recording) { op (RECTANGLE_MM, 4); put (xWC); put (yWC); put (horSide); put (vertSide); }
}

void Graphics_fillRectangle_mm (I, double xWC, double yWC, double horSide, double vertSide) {
	iam (Graphics);
	long xDC = wdx (xWC), yDC = wdy (yWC);
	long halfHorSide = ceil (0.5 * horSide * my resolution / 25.4);
	long halfVertSide = ceil (0.5 * vertSide * my resolution / 25.4);
	if (my yIsZeroAtTheTop) {
		_Graphics_fillRectangle (me, xDC - halfHorSide, xDC + halfHorSide, yDC + halfVertSide, yDC - halfVertSide);
	} else {
		_Graphics_fillRectangle (me, xDC - halfHorSide, xDC + halfHorSide, yDC - halfVertSide, yDC + halfVertSide);
	}
	if (my recording) { op (FILL_RECTANGLE_MM, 4); put (xWC); put (yWC); put (horSide); put (vertSide); }
}

void Graphics_ellipse (I, double x1, double x2, double y1, double y2) {
	iam (Graphics);
	ellipse (me, wdx (x1), wdx (x2), wdy (y1), wdy (y2));
	if (my recording) { op (ELLIPSE, 4); put (x1); put (x2); put (y1); put (y2); }
}

void Graphics_fillEllipse (I, double x1, double x2, double y1, double y2) {
	iam (Graphics);
	fillEllipse (me, wdx (x1), wdx (x2), wdy (y1), wdy (y2));
	if (my recording) { op (FILL_ELLIPSE, 4); put (x1); put (x2); put (y1); put (y2); }
}

void Graphics_arc (I, double xWC, double yWC, double rWC, double fromAngle, double toAngle) {
	iam (Graphics);
	arc (me, wdx (xWC), wdy (yWC), my scaleX * rWC, fromAngle, toAngle);
	if (my recording) { op (ARC, 5); put (xWC); put (yWC); put (rWC); put (fromAngle); put (toAngle); }
}

void Graphics_fillArc (I, double xWC, double yWC, double rWC, double fromAngle, double toAngle) {
	iam (Graphics);
	fillArc (me, wdx (xWC), wdy (yWC), my scaleX * rWC, fromAngle, toAngle);
	if (my recording) { op (FILL_ARC, 5); put (xWC); put (yWC); put (rWC); put (fromAngle); put (toAngle); }
}

/* Arrows. */

static void arrowHead (I, long xDC, long yDC, double angle) {
	iam (Graphics);
	if (my screen) {
		iam (GraphicsScreen);
		#if cairo
			if (my cr == NULL) return;
			double size = 10.0 * my resolution * my arrowSize / 75.0; // TODO: die 75 zou dat niet de scherm resolutie moeten worden?
			cairo_new_path (my cr);
			cairo_move_to (my cr, xDC + cos ((angle + 160) * NUMpi / 180) * size, yDC - sin ((angle + 160) * NUMpi / 180) * size);
			cairo_line_to (my cr, xDC, yDC);
			cairo_line_to (my cr, xDC + cos ((angle - 160) * NUMpi / 180) * size, yDC - sin ((angle - 160) * NUMpi / 180) * size);
			cairo_close_path (my cr);
			cairo_fill (my cr);
		#elif win
			double size = 10.0 * my resolution * my arrowSize / 72.0;
			MY_BRUSH
			BeginPath (my dc);
			MoveToEx (my dc, xDC + cos ((angle + 160) * NUMpi / 180) * size, yDC - sin ((angle + 160) * NUMpi / 180) * size, NULL);
			LineTo (my dc, xDC, yDC);
			LineTo (my dc, xDC + cos ((angle - 160) * NUMpi / 180) * size, yDC - sin ((angle - 160) * NUMpi / 180) * size);
			EndPath (my dc);
			FillPath (my dc);
			DEFAULT
		#elif mac
			if (my useQuartz) {
				GraphicsQuartz_initDraw (me);
				quartzPrepareFill (me);
				CGContextSaveGState (my macGraphicsContext);
				CGContextBeginPath (my macGraphicsContext);
				CGContextTranslateCTM (my macGraphicsContext, xDC, yDC);
				CGContextRotateCTM (my macGraphicsContext, - angle * NUMpi / 180);
				CGContextMoveToPoint (my macGraphicsContext, 0.0, 0.0);
				double size = 10.0 * my resolution * my arrowSize / 72.0;
				double radius = my resolution * my arrowSize / 30;
				CGContextAddArc (my macGraphicsContext, - size, 0.0, radius, - NUMpi / 3.0, NUMpi / 3.0, 0);
				CGContextAddLineToPoint (my macGraphicsContext, 0.0, 0.0);
				CGContextFillPath (my macGraphicsContext);
				CGContextRestoreGState (my macGraphicsContext);
				GraphicsQuartz_exitDraw (me);
			} else {   // QuickDraw
				double size = 10.0 * my resolution * my arrowSize / 72.0;
				PolyHandle macpolygon;
				MacintoshPattern pattern;
				if (my drawingArea) GuiMac_clipOn (my drawingArea);
				SetPort (my macPort);
				if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0)
					RGBForeColor (& my macColour);
				macpolygon = OpenPoly ();
				MoveTo (xDC + cos ((angle + 160) * NUMpi / 180) * size, yDC - sin ((angle + 160) * NUMpi / 180) * size);
				LineTo (xDC, yDC);
				LineTo (xDC + cos ((angle - 160) * NUMpi / 180) * size, yDC - sin ((angle - 160) * NUMpi / 180) * size);
				LineTo (xDC + cos ((angle - 180) * NUMpi / 180) * 0.7 * size, yDC - sin ((angle - 180) * NUMpi / 180) * 0.7 * size);
				ClosePoly ();
				FillPoly (macpolygon, GetQDGlobalsBlack (& pattern));
				KillPoly (macpolygon);
				if (my macColour.red != 0 || my macColour.green != 0 || my macColour.blue != 0)
					RGBForeColor (& theBlackColour);
				if (my drawingArea) GuiMac_clipOff ();
			}
		#endif
	} else if (my postScript) {
		iam (GraphicsPostscript);
		long length = my resolution * my arrowSize / 10, radius = my resolution * my arrowSize / 30;
		my printf (my file, "gsave %ld %ld translate %f rotate\n"
			"N 0 0 M -%ld 0 %ld -60 60 arc closepath fill grestore\n", xDC, yDC, angle, length, radius);
	}
}

void Graphics_arrow (I, double x1WC, double y1WC, double x2WC, double y2WC) {
	iam (Graphics);
	double angle = (180.0 / NUMpi) * atan2 ((wdy (y2WC) - wdy (y1WC)) * (my yIsZeroAtTheTop ? -1 : 1), wdx (x2WC) - wdx (x1WC));
	double size = my screen ? 10.0 * my resolution * my arrowSize / 72.0 : my resolution * my arrowSize / 10;
	long xyDC [4];
	xyDC [0] = wdx (x1WC);
	xyDC [1] = wdy (y1WC);
	xyDC [2] = wdx (x2WC) + (my screen ? 0.7 : 0.6) * cos ((angle - 180) * NUMpi / 180) * size;
	xyDC [3] = wdy (y2WC) + (my yIsZeroAtTheTop ? -1.0 : 1.0) * (my screen ? 0.7 : 0.6) * sin ((angle - 180) * NUMpi / 180) * size;
	polyline (me, 2, xyDC);
	arrowHead (me, wdx (x2WC), wdy (y2WC), angle);
	if (my recording) { op (ARROW, 4); put (x1WC); put (y1WC); put (x2WC); put (y2WC); }
}

void Graphics_doubleArrow (I, double x1WC, double y1WC, double x2WC, double y2WC) {
	iam (Graphics);
	double angle = (180.0 / NUMpi) * atan2 ((wdy (y2WC) - wdy (y1WC)) * (my yIsZeroAtTheTop ? -1 : 1), wdx (x2WC) - wdx (x1WC));
	double size = my screen ? 10.0 * my resolution * my arrowSize / 72.0 : my resolution * my arrowSize / 10;
	long xyDC [4];
	xyDC [0] = wdx (x1WC) + (my screen ? 0.7 : 0.6) * cos (angle * NUMpi / 180) * size;
	xyDC [1] = wdy (y1WC) + (my yIsZeroAtTheTop ? -1.0 : 1.0) * (my screen ? 0.7 : 0.6) * sin (angle * NUMpi / 180) * size;
	xyDC [2] = wdx (x2WC) + (my screen ? 0.7 : 0.6) * cos ((angle - 180) * NUMpi / 180) * size;
	xyDC [3] = wdy (y2WC) + (my yIsZeroAtTheTop ? -1.0 : 1.0) * (my screen ? 0.7 : 0.6) * sin ((angle - 180) * NUMpi / 180) * size;
	polyline (me, 2, xyDC);
	arrowHead (me, wdx (x1WC), wdy (y1WC), angle + 180);
	//polyline (me, 2, xyDC);
	arrowHead (me, wdx (x2WC), wdy (y2WC), angle);
	if (my recording) { op (DOUBLE_ARROW, 4); put (x1WC); put (y1WC); put (x2WC); put (y2WC); }
}

void Graphics_arcArrow (I, double xWC, double yWC, double rWC,
	double fromAngle, double toAngle, int arrowAtStart, int arrowAtEnd)
{
	iam (Graphics);
	arc (me, wdx (xWC), wdy (yWC), my scaleX * rWC, fromAngle, toAngle);
	if (arrowAtStart) arrowHead (me,
		wdx (xWC + rWC * cos (fromAngle * (NUMpi / 180.0))),
		wdy (yWC + rWC * sin (fromAngle * (NUMpi / 180.0))), fromAngle - 90);
	if (arrowAtEnd) arrowHead (me,
		wdx (xWC + rWC * cos (toAngle * (NUMpi / 180.0))),
		wdy (yWC + rWC * sin (toAngle * (NUMpi / 180.0))), toAngle + 90);
	if (my recording)
		{ op (ARC_ARROW, 7); put (xWC); put (yWC); put (rWC);
		  put (fromAngle); put (toAngle); put (arrowAtStart); put (arrowAtEnd); }
}

/* Output attributes. */

void Graphics_setLineType (I, int lineType) {
	iam (Graphics);
	my lineType = lineType;
	if (my recording) { op (SET_LINE_TYPE, 1); put (lineType); }
}

void Graphics_setLineWidth (I, double lineWidth) {
	iam (Graphics);
	my lineWidth = lineWidth;
	if (my recording) { op (SET_LINE_WIDTH, 1); put (lineWidth); }
}

void Graphics_setArrowSize (I, double arrowSize) {
	iam (Graphics);
	my arrowSize = arrowSize;
	if (my recording) { op (SET_ARROW_SIZE, 1); put (arrowSize); }
}

/* Inquiries. */

int Graphics_inqLineType (I) { iam (Graphics); return my lineType; }
double Graphics_inqLineWidth (I) { iam (Graphics); return my lineWidth; }
double Graphics_inqArrowSize (I) { iam (Graphics); return my arrowSize; }

/* End of file Graphics_linesAndAreas.cpp */
