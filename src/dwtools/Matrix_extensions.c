/* Matrix_extensions.c
 *
 * Copyright (C) 1993-2008 David Weenink
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
 djmw 20020813 GPL header
 djmw 20040226 Matrix_drawAsSquares: respect the colour environment (fill with current colour).
 djmw 20041110 Matrix_drawDistribution did't draw lowest bin correctly.
 djmw 20050221 Matrix_drawDistribution would draw outside window.
 djmw 20050405 Matrix_drawDistribution crashed if minimum > data minimum5
 djmw 20080122 float -> double
*/

#include "Matrix_extensions.h"
#include "dwsys/Eigen.h"
#include "num/NUM2.h"

void Matrix_scatterPlot (I, Any g, long icx, long icy,
    double xmin, double xmax, double ymin, double ymax,
    double size_mm, const wchar_t *mark, int garnish)
{
    iam (Matrix);
    long i, ix = labs (icx), iy = labs (icy);

    if (ix < 1 || ix > my nx || iy < 1 || iy > my nx) return;
    if (xmax <= xmin)
    {
		(void) Matrix_getWindowExtrema (me, ix, ix, 1, my ny, & xmin, & xmax);
		if (xmax <= xmin)
		{
			xmin -= 0.5; xmax += 0.5;
		}
    }
    if (ymax <= ymin)
    {
		(void) Matrix_getWindowExtrema (me, iy, iy, 1, my ny, & ymin, & ymax);
		if (ymax <= ymin)
		{
			ymin -= 0.5; ymax += 0.5;
		}
    }
    Graphics_setInner (g);
    if (icx < 0) { double t = xmin; xmin = xmax; xmax = t; }
    if (icy < 0) { double t = ymin; ymin = ymax; ymax = t; }
    Graphics_setWindow (g, xmin, xmax, ymin, ymax);
    for (i = 1; i <= my ny; i++)
    {
    	if (my z[i][ix] >= xmin && my z[i][ix] <= xmax &&
    	my z[i][iy] >= ymin && my z[i][iy] <= ymax)
    		Graphics_mark (g, my z[i][ix], my z[i][iy], size_mm, mark);
    }
    Graphics_unsetInner (g);
    if (garnish)
    {
		Graphics_drawInnerBox (g);
		Graphics_marksLeft (g, 2, 1, 1, 0);
		if (ymin * ymax < 0.0) Graphics_markLeft (g, 0.0, 1, 1, 1, NULL);
		Graphics_marksBottom (g, 2, 1, 1, 0);
		if (xmin * xmax < 0.0) Graphics_markBottom (g, 0.0, 1, 1, 1, NULL);
	}
}

void Matrix_scale (I, int choice)
{
	iam (Matrix); double min, max, extremum;
	long i, j, nZero = 0;

	if (choice == 2) /* by row */
	{
		for (i = 1; i <= my ny; i++)
		{
			Matrix_getWindowExtrema (me, 1, my nx, i, i, &min, &max);
			extremum = fabs (max) > fabs (min) ? fabs (max) : fabs (min);
			if (extremum == 0.0) nZero++;
			else for (j=1; j <= my nx; j++) my z[i][j] /= extremum;
		}
	}
	else if (choice == 3) /* by col */
	{
		for (j = 1; j <= my nx; j++)
		{
			Matrix_getWindowExtrema (me, j, j, 1, my ny, &min, &max);
			extremum =  fabs (max) > fabs (min) ? fabs (max) : fabs (min);
			if (extremum == 0.0) nZero++;
			else for (i=1; i <= my ny; i++) my z[i][j] /= extremum;
		}
	}
	else if (choice == 1) /* overall */
	{
		Matrix_getWindowExtrema (me, 1, my nx, 1, my ny, &min, &max);
		extremum =  fabs (max) > fabs (min) ? fabs (max) : fabs (min);
		if (extremum == 0.0) nZero++;
		else
		{
			for (i = 1; i <= my ny; i++)
			{
				for (j = 1; j <= my nx; j++) my z[i][j] /= extremum;
			}
		}
	}
	else
	{
		Melder_flushError ("Matrix_scale: choice must be >= 0 && < 3.");
		return;
	}
	if (nZero)  Melder_warning1 (L"Matrix_scale: extremum == 0, (part of) matrix unscaled.");
}

Any Matrix_transpose (I)
{
	iam (Matrix);
	long i, j;
	Matrix thee = Matrix_create (my ymin, my ymax, my ny, my dy, my y1,
		my xmin, my xmax, my nx, my dx, my x1);

	if (thee == NULL) return NULL;

	for (i = 1; i <= my ny; i++)
	{
		for (j=1; j <= my nx; j++)
		{
			thy z[j][i] = my z[i][j];
		}
	}
	return thee;

}

Matrix Matrix_solveEquation (I, double tolerance)
{
	iam (Matrix); Matrix thee = NULL; int status = 0;
	long i, j, m = my ny, n = my nx - 1;
	double **u = NULL, *b = NULL, *x = NULL;
	double tol = tolerance ? tolerance : NUMeps * m;

	if (n == 0) return Melder_errorp1 (L"Matrix_solveEquation: there must be at least 2 columns in the matrix.");

	if (m < n) Melder_warning1 (L"Matrix_solveEquation: solution is not unique (fewer equations than unknowns).");

	if (! (u = NUMdmatrix (1, m, 1, n)) ||
		! (b = NUMdvector (1, m)) ||
		! (x = NUMdvector (1, n)) ||
		! (thee = Matrix_create (0.5, 0.5+n, n, 1, 1, 0.5, 1.5, 1, 1, 1))) goto end;

	for (i=1; i <= m; i++)
	{
		for (j=1; j <= n; j++) u[i][j] = my z[i][j];
		b[i] = my z[i][my nx];
	}

	if (! NUMsolveEquation (u, m, n, b, tol, x)) goto end;
	for (j=1; j <= n; j++) thy z[1][j] = x[j];
	status = 1;

end:
	NUMdmatrix_free (u, 1, 1);
	NUMdvector_free (b, 1);
	NUMdvector_free (x, 1);
	if (! status) forget (thee);
	return thee;
}

/* End of file Matrix_extensions.c */
