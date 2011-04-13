/* Art_Speaker.cpp
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
 * pb 1996/09/04 latest change
 * pb 2002/07/16 GPL
 * pb 2007/08/12 wchar_t
 * pb 2008/01/19 double
 * pb 2009/03/21 modern enums
 * pb 2011/03/22 C++
 */

#include "Art_Speaker.h"

#define DLIP  5e-3

void Art_Speaker_toVocalTract (Art _art, Speaker speaker,
	double intX [], double intY [], double extX [], double extY [],
	double *bodyX, double *bodyY)
{
	double *art = _art -> art;
	double f = speaker -> relativeSize * 1e-3;
	struct { double x, y, da; } jaw;
	struct { double dx, dy; } hyoid;
	struct { double x, y, r, radius; } body;
	struct { double x, y, r, a; } teeth;
	struct { double a; } blade;
	struct { double x, y, a; } tip;
	struct { double dx, dy; } lowerLip, upperLip;
	double HBody_x, HBody_y, HC, Sp, p, a, b;

	/* Determine the position of the hyoid bone (Mermelstein's H).	*/
	/* The rest position is a characteristic of the speaker.		*/
	/* The stylohyoid muscle pulls the hyoid bone up.			*/
	/* The sternohyoid muscle pulls the hyoid bone down.			*/
	/* The sphincter muscle pulls the hyoid bone backwards.		*/

	hyoid.dx = -5 * f * art [kArt_muscle_SPHINCTER];
	hyoid.dy = 20 * f * (art [kArt_muscle_STYLOHYOID]
		- art [kArt_muscle_STERNOHYOID]);

	/* The larynx moves up and down with the hyoid bone.			*/
	/* Only the lowest point (Mermelstein's K)				*/
	/* does not follow completely the horizontal movements.		*/

	/* Anterior larynx. */
	intX [1] = -14 * f + 0.5 * hyoid.dx;		intY [1] = -53 * f + hyoid.dy;
	/* Top of larynx. */
	intX [2] = -20 * f + hyoid.dx;		intY [2] = -33 * f + hyoid.dy;
	/* Epiglottis. */
	intX [3] = -20 * f + hyoid.dx;		intY [3] = -26 * f + hyoid.dy;
	/* Hyoid bone. */
	intX [4] = -16 * f + hyoid.dx;		intY [4] = -26 * f + hyoid.dy;
	/* Posterior larynx. */
	extX [1] = -22 * f + hyoid.dx;		extY [1] = -53 * f + hyoid.dy;
	/* Esophagus. */
	extX [2] = -26 * f + hyoid.dx;		extY [2] = -40 * f + hyoid.dy;

	/* The lower pharynx moves up and down with the hyoid bone.			*/
	/* The lower constrictor muscle pulls the rear pharyngeal wall forwards.	*/

	extX [3] = -34 * f + art [kArt_muscle_SPHINCTER] * 5 * f;	extY [3] = extY [2];

	/* The upper pharynx is fixed at the height of the velum. */
	/* The upper constrictor muscle pulls the rear pharyngeal wall forwards. */

	extX [5] = -34 * f + art [kArt_muscle_SPHINCTER] * 5 * f;
	extY [5] = speaker -> velum.y;

	/* The height of the middle pharynx is in between the lower and upper pharynx. */
	/* The middle constrictor muscle pulls the rear pharyngeal wall forwards. */

	extX [4] = -34 * f + art [kArt_muscle_SPHINCTER] * 5 * f;
	extY [4] = 0.5 * (extY [3] + extY [5]);

	/* Tongue root. */

	jaw.x = -75 * f, jaw.y = 53 * f;   /* Position of the condyle. */
	jaw.da = art [kArt_muscle_MASSETER] * 0.15
		- art [kArt_muscle_MYLOHYOID] * 0.20;
	body.x = jaw.x + 81 * f * cos (-0.60 + jaw.da)
		- art [kArt_muscle_STYLOGLOSSUS] * 10 * f
		+ art [kArt_muscle_GENIOGLOSSUS] * 10 * f;
	body.y = jaw.y + 81 * f * sin (-0.60 + jaw.da)
		- art [kArt_muscle_HYOGLOSSUS] * 10 * f
		+ art [kArt_muscle_STYLOGLOSSUS] * 5 * f;
	*bodyX = body.x;
	*bodyY = body.y;
	body.r = sqrt ((jaw.x - body.x) * (jaw.x - body.x)
					 + (jaw.y - body.y) * (jaw.y - body.y));
	body.radius = 20 * f;
	HBody_x = body.x - intX [4];
	HBody_y = body.y - intY [4];
	HC = sqrt (HBody_x * HBody_x + HBody_y * HBody_y);
	if (HC <= body.radius) {
		HC = body.radius;
		Sp = 0.0;   /* 19980228 PowerPC processor BUG...  Needed with the 603e at least:
			00000344: CBC100B0  lfd      fp30,176(SP)          HC := body.radius
			00000348: C94100B0  lfd      fp10,176(SP)          fp10 := body.radius
			0000034C: C96100B0  lfd      fp11,176(SP)          fp11 := body.radius
			00000350: FD4A02F2  fmul     fp10,fp10,fp11          fp10 := body.radius ^ 2
			00000354: FC3E57B8  fmsub    fp1,fp30,fp30,fp10          fp1 := HC * HC - fp10   (should be zero)
			00000358: 48000001  bl       .sqrt          gives NAN(001) !!!
		*/
	} else {
		Sp = sqrt (HC * HC - body.radius * body.radius);
	}
	a = atan2 (HBody_y, HBody_x);
	b = asin (body.radius / HC);
	p = 0.57 * (34.8 * f - Sp);
	intX [5] = intX [4] + 0.5 * Sp * cos (a + b) - p * sin (a + b);
	intY [5] = intY [4] + 0.5 * Sp * sin (a + b) + p * cos (a + b);
	HBody_x = body.x - intX [5];
	HBody_y = body.y - intY [5];
	HC = sqrt (HBody_x * HBody_x + HBody_y * HBody_y);
	if (HC <= body.radius) { HC = body.radius; Sp = 0.0; } else Sp = sqrt (HC * HC - body.radius * body.radius);
	a = atan2 (HBody_y, HBody_x);
	b = asin (body.radius / HC);
	intX [6] = intX [5] + Sp * cos (a + b);
	intY [6] = intY [5] + Sp * sin (a + b);

	/* Posterior blade. */

	teeth.a = speaker -> lowerTeeth.a + jaw.da;
	intX [7] = body.x + body.radius * cos (1.73 + teeth.a);
	intY [7] = body.y + body.radius * sin (1.73 + teeth.a);

	/* Tip. */

	tip.a = (art [kArt_muscle_UPPER_TONGUE]
		- art [kArt_muscle_LOWER_TONGUE]) * 1.0;
	blade.a = teeth.a
		+ 0.004 * (body.r - speaker -> neutralBodyDistance) + tip.a;
	intX [8] = intX [7] + speaker -> tip.length * cos (blade.a);
	intY [8] = intY [7] + speaker -> tip.length * sin (blade.a);

	/* Jaw. */

	teeth.r = speaker -> lowerTeeth.r;
	teeth.x = jaw.x + teeth.r * cos (teeth.a);
	teeth.y = jaw.y + teeth.r * sin (teeth.a);
	intX [9] = teeth.x + speaker -> teethCavity.dx1;
	intY [9] = teeth.y + speaker -> teethCavity.dy;
	intX [10] = teeth.x + speaker -> teethCavity.dx2;
	intY [10] = intY [9];
	intX [11] = teeth.x;
	intY [11] = teeth.y;

	/* Lower lip. */

	lowerLip.dx = speaker -> lowerLip.dx + art [kArt_muscle_ORBICULARIS_ORIS] * 0.02 - 5e-3;
	lowerLip.dy = speaker -> lowerLip.dy + art [kArt_muscle_ORBICULARIS_ORIS] * 0.01;
	intX [12] = teeth.x;
	intY [12] = teeth.y + lowerLip.dy;
	intX [13] = teeth.x + lowerLip.dx;
	intY [13] = intY [12];

	/* Velum. */

	extX [6] = speaker -> velum.x;
	extY [6] = speaker -> velum.y;

	/* Palate. */

	extX [7] = speaker -> alveoli.x;
	extY [7] = speaker -> alveoli.y;
	extX [8] = speaker -> upperTeeth.x;
	extY [8] = speaker -> upperTeeth.y;

	/* Upper lip. */

	upperLip.dx = speaker -> upperLip.dx + art [kArt_muscle_ORBICULARIS_ORIS] * 0.02 - 5e-3;
	upperLip.dy = speaker -> upperLip.dy - art [kArt_muscle_ORBICULARIS_ORIS] * 0.01;
	extX [9] = extX [8];
	extY [9] = extY [8] + upperLip.dy;
	extX [10] = extX [9] + upperLip.dx;
	extY [10] = extY [9];
	extX [11] = extX [10] + 5e-3;
	extY [11] = extY [10] + DLIP;

	/* Chin. */

	intX [14] = intX [13] + 5e-3;
	intY [14] = intY [13] - DLIP;
	intX [15] = intX [11] + 0.5e-2;
	intY [15] = intY [11] - 3.0e-2;
	intX [16] = intX [1];
	intY [16] = intY [1];
}

void Art_Speaker_draw (Art art, Speaker speaker, Graphics g) {
	double f = speaker -> relativeSize * 1e-3;
	double intX [1 + 16], intY [1 + 16], extX [1 + 11], extY [1 + 11];
	double bodyX, bodyY;
	int i;
	Graphics_Viewport previous;

	Art_Speaker_toVocalTract (art, speaker, intX, intY, extX, extY, & bodyX, & bodyY);
	previous = Graphics_insetViewport (g, 0.1, 0.9, 0.1, 0.9);
	Graphics_setWindow (g, -0.05, 0.05, -0.05, 0.05);

	/* Draw inner contour. */

	for (i = 1; i <= 5; i ++)
		Graphics_line (g, intX [i], intY [i], intX [i + 1], intY [i + 1]);
	Graphics_arc (g, bodyX, bodyY, 20 * f,
		atan2 (intY [7] - bodyY, intX [7] - bodyX) * 180 / NUMpi,
		atan2 (intY [6] - bodyY, intX [6] - bodyX) * 180 / NUMpi);
	for (i = 7; i <= 15; i ++)
		Graphics_line (g, intX [i], intY [i], intX [i + 1], intY [i + 1]);

	/* Draw outer contour. */

	for (i = 1; i <= 5; i ++)
		Graphics_line (g, extX [i], extY [i], extX [i + 1], extY [i + 1]);
	Graphics_arc (g, 0, 0, speaker -> palate.radius,
		speaker -> alveoli.a * 180 / NUMpi,
		speaker -> velum.a * 180 / NUMpi);
	for (i = 7; i <= 10; i ++)
		Graphics_line (g, extX [i], extY [i], extX [i + 1], extY [i + 1]);
	Graphics_resetViewport (g, previous);
}

void Art_Speaker_fillInnerContour (Art art, Speaker speaker, Graphics g) {
	double f = speaker -> relativeSize * 1e-3;
	double intX [1 + 16], intY [1 + 16], extX [1 + 11], extY [1 + 11];
	double x [1 + 16], y [1 + 16];
	double bodyX, bodyY;
	int i;
	Graphics_Viewport previous;

	Art_Speaker_toVocalTract (art, speaker, intX, intY, extX, extY, & bodyX, & bodyY);
	previous = Graphics_insetViewport (g, 0.1, 0.9, 0.1, 0.9);
	Graphics_setWindow (g, -0.05, 0.05, -0.05, 0.05);
	for (i = 1; i <= 16; i ++) { x [i] = intX [i]; y [i] = intY [i]; }
	Graphics_setGrey (g, 0.8);
	Graphics_fillArea (g, 16, & x [1], & y [1]);
	Graphics_fillCircle (g, bodyX, bodyY, 20 * f);
	Graphics_setGrey (g, 0.0);
	Graphics_resetViewport (g, previous);
}

static double arcLength (double from, double to) {
	double result = to - from;
	while (result > 0.0) result -= 2 * NUMpi;
	while (result < 0.0) result += 2 * NUMpi;
	return result;
}

static int Art_Speaker_meshCount = 27;
static double bodyX, bodyY, bodyRadius;

static double toLine (double x, double y, const double intX [], const double intY [], int i) {
	int nearby;
	if (i == 6) {
		double a7 = atan2 (intY [7] - bodyY, intX [7] - bodyX);
		double a6 = atan2 (intY [6] - bodyY, intX [6] - bodyX);
		double a = atan2 (y - bodyY, x - bodyX);
		double da6 = arcLength (a7, a6);
		double da = arcLength (a7, a);
		if (da <= da6)
			return fabs (sqrt ((bodyX - x) * (bodyX - x) + (bodyY - y) * (bodyY - y)) - bodyRadius);
		else
			nearby = arcLength (a7 + 0.5 * da6, a) < NUMpi ? 6 : 7;
	} else if ((x - intX [i]) * (intX [i + 1] - intX [i]) +
				(y - intY [i]) * (intY [i + 1] - intY [i]) < 0) {
		nearby = i;
	} else if ((x - intX [i + 1]) * (intX [i] - intX [i + 1]) +
				(y - intY [i + 1]) * (intY [i] - intY [i + 1]) < 0) {
		nearby = i + 1;
	} else {
		double boundaryDistance =
			sqrt ((intX [i + 1] - intX [i]) * (intX [i + 1] - intX [i]) +
					(intY [i + 1] - intY [i]) * (intY [i + 1] - intY [i]));
		double outerProduct = (intX [i] - x) * (intY [i + 1] - intY [i]) - (intY [i] - y) * (intX [i + 1] - intX [i]);
		return fabs (outerProduct) / boundaryDistance;
	}
	return sqrt ((intX [nearby] - x) * (intX [nearby] - x) + (intY [nearby] - y) * (intY [nearby] - y));
}

static int inside (double x, double y,
	const double intX [], const double intY [])
{
	int i, up = 0;
	for (i = 1; i <= 16 - 1; i ++)
		if ((y > intY [i]) != (y > intY [i + 1])) {
			double slope = (intX [i + 1] - intX [i]) / (intY [i + 1] - intY [i]);
			if (x > intX [i] + (y - intY [i]) * slope)
				up += ( y > intY [i] ? 1 : -1 );
		}
	return up != 0 || bodyRadius * bodyRadius >
		(x - bodyX) * (x - bodyX) + (y - bodyY) * (y - bodyY);
}

void Art_Speaker_meshVocalTract (Art art, Speaker speaker,
	double xi [], double yi [], double xe [], double ye [],
	double xmm [], double ymm [], int closed [])
{
	double f = speaker -> relativeSize * 1e-3;
	double intX [1 + 16], intY [1 + 16], extX [1 + 11], extY [1 + 11], d_angle;
	double xm [40], ym [40];
	int i;

	Art_Speaker_toVocalTract (art, speaker, intX, intY, extX, extY, & bodyX, & bodyY);
	bodyRadius = 20 * f;

	xe [1] = extX [1];   /* Eq. 5.45 */
	ye [1] = extY [1];
	xe [2] = 0.2 * extX [2] + 0.8 * extX [1];
	ye [2] = 0.2 * extY [2] + 0.8 * extY [1];
	xe [3] = 0.6 * extX [2] + 0.4 * extX [1];
	ye [3] = 0.6 * extY [2] + 0.4 * extY [1];
	xe [4] = 0.9 * extX [3] + 0.1 * extX [4];   /* Eq. 5.46 */
	ye [4] = 0.9 * extY [3] + 0.1 * extY [4];
	xe [5] = 0.7 * extX [3] + 0.3 * extX [4];
	ye [5] = 0.7 * extY [3] + 0.3 * extY [4];
	xe [6] = 0.5 * extX [3] + 0.5 * extX [4];
	ye [6] = 0.5 * extY [3] + 0.5 * extY [4];
	xe [7] = 0.3 * extX [3] + 0.7 * extX [4];
	ye [7] = 0.3 * extY [3] + 0.7 * extY [4];
	xe [8] = 0.1 * extX [3] + 0.9 * extX [4];
	ye [8] = 0.1 * extY [3] + 0.9 * extY [4];
	xe [9] = 0.9 * extX [4] + 0.1 * extX [5];
	ye [9] = 0.9 * extY [4] + 0.1 * extY [5];
	xe [10] = 0.7 * extX [4] + 0.3 * extX [5];
	ye [10] = 0.7 * extY [4] + 0.3 * extY [5];
	xe [11] = 0.5 * extX [4] + 0.5 * extX [5];
	ye [11] = 0.5 * extY [4] + 0.5 * extY [5];
	xe [12] = 0.3 * extX [4] + 0.7 * extX [5];
	ye [12] = 0.3 * extY [4] + 0.7 * extY [5];
	xe [13] = 0.1 * extX [4] + 0.9 * extX [5];
	ye [13] = 0.1 * extY [4] + 0.9 * extY [5];
	d_angle = (atan2 (ye [13], xe [13]) - 0.5 * NUMpi) / 6;   /* Eq. 5.47 */
	for (i = 14; i <= 18; i ++) {
		double a = 0.5 * NUMpi + (19 - i) * d_angle;
		xe [i] = speaker -> palate.radius * cos (a);
		ye [i] = speaker -> palate.radius * sin (a);
	}
	xe [19] = 0;
	ye [19] = speaker -> palate.radius;
	xe [20] = 0.25 * extX [7];
	xe [21] = 0.50 * extX [7];
	xe [22] = 0.75 * extX [7];
	for (i = 20; i <= 22; i ++) {
		ye [i] = speaker -> palate.radius * sqrt (1.0 - xe [i] * xe [i] /
			(speaker -> palate.radius * speaker -> palate.radius));
	}
	xe [23] = extX [7];
	ye [23] = extY [7];
	xe [24] = 0.5 * (extX [7] + extX [8]);
	ye [24] = 0.5 * (extY [7] + extY [8]);
	xe [25] = extX [8];
	ye [25] = extY [8];
	xe [26] = 0.25 * extX [11] + 0.75 * extX [9];
	xe [27] = 0.75 * extX [11] + 0.25 * extX [9];
	ye [26] = extY [10];
	ye [27] = 0.5 * (extY [10] + extY [11]);
	for (i = 1; i <= 27; i ++) {   /* Every mesh point. */
		double minimum = 100000;
		int j;
		for (j = 1; j <= 15 - 1; j ++) {   /* Every internal segment. */
			double d = toLine (xe [i], ye [i], intX, intY, j);
			if (d < minimum) minimum = d;
		}
		if ((closed [i] = inside (xe [i], ye [i], intX, intY)) != 0)
			minimum = - minimum;
		if (xe [i] >= 0.0) {   /* Vertical line pieces. */
			xi [i] = xe [i];
			yi [i] = ye [i] - minimum;
		} else if (ye [i] <= 0.0) {   /* Horizontal line pieces. */
			xi [i] = xe [i] + minimum;
			yi [i] = ye [i];
		} else {   /* Radial line pieces, centre = centre of palate arc. */
			double angle = atan2 (ye [i], xe [i]);
			xi [i] = xe [i] - minimum * cos (angle);
			yi [i] = ye [i] - minimum * sin (angle);
		}
	}
	for (i = 1; i <= Art_Speaker_meshCount; i ++) {
		xm [i] = 0.5 * (xe [i] + xi [i]);
		ym [i] = 0.5 * (ye [i] + yi [i]);
	}
	for (i = 2; i <= Art_Speaker_meshCount; i ++) {
		xmm [i] = 0.5 * (xm [i - 1] + xm [i]);
		ymm [i] = 0.5 * (ym [i - 1] + ym [i]);
	}
	xmm [1] = 2 * xm [1] - xmm [2];
	ymm [1] = 2 * ym [1] - ymm [2];
	xmm [Art_Speaker_meshCount + 1] = 2 * xm [Art_Speaker_meshCount]
		- xmm [Art_Speaker_meshCount];
	ymm [Art_Speaker_meshCount + 1] = 2 * ym [Art_Speaker_meshCount]
		- ymm [Art_Speaker_meshCount];
}

void Art_Speaker_drawMesh (Art art, Speaker speaker, Graphics graphics) {
	double xi [40], yi [40], xe [40], ye [40], xmm [40], ymm [40];
	int closed [40];
	int i;
	Graphics_Viewport previous;
	int oldLineType = Graphics_inqLineType (graphics);
	Art_Speaker_meshVocalTract (art, speaker, xi, yi, xe, ye, xmm, ymm, closed);
	previous = Graphics_insetViewport (graphics, 0.1, 0.9, 0.1, 0.9);   /* Must be square. */
	Graphics_setWindow (graphics, -0.05, 0.05, -0.05, 0.05);

	/* Mesh lines. */
	for (i = 1; i <= Art_Speaker_meshCount; i ++)
		Graphics_line (graphics, xi [i], yi [i], xe [i], ye [i]);

	/* Radii. */
	Graphics_setLineType (graphics, Graphics_DOTTED);
	for (i = 1; i <= Art_Speaker_meshCount; i ++)
		if (xe [i] <= 0.0 && ye [i] >= 0.0)
			Graphics_line (graphics, 0.0, 0.0, 0.9 * xi [i], 0.9 * yi [i]);
	Graphics_setLineType (graphics, oldLineType);

	/* Lengths. */
	for (i = 1; i <= Art_Speaker_meshCount; i ++)
		Graphics_line (graphics, xmm [i], ymm [i], xmm [i + 1], ymm [i + 1]);

	for (i = 1; i <= Art_Speaker_meshCount + 1; i ++)
		Graphics_fillCircle_mm (graphics, xmm [i], ymm [i], 1.0);
	Graphics_setTextAlignment (graphics, Graphics_LEFT, Graphics_HALF);
	Graphics_text (graphics, 0.0, 0.0, L"O");   /* Origin. */
	Graphics_resetViewport (graphics, previous);
}

/* End of file Art_Speaker.cpp */
