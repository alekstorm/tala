/* SSCP_def.h
 *
 * Copyright (C) 1993-2010 David Weenink
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
 djmw 19981213
 djmw 20020813 GPL header
*/

#define ooSTRUCT SSCP
oo_DEFINE_CLASS (SSCP, TableOfReal)

	oo_DOUBLE (numberOfObservations)
	oo_DOUBLE_VECTOR (centroid, my numberOfColumns)
	/*
		The following definitions are only needed when we want to use many big diagonal or
		almost diagonal matrices like for example in a GaussianMixture,
		or for efficiently calculating many times a distance like a'S^(-1)a
	*/
	#if !oo_READING && !oo_WRITING
		oo_LONG (expansionNumberOfRows)
		oo_INT (dataChanged)
		oo_DOUBLE_MATRIX (expansion, my expansionNumberOfRows, my numberOfColumns)
		oo_DOUBLE (lnd)
		oo_DOUBLE_MATRIX (lowerCholesky, my numberOfColumns, my numberOfColumns)
		oo_OBJECT (PCA, 0, pca)
	#endif
oo_END_CLASS (SSCP)

#undef ooSTRUCT

/* End of file SSCP_def.h */
