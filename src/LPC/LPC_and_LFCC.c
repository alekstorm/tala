/* LPC_and_LFCC.c
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
 djmw 20020812 GPL header
 djmw 20030205 Latest modifiation
*/

#include "LPC_and_LFCC.h"
#include "num/NUM2.h"

#define MIN(m,n) ((m) < (n) ? (m) : (n))

void LPC_Frame_into_CC_Frame (LPC_Frame me, CC_Frame thee)
{
	double s, *c = thy c, *a = my a;
	long k, n;
	
	thy c0 = 0.5 * log (my gain);
	if (my nCoefficients < 1) return;
	
	c[1] = -a[1];
	for (n = 2; n <= MIN(my nCoefficients, thy numberOfCoefficients); n++)
	{
		s = 0;
		for (k = 1; k < n; k++)
		{
			s += a[k] * c[n-k] * (n-k);
		}
		c[n] = -a[n] - s / n;
	}
	for (n = my nCoefficients + 1; n <= thy numberOfCoefficients; n++)
	{
		s = 0;
		for (k = 1; k <= my nCoefficients; k++)
		{
			s += a[k] * c[n-k] * (n-k);
		}
		c[n] = - s / n;
	}
}

void CC_Frame_into_LPC_Frame (CC_Frame me, LPC_Frame thee)
{
	long i, j, n = MIN (my numberOfCoefficients, thy nCoefficients);
	double *c = my c, *a = thy a;
	
	thy gain = exp (2 * my c0);
	
	if (n < 1) return;
	
	a[1] = -c[1];
	for (i=2; i <= n; i++)
	{
		float ai = c[i] * i;
		for (j=1; j < i; j++)
		{
			ai += a[j] * c[i - j] * (i - j);
		}
		a[i] = -ai / i;
	}
}

LFCC LPC_to_LFCC (LPC me, long numberOfCoefficients)
{
	LFCC thee;
	long i;
	
	if (numberOfCoefficients < 1) numberOfCoefficients = my maxnCoefficients;
	
	if ((thee = LFCC_create (my xmin, my xmax, my nx, my dx, my x1, 
		numberOfCoefficients, 0, 0.5 / my samplingPeriod)) == NULL)
			return NULL;
			
	for (i = 1; i <= my nx; i++)
	{
		if (! CC_Frame_init (& thy frame[i], numberOfCoefficients)) break; 
		LPC_Frame_into_CC_Frame (& my frame[i], & thy frame[i]);
	}

	if (Melder_hasError ()) forget (thee);
	return thee;
}

LPC LFCC_to_LPC (LFCC me, long numberOfCoefficients)
{
	LPC thee;
	long i;
	
	if (numberOfCoefficients < 1) numberOfCoefficients =
		my maximumNumberOfCoefficients;
	
	numberOfCoefficients = MIN (numberOfCoefficients,
		my maximumNumberOfCoefficients);
	
	if ((thee = LPC_create (my xmin, my xmax, my nx, my dx, my x1, 
		numberOfCoefficients, 0.5 / my fmax)) == NULL)
			return NULL;
			
	for (i = 1; i <= my nx; i++)
	{
		if (! LPC_Frame_init (& thy frame[i], numberOfCoefficients)) break; 
		CC_Frame_into_LPC_Frame (& my frame[i], & thy frame[i]);
	}

	if (Melder_hasError ()) forget (thee);
	return thee;
}

#undef MIN

/* End of file LPC_and_LFCC.c  */
