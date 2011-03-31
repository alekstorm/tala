/* Sound_to_Formant.c
 *
 * Copyright (C) 1992-2010 Paul Boersma
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
 * pb 2002/07/16 GPL
 * pb 2003/05/15 replaced memcof with NUMburg
 * pb 2003/09/18 default time step is 4 times oversampling
 * pb 2006/05/10 better handling of interruption in Sound_to_Formant
 * pb 2006/05/10 better handling of NULL from Polynomial_to_Roots
 * pb 2007/01/26 made compatible with stereo Sounds
 * pb 2007/03/30 changed float to double (against compiler warnings)
 * pb 2010/12/13 removed some style bugs
 */

#include "Sound_to_Formant.h"
#include "NUM2.h"
#include "Polynomial.h"

static int burg (double sample [], long nsamp_window, double cof [], int nPoles,
	Formant_Frame frame, double nyquistFrequency, double safetyMargin)
{
	Polynomial polynomial = NULL;
	Roots roots = NULL;
//start:
	double a0;
	int i, iformant;

	NUMburg (sample, nsamp_window, cof, nPoles, & a0);

	/*
	 * Convert LP coefficients to polynomial.
	 */
	polynomial = Polynomial_create (-1, 1, nPoles); cherror
	for (i = 1; i <= nPoles; i ++)
		polynomial -> coefficients [i] = - cof [nPoles - i + 1];
	polynomial -> coefficients [nPoles + 1] = 1.0;

	/*
	 * Find the roots of the polynomial.
	 */
	roots = Polynomial_to_Roots (polynomial); cherror
	if (roots == NULL)
		error1 (L"Cannot find roots.")
	Roots_fixIntoUnitCircle (roots);

	Melder_assert (frame -> nFormants == 0 && frame -> formant == NULL);

	/*
	 * First pass: count the formants.
	 * The roots come in conjugate pairs, so we need only count those above the real axis.
	 */
	for (i = roots -> min; i <= roots -> max; i ++) if (roots -> v [i]. im >= 0) {
		double f = fabs (atan2 (roots -> v [i].im, roots -> v [i].re)) * nyquistFrequency / NUMpi;
		if (f >= safetyMargin && f <= nyquistFrequency - safetyMargin)
			frame -> nFormants ++;
	}

	/*
	 * Create space for formant data.
	 */
	if (frame -> nFormants > 0)
		frame -> formant = NUMstructvector (Formant_Formant, 1, frame -> nFormants);
	cherror

	/*
	 * Second pass: fill in the formants.
	 */
	iformant = 0;
	for (i = roots -> min; i <= roots -> max; i ++) if (roots -> v [i]. im >= 0) {
		double f = fabs (atan2 (roots -> v [i].im, roots -> v [i].re)) * nyquistFrequency / NUMpi;
		if (f >= safetyMargin && f <= nyquistFrequency - safetyMargin) {
			Formant_Formant formant = & frame -> formant [++ iformant];
			formant -> frequency = f;
			formant -> bandwidth = -
				log (roots -> v [i].re * roots -> v [i].re + roots -> v [i].im * roots -> v [i].im) * nyquistFrequency / NUMpi;
		}
	}
	Melder_assert (iformant == frame -> nFormants);   /* May fail if some frequency is NaN. */
end:
	forget (polynomial);
	forget (roots);
	iferror return 0;
	return 1;
}

static int findOneZero (int ijt, double vcx [], double a, double b, double *zero) {
	double x = 0.5 * (a + b), fa = 0.0, fb = 0.0, fx = 0.0;
	long k;
	for (k = ijt; k >= 0; k --) {
		fa = vcx [k] + a * fa;
		fb = vcx [k] + b * fb;
	}
	if (fa * fb >= 0.0) {   /* There must be a zero between a and b. */
		Melder_casual ("There is no zero between %.8g and %.8g.\n"
			"   The function values are %.8g and %.8g, respectively.",
			a, b, fa, fb);
		return 0;
	}
	do {
		fx = 0.0;
		/*x = fa == fb ? 0.5 * (a + b) : a + fa * (a - b) / (fb - fa);*/
		x = 0.5 * (a + b);   /* Simple bisection. */
		for (k = ijt; k >= 0; k --) fx = vcx [k] + x * fx;
		if (fa * fx > 0.0) { a = x; fa = fx; } else { b = x; fb = fx; }
	} while (fabs (fx) > 1e-5);
	*zero = x;
	return 1;   /* OK */
}

static int findNewZeroes (int ijt, double ppORIG [], int degree,
	double zeroes [])   /* In / out */
{
	static double cosa [7] [7] = {
		{  1,   0,   0,   0,   0,   0,   0 },
		{  0,   2,   0,   0,   0,   0,   0 },
		{ -2,   0,   4,   0,   0,   0,   0 },
		{  0,  -6,   0,   8,   0,   0,   0 },
		{  2,   0, -16,   0,  16,   0,   0 },
		{  0,  10,   0, -40,   0,  32,   0 },
		{ -2,   0,  36,   0, -96,   0,  64 } };
	double pp [33], newZeroes [33], px [33];
	int pt, vt, i, half_degree = (degree + 1) / 2;
	for (vt = 0; vt <= half_degree; vt ++) pp [vt] = ppORIG [vt];
	if (! (degree & 1))
		for (vt = 1; vt <= half_degree; vt ++) pp [vt] -= pp [vt - 1];
	for (i = 0; i <= half_degree; i ++)
		px [i] = cosa [half_degree] [i];
	for (pt = half_degree - 1; pt >= 0; pt --)
		for (vt = 0; vt <= half_degree; vt ++)
			px [vt] += pp [half_degree - pt] * cosa [pt] [vt];
	/* Fill an array with the new zeroes, which lie between the old zeroes. */
	newZeroes [0] = 1.0;
	for (i = 1; i <= half_degree; i ++) {
		if (! findOneZero (ijt, px, zeroes [i - 1], zeroes [i], & newZeroes [i])) {
			Melder_casual ("Degree %d not completed.", degree);
			return 0;
		}
	}
	newZeroes [half_degree + 1] = -1.0;
	/* Grow older. */
	for (i = 0; i <= half_degree + 1; i ++)
		zeroes [i] = newZeroes [i];
	return 1;
}

static int splitLevinson (
	double xw [], long nx,	/* The windowed signal xw [1..nx] */
	int ncof,	/* The coefficients cof [1..ncof] */
	Formant_Frame frame, double nyquistFrequency)	/* Put the results here. */
{
	int i, j, degree, iformant;
	int result = 1;
	double rx [100], zeroes [33];
	for (i = 0; i <= 32; i ++) zeroes [i] = 0.0;
	/* Compute the autocorrelation of the windowed signal. */
	for (i = 0; i < ncof; i ++)
	{
		rx [i] = 0.0;
		for (j = 1; j <= nx - i; j ++) rx [i] += xw [j] * xw [j + i];
	}
	/* Normalize autocorrelation; (should we also divide by the autocorrelation of the window?). */
	for (i = 1; i < ncof; i ++) rx [i] /= rx [0]; rx [0] = 1.0;
	/* Compute zeroes. */
	{
		double tau = 0.5 * rx [0];
		double pnu [33], pk [33], pkz [33];
		for (i = 0; i <= 32; i ++) pkz [i] = pk [i] = 0.0;
		pkz [0] = 1.0; pk [0] = 1.0; pk [1] = 1.0;
		for (degree = 1; degree < ncof; degree ++) {
			int t = degree / 2, it, ijt;
			double tauk = rx [0] + rx [degree], alfak;
			for (it = 1; it <= t; it ++)
				tauk += pk [it] * ( 2 * it == degree ? rx [it] : rx [it] + rx [degree - it] );
			alfak = tauk / tau;
			tau = tauk;
			pnu [0] = 1.0;
			ijt = (degree + 1) / 2;
			for (it = ijt; it > 0; it --)
				pnu [it] = pk [it] + pk [it - 1] - alfak * pkz [it - 1];
			if (2 * ijt == degree) pnu [ijt + 1] = pnu [ijt];
			if (degree == 1) {
				(void) 0;
			} else if (degree == 2) {
				zeroes [0] = 1.0;   /* Starting values. */
				zeroes [1] = 0.5 - 0.5 * pnu [1];
				zeroes [2] = -1.0;
			}
			else
				if (! findNewZeroes (ijt, pnu, degree, zeroes)) {
					result = 0;
					goto loopEnd;
				}
			/* Grow older. */
			for (i = 0; i <= 32; i ++) { pkz [i] = pk [i]; pk [i] = pnu [i]; }
		}
	}
loopEnd:
	/* First pass: count the poles. */
	for (i = 1; i <= ncof / 2; i ++) {
		if (zeroes [i] == 0.0 || zeroes [i] == -1.0) break;
		frame -> nFormants ++;
	}

	/* Create space for formant data. */
	if (frame -> nFormants > 0 &&
	    ! (frame -> formant = NUMstructvector (Formant_Formant, 1, frame -> nFormants))) return 0;

	/* Second pass: fill in the poles. */
	iformant = 0;
	for (i = 1; i <= ncof / 2; i ++) {
		Formant_Formant formant = & frame -> formant [++ iformant];
		if (zeroes [i] == 0.0 || zeroes [i] == -1.0) break;
		formant -> frequency =  acos (zeroes [i]) * nyquistFrequency / NUMpi;
		formant -> bandwidth = 50.0;
	}

	return result;
}

static void Sound_preEmphasis (Sound me, double preEmphasisFrequency) {
	double preEmphasis = exp (-2.0 * NUMpi * preEmphasisFrequency * my dx);
	for (long channel = 1; channel <= my ny; channel ++) {
		double *s = my z [channel]; 
		for (long i = my nx; i >= 2; i --) s [i] -= preEmphasis * s [i - 1];
	}
}

void Formant_sort (Formant me) {
	for (long iframe = 1; iframe <= my nx; iframe ++) {
		Formant_Frame frame = & my frame [iframe];
		long n = frame -> nFormants;
		for (long i = 1; i < n; i ++) {
			double min = frame -> formant [i]. frequency;
			long imin = i;
			for (long j = i + 1; j <= n; j ++)
				if (frame -> formant [j]. frequency < min) {
					min = frame -> formant [j]. frequency;
					imin = j;
				}
			if (imin != i) {
				double min_bandwidth = frame -> formant [imin]. bandwidth;
				frame -> formant [imin]. frequency = frame -> formant [i]. frequency;
				frame -> formant [imin]. bandwidth = frame -> formant [i]. bandwidth;
				frame -> formant [i]. frequency = min;
				frame -> formant [i]. bandwidth = min_bandwidth;
			}
		}
	}
}

static Formant Sound_to_Formant_any_inline (Sound me, double dt_in, int numberOfPoles,
	double halfdt_window, int which, double preemphasisFrequency, double safetyMargin)
{
	Formant thee = NULL;
	double *frame = NULL, *window = NULL, *cof = NULL;
//start:
	double dt = dt_in > 0.0 ? dt_in : halfdt_window / 4.0;
	double duration = my nx * my dx, t1;
	double dt_window = 2.0 * halfdt_window;
	long nFrames = 1 + (long) floor ((duration - dt_window) / dt);
	long nsamp_window = (long) floor (dt_window / my dx), halfnsamp_window = nsamp_window / 2;

	if (nsamp_window < numberOfPoles + 1)
		return Melder_errorp ("(Sound_to_Formant:) Window too short.");
	t1 = my x1 + 0.5 * (duration - my dx - (nFrames - 1) * dt); /* Centre of first frame. */
	if (nFrames < 1) {
		nFrames = 1;
		t1 = my x1 + 0.5 * duration;
		dt_window = duration;
		nsamp_window = my nx;
	}
	thee = Formant_create (my xmin, my xmax, nFrames, dt, t1, (numberOfPoles + 1) / 2); cherror   // e.g. 11 poles -> maximally 6 formants
	window = NUMdvector (1, nsamp_window); cherror
	frame = NUMdvector (1, nsamp_window); cherror
	if (which == 1) { cof = NUMdvector (1, numberOfPoles); cherror }

	Melder_progress1 (0.0, L"Formant analysis");

	/* Pre-emphasis. */
	Sound_preEmphasis (me, preemphasisFrequency);

	/* Gaussian window. */
	for (long i = 1; i <= nsamp_window; i ++) {
		double imid = 0.5 * (nsamp_window + 1), edge = exp (-12.0);
		window [i] = (exp (-48.0 * (i - imid) * (i - imid) / (nsamp_window + 1) / (nsamp_window + 1)) - edge) / (1 - edge);
	}

	for (long iframe = 1; iframe <= nFrames; iframe ++) {
		double t = Sampled_indexToX (thee, iframe);
		long leftSample = Sampled_xToLowIndex (me, t);
		long rightSample = leftSample + 1;
		long startSample = rightSample - halfnsamp_window;
		long endSample = leftSample + halfnsamp_window;
		double maximumIntensity = 0.0;
		if (startSample < 1) startSample = 1;
		if (endSample > my nx) endSample = my nx;
		for (long i = startSample; i <= endSample; i ++) {
			double value = Sampled_getValueAtSample (me, i, Sound_LEVEL_MONO, 0);
			if (value * value > maximumIntensity) {
				maximumIntensity = value * value;
			}
		}
		if (maximumIntensity == HUGE_VAL) error1 (L"Sound contains infinities.")
		thy frame [iframe]. intensity = maximumIntensity;
		if (maximumIntensity == 0.0) continue;   /* Burg cannot stand all zeroes. */

		/* Copy a pre-emphasized window to a frame. */
		for (long j = 1, i = startSample; j <= nsamp_window; j ++)
			frame [j] = Sampled_getValueAtSample (me, i ++, Sound_LEVEL_MONO, 0) * window [j];

		if ((which == 1 && ! burg (frame, endSample - startSample + 1, cof, numberOfPoles, & thy frame [iframe], 0.5 / my dx, safetyMargin)) ||
		    (which == 2 && ! splitLevinson (frame, endSample - startSample + 1, numberOfPoles, & thy frame [iframe], 0.5 / my dx)))
		{
			Melder_clearError ();
			Melder_casual ("(Sound_to_Formant:) Analysis results of frame %ld will be wrong.", iframe);
		}
		if (! Melder_progress2 ((double) iframe / (double) nFrames, L"Formant analysis: frame ", Melder_integer (iframe)))
			break;
	}
end:
	Melder_progress1 (1.0, NULL);
	NUMdvector_free (window, 1);
	NUMdvector_free (frame, 1);
	NUMdvector_free (cof, 1);
	iferror { forget (thee); return Melder_errorp ("(Sound_to_Formant:) Not performed."); }
	if (thee) Formant_sort (thee);
	return thee;
}

Formant Sound_to_Formant_any (Sound me, double dt, int numberOfPoles, double maximumFrequency,
	double halfdt_window, int which, double preemphasisFrequency, double safetyMargin)
{
	Sound sound = NULL;
	Formant thee = NULL;
	double nyquist = 0.5 / my dx;
	if (maximumFrequency <= 0.0 || fabs (maximumFrequency / nyquist - 1) < 1.0e-12) {
		sound = Data_copy (me);   /* Will be modified. */
	} else {
		sound = Sound_resample (me, maximumFrequency * 2, 50);
	}
	if (! sound) return NULL;
	thee = Sound_to_Formant_any_inline (sound, dt, numberOfPoles, halfdt_window, which, preemphasisFrequency, safetyMargin);
	forget (sound);
	return thee;
}

Formant Sound_to_Formant_burg (Sound me, double dt, double nFormants, double maximumFrequency, double halfdt_window, double preemphasisFrequency)
	{ return Sound_to_Formant_any (me, dt, (int) (2 * nFormants), maximumFrequency, halfdt_window, 1, preemphasisFrequency, 50.0); }

Formant Sound_to_Formant_keepAll (Sound me, double dt, double nFormants, double maximumFrequency, double halfdt_window, double preemphasisFrequency)
	{ return Sound_to_Formant_any (me, dt, (int) (2 * nFormants), maximumFrequency, halfdt_window, 1, preemphasisFrequency, 0.0); }

Formant Sound_to_Formant_willems (Sound me, double dt, double nFormants, double maximumFrequency, double halfdt_window, double preemphasisFrequency)
	{ return Sound_to_Formant_any (me, dt, (int) (2 * nFormants), maximumFrequency, halfdt_window, 2, preemphasisFrequency, 50.0); }

/* End of file Sound_to_Formant.c */
