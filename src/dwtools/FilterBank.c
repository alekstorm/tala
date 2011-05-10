/* FilterBank.c
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
 djmw 20010718
 djmw 20020813 GPL header
 djmw 20030901 Added fiter function drawing and frequency scale drawing.
 djmw 20050731 +FilterBank_and_PCA_drawComponent
 djmw 20071017 Melder_error<n>
 djmw 20071201 Melder_warning<n>
 djmw 20080122 float -> double
 djmw 20110304 Thing_new
*/

#include "Eigen_and_Matrix.h"
#include "FilterBank.h"
#include "Matrix_extensions.h"
#include "num/NUM2.h"

static int classFilterBank_getFrequencyScale (I)
{
	(void) void_me;
	return FilterBank_HERTZ;
}

class_methods (FilterBank, Matrix)
	class_method_local (FilterBank, getFrequencyScale)
class_methods_end

static int classBarkFilter_getFrequencyScale (I)
{
	(void) void_me;
	return FilterBank_BARK;
}

class_methods (BarkFilter, FilterBank)
	class_method_local (BarkFilter, getFrequencyScale)
class_methods_end

BarkFilter BarkFilter_create (double tmin, double tmax, long nt, double dt,
	double t1, double fmin, double fmax, long nf, double df, long f1)
{
	BarkFilter me = Thing_new (BarkFilter);
	if (me == NULL || ! Matrix_init (me, tmin, tmax, nt, dt, t1,
		fmin, fmax, nf, df, f1)) forget (me);
	return me;
}

int FilterBank_getFrequencyScale (I)
{
	iam (FilterBank);
	return our getFrequencyScale (me);
}

static int classMelFilter_getFrequencyScale (I)
{
	(void) void_me;
	return FilterBank_MEL;
}

class_methods (MelFilter, FilterBank)
	class_method_local (MelFilter, getFrequencyScale)
class_methods_end

MelFilter MelFilter_create (double tmin, double tmax, long nt, double dt,
	double t1, double fmin, double fmax, long nf, double df, double f1)
{
	MelFilter me = Thing_new (MelFilter);
	
	if (me == NULL || ! Matrix_init (me, tmin, tmax, nt, dt, t1,
		fmin, fmax, nf, df, f1)) forget (me);
	
	return me;
}

Matrix FilterBank_to_Matrix (I)
{
	iam (Matrix); Matrix thee;
	
	if ((thee = Matrix_create (my xmin, my xmax, my nx, my dx, my x1,
		my ymin, my ymax, my ny, my dy, my y1)) == NULL) return NULL;
		
	NUMdmatrix_copyElements (my z, thy z, 1, my ny, 1, my nx);
	
	return thee;
}

BarkFilter Matrix_to_BarkFilter (I)
{
	iam (Matrix); BarkFilter thee;
	
	if ((thee = BarkFilter_create (my xmin, my xmax, my nx, my dx, my x1,
		my ymin, my ymax, my ny, my dy, my y1)) == NULL) return NULL;
		
	NUMdmatrix_copyElements (my z, thy z, 1, my ny, 1, my nx);
	
	return thee;
}

MelFilter Matrix_to_MelFilter (I)
{
	iam (Matrix); MelFilter thee;
	
	if ((thee = MelFilter_create (my xmin, my xmax, my nx, my dx, my x1,
		my ymin, my ymax, my ny, my dy, my y1)) == NULL) return NULL;
		
	NUMdmatrix_copyElements (my z, thy z, 1, my ny, 1, my nx);
	
	return thee;
}

class_methods (FormantFilter, FilterBank)
class_methods_end

FormantFilter FormantFilter_create (double tmin, double tmax, long nt,
	double dt, double t1, double fmin, double fmax, long nf, double df,
	double f1)
{
	FormantFilter me = Thing_new (FormantFilter);
	
	if (me == NULL || ! Matrix_init (me, tmin, tmax, nt, dt, t1,
		fmin, fmax, nf, df, f1)) forget (me);
	
	return me;
}

FormantFilter Matrix_to_FormantFilter (I)
{
	iam (Matrix); FormantFilter thee;
	
	if ((thee = FormantFilter_create (my xmin, my xmax, my nx, my dx, my x1,
		my ymin, my ymax, my ny, my dy, my y1)) == NULL) return NULL;
		
	NUMdmatrix_copyElements (my z, thy z, 1, my ny, 1, my nx);
	
	return thee;
}

Spectrum FormantFilter_to_Spectrum_slice (FormantFilter me, double t)
{
	Spectrum thee;
	long i, frame;
	double sqrtref = sqrt (FilterBank_DBREF);
	double factor2 = 2 * 10 * FilterBank_DBFAC;

	if ((thee = Spectrum_create (my ymax, my ny)) == NULL) return NULL;
	
	thy xmin = my ymin;
	thy xmax = my ymax;
	thy x1 = my y1;
	thy dx = my dy;   /* Frequency step. */
	
	frame = Sampled_xToIndex (me, t);
	if (frame < 1) frame = 1;
	if (frame > my nx) frame = my nx;

	for (i = 1; i <= my ny; i++)
	{	/*
			power = ref * 10 ^ (value / 10)
			sqrt(power) = sqrt(ref) * 10 ^ (value / (2*10))
		*/
		thy z[1][i] = sqrtref * pow (10, my z[i][frame] / factor2);
		thy z[2][i] = 0.0;
	}

	return thee;	
}

Intensity FilterBank_to_Intensity (I)
{
	iam (Matrix);
	Intensity thee;
	double db_ref = 10 * log10 (FilterBank_DBREF);
	long i, j;
	
	thee = Intensity_create (my xmin, my xmax, my nx, my dx, my x1);
	if (thee == NULL) return NULL;

	for (j = 1; j <= my nx; j++)
	{
		double p = 0;
		for (i = 1; i <= my ny; i++)
		{
			p += FilterBank_DBREF * exp (NUMln10 * my z[i][j] / 10);
		}
		thy z[1][j] = 10 * log10 (p) - db_ref;
	}
	return thee;
}

void FilterBank_equalizeIntensities (I, double intensity_db)
{
	iam (Matrix);
	long i, j;
	
	for (j=1; j <= my nx; j++)
	{
		double p = 0, delta_db;
		
		for (i=1; i <= my ny; i++)
		{
			p += FilterBank_DBREF * exp (NUMln10 * my z[i][j] / 10);
		}
		
		delta_db = intensity_db - 10 * log10 (p / FilterBank_DBREF);
		
		for (i=1; i <= my ny; i++)
		{
			my z[i][j] += delta_db;
		}
	}
}
 
/* End of file Filterbank.c */
