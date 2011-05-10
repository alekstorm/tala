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
