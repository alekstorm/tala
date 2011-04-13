#ifndef _Sound_and_LPC_h_
#define _Sound_and_LPC_h_
/* Sound_and_LPC.h
 *
 * Copyright (C) 1994-2011 David Weenink
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
 djmw 19971103
 djmw 20020812 GPL header
 djmw 20110307 Latest modification
*/

#ifndef _LPC_h_
	#include "LPC.h"
#endif
#ifndef _Sound_h_
	#include "fon/Sound.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

LPC Sound_to_LPC_auto (Sound me, int predictionOrder, double analysisWidth, double dt,
	double preEmphasisFrequency);

LPC Sound_to_LPC_covar (Sound me, int predictionOrder, double analysisWidth, double dt,
	double preEmphasisFrequency);

LPC Sound_to_LPC_burg (Sound me, int predictionOrder, double analysisWidth, double dt,
	double preEmphasisFrequency);

LPC Sound_to_LPC_marple (Sound me, int predictionOrder, double analysisWidth, double dt,
	double preEmphasisFrequency, double tol1, double tol2);

/*
 * Function:
 *	Calculate linear prediction coefficients according to following model:
 *  Minimize E(m) = Sum(n=n0;n=n1; (x[n] + Sum(k=1;k=m; a[k]*x[n-k])))
 * Method:
 *  The minimization is carried out by solving the equations:
 *  Sum(i=1;i=m; a[i]*c[i][k]) = -c[0][k] for k=1,2,...,m
 *  where c[i][k] = Sum(n=n0;n=n1;x[n-i]*x[n-k])
 *  1. Covariance:
 *		n0=m; n1 = N-1;
 *      c[i][k] is symmetric, positive semi-definite matrix
 *  	Markel&Gray, LP of Speech, page 221;
 *  2. Autocorrelation
 *		signal is zero outside the interval;
 *      n0=-infinity; n1=infinity
 *      c[i][k] symmetric, positive definite Toeplitz matrix
 *  	Markel&Gray, LP of Speech, page 219;
 * Preconditions:
 *	predictionOrder > 0;
 *  preEmphasisFrequency >= 0;
 *
 * Burg method: see Numerical recipes Chapter 13.
 *
 * Marple method: see Marple, L. (1980), A new autoregressive spectrum analysis
 *		algorithm, IEEE Trans. on ASSP 28, 441-453.
 *	tol1 : stop iteration when E(m) / E(0) < tol1
 *	tol2 : stop iteration when (E(m)-E(m-1)) / E(m-1) < tol2,
 */

int LPC_Frame_and_Sound_filterInverse (LPC_Frame me, Sound thee, int channel);
Sound LPC_and_Sound_filter (LPC me, Sound thee, int useGain);
/*
	E(z) = X(z)A(z),
	A(z) = 1 + Sum (k=1, k=m, a(k)z^-k);

	filter:
		given e & a, determine x;
		x(n) = e(n) - Sum (k=1, m, a(k)x(n-k))
	useGain determines whether the LPC-gain is used in the synthesis.
*/

int LPC_and_Sound_filterWithFilterAtTime_inline (LPC me, Sound thee, int channel, double time);
Sound LPC_and_Sound_filterWithFilterAtTime (LPC me, Sound thee, int channel, double time);

Sound LPC_and_Sound_filterInverse (LPC me, Sound thee);
/*
	E(z) = X(z)A(z),
	A(z) = 1 + Sum (k=1, k=m, a(k)z^-k);

	filter inverse:
		given x & a, determine e;
		e(n) = x(n) + Sum (k=1, m, a(k)x(n-k))
*/

Sound LPC_and_Sound_filterInverseWithFilterAtTime (LPC me, Sound thee, int channel, double time);
int LPC_and_Sound_filterInverseWithFilterAtTime_inline (LPC me, Sound thee, int channel, double time);

#ifdef __cplusplus
	}
#endif

#endif /* _Sound_and_LPC_h_ */
