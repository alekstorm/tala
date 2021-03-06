/* Confusion.c
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
 djmw 20010628
 djmw 20020813 GPL header
 djmw 20061214 Changed info to Melder_writeLine<x> format.
 djmw 20070620 Latest modification.
 djmw 20080521 +Confusion_drawAsNumbers
 djmw 20110304 Thing_new
*/

#include "Confusion.h"
#include "Polygon_extensions.h"
#include "Matrix_extensions.h"
#include "TableOfReal_extensions.h"
#include "dwsys/Collection_extensions.h"
#include "stat/Distributions_and_Strings.h"
#include "num/NUM2.h"

static void info (I)
{
    iam (Confusion); 
    double h, hx, hy, hygx, hxgy, uygx, uxgy, uxy, frac;
    long nCorrect;
	
    Confusion_getEntropies (me, & h, & hx, & hy, & hygx, & hxgy, & uygx, 
		& uxgy, & uxy);
    Confusion_getFractionCorrect (me, & frac, & nCorrect);
    MelderInfo_writeLine2 (L"Number of rows: ", Melder_integer (my numberOfRows));
    MelderInfo_writeLine2 (L"Number of colums: ", Melder_integer (my numberOfColumns));
    MelderInfo_writeLine1 (L"Entropies (y is row variable):");
    MelderInfo_writeLine2 (L"  Total: ", Melder_double(h));
    MelderInfo_writeLine2 (L"  Y: ", Melder_double(hy));
    MelderInfo_writeLine2 (L"  X: ", Melder_double(hx));
    MelderInfo_writeLine2 (L"  Y given x: ", Melder_double(hygx));
    MelderInfo_writeLine2 (L"  X given y: ", Melder_double(hxgy));
    MelderInfo_writeLine2 (L"  Dependency of y on x; ", Melder_double(uygx));
    MelderInfo_writeLine2 (L"  Dependency of x on y: ", Melder_double(uxgy));
    MelderInfo_writeLine2 (L"  Symmetrical dependency: ", Melder_double(uxy));
    MelderInfo_writeLine2 (L"  Total number of entries: ", 
    	Melder_integer (Confusion_getNumberOfEntries (me)));
	MelderInfo_writeLine2 (L" Fraction correct: ", Melder_double (frac));
}

class_methods (Confusion, TableOfReal)
    class_method (info);
class_methods_end

Confusion Confusion_create (long numberOfStimuli, long numberOfResponses)
{
    Confusion me = Thing_new (Confusion);
    if ((me == NULL) || ! TableOfReal_init (me, numberOfStimuli,
		 numberOfResponses)) forget (me);
    return me;
}

Confusion Categories_to_Confusion (Categories me, Categories thee)
{
	Categories ul1 = NULL, ul2 = NULL;
	Confusion him = NULL;
	long i;
	
 	if (my size != thy size) return Melder_errorp
		("Categories_to_Confusion: dimensions do not agree.");
		
 	if (((ul1 = Categories_selectUniqueItems (me, 1)) == NULL) ||
 		 ((ul2 = Categories_selectUniqueItems (thee, 1)) == NULL)) goto end;
	
 	if ((him = Confusion_create (ul1 -> size, ul2 -> size)) == NULL) goto end;
	
 	for (i=1; i <= ul1 -> size; i++)
 	{
 		SimpleString s = ul1 -> item[i];
 		TableOfReal_setRowLabel (him, i, s -> string);
 	}
 	for (i=1; i <= ul2 -> size; i++)
 	{
 		SimpleString s = ul2 -> item[i];
 		TableOfReal_setColumnLabel (him, i, s -> string);
 	}
 	for (i=1; i <= my size; i++)
	{
 		if (! Confusion_addEntry (him, SimpleString_c (my item[i]), 
 			SimpleString_c (thy item[i])))
		{
			forget (him); goto end;
		}
	}

end:

	forget (ul1);
	forget (ul2);
	return him;
}

#define TINY 1.0e-30

void Confusion_getEntropies (Confusion me, double *h, double *hx, double *hy,
	double *hygx, double *hxgy, double *uygx, double *uxgy, double *uxy)
{
    long i, j; 
    double *rowSum = NULL, *colSum = NULL, sum = 0.0;
	
    *h = *hx = *hy = *hxgy = *hygx = *uygx = *uxgy = *uxy = 0;
	
    if (((rowSum = NUMdvector (1, my numberOfRows)) == NULL) ||
		((colSum = NUMdvector (1, my numberOfColumns)) == NULL)) goto end;
	
    for (i=1; i <= my numberOfRows; i++)
	{
    	for (j=1; j <= my numberOfColumns; j++)
		{
	    	rowSum[i] += my data[i][j];
	    	colSum[j] += my data[i][j];
	    	sum += my data[i][j];
		}
	}
    for (i=1; i <= my numberOfRows; i++)
	{
    	if (rowSum[i] > 0) *hy -= rowSum[i] / sum * NUMlog2 (rowSum[i] / sum);
	}
    for (j=1; j <= my numberOfColumns; j++)
	{
    	if (colSum[j] > 0) *hx -= colSum[j] / sum * NUMlog2 (colSum[j] / sum);
	}
    for (i=1; i <= my numberOfRows; i++)
	{
		for (j=1; j <= my numberOfColumns; j++)
		{
			if (my data[i][j] > 0)
			{
				*h -= my data[i][j] / sum * NUMlog2 (my data[i][j] / sum);
			}
		}
	}
	
    *hygx = *h - *hx;
    *hxgy = *h - *hy;
    *uygx = (*hy - *hygx) / (*hy + TINY);
    *uxgy = (*hx - *hxgy) / (*hx + TINY);
    *uxy = 2.0 * (*hx + *hy - *h) / (*hx + *hy + TINY);
	
end:

    NUMdvector_free (rowSum, 1);
    NUMdvector_free (colSum, 1);
}

int Confusion_addEntry (Confusion me, const wchar_t *stim, const wchar_t *resp)
{
    long stimIndex = TableOfReal_rowLabelToIndex (me, stim);
    long respIndex = TableOfReal_columnLabelToIndex (me, resp);
	
    if (stimIndex < 1 || respIndex < 1) return Melder_error
		("Confusion_addConfusion: stimulus or response not valid.");

    my data[stimIndex][respIndex] += 1;
    return 1;
}

void Confusion_getFractionCorrect (Confusion me, double *fraction,
	long *numberOfCorrect)
{
    double c = 0, ct = 0;
    long i, j;
	
    *fraction = NUMundefined;
    *numberOfCorrect = -1;
    
	for (i=1; i <= my numberOfRows; i++)
    {
    	for (j=1; j <= my numberOfColumns; j++)
    	{
    		if (my rowLabels[i] == NULL || my columnLabels[j] == NULL)
    		{
     			return;
    		}
   			ct += my data[i][j];
    		if (! wcscmp (my rowLabels[i], my columnLabels[j]))
				c += my data[i][j];
    	}
    }
	
	if (ct != 0)  *fraction = c / ct;
    *numberOfCorrect = c;
}

Any Confusion_difference (Confusion me, Confusion thee)
{
	Matrix him; long i, j;
	
	/* categories must be the same too*/
	if (my numberOfColumns != thy numberOfColumns ||
		my numberOfRows != thy numberOfRows) return Melder_errorp1
			(L"Confusion_difference: dimensions not equal.");

	if ((him = Matrix_create (0.5, my numberOfColumns + 0.5, my numberOfColumns,
		1, 1, 0.5, my numberOfRows+0.5, my numberOfRows, 1, 1)) == NULL)
			return him;
		
	for (i=1; i <= my numberOfRows; i++)
	{
		for (j=1; j <= my numberOfColumns; j++)
		{
			his z[i][j] = my data[i][j] - thy data[i][j];
		}
	}
	return him;
}

long Confusion_getNumberOfEntries (Confusion me)
{
	long i, j; double total = 0;
	for (i=1; i <= my numberOfRows; i++)
	{
		for (j=1; j <= my numberOfColumns; j++)
		{
			total += my data[i][j];
		}
	}
	return total;
}

static void create_index (wchar_t**s, long sb, long se, 
	wchar_t **ref, long rb, long re, long *index)
{
	long i, j, indxj;
	for (i = sb; i <= se; i++)
	{
		indxj = 0;
		for (j = rb; j <= re; j++)
		{
			if (wcsequ (s[i], ref[j]))
			{
				indxj = j; break;
			}
		}
		index[i] = indxj;		
	}
}
 
Confusion Confusion_condense (Confusion me, wchar_t *search, wchar_t *replace,
	long maximumNumberOfReplaces, int use_regexp)
{
	Confusion thee = NULL;
	Strings srow = NULL, scol = NULL;
	Distributions drow = NULL, dcol = NULL;
	wchar_t **rowLabels = NULL, **columnLabels = NULL;
	long i, j, nmatches, nstringmatches, nstim, nresp;
	long *rowIndex = NULL, *columnIndex = NULL;
	
	if (my rowLabels == NULL || my columnLabels == NULL) return 
		Melder_errorp1 (L"Confusion_condense: no row or column labels.");
	rowLabels = strs_replace (my rowLabels, 1, my numberOfRows, 
		search, replace, maximumNumberOfReplaces, &nmatches, 
		&nstringmatches, use_regexp);
	if (rowLabels == NULL) return NULL;
	
	columnLabels = strs_replace (my columnLabels, 1, my numberOfColumns, 
		search, replace, maximumNumberOfReplaces, &nmatches, 
		&nstringmatches, use_regexp);
	if (columnLabels == NULL) goto end;
	
	srow = Thing_new (Strings);
	if (srow == NULL) goto end;
	srow -> numberOfStrings = my numberOfRows;
	srow -> strings = rowLabels;
	
	scol = Thing_new (Strings);
	if (scol == NULL) goto end;
	scol -> numberOfStrings = my numberOfColumns;
	scol -> strings = columnLabels;

	/* Find dimension of new Confusion */
	dcol = Strings_to_Distributions (scol);
	if (dcol == NULL) goto end;
	nresp = dcol -> numberOfRows;
		
	drow = Strings_to_Distributions (srow);
	if (drow == NULL) goto end;
	nstim = drow -> numberOfRows;

	thee = Confusion_create (nstim, nresp);
	if (thee == NULL) goto end;
	
	if(! NUMstrings_copyElements (drow -> rowLabels, thy rowLabels, 
		1, nstim)) goto end;
	if (! NUMstrings_copyElements (dcol -> rowLabels, thy columnLabels, 
		1, nresp)) goto end;
	
	rowIndex = NUMlvector (1, my numberOfRows);
	if (rowIndex == NULL) goto end;
	create_index (rowLabels, 1, my numberOfRows, drow -> rowLabels, 
		1, nstim, rowIndex);
	
	columnIndex = NUMlvector (1, my numberOfColumns);
	if (columnIndex == NULL) goto end;
	create_index (columnLabels, 1, my numberOfColumns, dcol -> rowLabels, 
		1, nresp, columnIndex);
	
	for (i = 1; i <= my numberOfRows; i++)
	{
		for (j = 1; j <= my numberOfColumns; j++)
		{
			thy data [rowIndex [i]][columnIndex[j]] += my data[i][j];
		}
	}
		
end:
	NUMlvector_free (rowIndex, 1);
	NUMlvector_free (columnIndex, 1);
	NUMstrings_free (rowLabels, 1, my numberOfRows);
	NUMstrings_free (columnLabels, 1, my numberOfColumns);
	if (srow != NULL) srow -> strings = NULL;
	if (scol != NULL) scol -> strings = NULL;
	forget (srow); forget (scol);
	forget (drow); forget (dcol);
	if (Melder_hasError ())forget (thee);	
	return thee;
}

Confusion TableOfReal_to_Confusion (I)
{
	iam (TableOfReal); 
	Confusion thee;
	
	if (! TableOfReal_checkPositive (me) ||
		((thee = Data_copy (me)) == NULL)) return NULL;
	Thing_overrideClass (thee, classConfusion);
	return thee;
}

TableOfReal Confusion_to_TableOfReal_marginals (I)
{
	iam (Confusion);
	TableOfReal thee;
	long i, j;
	double total = 0;
	
	thee = TableOfReal_create (my numberOfRows + 1, my numberOfColumns + 1);
	if (thee == NULL) return NULL;
	
	for (i = 1; i <= my numberOfRows; i++)
	{
		double rowsum = 0;
		for (j = 1; j <= my numberOfColumns; j++)
		{
			thy data[i][j] = my data[i][j];
			rowsum += my data[i][j];
		}
		thy data[i][my numberOfColumns + 1] = rowsum;
		total += rowsum;
	}
	
	thy data[my numberOfRows + 1][my numberOfColumns + 1] = total;
	
	for (j = 1; j <= my numberOfColumns; j++)
	{
		double colsum = 0;
		for (i = 1; i <= my numberOfRows; i++)
		{
			colsum += my data[i][j];
		}
		thy data[my numberOfRows + 1][j] = colsum;
	}
	
	if (! NUMstrings_copyElements (my rowLabels, thy rowLabels, 
			1, my numberOfRows) || 
		! NUMstrings_copyElements (my columnLabels, thy columnLabels,
			1, my numberOfColumns)) forget (thee);
	return thee; 
}

/* End of file Confusion.c */
