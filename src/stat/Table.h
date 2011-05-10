#ifndef _Table_h_
#define _Table_h_
/* Table.h
 *
 * Copyright (C) 2002-2011 Paul Boersma
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

#include "sys/Collection.h"
#include "ui/Interpreter.h"

#ifdef __cplusplus
	extern "C" {
#endif

/* For the inheritors. */
#include "Table_def.h"
#define TableRow_methods Data_methods
oo_CLASS_CREATE (TableRow, Data);
#define Table_members Data_members \
	long numberOfColumns; \
	TableColumnHeader *columnHeaders; \
	Ordered rows;
#define Table_methods Data_methods
oo_CLASS_CREATE (Table, Data);

int Table_initWithColumnNames (I, long numberOfRows, const wchar_t *columnNames);
Table Table_createWithColumnNames (long numberOfRows, const wchar_t *columnNames);
int Table_initWithoutColumnNames (I, long numberOfRows, long numberOfColumns);
Table Table_createWithoutColumnNames (long numberOfRows, long numberOfColumns);
#define Table_create Table_createWithoutColumnNames

Table Tables_append (Collection me);
int Table_appendRow (Table me);
int Table_appendColumn (Table me, const wchar_t *label);
void Table_appendSumColumn (Table me, long column1, long column2, const wchar_t *label);
void Table_appendDifferenceColumn (Table me, long column1, long column2, const wchar_t *label);
void Table_appendProductColumn (Table me, long column1, long column2, const wchar_t *label);
void Table_appendQuotientColumn (Table me, long column1, long column2, const wchar_t *label);
void Table_removeRow (Table me, long row);
void Table_removeColumn (Table me, long column);
void Table_insertRow (Table me, long row);
void Table_insertColumn (Table me, long column, const wchar_t *label);
void Table_setColumnLabel (Table me, long column, const wchar_t *label);
long Table_findColumnIndexFromColumnLabel (Table me, const wchar_t *label);
long Table_getColumnIndexFromColumnLabel (Table me, const wchar_t *columnLabel);
long * Table_getColumnIndicesFromColumnLabelString (Table me, const wchar_t *string, long *numberOfTokens);
long Table_searchColumn (Table me, long column, const wchar_t *value);

/*
 * Procedure for reading strings or numbers from table cells:
 * use the following two calls exclusively.
 */
const wchar_t * Table_getStringValue_Assert (Table me, long row, long column);
double Table_getNumericValue_Assert (Table me, long row, long column);

/*
 * Procedure for writing strings or numbers into table cells:
 * use the following two calls exclusively.
 */
int Table_setStringValue (Table me, long row, long column, const wchar_t *value);
int Table_setNumericValue (Table me, long row, long column, double value);

/* For optimizations only (e.g. conversion to Matrix or TableOfReal). */
void Table_numericize_Assert (Table me, long columnNumber);
void Table_numericize_checkDefined (Table me, long columnNumber);

double Table_getQuantile (Table me, long column, double quantile);
double Table_getMean (Table me, long column);
double Table_getMaximum (Table me, long icol);
double Table_getMinimum (Table me, long icol);
double Table_getGroupMean (Table me, long column, long groupColumn, const wchar_t *group);
double Table_getStdev (Table me, long column);
double Table_getCorrelation_pearsonR (Table me, long column1, long column2, double significanceLevel,
	double *out_significance, double *out_lowerLimit, double *out_upperLimit);
double Table_getCorrelation_kendallTau (Table me, long column1, long column2, double significanceLevel,
	double *out_significance, double *out_lowerLimit, double *out_upperLimit);
double Table_getMean_studentT (Table me, long column, double significanceLevel,
	double *out_tFromZero, double *out_numberOfDegreesOfFreedom, double *out_significanceFromZero, double *out_lowerLimit, double *out_upperLimit);
double Table_getDifference_studentT (Table me, long column1, long column2, double significanceLevel,
	double *out_t, double *out_numberOfDegreesOfFreedom, double *out_significance, double *out_lowerLimit, double *out_upperLimit);
double Table_getGroupMean_studentT (Table me, long column, long groupColumn, const wchar_t *group1, double significanceLevel,
	double *out_tFromZero, double *out_numberOfDegreesOfFreedom, double *out_significanceFromZero, double *out_lowerLimit, double *out_upperLimit);
double Table_getGroupDifference_studentT (Table me, long column, long groupColumn, const wchar_t *group1, const wchar_t *group2, double significanceLevel,
	double *out_tFromZero, double *out_numberOfDegreesOfFreedom, double *out_significanceFromZero, double *out_lowerLimit, double *out_upperLimit);
double Table_getGroupDifference_wilcoxonRankSum (Table me, long column, long groupColumn, const wchar_t *group1, const wchar_t *group2,
	double *out_rankSum, double *out_significanceFromZero);
double Table_getVarianceRatio (Table me, long column1, long column2, double significanceLevel,
	double *out_significance, double *out_lowerLimit, double *out_upperLimit);
bool Table_getExtrema (Table me, long icol, double *minimum, double *maximum);

int Table_formula (Table me, long column, const wchar_t *formula, Interpreter *interpreter);
int Table_formula_columnRange (Table me, long column1, long column2, const wchar_t *expression, Interpreter *interpreter);

void Table_sortRows_Assert (Table me, long *columns, long numberOfColumns);
void Table_sortRows_string (Table me, const wchar_t *columns_string);
void Table_randomizeRows (Table me);

void Table_list (Table me, bool includeRowNumbers);
int Table_writeToTableFile (Table me, MelderFile file);
Table Table_readFromTableFile (MelderFile file);
Table Table_readFromCharacterSeparatedTextFile (MelderFile file, wchar_t separator);

Table Table_extractRowsWhereColumn_number (Table me, long column, int which_Melder_NUMBER, double criterion);
Table Table_extractRowsWhereColumn_string (Table me, long column, int which_Melder_STRING, const wchar_t *criterion);
Table Table_collapseRows (Table me, const wchar_t *factors_string, const wchar_t *columnsToSum_string,
	const wchar_t *columnsToAverage_string, const wchar_t *columnsToMedianize_string,
	const wchar_t *columnsToAverageLogarithmically_string, const wchar_t *columnsToMedianizeLogarithmically_string);
Table Table_rowsToColumns (Table me, const wchar_t *factors_string, long columnToTranspose, const wchar_t *columnsToExpand_string);

#ifdef __cplusplus
	}
void Table_checkSpecifiedRowNumberWithinRange (Table me, long rowNumber);
void Table_checkSpecifiedColumnNumberWithinRange (Table me, long columnNumber);
#endif

/* End of file Table.h */
#endif
