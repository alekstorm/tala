#ifndef _CCA_h_
#define _CCA_h_
/* CCA.h
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
 djmw 2001
 djmw 20020423 GPL header
 djmw 20110306 Latest modification.
*/

#ifndef _Eigen_h_
	#include "dwsys/Eigen.h"
#endif

#ifndef _TableOfReal_h_
	#include "stat/TableOfReal.h"
#endif

#ifndef _Strings_h_
	#include "sys/Strings.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#define CCA_members Data_members \
	long numberOfCoefficients; \
	long numberOfObservations; \
	Strings yLabels; \
	Strings xLabels; \
	Eigen y; \
	Eigen x;
#define CCA_methods Data_methods
class_create (CCA, Data);

/*
	Class CCA represents the Canonical Correlation Analysis of two datasets
	(two tables with multivariate data, Table 1 was N rows x p columns, 
	Table 2 was N rows x q columns, and p <= q).
	
	Interpretation:
		
	The eigenvectors v1[i] en v2[i] have the property that for the linear
	compounds
	
	c1[1] = v1[1]' . Table1   c2[1]= v2[1]' . Table2
	..............................................
	c1[p] = v1[p]' . Table1   c2[p]= v2[p]' . Table2
	
	the sample correlation of c1[1] and c2[1] is greatest, the sample
	correlation of c1[2] and c2[2] is greatest amoung all linear compounds
	uncorrelated with c1[1] and c2[1], and so on, for all p possible pairs.	   	
*/

CCA CCA_create (long numberOfCoefficients, long ny, long nx);

double CCA_getEigenvectorElement (CCA me, int x_or_y, long ivec, long element);

CCA TableOfReal_to_CCA (TableOfReal me, long ny);
/*
	Solves the canonical correlation analysis equations:

	(S12*inv(S22)*S12' - lambda S11)X1 = 0 (1)
	(S12'*inv(S11)*S12 - lambda S22)X2 = 0 (2)
	
	Where S12 = T1' * T2, S11 = T1' * T1 and S22 = T2' * T2.
	Given the following svd's:
	
  	svd (T1) = U1 D1 V1'
    svd (T2) = U2 D2 V2'
	
	We can write down:
	
	inv(S11) = V1 * D1^-2 * V1' and inv(S22) = V2 * D2^-2 * V2',
	and S12*inv(S22)*S12' simplifies to: V1*D1*U1'*U2 * U2'*U1*D1*V1'
	
	and (1) becomes:
	
	(V1*D1*U1'*U2 * U2'*U1*D1*V1' -lambda V1*D1 * D1*V1')X1 = 0
	
	This can be written as:
	
	(V1*D1*U1'*U2 * U2'*U1 -lambda V1*D1) D1*V1'*X1 = 0
	
	multiplying from the left with: D1^-1*V1' results in
	
	(U1'*U2 * U2'*U1 -lambda) D1*V1'*X1 = 0
	
	Taking the svd(U2'*U1) = U D V' we get:
	
	(D^2 -lambda)V'*D1*V1'*X1 = 0
	
	The eigenvectors X1 can be formally written as:
	
	X1 = V1*inv(D1)*V
	
	Equation (2) results in:
	
	X2 = V2*inv(D2)*U
*/

TableOfReal CCA_and_TableOfReal_scores (CCA me, TableOfReal thee, long numberOfFactors);
/*
	Return the factors in a table with 2*numberOfFactors columns.
	The first 'numberOfFactors' columns are the scores for the dependent part
	of the table the following 'numberOfFactors' columns are for the 
	independent part.
*/

TableOfReal CCA_and_TableOfReal_factorLoadings (CCA me, TableOfReal thee);
/*
	Get the canonical factor loadings (also structure correlation coefficients),
	the correlation of a canonical variable with an original variable.
*/

double CCA_getCorrelationCoefficient (CCA me, long index);

void CCA_getZeroCorrelationProbability (CCA me, long index, double *chisq,
	long *ndf, double *probability);

TableOfReal CCA_and_TableOfReal_predict (CCA me, TableOfReal thee, long from);
/*
	Given independent table, predict the dependent one, on the basis of 
	the canonical correlations.
*/

#ifdef __cplusplus
	}
#endif

#endif /* CCA.h */
