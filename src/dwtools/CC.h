#ifndef _CC_h_
#define _CC_h_
/* CC.h
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
 djmw 20010219 Cepstral Coefficients (abstract) class.
 djmw 20020402 GPL header
 djmw 20030612 Include CC_def.h
 djmw 20110306 Latest modification.
*/

#ifndef _Matrix_h_
	#include "fon/Matrix.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#include "CC_def.h"
#define CC_members Sampled_members \
        double fmin; \
        double fmax; \
        long maximumNumberOfCoefficients; \
        CC_Frame frame;
#define CC_methods Sampled_methods
oo_CLASS_CREATE (CC, Sampled);

int CC_init (I, double tmin, double tmax, long nt, double dt, double t1,
	long maximumNumberOfCoefficients, double fmin, double fmax);
	
void CC_getNumberOfCoefficients_extrema (I, long startframe, 
	long endframe, long *min, long *max);
	
long CC_getMinimumNumberOfCoefficients (I, long startframe, long endframe);

long CC_getMaximumNumberOfCoefficients (I, long startframe, long endframe);

Matrix CC_to_Matrix (I);

double CC_getValue (I, double t, long index);

/******************* Frames ************************************************/

int CC_Frame_init (CC_Frame me, long numberOfCoefficients);

#ifdef __cplusplus
	}
#endif

#endif /* _CC_h_ */
