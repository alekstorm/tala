#ifndef _CCs_to_DTW_h_
#define _CCs_to_DTW_h_
/* CCs_to_DTW.h
 *
 *	Dynamic Time Warp of two CCs.
 * 
 * Copyright (C) 1993-2002 David Weenink
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
 djmw 20020315 GPL header
 */

#ifndef _CC_h_
	#include "CC.h"
#endif

#ifndef _DTW_h_
	#include "DTW.h"
#endif

DTW CCs_to_DTW (I, thou, double wc, double wle, double wr, double wer,
	double dtr, int matchStart, int matchEnd, int constraint);
/*
	1. Calculate distances between CCs:
		Distance between frame i (from me) and j (from thee) is
		wc * d1 + wle * d2 + wr * d3 + wer * d4,
			where wc, wle, wr & wer are weights and
			d1 = Sum (k=1; k=nCoefficients; (c[i,k]-c[j,k])^2)
			d2 = (c[0,k]-c[0,k])^2
			d3 = Sum (k=1; k=nCoefficients; (r[i,k]-r[j,k])^2), with
				r[i,k] the regression coefficient of the cepstral coefficients
				from the frames within a time span of 'dtr' seconds.
				c[i,j] is jth cepstral coefficient in frame i.
			d4 = regression on energy (c[0])
	2. Find optimum path through the distance matrix (see DTW).
	
	PRECONDITIONS:
	
	at least one of wc, wle, wr, wer != 0
*/

#endif /* _CCs_to_DTW_h_ */
