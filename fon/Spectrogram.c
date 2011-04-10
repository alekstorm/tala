/* Spectrogram.c
 *
 * Copyright (C) 1992-2008 Paul Boersma
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
 * pb 2003/03/08 more info
 * pb 2003/05/27 autoscaling
 * pb 2007/03/17 domain quantity
 * pb 2008/01/19 double
 */

#include <time.h>
#include "Spectrogram.h"

static void info (I) {
	iam (Spectrogram);
	classData -> info (me);
	MelderInfo_writeLine1 (L"Time domain:");
	MelderInfo_writeLine3 (L"   Start time: ", Melder_double (my xmin), L" seconds");
	MelderInfo_writeLine3 (L"   End time: ", Melder_double (my xmax), L" seconds");
	MelderInfo_writeLine3 (L"   Total duration: ", Melder_double (my xmax - my xmin), L" seconds");
	MelderInfo_writeLine1 (L"Time sampling:");
	MelderInfo_writeLine2 (L"   Number of time slices (frames): ", Melder_integer (my nx));
	MelderInfo_writeLine3 (L"   Time step (frame distance): ", Melder_double (my dx), L" seconds");
	MelderInfo_writeLine3 (L"   First time slice (frame centre) at: ", Melder_double (my x1), L" seconds");
	MelderInfo_writeLine1 (L"Frequency domain:");
	MelderInfo_writeLine3 (L"   Lowest frequency: ", Melder_double (my ymin), L" Hz");
	MelderInfo_writeLine3 (L"   Highest frequency: ", Melder_double (my ymax), L" Hz");
	MelderInfo_writeLine3 (L"   Total bandwidth: ", Melder_double (my xmax - my xmin), L" Hz");
	MelderInfo_writeLine1 (L"Frequency sampling:");
	MelderInfo_writeLine2 (L"   Number of frequency bands (bins): ", Melder_integer (my ny));
	MelderInfo_writeLine3 (L"   Frequency step (bin width): ", Melder_double (my dy), L" Hz");
	MelderInfo_writeLine3 (L"   First frequency band around (bin centre at): ", Melder_double (my y1), L" Hz");
}

class_methods (Spectrogram, Matrix)
	class_method (info)
	us -> domainQuantity = MelderQuantity_TIME_SECONDS;
class_methods_end

Any Spectrogram_create (double tmin, double tmax, long nt, double dt, double t1,
	double fmin, double fmax, long nf, double df, double f1)
{
	Spectrogram me = Thing_new (Spectrogram);
	if (! me || ! Matrix_init (me, tmin, tmax, nt, dt, t1, fmin, fmax, nf, df, f1))
		forget (me);
	return me;    
}

void Spectrogram_paintInside (I, Graphics g, double tmin, double tmax, double fmin, double fmax,
	double maximum, int autoscaling, double dynamic, double preemphasis, double dynamicCompression)
{
	iam (Spectrogram);
	long itmin, itmax, ifmin, ifmax, ifreq, itime;
	double *preemphasisFactor, *dynamicFactor;
	if (tmax <= tmin) { tmin = my xmin; tmax = my xmax; }
	if (fmax <= fmin) { fmin = my ymin; fmax = my ymax; }
	if (! Matrix_getWindowSamplesX (me, tmin - 0.49999 * my dx, tmax + 0.49999 * my dx, & itmin, & itmax) ||
		 ! Matrix_getWindowSamplesY (me, fmin - 0.49999 * my dy, fmax + 0.49999 * my dy, & ifmin, & ifmax))
		return;
	Graphics_setWindow (g, tmin, tmax, fmin, fmax);
	if (! (preemphasisFactor = NUMdvector (ifmin, ifmax)) ||
		 ! (dynamicFactor = NUMdvector (itmin, itmax)))
		return;
	/* Pre-emphasis in place; also compute maximum after pre-emphasis. */
	for (ifreq = ifmin; ifreq <= ifmax; ifreq ++) {
		preemphasisFactor [ifreq] = (preemphasis / NUMln2) * log (ifreq * my dy / 1000.0);
		for (itime = itmin; itime <= itmax; itime ++) {
			double value = my z [ifreq] [itime];   /* Power. */
			value = (10.0/NUMln10) * log ((value + 1e-30) / 4.0e-10) + preemphasisFactor [ifreq];   /* dB */
			if (value > dynamicFactor [itime]) dynamicFactor [itime] = value;   /* Local maximum. */
			my z [ifreq] [itime] = value;
		}
	}
	/* Compute global maximum. */
	if (autoscaling) {
		maximum = 0.0;
		for (itime = itmin; itime <= itmax; itime ++)
			if (dynamicFactor [itime] > maximum) maximum = dynamicFactor [itime];
	}
	/* Dynamic compression in place. */
	for (itime = itmin; itime <= itmax; itime ++) {
		dynamicFactor [itime] = dynamicCompression * (maximum - dynamicFactor [itime]);
		for (ifreq = ifmin; ifreq <= ifmax; ifreq ++)
			my z [ifreq] [itime] += dynamicFactor [itime];
	}
	Graphics_image (g, my z,
		itmin, itmax,
		Matrix_columnToX (me, itmin - 0.5),
		Matrix_columnToX (me, itmax + 0.5),
		ifmin, ifmax,
		Matrix_rowToY (me, ifmin - 0.5),
		Matrix_rowToY (me, ifmax + 0.5),
		maximum - dynamic, maximum);
	for (ifreq = ifmin; ifreq <= ifmax; ifreq ++)
		for (itime = itmin; itime <= itmax; itime ++) {
			double value = 4.0e-10 * exp ((my z [ifreq] [itime] - dynamicFactor [itime]
				- preemphasisFactor [ifreq]) * (NUMln10 / 10.0)) - 1e-30;
			my z [ifreq] [itime] = value > 0.0 ? value : 0.0;
		}
	NUMdvector_free (preemphasisFactor, ifmin);
	NUMdvector_free (dynamicFactor, itmin);
}

void Spectrogram_paint (I, Graphics g,
	double tmin, double tmax, double fmin, double fmax, double maximum, int autoscaling,
	double dynamic, double preemphasis, double dynamicCompression,
	int garnish)
{
	iam (Spectrogram);
	Graphics_setInner (g);
	Spectrogram_paintInside (me, g, tmin, tmax, fmin, fmax, maximum, autoscaling,
		dynamic, preemphasis, dynamicCompression);
	Graphics_unsetInner (g);
	if (garnish) {
		Graphics_drawInnerBox (g);
		Graphics_textBottom (g, 1, L"Time (s)");
		Graphics_marksBottom (g, 2, 1, 1, 0);
		Graphics_marksLeft (g, 2, 1, 1, 0);
		Graphics_textLeft (g, 1, L"Frequency (Hz)");
	}
}

Spectrogram Matrix_to_Spectrogram (I) {
	iam (Matrix);
	Spectrogram thee = (structSpectrogram *)Spectrogram_create (my xmin, my xmax, my nx, my dx, my x1,
			my ymin, my ymax, my ny, my dy, my y1);
	if (! thee) return NULL;
	NUMdmatrix_copyElements (my z, thy z, 1, my ny, 1, my nx);
	return thee;
}

Matrix Spectrogram_to_Matrix (I) {
	iam (Spectrogram);
	Matrix thee = Matrix_create (my xmin, my xmax, my nx, my dx, my x1,
			my ymin, my ymax, my ny, my dy, my y1);
	if (! thee) return NULL;
	NUMdmatrix_copyElements (my z, thy z, 1, my ny, 1, my nx);
	return thee;
}

/* End of Spectrogram.c */ 
