/* Cepstrum.c
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
 djmw 20010514
 djmw 20020812 GPL header
 djmw 20080122 Version 1: float -> double
 djmw 20110304 Thing_new
*/

#include "Cepstrum.h"
#include "num/NUM2.h"

class_methods (Cepstrum, Matrix)
	us -> version = 1;
class_methods_end

Cepstrum Cepstrum_create (double qmin, double qmax, long nq)
{
	Cepstrum me = Thing_new (Cepstrum);
	double dx = (qmax - qmin) / nq;

	if (me == NULL || ! Matrix_init (me, qmin, qmax, nq, dx, qmin + dx/2, 
		1, 1, 1, 1, 1)) forget (me);
	return me;
}

void Cepstrum_draw (Cepstrum me, Graphics g, double qmin, double qmax,
	double minimum, double maximum, int garnish)
{
	double *y = NULL, *z;
	long i, imin, imax;
	int autoscaling = minimum >= maximum;

	Graphics_setInner (g);

	if (qmax <= qmin)
	{
		qmin = my xmin; qmax = my xmax;
	}
	
	if (! Matrix_getWindowSamplesX (me, qmin, qmax, & imin, & imax)) return;

	if ((y = NUMdvector (imin, imax)) == NULL) return;
	
	z = my z[1];
	
	for (i = imin; i <= imax; i++)
	{
		y[i] = z[i];
	}
	
	if (autoscaling) NUMdvector_extrema (y, imin, imax, & minimum, & maximum);

	for (i = imin; i <= imax; i ++)
	{
		if (y[i] > maximum) y[i] = maximum;
		else if (y[i] < minimum) y[i] = minimum;
	}
	
	Graphics_setWindow (g, qmin, qmax, minimum, maximum);
	Graphics_function (g, y, imin, imax, Matrix_columnToX (me, imin),
		Matrix_columnToX (me, imax));

	Graphics_unsetInner (g);
	
	if (garnish) {
		Graphics_drawInnerBox (g);
		Graphics_textBottom (g, 1, L"Quefrency");
		Graphics_marksBottom (g, 2, TRUE, TRUE, FALSE);
		Graphics_textLeft (g, 1, L"Amplitude");
	}

	NUMdvector_free (y, imin);
}

Matrix Cepstrum_to_Matrix (Cepstrum me)
{
	Matrix thee = Data_copy (me);
	if (thee == NULL) return NULL;
	Thing_overrideClass (thee, classMatrix);
	return thee;
}

Cepstrum Matrix_to_Cepstrum (Matrix me, long row)
{
	Cepstrum thee;

	if ((thee = Cepstrum_create (my xmin, my xmax, my nx)) == NULL)
		 return NULL;
	if (row < 0) row = my ny + 1 - row;
	if (row < 1) row = 1;
	if (row > my ny) row = my ny;
	NUMdvector_copyElements (my z[row], thy z[1], 1, my nx);
	return thee;
}


/* End of file Cepstrum.c */
