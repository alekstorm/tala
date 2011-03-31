#ifndef _Sound_to_MFCC_h_
#define _Sound_to_MFCC_h_
/* Sound_to_MFCC.h
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
 djmw 20010410
 djmw 20020813 GPL header
 djmw 20110307 Latest modification
*/

#ifndef _MFCC_h_
	#include "MFCC.h"
#endif

#ifndef _Sound_h_
	#include "Sound.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

MFCC Sound_to_MFCC (Sound me, long numberOfCoefficients, double analysisWidth,
	double dt, double f1_mel, double fmax_mel, double df_mel);

#ifdef __cplusplus
	}
#endif

#endif /* _Sound_to_MFCC_h_ */
