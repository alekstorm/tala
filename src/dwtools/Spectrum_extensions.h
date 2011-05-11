#ifndef _Spectrum_extensions_h_
#define _Spectrum_extensions_h_
/* Spectrum_extensions.h
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
 djmw 20010114
 djmw 20020813 GPL header
 djmw 20110307 Latest modification
*/

#include "fon/Spectrum.h"
#include "fon/Sound.h"

#define SIGN(x,s) ((s) < 0 ? -fabs (x) : fabs(x))

#define THLCON 0.5
#define THLINC 1.5
#define EXP2   12

#define PPVPHA(x,y,test) ((test) ? atan2 (-(y),-(x)) : atan2 ((y),(x)))
#define PHADVT(xr,xi,yr,yi,xa) ((xa) > 0 ? ((xr)*(yr)+(xi)*(yi))/ (xa) : 0)

#ifdef __cplusplus
	extern "C" {
#endif

Matrix Spectrum_unwrap (Spectrum me);
/*
	Unwrap the phases of the spectrum according to an algorithm by
	Tribolet as published in:
	
	Tribolet, J.M. & Quatieri, T.F. (1979), Computation of the Complex
		Spectrum, in: Programs for Digital Signal Processing, 
		Digital Signal Processing Commitee (eds), IEEE Press,
		chapter 7.1.
		
	First row of returned matrix contains the amplitudes-squared,
	second row contains the unwrapped phases.
*/

Spectrum Spectra_multiply (Spectrum me, Spectrum thee);
void Spectrum_conjugate (Spectrum me);

#ifdef __cplusplus
	}
#endif

#endif /* _Spectrum_extensions_h_ */
