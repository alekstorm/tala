/* Pattern.c
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
 djmw 20020813 GPL header
 djmw 20041203 Added _Pattern_checkElements.
 djmw 20071017 Melder_error<p>
  djmw 20110304 Thing_new
*/

#include "Pattern.h"

class_methods (Pattern, Matrix)
class_methods_end

int _Pattern_checkElements (Pattern me)
{
	long i, j;
	for (i = 1; i <= my ny; i++)
	{
		for (j = 1; j <= my nx; j++)
		{
			if (my z[i][j] < 0 || my z[i][j] > 1) return 0;
		}
	}
	return 1;
}

int Pattern_init (I, long ny, long nx)
{
    iam (Pattern);
    my ny = ny;
    my nx = nx;
    if (! Matrix_init (me, 1, nx, nx, 1, 1, 1, ny, ny, 1, 1)) return 0;
    return 1;
}

Any Pattern_create (long ny, long nx)
{
    Pattern me = Thing_new (Pattern);
    if (! me || ! Pattern_init (me, ny, nx)) forget (me);
    return me;
}

void Pattern_normalize (I, int choice, double pmin, double pmax)
{
    iam (Pattern); long i, j;
    if (pmin == pmax) (void) Matrix_getWindowExtrema (me, 1, my nx, 1, my ny, & pmin, & pmax);
    if (pmin == pmax) return;
    if (choice == 1)
    {
		for (i=1; i <= my ny; i++) for (j=1; j <= my nx; j++)
			my z[i][j] = ( my z[i][j] - pmin) / ( pmax - pmin);
    }
    else /* default choice */
    {
		for (i=1; i <= my ny; i++)
		{
			double sum = 0;
			for (j=1; j <= my nx; j++) sum += ( my z[i][j] -= pmin);
			for (j=1; j <= my nx; j++) my z[i][j] *= 1.0 / sum;
		}
	}
}

Pattern Matrix_to_Pattern (I, int join)
{
    iam (Matrix);
	long i, j, r = 0, c = 1;
	Pattern thee = NULL;
	
    if (join < 1) join = 1;
	if ((my ny % join) != 0) return Melder_errorp1 (L"Matrix_to_Pattern:"
		"number of rows is not a multiple of join."); 
	if (! (thee = Pattern_create (my ny / join, join * my nx))) return thee;
	for (i = 1; i <= my ny; i++)
	{
		if ((i - 1) % join == 0)
		{
			r++; c = 1;
		}
		for (j = 1; j <= my nx; j++)
		{
			thy z[r][c++] = my z[i][j];
		}
	}
    return thee;
}

Matrix Pattern_to_Matrix (Pattern me)
{
	Matrix thee = Data_copy (me);
	if (thee) Thing_overrideClass (thee, classMatrix);
	return thee;
}

/* End of file Pattern.c */
