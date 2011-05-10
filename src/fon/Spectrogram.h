#ifndef _Spectrogram_h_
#define _Spectrogram_h_
/* Spectrogram.h
 *
 * Copyright (C) 1992-2011 David Weenink & Paul Boersma
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

#define Spectrogram_members  Matrix_members
#define Spectrogram_methods  Matrix_methods
class_create (Spectrogram, Matrix);

/* Attributes:
	xmin			// Start time (seconds).
	xmax		// End time (seconds).
	nx			// Number of time slices.
	dx			// The time between two subsequent time slices.
	x1			// The centre of the first time slice.
	ymin			// Minimum frequency (Hertz).
	ymax		// Maximum frequency (Hertz).
	dy			// Frequency step (Hertz).
	y1			// Centre of first frequency band (Hertz).
	z [iy] [ix]		// Power spectrum density.
*/

Any Spectrogram_create (double tmin, double tmax, long nt, double dt, double t1,
					double fmin, double fmax, long nf, double df, double f1);
/*
	Function:
		Create the spectrogram data structure.
	Preconditions:
		nt > 0;
		nf > 0;
		dt > 0.0;
		df > 0.0;
	Postconditions:
		result -> xmin = tmin;		result -> ymin = fmin;
		result -> xmax = tmax;		result -> ymax = fmax;
		result -> nx = nt;			result -> ny = nf;
		result -> dx = dt;			result -> dy = df;
		result -> x1 = t1;			result -> y1 = f1;
		result -> z [1..nf] [1..nt] = 0.0;
*/

Spectrogram Matrix_to_Spectrogram (I);
/*
	Create a Spectrogram from a Matrix,
	with deep copy of all its attributes, except class information and methods.
	Return NULL if out of memory.  
*/

Matrix Spectrogram_to_Matrix (I);
/*
	Create a Matrix from a Spectrogram,
	with deep copy of all its attributes, except class information and methods.
	Return NULL if out of memory.  
*/

#ifdef __cplusplus
	}
#endif

/* End of file Spectrogram.h */
#endif
