#ifndef _DTW_h_
#define _DTW_h_
/* DTW.h
 *
 * Copyright (C) 1993-2011 David Weenink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 djmw 20020813 GPL header
 djmw 20110306 Latest modification.
*/

#ifndef _Spectrogram_h_
	#include "fon/Spectrogram.h"
#endif
#ifndef _Graphics_h_
	#include "ui/Graphics.h"
#endif
#ifndef _Polygon_h
	#include "fon/Polygon.h"
#endif
#ifndef _Pitch_h
	#include "fon/Pitch.h"
#endif
#ifndef _DurationTier_h
	#include "fon/DurationTier.h"
#endif
#ifndef _Sound_h
	#include "fon/Sound.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#include "DTW_def.h"
#define DTW_methods Matrix_methods
oo_CLASS_CREATE (DTW, Matrix);

#define DTW_SAKOECHIBA 1
#define DTW_SLOPES 2

#define DTW_UNREACHABLE 0
#define DTW_FORBIDDEN 1
#define DTW_START 3
#define DTW_XANDY 2
#define DTW_X 4
#define DTW_Y 6

int DTW_Path_Query_init (DTW_Path_Query me, long ny, long nx);

/* Prototype on y-axis and test on x-axis */
Any DTW_create (double tminp, double tmaxp, long ntp, double dtp, double t1p,
	double tminc, double tmaxc, long ntc, double dtc, double t1c);
	
DTW DTW_swapAxes (DTW me);

int DTW_pathFinder_band (DTW me, double adjustment_window_duration, int adjustment_window_includes_end, 
	 double costs_x, double costs_y, double costs_xandy);
int DTW_pathFinder_slopes (DTW me, long nsteps_xory, long nsteps_xandy, double costs_x, double costs_y, double costs_xandy);

void DTW_findPath (DTW me, int matchStart, int matchEnd, int slope);
/* Obsolete
	Function:
		Calculate the minimum path (through a distance matrix).
		 
	Possible constraints:
	
	matchStart:	path starts at position (1,1) (bottom-left)
	matchEnd:	path ends at (I,J) (top-right)
	slope:	1	no constraints
			2	1/3 < slope < 3
			3	1/2 < slope < 2
			4	2/3 < slope < 3/2
					
	For algorithm and DP-equations see the article (& especially table I) in:
	Sakoe, H. & Chiba, S. (1978), Dynamic Programming Algorithm Optimization
		for Spoken Word	recognition, IEEE Trans. on ASSP, vol 26, 43-49.
*/

void DTW_Path_recode (DTW me);

double DTW_getPathY (DTW me, double tx);
/*
	Get the time Y-time that corresponds to time t (along X).
*/
double DTW_getYTime (DTW me, double tx);
double DTW_getXTime (DTW me, double ty);

long DTW_getMaximumConsecutiveSteps (DTW me, int direction);

void DTW_paintDistances (DTW me, Any g, double xmin, double xmax, double ymin,
	double ymax, double minimum, double maximum, int garnish);

void DTW_drawPath (DTW me, Any g, double xmin, double xmax, double ymin,
	double ymax, int garnish);
void DTW_drawWarpX (DTW me, Graphics g, double xmin, double xmax, double ymin, double ymax, double tx, int garnish);	
void DTW_pathRemoveRedundantNodes (DTW me);
void DTW_pathQueryRecode (DTW me);

void DTW_drawDistancesAlongPath (DTW me, Any g, double xmin, double xmax,
	double dmin, double dmax, int garnish);
	
void DTW_and_Sounds_draw (DTW me, Sound yy, Sound xx, Graphics g, double xmin, double xmax, 
	double ymin, double ymax, int garnish);
void DTW_and_Sounds_drawWarpX (DTW me, Sound yy, Sound xx, Graphics g, double xmin, double xmax, 
	double ymin, double ymax, double tx, int garnish);
	
Polygon DTW_to_Polygon_band (DTW me, double adjustment_window_duration, int adjustment_window_includes_end);
Polygon DTW_to_Polygon_slopes (DTW me, long nsteps_xory, long nsteps_xandy);
	
Matrix DTW_distancesToMatrix (DTW me);

DTW Matrices_to_DTW (I, thou, int matchStart, int matchEnd, int slope, int metric);

DTW Spectrograms_to_DTW (Spectrogram me, Spectrogram thee, int matchStart,
	int matchEnd, int slope, int metric);

DTW Pitches_to_DTW (Pitch me, Pitch thee, double vuv_costs, double time_weight, int matchStart, int matchEnd, int slope);

DurationTier DTW_to_DurationTier (DTW me);

#ifdef __cplusplus
	}
#endif

#endif /* _DTW_h_ */
