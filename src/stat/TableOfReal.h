#ifndef _TableOfReal_h_
#define _TableOfReal_h_
/* TableOfReal.h
 *
 * Copyright (C) 1992-2011 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * pb 2011/03/03
 */

/* TableOfReal inherits from Data */
#include "sys/Collection.h"
#include "sys/Strings.h"
#include "Table.h"

#ifdef __cplusplus
	extern "C" {
#endif

/* For the inheritors. */
#define TableOfReal_members Data_members \
	long numberOfRows, numberOfColumns; \
	wchar_t **rowLabels, **columnLabels; \
	double **data;
#define TableOfReal_methods Data_methods
class_create (TableOfReal, Data);

int TableOfReal_init (I, long numberOfRows, long numberOfColumns);
TableOfReal TableOfReal_create (long numberOfRows, long numberOfColumns);
void TableOfReal_removeRow (I, long irow);
void TableOfReal_removeColumn (I, long icol);
void TableOfReal_insertRow (I, long irow);
void TableOfReal_insertColumn (I, long icol);
void TableOfReal_setRowLabel (I, long irow, const wchar_t *label);
void TableOfReal_setColumnLabel (I, long icol, const wchar_t *label);
void TableOfReal_copyRowLabels (TableOfReal me, TableOfReal thee);
void TableOfReal_copyColumnLabels (TableOfReal me, TableOfReal thee);
void TableOfReal_copyRow (TableOfReal me, long myRow, TableOfReal thee, long thyRow);
void TableOfReal_copyColumn (TableOfReal me, long myCol, TableOfReal thee, long thyCol);
long TableOfReal_rowLabelToIndex (I, const wchar_t *label);
long TableOfReal_columnLabelToIndex (I, const wchar_t *label);
double TableOfReal_getColumnMean (I, long icol);
double TableOfReal_getColumnStdev (I, long icol);

TableOfReal Table_to_TableOfReal (Table me, long labelColumn);
Table TableOfReal_to_Table (TableOfReal me, const wchar_t *labelOfFirstColumn);

Any TablesOfReal_append (I, thou);
Any TablesOfReal_appendMany (Collection me);
void TableOfReal_sortByLabel (I, long column1, long column2);
void TableOfReal_sortByColumn (I, long column1, long column2);

int TableOfReal_writeToHeaderlessSpreadsheetFile (TableOfReal me, MelderFile file);
TableOfReal TableOfReal_readFromHeaderlessSpreadsheetFile (MelderFile file);

TableOfReal TableOfReal_extractRowRanges (I, const wchar_t *ranges);
TableOfReal TableOfReal_extractColumnRanges (I, const wchar_t *ranges);

TableOfReal TableOfReal_extractRowsWhereColumn (I, long icol, int which_Melder_NUMBER, double criterion);
TableOfReal TableOfReal_extractColumnsWhereRow (I, long icol, int which_Melder_NUMBER, double criterion);

TableOfReal TableOfReal_extractRowsWhereLabel (I, int which_Melder_STRING, const wchar_t *criterion);
TableOfReal TableOfReal_extractColumnsWhereLabel (I, int which_Melder_STRING, const wchar_t *criterion);

Strings TableOfReal_extractRowLabelsAsStrings (I);
Strings TableOfReal_extractColumnLabelsAsStrings (I);

#ifdef __cplusplus
	}
#endif

/* End of file TableOfReal.h */
#endif
