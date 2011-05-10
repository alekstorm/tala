#ifndef _SPINET_h_
#define _SPINET_h_
/* SPINET.h
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
 djmw 19970408
 djmw 20020813 GPL header
 djmw 20110306 Latest modification.
*/

#ifndef _Sampled2_h_
	#include "Sampled2.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#define SPINET_members Sampled2_members			\
	long gamma			;						\
	double excitationErbProportion;				\
	double inhibitionErbProportion;				\
	double **y, **s;

#define SPINET_methods Sampled2_methods
class_create (SPINET, Sampled2);

SPINET SPINET_create (double tmin, double tmax, long nt, double dt, double t1,
	 double minimumFrequency, double maximumFrequency, long nFilters,
	 double excitationErbProportion, double inhibitionErbProportion);

#ifdef __cplusplus
	}
#endif

#endif /* _SPINET_h_ */
