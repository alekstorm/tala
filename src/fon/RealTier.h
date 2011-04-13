#ifndef _RealTier_h_
#define _RealTier_h_
/* RealTier.h
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
 * pb 2011/03/03
 */

#include "AnyTier.h"
#include "Graphics.h"
#include "TableOfReal.h"
#include "Vector.h"

#ifdef __cplusplus
	extern "C" {
#endif

/********** class RealPoint **********/

#define RealPoint_members Data_members \
	double time; \
	double value;
#define RealPoint_methods Data_methods
class_create (RealPoint, Data);

RealPoint RealPoint_create (double time, double value);
/*
	Postconditions:
		result -> time == time;
		result -> value == value;
*/

/********** class RealTier **********/

#define RealTier_members Function_members \
	SortedSetOfDouble points;
#define RealTier_methods Function_methods
class_create (RealTier, Function);

void RealTier_init_e (I, double tmin, double tmax);
RealTier RealTier_create (double tmin, double tmax);
/*
	Postconditions:
		result -> xmin == tmin;
		result -> xmax == tmax;
		result -> points -> size == 0;
*/

double RealTier_getValueAtIndex (I, long point);
/* No points or 'point' out of range: NUMundefined. */

double RealTier_getValueAtTime (I, double t);
/* Inside points: linear intrapolation. */
/* Outside points: constant extrapolation. */
/* No points: NUMundefined. */

double RealTier_getMinimumValue (I);
double RealTier_getMaximumValue (I);
double RealTier_getArea (I, double tmin, double tmax);
double RealTier_getMean_curve (I, double tmin, double tmax);
double RealTier_getMean_points (I, double tmin, double tmax);
double RealTier_getStandardDeviation_curve (I, double tmin, double tmax);
double RealTier_getStandardDeviation_points (I, double tmin, double tmax);

int RealTier_addPoint (I, double t, double value);
void RealTier_draw (I, Graphics g, double tmin, double tmax,
	double ymin, double ymax, int garnish, const wchar_t *method, const wchar_t *quantity);
TableOfReal RealTier_downto_TableOfReal (I, const wchar_t *timeLabel, const wchar_t *valueLabel);

int RealTier_interpolateQuadratically (I, long numberOfPointsPerParabola, int logarithmically);

Table RealTier_downto_Table (I, const wchar_t *indexText, const wchar_t *timeText, const wchar_t *valueText);
RealTier Vector_to_RealTier (I, long channel);
RealTier Vector_to_RealTier_peaks (I, long channel);
RealTier Vector_to_RealTier_valleys (I, long channel);
RealTier PointProcess_upto_RealTier (PointProcess me, double value);

int RealTier_formula (I, const wchar_t *expression, Interpreter *interpreter, thou);
void RealTier_multiplyPart (I, double tmin, double tmax, double factor);
void RealTier_removePointsBelow (RealTier me, double level);

#ifdef __cplusplus
	}
#endif

/* End of file RealTier.h */
#endif
