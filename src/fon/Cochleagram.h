#ifndef _Cochleagram_h_
#define _Cochleagram_h_
/* Cochleagram.h
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

#include "Matrix.h"

#ifdef __cplusplus
	extern "C" {
#endif

#define Cochleagram_members  Matrix_members
#define Cochleagram_methods  Matrix_methods
class_create (Cochleagram, Matrix);

/* Normally, the attributes will meet the following:
	xmin;			// Start time (seconds).
	xmax;			// End time (seconds).
	nx;				// Number of time slices.
	dx;				// Time step (seconds).
	x1;				// Centre of first time sample (seconds).
	ymin = 0;			// Minimum frequency (Bark).
	ymax = 25.6;		// Maximum frequency (Bark).
	ny;				// Number of frequencies.
	dy = 25.6 / ny;		// Frequency step (Bark).
	y1 = 0.5 * dy;		// Centre of first frequency band (Bark).
	z;				// Basilar filter output (milliVolt), or firing rate (Hertz), or intensity (phon).
*/

Cochleagram Cochleagram_create (double tmin, double tmax, long nt, double dt, double t1,
	double df, long nf);
/*
	Function:
		return a new instance of Cochleagram, or NULL if out of memory.
	Preconditions:
		dt > 0.0;						df > 0.0;
		nt >= 1;						nf >= 1;
	Postconditions:
		result -> xmin == tmin;			result -> ymin == 0.0;
		result -> xmax == tmax;			result -> ymax == 25.6;
		result -> nx == nt;				result -> ny == nf;
		result -> dx == dt;				result -> dy == df;
		result -> x1 == t1;				result -> y1 == 0.5 * df;
		result -> z [1..nf] [1..nt] == 0.0;
*/

double Cochleagram_difference (Cochleagram me, Cochleagram thee, double tmin, double tmax);

Cochleagram Matrix_to_Cochleagram (Matrix me);
/*
	Function:
		create a Cochleagram from a Matrix,
		with deep copy of all its attributes, except class information and methods.
	Return NULL if out of memory.  
*/

Matrix Cochleagram_to_Matrix (Cochleagram me);
/*
	Function:
		create a Matrix from a Cochleagram,
		with deep copy of all its attributes, except class information and methods.
	Return NULL if out of memory.  
*/

#ifdef __cplusplus
	}
#endif

/* End of file Cochleagram.h */
#endif
