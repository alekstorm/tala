/* TableOfReal_extensions.c
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
 djmw 20000202 17 typos in F1-3,L1-3 table corrected
 djmw 20030707 Changed TableOfReal_drawVectors interface.
 djmw 20031030 Added TableOfReal_appendColumns
 djmw 20031216 Interface change in TableOfReal_choleskyDecomposition
 djmw 20040108 Corrected a memory leak in TableOfReal_drawBoxPlots
 djmw 20040211 Modified TableOfReal_copyLabels behaviour: copy NULL-labels too.
 djmw 20040511 Removed TableOfReal_extractRowsByRowLabel,TableOfReal_selectColumnsWhereRow
 djmw 20040617 Removed column selection bug in TableOfReal_drawRowsAsHistogram
 djmw 20041105 Added TableOfReal_createFromVanNieropData_25females
 djmw 20041115 TableOfReal_drawScatterPlot: plotting a NULL-label crashed Praat.
 djmw 20041213 Added TableOfReal_createFromWeeninkData.
 djmw 20050221 TableOfReal_meansByRowLabels, extra reduce parameter.
 djmw 20050222 TableOfReal_drawVectors didn't draw rowlabels.
 djmw 20050512 TableOfReal TableOfReal_meansByRowLabels crashed if first label in sorted was NULL.
 djmw 20051116 TableOfReal_drawScatterPlot draw reverse permited by choosing xmin > xmax and/or ymin>ymax
 djmw 20060301 TableOfReal_meansByRowLabels extra medianize
 djmw 20060626 Extra NULL argument for ExecRE.
 djmw 20061021 printf expects %ld for 'long int'
 djmw 20070822 wchar_t
 djmw 20070902 Better error messages (object type and name feedback)
 djmw 20070614 updated to version 1.30 of regular expressions.
 djmw 20071202 Melder_warning<n>
 djmw 20080122 float -> double
 djmw 20081119 +TableOfReal_areAllCellsDefined
 djmw 20090506 +setInner for _drawScatterPlotMatrix
 djmw 20091009 +TableOfReal_drawColumnAsDistribution
 djmw 20100222 Corrected a bug in TableOfReal_copyOneRowWithLabel which caused label corruption if 
               from and to table were equal and rows were equal too.
*/

#include <ctype.h>
#include "SSCP.h"
#include "Matrix_extensions.h"
#include "num/NUMclapack.h"
#include "num/NUM2.h"
#include "dwsys/SVD.h"
#include "TableOfReal_extensions.h"
#include "TableOfReal_and_Permutation.h"
#include "dwsys/regularExp.h"
#include "ui/Formula.h"
#include "Table_extensions.h"

#define MAX(m,n) ((m) > (n) ? (m) : (n))
#define MIN(m,n) ((m) < (n) ? (m) : (n))

TableOfReal TableOfReal_and_TableOfReal_columnCorrelations (I, thou, int center, int normalize);
TableOfReal TableOfReal_and_TableOfReal_rowCorrelations (I, thou, int center, int normalize);

int TableOfReal_areAllCellsDefined (I, long rb, long re, long cb, long ce)
{
	iam (TableOfReal);
	long *invalid_columns;
	long i, j, numberOfInvalidRows = 0, numberOfInvalidColumns = 0;

	if (re <= rb || rb < 1 || re > my numberOfRows) { rb = 1; re = my numberOfRows; }
	if (ce <= cb || cb < 1 || ce > my numberOfColumns) { cb = 1; ce = my numberOfColumns; }

	invalid_columns = NUMlvector (1, my numberOfColumns);
	if (invalid_columns == NULL) return 0;

	for (i = rb; i <= re; i++)
	{
		for (j = cb; j <= ce; j++)
		{
			long rowcount = 0;
			if (my data[i][j] == NUMundefined)
			{
				invalid_columns[j]++;
				rowcount++;
			}
			if (rowcount > 0) numberOfInvalidRows++;
		}
	}
	if (numberOfInvalidRows != 0)
	{
		for (j = 1; j <= my numberOfColumns; j++)
		{
			if (invalid_columns[j] > 0) numberOfInvalidColumns++;
		}
		(void) Melder_error1 (numberOfInvalidRows == 1 ? L"One row contains invalid data." :
			(numberOfInvalidColumns == 1 ?  L"One column contains invalid data." : L"Several rows and columns contain invalid data."));
	}
	NUMlvector_free (invalid_columns, 1);
	return numberOfInvalidRows == 0 ? 1 : 0;
}

int TableOfReal_copyOneRowWithLabel (I, thou, long myrow, long thyrow)
{
	iam (TableOfReal); thouart (TableOfReal);

	if ( myrow < 1 ||  myrow > my  numberOfRows ||
		thyrow < 1 || thyrow > thy numberOfRows ||
		my numberOfColumns != thy numberOfColumns) return 0;
	
	if (me == thee && myrow == thyrow) return 1; 
	
	Melder_free (thy rowLabels[thyrow]);
	if (my rowLabels[myrow] != NULL && thy rowLabels[thyrow] != my rowLabels[myrow])
	{
		thy rowLabels[thyrow] = Melder_wcsdup_e (my rowLabels[myrow]);
		if (thy rowLabels[thyrow] == NULL) return 0;
	}
	if (my data[myrow] != thy data[thyrow]) NUMdvector_copyElements (my data[myrow], thy data[thyrow], 1, my numberOfColumns);
	return 1;
}

int TableOfReal_hasRowLabels (I)
{
	iam (TableOfReal);
	long i;

	if (my rowLabels == NULL) return 0;
	for (i = 1; i <= my numberOfRows; i++)
	{
		if (EMPTY_STRING(my rowLabels[i])) return 0;
	}
	return 1;
}

int TableOfReal_hasColumnLabels (I)
{
	iam (TableOfReal);
	long i;

	if (my columnLabels == NULL) return 0;
	for (i = 1; i <= my numberOfColumns; i++)
	{
		if (EMPTY_STRING (my columnLabels[i])) return 0;
	}
	return 1;
}

TableOfReal TableOfReal_createIrisDataset (void)
{
	TableOfReal me = NULL;
	long i, j;
	float iris[150][4] = {
	5.1,3.5,1.4,0.2,4.9,3.0,1.4,0.2,4.7,3.2,1.3,0.2,4.6,3.1,1.5,0.2,5.0,3.6,1.4,0.2,
	5.4,3.9,1.7,0.4,4.6,3.4,1.4,0.3,5.0,3.4,1.5,0.2,4.4,2.9,1.4,0.2,4.9,3.1,1.5,0.1,
	5.4,3.7,1.5,0.2,4.8,3.4,1.6,0.2,4.8,3.0,1.4,0.1,4.3,3.0,1.1,0.1,5.8,4.0,1.2,0.2,
	5.7,4.4,1.5,0.4,5.4,3.9,1.3,0.4,5.1,3.5,1.4,0.3,5.7,3.8,1.7,0.3,5.1,3.8,1.5,0.3,
	5.4,3.4,1.7,0.2,5.1,3.7,1.5,0.4,4.6,3.6,1.0,0.2,5.1,3.3,1.7,0.5,4.8,3.4,1.9,0.2,
	5.0,3.0,1.6,0.2,5.0,3.4,1.6,0.4,5.2,3.5,1.5,0.2,5.2,3.4,1.4,0.2,4.7,3.2,1.6,0.2,
	4.8,3.1,1.6,0.2,5.4,3.4,1.5,0.4,5.2,4.1,1.5,0.1,5.5,4.2,1.4,0.2,4.9,3.1,1.5,0.2,
	5.0,3.2,1.2,0.2,5.5,3.5,1.3,0.2,4.9,3.6,1.4,0.1,4.4,3.0,1.3,0.2,5.1,3.4,1.5,0.2,
	5.0,3.5,1.3,0.3,4.5,2.3,1.3,0.3,4.4,3.2,1.3,0.2,5.0,3.5,1.6,0.6,5.1,3.8,1.9,0.4,
	4.8,3.0,1.4,0.3,5.1,3.8,1.6,0.2,4.6,3.2,1.4,0.2,5.3,3.7,1.5,0.2,5.0,3.3,1.4,0.2,
	7.0,3.2,4.7,1.4,6.4,3.2,4.5,1.5,6.9,3.1,4.9,1.5,5.5,2.3,4.0,1.3,6.5,2.8,4.6,1.5,
	5.7,2.8,4.5,1.3,6.3,3.3,4.7,1.6,4.9,2.4,3.3,1.0,6.6,2.9,4.6,1.3,5.2,2.7,3.9,1.4,
	5.0,2.0,3.5,1.0,5.9,3.0,4.2,1.5,6.0,2.2,4.0,1.0,6.1,2.9,4.7,1.4,5.6,2.9,3.6,1.3,
	6.7,3.1,4.4,1.4,5.6,3.0,4.5,1.5,5.8,2.7,4.1,1.0,6.2,2.2,4.5,1.5,5.6,2.5,3.9,1.1,
	5.9,3.2,4.8,1.8,6.1,2.8,4.0,1.3,6.3,2.5,4.9,1.5,6.1,2.8,4.7,1.2,6.4,2.9,4.3,1.3,
	6.6,3.0,4.4,1.4,6.8,2.8,4.8,1.4,6.7,3.0,5.0,1.7,6.0,2.9,4.5,1.5,5.7,2.6,3.5,1.0,
	5.5,2.4,3.8,1.1,5.5,2.4,3.7,1.0,5.8,2.7,3.9,1.2,6.0,2.7,5.1,1.6,5.4,3.0,4.5,1.5,
	6.0,3.4,4.5,1.6,6.7,3.1,4.7,1.5,6.3,2.3,4.4,1.3,5.6,3.0,4.1,1.3,5.5,2.5,4.0,1.3,
	5.5,2.6,4.4,1.2,6.1,3.0,4.6,1.4,5.8,2.6,4.0,1.2,5.0,2.3,3.3,1.0,5.6,2.7,4.2,1.3,
	5.7,3.0,4.2,1.2,5.7,2.9,4.2,1.3,6.2,2.9,4.3,1.3,5.1,2.5,3.0,1.1,5.7,2.8,4.1,1.3,
	6.3,3.3,6.0,2.5,5.8,2.7,5.1,1.9,7.1,3.0,5.9,2.1,6.3,2.9,5.6,1.8,6.5,3.0,5.8,2.2,
	7.6,3.0,6.6,2.1,4.9,2.5,4.5,1.7,7.3,2.9,6.3,1.8,6.7,2.5,5.8,1.8,7.2,3.6,6.1,2.5,
	6.5,3.2,5.1,2.0,6.4,2.7,5.3,1.9,6.8,3.0,5.5,2.1,5.7,2.5,5.0,2.0,5.8,2.8,5.1,2.4,
	6.4,3.2,5.3,2.3,6.5,3.0,5.5,1.8,7.7,3.8,6.7,2.2,7.7,2.6,6.9,2.3,6.0,2.2,5.0,1.5,
	6.9,3.2,5.7,2.3,5.6,2.8,4.9,2.0,7.7,2.8,6.7,2.0,6.3,2.7,4.9,1.8,6.7,3.3,5.7,2.1,
	7.2,3.2,6.0,1.8,6.2,2.8,4.8,1.8,6.1,3.0,4.9,1.8,6.4,2.8,5.6,2.1,7.2,3.0,5.8,1.6,
	7.4,2.8,6.1,1.9,7.9,3.8,6.4,2.0,6.4,2.8,5.6,2.2,6.3,2.8,5.1,1.5,6.1,2.6,5.6,1.4,
	7.7,3.0,6.1,2.3,6.3,3.4,5.6,2.4,6.4,3.1,5.5,1.8,6.0,3.0,4.8,1.8,6.9,3.1,5.4,2.1,
	6.7,3.1,5.6,2.4,6.9,3.1,5.1,2.3,5.8,2.7,5.1,1.9,6.8,3.2,5.9,2.3,6.7,3.3,5.7,2.5,
	6.7,3.0,5.2,2.3,6.3,2.5,5.0,1.9,6.5,3.0,5.2,2.0,6.2,3.4,5.4,2.3,5.9,3.0,5.1,1.8
	};

	if (! (me = TableOfReal_create (150, 4))) return NULL;

	TableOfReal_setColumnLabel (me, 1, L"sl");
	TableOfReal_setColumnLabel (me, 2, L"sw");
	TableOfReal_setColumnLabel (me, 3, L"pl");
	TableOfReal_setColumnLabel (me, 4, L"pw");
	for (i = 1; i <= 150; i++)
	{
		int kind = (i - 1) / 50 + 1;
		wchar_t *label = kind == 1 ? L"1" : kind == 2 ? L"2" : L"3";
		for (j=1; j <= 4; j++) my data[i][j] = iris[i-1][j-1];
		TableOfReal_setRowLabel (me, i, label);
	}
	Thing_setName (me, L"iris");
	return me;
}

Strings TableOfReal_extractRowLabels (I)
{
	iam (TableOfReal);
	Strings thee = Thing_new (Strings);
	long i, n = my numberOfRows;

	if (thee == NULL) return NULL;
	if (n < 1) return thee;

	thy strings = NUMpvector (1, n);
	if (thy strings == NULL) goto end;
	thy numberOfStrings = n;

	for (i = 1; i <= n; i++)
	{
		wchar_t *label = my rowLabels[i] ? my rowLabels[i] : L"?";
		thy strings[i] = Melder_wcsdup_e (label);
		if (thy strings[i] == NULL) goto end;
	}

end:

	if (Melder_hasError()) forget (thee);
	return thee;
}


Strings TableOfReal_extractColumnLabels (I)
{
	iam (TableOfReal);
	Strings thee = Thing_new (Strings);
	long i, n = my numberOfColumns;

	if (thee == NULL) return NULL;
	if (n < 1) return thee;

	thy strings = NUMpvector (1, n);
	if (thy strings == NULL) goto end;
	thy numberOfStrings = n;

	for (i = 1; i <= n; i++)
	{
		wchar_t *label = my columnLabels[i] ? my columnLabels[i] : L"?";
		thy strings[i] = Melder_wcsdup_e (label);
		if (thy strings[i] == NULL) goto end;
	}

end:

	if (Melder_hasError()) forget (thee);
	return thee;
}

TableOfReal TableOfReal_transpose (I)
{
	iam (TableOfReal);
	TableOfReal thee;
	long i, j;

	thee = TableOfReal_create (my numberOfColumns, my numberOfRows);
	if (thee == NULL) return NULL;

	for (i = 1; i <= my numberOfRows; i++)
	{
		for (j = 1; j <= my numberOfColumns; j++)
		{
			thy data[j][i] = my data[i][j];
		}
	}

	if (! NUMstrings_copyElements (my rowLabels, thy columnLabels,
		1, my numberOfRows) ||
		! NUMstrings_copyElements (my columnLabels, thy rowLabels,
		1, my numberOfColumns)) forget (thee);
	return thee;
}

int TableOfReal_to_Pattern_and_Categories (I, long fromrow, long torow, long fromcol, long tocol,
	Pattern *p, Categories *c)
{
	iam (TableOfReal);
	long i, j, ncol = my numberOfColumns, nrow = my numberOfRows, row, col;
	int status = 1;

	if (fromrow == torow && fromrow == 0)
	{
		fromrow = 1; torow = nrow;
	}
	else if (fromrow > 0 && fromrow <= nrow && torow == 0)
	{
		torow = nrow;
	}
	else if (! (fromrow > 0 && torow <= nrow && fromrow <= torow))
	{
		return Melder_error2 (L"Illegal row selection for ", Thing_messageName(me));
	}
	if (fromcol == tocol && fromcol == 0)
	{
		fromcol = 1; tocol = ncol;
	}
	else if (fromcol > 0 && fromcol <= ncol && tocol == 0)
	{
		tocol = ncol;
	}
	else if (! (fromcol > 0 && tocol <= ncol && fromcol <= tocol))
	{
		return Melder_error2 (L"Illegal col selection for ", Thing_messageName(me));
	}
	nrow = torow - fromrow + 1;
	ncol = tocol - fromcol + 1;

	*c = NULL;
	if (! (*p = Pattern_create (nrow, ncol)) ||
		! (*c = Categories_create ())) goto end;

	for (row=1, i=fromrow; i <= torow; i++, row++)
	{
		wchar_t *s = my rowLabels[i] ? my rowLabels[i] : L"?";
		SimpleString item = SimpleString_create (s);
		if (! item || ! Collection_addItem (*c, item)) goto end;
		for (col=1, j=fromcol; j <= tocol; j++, col++)
		{
			(*p)->z[row][col] = my data[i][j];
		}
	}

end:

	if (Melder_hasError ())
	{
		forget (*p);
		forget (*c);
		status = 0;
	}
	return status;
}

void TableOfReal_getColumnExtrema (I, long col, double *min, double *max)
{
	iam (TableOfReal); long i;
	if (col < 1 || col > my numberOfColumns)
	{
		(void) Melder_error2 (L"Not a valid column for ", Thing_messageName(me));
		*min = NUMundefined; *max = NUMundefined; return;
	}
	*min = *max = my data[1][col];
	for (i=2; i <= my numberOfRows; i++)
	{
		if (my data[i][col] > *max) *max = my data[i][col];
		else if (my data[i][col] < *min) *min = my data[i][col];
	}
}

int TableOfReal_equalLabels (I, thou, int rowLabels, int columnLabels)
{
	iam (TableOfReal); thouart (TableOfReal); long i;
	Melder_assert (rowLabels || columnLabels);
	if (rowLabels)
	{
		if (my numberOfRows != thy numberOfRows) return 0;
		if (my rowLabels == thy rowLabels) return 1;
		for (i=1; i <= my numberOfRows; i++)
		{
			if (Melder_wcscmp (my rowLabels[i], thy rowLabels[i])) return 0;
		}
	}
	if (columnLabels)
	{
		if (my numberOfColumns != thy numberOfColumns) return 0;
		if (my columnLabels == thy columnLabels) return 1;
		for (i=1; i <= my numberOfColumns; i++)
		{
			if (Melder_wcscmp (my columnLabels[i], thy columnLabels[i])) return 0;
		}
	}
	return 1;
}

int TableOfReal_copyLabels (I, thou, int rowOrigin, int columnOrigin)
{
	iam (TableOfReal);
	thouart (TableOfReal);

	if (rowOrigin == 1)
	{
		if (my numberOfRows != thy numberOfRows ||
			! NUMstrings_copyElements (my rowLabels, thy rowLabels, 1, thy numberOfRows)) return 0;
	}
	else if (rowOrigin == -1)
	{
		if (my numberOfColumns != thy numberOfRows ||
			! NUMstrings_copyElements (my columnLabels, thy rowLabels, 1, thy numberOfRows)) return 0;
	}
	if (columnOrigin == 1)
	{
		if (my numberOfColumns != thy numberOfColumns ||
			! NUMstrings_copyElements (my columnLabels, thy columnLabels, 1, thy numberOfColumns)) return 0;
	}
	else if (columnOrigin == -1)
	{
		if (my numberOfRows != thy numberOfColumns ||
			! NUMstrings_copyElements (my rowLabels, thy columnLabels, 1, thy numberOfColumns)) return 0;
	}
	return 1;
}

void TableOfReal_labelsFromCollectionItemNames (I, thou, int row, int column)
{
	iam (TableOfReal);
	thouart (Collection);
	long i;
	wchar_t *name;

	if (row)
	{
		Melder_assert (my numberOfRows == thy size);
		for (i = 1; i <= my numberOfRows; i++)
		{
			name = Thing_getName (thy item[i]);
			if (name != NULL) TableOfReal_setRowLabel (me, i, name);
		}
	}
	if (column)
	{
		Melder_assert (my numberOfColumns == thy size);
		for (i=1; i <= my numberOfColumns; i++)
		{
			name = Thing_getName (thy item[i]);
			if (name != NULL) TableOfReal_setColumnLabel (me, i, name);
		}
	}
}

void TableOfReal_centreColumns (I)
{
	iam (TableOfReal);
	NUMcentreColumns (my data, 1, my numberOfRows, 1, my numberOfColumns, NULL);
}

int TableOfReal_and_Categories_setRowLabels (I, Categories thee)
{
	iam (TableOfReal);
	Categories c = NULL;
	long i;
	if (my numberOfRows != thy size) return Melder_error5
		(L"The number of items in ", Thing_messageName(me), L"and ", Thing_messageName(thee), L" must be equal.");

	/*
		If anything goes wrong we must leave the Table intact.
		We first copy the Categories, swap the labels
		and then delete the newly created categories.
	*/

	c = Data_copy (thee);
	if (c == NULL) return 0;

	for (i=1; i <= my numberOfRows; i++)
	{
		SimpleString s = c -> item[i];
		wchar_t *t = s -> string;
		s -> string = my rowLabels[i];
		my rowLabels[i] = t;
	}

	forget (c);
	return 1;
}

void TableOfReal_centreColumns_byRowLabel (I)
{
	iam (TableOfReal); wchar_t *label = my rowLabels[1];
	long i, index = 1;

	for (i=2; i <= my numberOfRows; i++)
	{
		wchar_t *li = my rowLabels[i];
		if (li != NULL && li != label && wcscmp (li, label))
		{
			NUMcentreColumns (my data, index, i - 1, 1, my numberOfColumns, NULL);
			label = li; index = i;
		}
	}
	NUMcentreColumns (my data, index, my numberOfRows, 1, my numberOfColumns, NULL);
}

double TableOfReal_getRowSum (I, long index)
{
	iam (TableOfReal);
	double sum = 0;
	long j;
	if (index < 1 || index > my numberOfRows)
	{
		return NUMundefined;
	}
	for (j = 1; j <= my numberOfColumns; j++)
	{
		sum += my data[index][j];
	}
	return sum;
}

double TableOfReal_getColumnSum (I, long index)
{
	iam (TableOfReal);
	double sum = 0;
	long i;
	if (index < 1 || index > my numberOfColumns)
	{
		return NUMundefined;
	}
	for (i = 1; i <= my numberOfRows; i++)
	{
		sum += my data[i][index];
	}
	return sum;
}

double TableOfReal_getGrandSum (I)
{
	iam (TableOfReal);
	double sum = 0;
	long i, j;
	for (i = 1; i <= my numberOfRows; i++)
	{
		for (j = 1; j <= my numberOfColumns; j++)
		{
			sum += my data[i][j];
		}
	}
	return sum;
}

void TableOfReal_centreRows (I)
{
	iam (TableOfReal);
	NUMcentreRows (my data, 1, my numberOfRows, 1, my numberOfColumns);
}

void TableOfReal_doubleCentre (I)
{
	iam (TableOfReal);
	NUMdoubleCentre (my data, 1, my numberOfRows, 1, my numberOfColumns);
}

void TableOfReal_normalizeColumns (I, double norm)
{
	iam (TableOfReal);
	NUMnormalizeColumns (my data, my numberOfRows, my numberOfColumns, norm);
}

void TableOfReal_normalizeRows (I, double norm)
{
	iam (TableOfReal);
	NUMnormalizeRows (my data, my numberOfRows, my numberOfColumns, norm);
}

void TableOfReal_standardizeColumns (I)
{
	iam (TableOfReal);
	NUMstandardizeColumns (my data, 1, my numberOfRows, 1, my numberOfColumns);
}

void TableOfReal_standardizeRows (I)
{
	iam (TableOfReal);
	NUMstandardizeRows (my data, 1, my numberOfRows, 1, my numberOfColumns);
}

void TableOfReal_normalizeTable (I, double norm)
{
	iam (TableOfReal);
	NUMnormalize (my data, my numberOfRows, my numberOfColumns, norm);
}

double TableOfReal_getTableNorm (I)
{
	iam (TableOfReal);
	double sumsq = 0;
	long i, j;
	for (i = 1; i <= my numberOfRows; i++)
	{
		for (j = 1; j <= my numberOfColumns; j++)
		{
			sumsq += my data[i][j] * my data[i][j];
		}
	}
	return sqrt (sumsq);
}

int TableOfReal_checkPositive (I)
{
	iam (TableOfReal);
	long i, j, negative = 0;

	for (i = 1; i <= my numberOfRows; i++)
	{
		for (j = 1; j <= my numberOfColumns; j++)
		{
			if (my data[i][j] < 0) { negative ++; break; }
		}
	}
	return negative == 0 ? 1 :
		Melder_error2 (L"All matrix entries should be positive for ", Thing_messageName(me));
}

/****************  TABLESOFREAL **************************************/

class_methods (TablesOfReal, Ordered)
class_methods_end

int TablesOfReal_init (I, void *klas)
{
	iam (TablesOfReal);
	if (! me || ! Ordered_init (me, klas, 10)) return 0;
	return 1;
}

TablesOfReal TablesOfReal_create (void)
{
	TablesOfReal me = Thing_new (TablesOfReal);
	if (! me || ! TablesOfReal_init (me, classTableOfReal)) forget (me);
	return me;
}

TableOfReal TablesOfReal_sum (I)
{
	iam (TablesOfReal); TableOfReal thee;
	long i, j, k;
	if (my size <= 0) return NULL;
	if (! (thee = Data_copy (my item[1]))) { forget (thee); return NULL; }
	for (i=2; i <= my size; i++)
	{
		TableOfReal him = my item[i];
		if (thy numberOfRows != his numberOfRows || thy numberOfColumns != his numberOfColumns ||
			! TableOfReal_equalLabels (thee, him, 1, 1))
		{
			forget (thee);
			return Melder_errorp5 (L"TablesOfReal_sum: dimensions or labels differ for items 1 and ", Melder_integer(i), L" in ", Thing_messageName(thee), Thing_messageName(him));
		}
		for (j=1; j <= thy numberOfRows; j++)
			for (k=1; k <= thy numberOfColumns; k++) thy data[j][k] += his data[j][k];
	}
	return thee;
}

int TablesOfReal_checkDimensions (I)
{
	iam (TablesOfReal); TableOfReal t1; long i;
	if (my size < 2) return 1;
	t1 = my item[1];
	for (i=2; i <= my size; i++)
	{
		TableOfReal t = my item[i];
		if (t -> numberOfColumns != t1 -> numberOfColumns ||
			t -> numberOfRows != t1 -> numberOfRows) return 0;
	}
	return 1;
}

double TableOfReal_getColumnQuantile (I, long col, double quantile)
{
	iam (TableOfReal); long i, m = my numberOfRows;
	double *values, r;

	if (col < 1 || col > my numberOfColumns) return NUMundefined;

	if ((values = NUMdvector (1, m)) == NULL) return NUMundefined;

	for (i = 1; i <= m; i++)
	{
		values[i] = my data[i][col];
	}

	NUMsort_d (m, values);
	r = NUMquantile (m, values, quantile);

	NUMdvector_free (values, 1);

	return r;
}

static TableOfReal TableOfReal_createPolsVanNieropData (int choice, int include_levels)
{
	Table table;
	TableOfReal thee = NULL;
	long i, j, ib, nrows, ncols = include_levels ? 6 : 3;

	table = Table_createFromPolsVanNieropData ();
	if (table == NULL) return NULL;

	/* Default: Pols 50 males, first part of the table. */

	nrows = 50 * 12;
	ib = 1;

	if (choice == 2) /* Van Nierop 25 females */
	{
		ib = nrows + 1;
		nrows = 25 * 12;
	}

	thee = TableOfReal_create (nrows, ncols);
	if (thee == NULL) goto end;

	for (i = 1; i <= nrows; i++)
	{
		TableRow row = table -> rows -> item[ib + i - 1];
		TableOfReal_setRowLabel (thee, i, row -> cells[4].string);
		for (j = 1; j <= 3; j++)
		{
			thy data[i][j] = Melder_atof (row -> cells[4+j].string);
			if (include_levels) thy data[i][3+j] = Melder_atof (row -> cells[7+j].string);
		}
	}
	for (j = 1; j <= 3; j++)
	{
		wchar_t *label = table -> columnHeaders[4+j].label;
		TableOfReal_setColumnLabel (thee, j, label);
		if (include_levels)
		{
			label = table -> columnHeaders[7+j].label;
			TableOfReal_setColumnLabel (thee, 3+j, label);
		}
	}

end:
	forget (table);
	return thee;
}

TableOfReal TableOfReal_createFromPolsData_50males (int include_levels)
{
	return TableOfReal_createPolsVanNieropData (1, include_levels);
}

TableOfReal TableOfReal_createFromVanNieropData_25females (int include_levels)
{
	return TableOfReal_createPolsVanNieropData (2, include_levels);
}

TableOfReal TableOfReal_createFromWeeninkData (int option)
{
	Table table;
	TableOfReal thee = NULL;
	long i, ib, j, nvowels = 12, ncols = 3, nrows = 10 * nvowels;

	table = Table_createFromWeeninkData ();
	if (table == NULL) return NULL;

	ib = option == 1 ? 1 : option == 2 ? 11 : 21; /* m f c*/
	ib = (ib -1) * nvowels + 1;

	thee = TableOfReal_create (nrows, ncols);
	if (thee == NULL) goto end;

	for (i = 1; i <= nrows; i++)
	{
		TableRow row = table -> rows -> item[ib + i - 1];
		TableOfReal_setRowLabel (thee, i, row -> cells[5].string);
		for (j = 1; j <= 3; j++)
		{
			thy data[i][j] = Melder_atof (row -> cells[6+j].string); /* Skip F0 */
		}
	}
	for (j = 1; j <= 3; j++)
	{
		wchar_t *label = table -> columnHeaders[6+j].label;
		TableOfReal_setColumnLabel (thee, j, label);
	}
end:
	forget (table);
	return thee;
}

TableOfReal TableOfReal_randomizeRows (TableOfReal me)
{
	TableOfReal thee = NULL;
	Permutation p = NULL, pp = NULL;

	p = Permutation_create (my numberOfRows);
	if (p == NULL) return NULL;
	pp = Permutation_permuteRandomly (p, 0, 0);
	if (pp == NULL) goto end;
	thee = TableOfReal_and_Permutation_permuteRows (me, pp);
end:
	forget (p);
	forget (pp);
	return thee;
}

TableOfReal TableOfReal_bootstrap (TableOfReal me)
{
	TableOfReal thee = NULL;
	long i;

	thee = TableOfReal_create (my numberOfRows, my numberOfColumns);
	if (thee == NULL) return NULL;

	/*
		Copy column labels.
	*/

	for (i = 1; i <= my numberOfColumns; i++)
	{
		if (my columnLabels[i])
		{
			TableOfReal_setColumnLabel (thee, i, my columnLabels[i]);
		}
	}

	/*
		Select randomly from table with replacement. Because of replacement
		you do not get back the original data set. A random fraction,
		typically 1/e (37%) are replaced by duplicates.
	*/

	for (i = 1; i <= my numberOfRows; i++)
	{
		long p = NUMrandomInteger (1, my numberOfRows);
		NUMdvector_copyElements (my data[p], thy data[i],
			1, my numberOfColumns);
		if (my rowLabels[p])
		{
			TableOfReal_setRowLabel (thee, i, my rowLabels[p]);
		}
	}

	if (Melder_hasError ()) forget (thee);
	return thee;
}

int TableOfReal_changeRowLabels (I, wchar_t *search, wchar_t *replace,
	int maximumNumberOfReplaces, long *nmatches, long *nstringmatches,
	int use_regexp)
{
	iam (TableOfReal);
	wchar_t ** rowLabels = strs_replace (my rowLabels, 1, my numberOfRows,
		search, replace, maximumNumberOfReplaces, nmatches,
		nstringmatches, use_regexp);
	if (rowLabels == NULL) return 0;
	NUMstrings_free (my rowLabels, 1, my numberOfRows);
	my rowLabels = rowLabels;
	return 1;
}

int TableOfReal_changeColumnLabels (I, wchar_t *search, wchar_t *replace,
	int maximumNumberOfReplaces, long *nmatches, long *nstringmatches,
	int use_regexp)
{
	iam (TableOfReal);
	wchar_t ** columnLabels = strs_replace (my columnLabels, 1, my numberOfColumns,
		search, replace, maximumNumberOfReplaces, nmatches,
		nstringmatches, use_regexp);
	if (columnLabels == NULL) return 0;
	NUMstrings_free (my columnLabels, 1, my numberOfColumns);
	my columnLabels = columnLabels;
	return 1;
}

long TableOfReal_getNumberOfLabelMatches (I, wchar_t *search, int columnLabels,
	int use_regexp)
{
	iam (TableOfReal);
	long i, nmatches = 0, numberOfLabels = my numberOfRows;
	wchar_t **labels = my rowLabels;
	regexp *compiled_regexp = NULL;

	if (search == NULL || wcslen (search) == 0) return 0;
	if (columnLabels)
	{
		numberOfLabels = my numberOfColumns;
		labels = my columnLabels;
	}
	if (use_regexp)
	{
		wchar_t *compileMsg;
		compiled_regexp = CompileRE (search, &compileMsg, 0);
		if (compiled_regexp == NULL) return Melder_error1 (compileMsg);
	}
	for (i = 1; i <= numberOfLabels; i++)
	{
		if (labels[i] == NULL) continue;
		if (use_regexp)
		{
			if (ExecRE (compiled_regexp, NULL, labels[i], NULL, 0,
				'\0', '\0', NULL, NULL, NULL)) nmatches++;
		}
		else if (wcsequ (labels[i], search)) nmatches++;
	}
	if (use_regexp) free (compiled_regexp);
	return nmatches;
}

TableOfReal TableOfReal_sortRowsByIndex (I, long *index, int reverse)
{
	iam (TableOfReal);
	TableOfReal thee = NULL;
	double min, max;
	long i, j;

	if (my rowLabels == NULL) return NULL;

	NUMlvector_extrema (index, 1, my numberOfRows, &min, &max);
	if (min < 1 || max > my numberOfRows) return Melder_errorp
		("TableOfReal_sortRowsByIndex: one or more indices out of range [1, %d].",
		my numberOfRows);

	thee = TableOfReal_create (my numberOfRows, my numberOfColumns);
	if (thee == NULL) return NULL;

	for (i = 1; i <= my numberOfRows; i++)
	{
		long    myindex = reverse ? i : index[i];
		long   thyindex = reverse ? index[i] : i;
		wchar_t   *mylabel = my rowLabels[myindex];
		double  *mydata = my data[myindex];
		double *thydata = thy data[thyindex];

		/*
			Copy the row label
		*/

		if (mylabel != NULL)
		{
			thy rowLabels[i] = Melder_wcsdup_e (mylabel);
			if (thy rowLabels[i] == NULL) goto end;
		}

		/*
			Copy the row values
		*/

		for (j = 1; j <= my numberOfColumns; j++)
		{
			thydata[j] = mydata[j];
		}
	}

	/*
		Copy column labels.
	*/

	(void) NUMstrings_copyElements (my columnLabels, thy columnLabels,
		1, my numberOfColumns);
end:

	if (Melder_hasError()) forget (thee);
	return thee;
}

long *TableOfReal_getSortedIndexFromRowLabels (I)
{
	iam (TableOfReal);
	long *index = NUMlvector (1, my numberOfRows);

	if (index == NULL) return NULL;
	NUMindexx_s (my rowLabels, my numberOfRows, index);
	return index;
}

TableOfReal TableOfReal_sortOnlyByRowLabels (I)
{
	iam (TableOfReal);
	TableOfReal thee = NULL;
	Permutation index = NULL;

	index = TableOfReal_to_Permutation_sortRowLabels (me);
	if (index == NULL) return NULL;

	thee = TableOfReal_and_Permutation_permuteRows (me, index);

	forget (index);
	return thee;
}

static void NUMmedianizeColumns (double **a, long rb, long re, long cb, long ce)
{
	long i, j, k, n = re - rb + 1;
	double *tmp, median;

	if (n < 2) return;
	tmp = NUMdvector (1, n);
	if (tmp == NULL) return;
	for (j = cb; j <= ce; j++)
	{
		k = 1;
		for (i = rb; i <= re; i++, k++) tmp[k]= a[i][j];
		NUMsort_d (n, tmp);
		median = NUMquantile (n, tmp, 0.5);
		for (i = rb; i <= re; i++) a[i][j] = median;
	}
	NUMdvector_free (tmp, 1);
}

static void NUMstatsColumns (double **a, long rb, long re, long cb, long ce, int stats)
{
	if (stats == 0)
	{
		NUMaverageColumns (a, rb, re, cb, ce);
	}
	else
	{
		NUMmedianizeColumns (a, rb, re, cb, ce);
	}
}

TableOfReal TableOfReal_meansByRowLabels (I, int expand, int stats)
{
	iam (TableOfReal);
	TableOfReal thee = NULL, sorted = NULL;
	wchar_t *label, **tmp;
	long *index = NULL, indexi = 1, indexr = 0, i;

	index = TableOfReal_getSortedIndexFromRowLabels (me);
	if (index == NULL) return NULL;

	sorted = TableOfReal_sortRowsByIndex (me, index, 0);
	if (sorted == NULL) goto end;

	label = sorted -> rowLabels[1];
	for (i = 2; i <= my numberOfRows; i++)
	{
		wchar_t *li = sorted -> rowLabels[i];
		if (li != NULL && li != label && (label == NULL || wcscmp (li, label)))
		{
			NUMstatsColumns (sorted -> data, indexi, i - 1, 1, my numberOfColumns, stats);

			if (expand == 0)
			{
				indexr++;
				if (!TableOfReal_copyOneRowWithLabel (sorted, sorted, indexi, indexr)) goto end;
			}
			label = li; indexi = i;
		}
	}

	NUMstatsColumns (sorted -> data, indexi, my numberOfRows, 1, my numberOfColumns, stats);

	if (expand != 0)
	{
		/*
			Now inverse the table.
		*/
		tmp = sorted -> rowLabels; sorted -> rowLabels = my rowLabels;
		thee = TableOfReal_sortRowsByIndex (sorted, index, 1);
		sorted -> rowLabels = tmp;
	}
	else
	{
		indexr++;
		if (! TableOfReal_copyOneRowWithLabel (sorted, sorted, indexi, indexr)) goto end;
		thee = TableOfReal_create (indexr, my numberOfColumns);
		if (thee == NULL) goto end;
		for (i = 1; i <= indexr; i++)
		{
			if (!TableOfReal_copyOneRowWithLabel (sorted, thee, i, i)) goto end;
		}
		if (! NUMstrings_copyElements (sorted -> columnLabels, thy columnLabels, 1, my numberOfColumns)) goto end;
	}

end:
	forget (sorted);
	NUMlvector_free (index, 1);
	if (Melder_hasError()) forget (thee);
	return thee;
}

TableOfReal TableOfReal_rankColumns (I)
{
	iam (TableOfReal);
	TableOfReal thee = Data_copy (me);

	if (thee == NULL) return NULL;
	if (! NUMrankColumns (thy data, 1, thy numberOfRows,
		1, thy numberOfColumns)) forget (thee);
	return thee;
}

int TableOfReal_setSequentialColumnLabels (I, long from, long to,
	wchar_t *precursor, long number, long increment)
{
	iam (TableOfReal);

	if (from == 0) from = 1;
	if (to == 0) to = my numberOfColumns;
	if (from < 1 || from > my numberOfColumns || to < from ||
		to > my numberOfColumns) return Melder_error2
			(L"TableOfReal_setSequentialColumnLabels: wrong column indices for ", Thing_messageName(me));
	return NUMstrings_setSequentialNumbering (my columnLabels, from, to,
		precursor, number, increment);
}

int TableOfReal_setSequentialRowLabels (I, long from, long to,
	wchar_t *precursor, long number, long increment)
{
	iam (TableOfReal);

	if (from == 0) from = 1;
	if (to == 0) to = my numberOfRows;
	if (from < 1 || from > my numberOfRows || to < from ||
		to > my numberOfRows) return Melder_error2
			(L"TableOfReal_setSequentialRowLabels: wrong row indices for ", Thing_messageName(me));
	return NUMstrings_setSequentialNumbering (my rowLabels, from, to,
		precursor, number, increment);
}

/* For the inheritors */
TableOfReal TableOfReal_to_TableOfReal (I)
{
	iam (TableOfReal);
	TableOfReal thee = TableOfReal_create (my numberOfRows, my numberOfColumns);
	if (thee == NULL) return NULL;

	NUMdmatrix_copyElements (my data, thy data, 1, my numberOfRows,
		1, my numberOfColumns);
	if (! TableOfReal_copyLabels (me, thee, 1, 1)) forget (thee);
	return thee;
}

TableOfReal TableOfReal_choleskyDecomposition (I, int upper, int inverse)
{
	iam (TableOfReal);
	wchar_t *proc = L"TableOfReal_choleskyDecomposition"; char uplo = 'U', diag = 'N';
	long i, j, n = my numberOfColumns, lda = my numberOfRows, info;
	TableOfReal thee;

	if (n != lda) return Melder_errorp4 (proc, L": The matrix part of ", Thing_messageName(me), L" must be a square symmetric matrix.");
	if ((thee = Data_copy (me)) == NULL) return NULL;

	if (upper)
	{
		uplo = 'L'; /* Fortran storage */
		for (i = 2; i <= n; i++) for (j = 1; j < i; j++) thy data[i][j] = 0;
	}
	else
	{
		for (i = 1; i < n; i++) for (j = i+1; j <= n; j++) thy data[i][j] = 0;
	}
	(void) NUMlapack_dpotf2 (&uplo, &n, &thy data[1][1], &lda, &info);
	if (info != 0) goto end;

	if (inverse)
	{
		(void) NUMlapack_dtrtri (&uplo, &diag, &n, &thy data[1][1], &lda, &info);
	}

end:
	if (Melder_hasError()) forget (thee);
	return thee;
}

TableOfReal TableOfReal_appendColumns (I, thou)
{
	iam (TableOfReal); thouart (TableOfReal);
	TableOfReal him;
	long i, ncols = my numberOfColumns + thy numberOfColumns;
	long labeldiffs = 0;

	if (my numberOfRows != thy numberOfRows) return Melder_errorp4 (Thing_messageName(me), L" and ",
		Thing_messageName(thee), L" must have an equal number of rows.");
	/* Stricter label checking???
		append only if
		  (my rowLabels[i] == thy rowlabels[i], i=1..my numberOfRows) or
		  (my rowLabels[i] == 'empty', i=1..my numberOfRows)  or
		  (thy rowLabels[i] == 'empty', i=1..my numberOfRows);
		'empty':  NULL or \w*
	*/
	him = TableOfReal_create (my numberOfRows, ncols);
	if (him == NULL) return NULL;
	if (! NUMstrings_copyElements (my rowLabels, his rowLabels, 1, my numberOfRows) ||
		! NUMstrings_copyElements (my columnLabels, his columnLabels,  1, my numberOfColumns) ||
		! NUMstrings_copyElements (thy columnLabels, &his columnLabels[my numberOfColumns],
			1, thy numberOfColumns)) goto end;
	for (i = 1; i <= my numberOfRows; i++)
	{
		if (Melder_wcscmp (my rowLabels[i], thy rowLabels[i])) labeldiffs++;
		NUMdvector_copyElements (my data[i], his data[i], 1, my numberOfColumns);
		NUMdvector_copyElements (thy data[i], &his data[i][my numberOfColumns], 1, thy numberOfColumns);
	}
end:
	if (Melder_hasError())
	{
		forget (him);
	}
	else if (labeldiffs > 0)
	{
		Melder_warning2 (Melder_integer (labeldiffs), L" row labels differed.");
	}
	return him;
}

Any TableOfReal_appendColumnsMany (Collection me)
{
	TableOfReal him = NULL, thee;
	long itab, irow, icol, nrow, ncol;

	if (my size == 0) return Melder_errorp1 (L"No tables selected.");
	thee = my item [1];
	nrow = thy numberOfRows;
	ncol = thy numberOfColumns;
	for (itab = 2; itab <= my size; itab++)
	{
		thee = my item [itab];
		ncol += thy numberOfColumns;
		if (thy numberOfRows != nrow)
		{
			Melder_error3 (L"Numbers of rows in ", Thing_messageName(thee), L" does not match the others.");
			goto end;
		}
	}
	if ((him = Thing_new (TableOfReal)) == NULL ||
		! TableOfReal_init (him, nrow, ncol)) goto end;
	/* Unsafe: new attributes not initialized. */
	for (irow = 1; irow <= nrow; irow++)
	{
		TableOfReal_setRowLabel (him, irow, thy rowLabels [irow]);
		if (Melder_hasError ()) goto end;
	}
	ncol = 0;
	for (itab = 1; itab <= my size; itab++)
	{
		thee = my item [itab];
		for (icol = 1; icol <= thy numberOfColumns; icol++)
		{
			ncol++;
			TableOfReal_setColumnLabel (him, ncol, thy columnLabels [icol]);
			if (Melder_hasError ()) goto end;
			for (irow = 1; irow <= nrow; irow++)
			{
				his data[irow][ncol] = thy data[irow][icol];
			}
		}
	}
	Melder_assert (ncol == his numberOfColumns);

end:

	if (Melder_hasError ()) forget (him);
	return him;
}

double TableOfReal_normalityTest_BHEP (I, double *h, double *tnb, double *lnmu, double *lnvar)
{
	iam (TableOfReal);
	long n = my numberOfRows, p = my numberOfColumns;
	double beta = *h > 0 ? NUMsqrt1_2 / *h :
		NUMsqrt1_2 * pow ((1.0 + 2 * p ) / 4, 1.0 / (p + 4 )) * pow (n, 1.0 / (p + 4));
	double p2 = p / 2.0;
	double beta2 = beta * beta, beta4 = beta2 * beta2, beta8 = beta4 * beta4;
	double gamma = 1 + 2 * beta2, gamma2 = gamma * gamma, gamma4 = gamma2 * gamma2;
	double delta = 1.0 + beta2 * (4 + 3 * beta2), delta2 = delta * delta;
	double mu, mu2, var, prob = NUMundefined;

	if (*h <= 0) *h = NUMsqrt1_2 / beta;

	*tnb = *lnmu = *lnvar = NUMundefined;

	if (n < 2 || p < 1) return prob;

	Covariance thee = TableOfReal_to_Covariance (me);
	if (thee == NULL) goto end;
	if (! SSCP_expandLowerCholesky (thee))
	{
		*tnb = 4 * n;
	}
	else
	{
		double djk, djj, sumjk = 0, sumj = 0;
		double b1 = beta2 / 2, b2 = b1 / (1.0 + beta2);
		/* Heinze & Wagner (1997), page 3
			We use d[j][k] = ||Y[j]-Y[k]||^2 = (Y[j]-Y[k])'S^(-1)(Y[j]-Y[k])
			So d[j][k]= d[k][j] and d[j][j] = 0
		*/
		for (long j = 1; j <= n; j++)
		{
			for (long k = 1; k < j; k++)
			{
				djk = NUMmahalanobisDistance_chi (thy lowerCholesky, my data[j], my data[k], p, p);
				sumjk += 2 * exp (-b1 * djk); // factor 2 because d[j][k] == d[k][j]
			}
			sumjk += 1; // for k == j
			djj = NUMmahalanobisDistance_chi (thy lowerCholesky, my data[j], thy centroid, p, p);
			sumj += exp (-b2 * djj);
		}
		*tnb = (1.0 / n) * sumjk - 2.0 * pow (1.0 + beta2, - p2) * sumj + n * pow (gamma, - p2); // n *
	}
	mu = 1.0 - pow (gamma, -p2) * (1.0 + p * beta2 / gamma + p * (p + 2) * beta4 / (2 * gamma2));
	var = 2.0 * pow (1 + 4 * beta2, -p2)
		+ 2.0 * pow (gamma,  -p) * (1.0 + 2 * p * beta4 / gamma2  + 3 * p * (p + 2) * beta8 / (4 * gamma4))
		- 4.0 * pow (delta, -p2) * (1.0 + 3 * p * beta4 / (2 * delta) + p * (p + 2) * beta8 / (2 * delta2));
	mu2 = mu * mu;
	*lnmu = 0.5 * log (mu2 * mu2 / (mu2 + var)); //log (sqrt (mu2 * mu2 /(mu2 + var)));
	*lnvar = sqrt (log ((mu2 + var) / mu2));
	prob = NUMlogNormalQ (*tnb, *lnmu, *lnvar);
end:
	forget (thee);
	return prob;
}

TableOfReal TableOfReal_and_TableOfReal_crossCorrelations (I, thou, int by_columns, int center, int normalize)
{
	iam (TableOfReal); thouart (TableOfReal);
	return by_columns ? TableOfReal_and_TableOfReal_columnCorrelations (me, thee, center, normalize) :
		TableOfReal_and_TableOfReal_rowCorrelations (me, thee, center, normalize);
}

TableOfReal TableOfReal_and_TableOfReal_rowCorrelations (I, thou, int center, int normalize)
{
	iam (TableOfReal); thouart (TableOfReal);
	TableOfReal him = NULL;
	double **my_data = NULL, **thy_data = NULL;
	if (my numberOfColumns != thy numberOfColumns) return Melder_errorp1 (L"Both tables must have the same number of columns.");

//start:

	him = TableOfReal_create (my numberOfRows, thy numberOfRows); cherror
	my_data = NUMdmatrix_copy (my data, 1, my numberOfRows, 1, my numberOfColumns); cherror
	thy_data = NUMdmatrix_copy (thy data, 1, thy numberOfRows, 1, thy numberOfColumns); cherror
	if (center)
	{
		NUMcentreRows (my_data, 1, my numberOfRows, 1, my numberOfColumns);
		NUMcentreRows (thy_data, 1, thy numberOfRows, 1, thy numberOfColumns);
	}
	if (normalize)
	{
		NUMnormalizeRows (my_data, my numberOfRows, my numberOfColumns, 1);
		NUMnormalizeRows (thy_data, thy numberOfRows, thy numberOfColumns, 1);
	}
	if (! NUMstrings_copyElements (my rowLabels, his rowLabels, 1, his numberOfRows) ||
		! NUMstrings_copyElements (thy rowLabels, his columnLabels, 1, his numberOfColumns)) goto end;
	for (long i = 1; i <= my numberOfRows; i++)
	{
		for (long k = 1; k <= thy numberOfRows; k++)
		{
			double ctmp = 0;
			for (long j = 1; j <= my numberOfColumns; j++)
			{ ctmp += my_data[i][j] * thy_data[k][j]; }
			his data[i][k] = ctmp;
		}
	}
end:
	NUMdmatrix_free (my_data, 1, 1); NUMdmatrix_free (thy_data, 1, 1);
	if (Melder_hasError ()) forget (him);
	return him;
}

TableOfReal TableOfReal_and_TableOfReal_columnCorrelations (I, thou, int center, int normalize)
{
	iam (TableOfReal); thouart (TableOfReal);
	TableOfReal him = NULL;
	double **my_data = NULL, **thy_data = NULL;
	if (my numberOfRows != thy numberOfRows) return Melder_errorp1 (L"Both tables must have the same number of rows.");

//start:

	him = TableOfReal_create (my numberOfColumns, thy numberOfColumns); cherror
	my_data = NUMdmatrix_copy (my data, 1, my numberOfRows, 1, my numberOfColumns); cherror
	thy_data = NUMdmatrix_copy (thy data, 1, thy numberOfRows, 1, thy numberOfColumns); cherror
	if (center)
	{
		NUMcentreColumns (my_data, 1, my numberOfRows, 1, my numberOfColumns, NULL);
		NUMcentreColumns (thy_data, 1, thy numberOfRows, 1, thy numberOfColumns, NULL);
	}
	if (normalize)
	{
		NUMnormalizeColumns (my_data, my numberOfRows, my numberOfColumns, 1);
		NUMnormalizeColumns (thy_data, thy numberOfRows, thy numberOfColumns, 1);
	}
	if (! NUMstrings_copyElements (my columnLabels, his rowLabels, 1, his numberOfRows) ||
		! NUMstrings_copyElements (thy columnLabels, his columnLabels, 1, his numberOfColumns)) goto end;

	for (long j = 1; j <= my numberOfColumns; j++)
	{
		for (long k = 1; k <= thy numberOfColumns; k++)
		{
			double ctmp = 0;
			for (long i = 1; i <= my numberOfRows; i++)
			{ ctmp += my_data[i][j] * thy_data[i][k]; }
			his data[j][k] = ctmp;
		}
	}
end:
	NUMdmatrix_free (my_data, 1, 1); NUMdmatrix_free (thy_data, 1, 1);
	if (Melder_hasError ()) forget (him);
	return him;
}

#undef MAX
#undef MIN

/* End of file TableOfReal_extensions.c */
