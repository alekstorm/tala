/* TableOfReal_and_SVD.c
 *
 * Copyright (C) 1993-2005 David Weenink
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
 djmw 20021009 GPL header
 djmw 20031107 Added TablesOfReal_to_GSVD.
 djmw 20051202 Extract left/right singular vectors.
*/

#include "TableOfReal_and_SVD.h"

#define MIN(m,n) ((m) < (n) ? (m) : (n))

TableOfReal SVD_to_TableOfReal (SVD me, long from, long to)
{
	TableOfReal thee = TableOfReal_create (my numberOfRows, my numberOfColumns);

	if (thee == NULL) return NULL;
	if (! SVD_synthesize (me, from, to, thy data)) forget (thee);
	return thee;
}

SVD TableOfReal_to_SVD (I)
{
	iam (TableOfReal);
	SVD thee = SVD_create_d (my data, my numberOfRows, my numberOfColumns);
	return thee;
}

TableOfReal SVD_extractLeftSingularVectors (I)
{
	iam (SVD);
	TableOfReal thee;
	long mn_min = MIN (my numberOfRows, my numberOfColumns);

	thee = TableOfReal_create (my numberOfRows, mn_min);
	if (thee == NULL) return NULL;
	NUMdmatrix_copyElements (my u, thy data, 1, my numberOfRows, 1, mn_min);
	return thee;
}

TableOfReal SVD_extractRightSingularVectors (I)
{
	iam (SVD);
	TableOfReal thee;
	long mn_min = MIN (my numberOfRows, my numberOfColumns);

	thee = TableOfReal_create (my numberOfColumns, mn_min);
	if (thee == NULL) return NULL;
	NUMdmatrix_copyElements (my v, thy data, 1, my numberOfColumns, 1, mn_min);
	return thee;
}

TableOfReal SVD_extractSingularValues (I)
{
	iam (SVD);
	TableOfReal thee;
	long mn_min = MIN (my numberOfRows, my numberOfColumns);

	thee = TableOfReal_create (1, mn_min);
	if (thee == NULL) return NULL;
	NUMdvector_copyElements (my d, thy data[1], 1, mn_min);
	return thee;
}

GSVD TablesOfReal_to_GSVD (I, thou)
{
	iam (TableOfReal);
	thouart (TableOfReal);
	GSVD him;

	if (my numberOfColumns != thy numberOfColumns) return Melder_errorp
		("TablesOfReal_to_GSVD: Both tables must have the same number of columns.");
	him = GSVD_create_d (my data, my numberOfRows, my numberOfColumns, thy data, thy numberOfRows);
	return him;
}

/* End of file SVD_and_TableOfReal.c */
