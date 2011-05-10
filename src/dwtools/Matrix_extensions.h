#ifndef _Matrix_extensions_h_
#define _Matrix_extensions_h_
/* Matrix_extensions.h
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
 djmw 20110307 Latest modification
*/

#include "fon/Matrix.h"

#ifdef __cplusplus
	extern "C" {
#endif

void Matrix_scatterPlot (I, Any g, long icx, long icy,
    double xmin, double xmax, double ymin, double ymax,
    double size_mm, const wchar_t *mark, int garnish);
/* Draw my columns ix and iy as a scatterplot (with squares)				*/

void Matrix_scale (I, int choice);
/* choice = 1 :divide each elmnt by the maximum (abs) */
/* choice = 2 :rows, divide each row elmnt by the maximum (abs) of that row	*/
/* choice = 3 :columns, divide each col elmnt by the maximum of that col	*/

Any Matrix_transpose (I);

int Matrix_fitPolynomial (I, long maxDegree);

Matrix Matrix_solveEquation (I, double tolerance);

#ifdef __cplusplus
	}
#endif

#endif /* _Matrix_extensions_h_ */
