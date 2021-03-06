/* TableOfReal.cpp
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
 * pb 2002/07/16 GPL
 * pb 2003/09/15 TableOfReal_readFromHeaderlessSpreadsheetFile: guard against cells with strings
 * dw 2004/02/27 drawAsSquares: fill colour same as outline colour
 * pb 2004/05/09 more Extract Part and Extract commands
 * pb 2004/10/01 Melder_double instead of %.17g
 * pb 2005/03/04 Melder_NUMBER and Melder_STRING as enums
 * pb 2005/06/16 Melder_NUMBER and Melder_STRING as ints
 * pb 2005/07/19 TableOfReal_readFromHeaderlessSpreadsheetFile: allow 30k row and column labels
 * pb 2005/09/18 SILIPA versus XIPA widths
 * pb 2006/04/17 getRowStr, getColStr
 * pb 2006/12/10 MelderInfo
 * pb 2007/03/17 moved Table stuff here
 * pb 2007/06/21 tex
 * pb 2007/10/01 can write as encoding
 * pb 2008/04/30 new Formula API
 * pb 2009/01/18 Interpreter argument to formula
 * pb 2011/03/20 C++
 */

#include <ctype.h>
#include "TableOfReal.h"
#include "num/NUM2.h"
#include "fon/Matrix.h"

#include "sys/oo/oo_DESTROY.h"
#include "TableOfReal_def.h"
#include "sys/oo/oo_COPY.h"
#include "TableOfReal_def.h"
#include "sys/oo/oo_EQUAL.h"
#include "TableOfReal_def.h"
#include "sys/oo/oo_CAN_WRITE_AS_ENCODING.h"
#include "TableOfReal_def.h"
#include "sys/oo/oo_WRITE_BINARY.h"
#include "TableOfReal_def.h"
#include "sys/oo/oo_READ_BINARY.h"
#include "TableOfReal_def.h"
#include "sys/oo/oo_DESCRIPTION.h"
#include "TableOfReal_def.h"

static void fprintquotedstring (MelderFile file, const wchar_t *s) {
	MelderFile_writeCharacter (file, '\"');
	if (s) { wchar_t c; while ((c = *s ++) != '\0') { MelderFile_writeCharacter (file, c); if (c == '\"') MelderFile_writeCharacter (file, c); } }
	MelderFile_writeCharacter (file, '\"');
}

static int writeText (I, MelderFile file) {
	iam (TableOfReal);
	texputi4 (file, my numberOfColumns, L"numberOfColumns", 0,0,0,0,0);
	MelderFile_write1 (file, L"\ncolumnLabels []: ");
	if (my numberOfColumns < 1) MelderFile_write1 (file, L"(empty)");
	MelderFile_write1 (file, L"\n");
	for (long i = 1; i <= my numberOfColumns; i ++) {
		fprintquotedstring (file, my columnLabels [i]);
		MelderFile_writeCharacter (file, '\t');
	}
	texputi4 (file, my numberOfRows, L"numberOfRows", 0,0,0,0,0);
	for (long i = 1; i <= my numberOfRows; i ++) {
		MelderFile_write3 (file, L"\nrow [", Melder_integer (i), L"]: ");
		fprintquotedstring (file, my rowLabels [i]);
		for (long j = 1; j <= my numberOfColumns; j ++) {
			double x = my data [i] [j];
			MelderFile_write2 (file, L"\t", Melder_double (x));
		}
	}
	return 1;
}

static int readText (I, MelderReadText text) {
	iam (TableOfReal);
	my numberOfColumns = texgeti4 (text);
	if (my numberOfColumns >= 1) {
		if (! (my columnLabels = (wchar **) NUMvector (sizeof (wchar_t *), 1, my numberOfColumns))) return 0;
		for (long i = 1; i <= my numberOfColumns; i ++)
			if (! (my columnLabels [i] = texgetw2 (text))) return 0;
	}
	my numberOfRows = texgeti4 (text);
	if (my numberOfRows >= 1) {
		if (! (my rowLabels = (wchar **) NUMvector (sizeof (wchar_t *), 1, my numberOfRows))) return 0;
	}
	if (my numberOfRows >= 1 && my numberOfColumns >= 1) {
		if (! (my data = NUMdmatrix (1, my numberOfRows, 1, my numberOfColumns))) return 0;
		for (long i = 1; i <= my numberOfRows; i ++) {
			if (! (my rowLabels [i] = texgetw2 (text))) return 0;
			for (long j = 1; j <= my numberOfColumns; j ++)
				my data [i] [j] = texgetr8 (text);
		}
	}
	return 1;
}

static void info (I) {
	iam (TableOfReal);
	classData -> info (me);
	MelderInfo_writeLine2 (L"Number of rows: ", Melder_integer (my numberOfRows));
	MelderInfo_writeLine2 (L"Number of columns: ", Melder_integer (my numberOfColumns));
}

static double getNrow (I) { iam (TableOfReal); return my numberOfRows; }
static double getNcol (I) { iam (TableOfReal); return my numberOfColumns; }
static const wchar * getRowStr (I, long irow) {
	iam (TableOfReal);
	if (irow < 1 || irow > my numberOfRows) return NULL;
	return my rowLabels [irow] ? my rowLabels [irow] : L"";
}
static const wchar * getColStr (I, long icol) {
	iam (TableOfReal);
	if (icol < 1 || icol > my numberOfColumns) return NULL;
	return my columnLabels [icol] ? my columnLabels [icol] : L"";
}
static double getMatrix (I, long irow, long icol) {
	iam (TableOfReal);
	if (irow < 1 || irow > my numberOfRows) return NUMundefined;
	if (icol < 1 || icol > my numberOfColumns) return NUMundefined;
	return my data [irow] [icol];
}
static double getRowIndex (I, const wchar_t *rowLabel) {
	iam (TableOfReal);
	return TableOfReal_rowLabelToIndex (me, rowLabel);
}
static double getColumnIndex (I, const wchar_t *columnLabel) {
	iam (TableOfReal);
	return TableOfReal_columnLabelToIndex (me, columnLabel);
}

class_methods (TableOfReal, Data) {
	class_method_local (TableOfReal, destroy)
	class_method_local (TableOfReal, description)
	class_method_local (TableOfReal, copy)
	class_method_local (TableOfReal, equal)
	class_method_local (TableOfReal, canWriteAsEncoding)
	class_method (writeText)
	class_method (readText)
	class_method_local (TableOfReal, writeBinary)
	class_method_local (TableOfReal, readBinary)
	class_method (info)
	class_method (getNrow)
	class_method (getNcol)
	class_method (getRowStr)
	class_method (getColStr)
	class_method (getMatrix)
	class_method (getRowIndex)
	class_method (getColumnIndex)
	class_methods_end
}

int TableOfReal_init (I, long numberOfRows, long numberOfColumns) {
	try {
		iam (TableOfReal);
		if (numberOfRows < 1 || numberOfColumns < 1)
			Melder_throw (L"Cannot create cell-less table.");
		my numberOfRows = numberOfRows;
		my numberOfColumns = numberOfColumns;
		my rowLabels = NUMwvector (1, numberOfRows); therror
		my columnLabels = NUMwvector (1, numberOfColumns); therror
		my data = NUMdmatrix (1, my numberOfRows, 1, my numberOfColumns); therror
		return 1;
	} catch (...) {
		rethrowzero;
	}
}

TableOfReal TableOfReal_create (long numberOfRows, long numberOfColumns) {
	try {
		autoTableOfReal me = Thing_new (TableOfReal);
		TableOfReal_init (me.peek(), numberOfRows, numberOfColumns);
		return me.transfer();
	} catch (...) {
		rethrowmzero ("TableOfReal not created.");
	}
}

/***** QUERY *****/

long TableOfReal_rowLabelToIndex (I, const wchar_t *label) {
	iam (TableOfReal);
	for (long irow = 1; irow <= my numberOfRows; irow ++)
		if (my rowLabels [irow] && wcsequ (my rowLabels [irow], label))
			return irow;
	return 0;
}

long TableOfReal_columnLabelToIndex (I, const wchar_t *label) {
	iam (TableOfReal);
	for (long icol = 1; icol <= my numberOfColumns; icol ++)
		if (my columnLabels [icol] && wcsequ (my columnLabels [icol], label))
			return icol;
	return 0;
}

double TableOfReal_getColumnMean (I, long columnNumber) {
	iam (TableOfReal);
	double sum = 0.0;
	if (columnNumber < 1 || columnNumber > my numberOfColumns) return NUMundefined;
	if (my numberOfRows < 1) return NUMundefined;
	for (long irow = 1; irow <= my numberOfRows; irow ++)
		sum += my data [irow] [columnNumber];
	return sum / my numberOfRows;
}

double TableOfReal_getColumnStdev (I, long columnNumber) {
	iam (TableOfReal);
	double mean = TableOfReal_getColumnMean (me, columnNumber), sum = 0.0, d;
	if (columnNumber < 1 || columnNumber > my numberOfColumns) return NUMundefined;
	if (my numberOfRows < 2) return NUMundefined;
	for (long irow = 1; irow <= my numberOfRows; irow ++)
		sum += ( d = my data [irow] [columnNumber] - mean, d * d );
	return sqrt (sum / (my numberOfRows - 1));
}

/***** MODIFY *****/

void TableOfReal_removeRow (I, long rowNumber) {
	iam (TableOfReal);
	try {
		if (my numberOfRows == 1)
			Melder_throw (Thing_messageName (me), " has only one row, and a TableOfReal without rows cannot exist.");
		if (rowNumber < 1 || rowNumber > my numberOfRows)
			Melder_throw ("No row ", rowNumber, ".");
		autoNUMmatrix <double> data (1, my numberOfRows - 1, 1, my numberOfColumns);
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			for (long irow = 1; irow < rowNumber; irow ++)
				data [irow] [icol] = my data [irow] [icol];
			for (long irow = rowNumber; irow < my numberOfRows; irow ++)
				data [irow] [icol] = my data [irow + 1] [icol];
		}
		/*
		 * Change without error.
		 */
		Melder_free (my rowLabels [rowNumber]);
		for (long irow = rowNumber; irow < my numberOfRows; irow ++)
			my rowLabels [irow] = my rowLabels [irow + 1];
		NUMmatrix_free <double> (my data, 1, 1);
		my data = data.transfer();
		my numberOfRows --;
	} catch (...) {
		rethrowm (me, ": row ", rowNumber, " not removed.");
	}
}

void TableOfReal_insertRow (I, long rowNumber) {
	iam (TableOfReal);
	try {
		if (rowNumber < 1 || rowNumber > my numberOfRows + 1)
			Melder_throw ("Cannot create row ", rowNumber, ".");
		autoNUMmatrix <double> data (1, my numberOfRows + 1, 1, my numberOfColumns);
		autoNUMvector <wchar *> rowLabels (1, my numberOfRows + 1);
		for (long irow = 1; irow < rowNumber; irow ++)	{
			rowLabels [irow] = my rowLabels [irow];
			for (long icol = 1; icol <= my numberOfColumns; icol ++)
				data [irow] [icol] = my data [irow] [icol];
		}
		for (long irow = my numberOfRows + 1; irow > rowNumber; irow --) {
			rowLabels [irow] = my rowLabels [irow - 1];
			for (long icol = 1; icol <= my numberOfColumns; icol ++)
				data [irow] [icol] = my data [irow - 1] [icol];
		}
		/*
		 * Change without error.
		 */
		NUMvector_free <wchar *> (my rowLabels, 1);
		my rowLabels = rowLabels.transfer();
		NUMmatrix_free <double> (my data, 1, 1);
		my data = data.transfer();
		my numberOfRows ++;
	} catch (...) {
		rethrowm (me, ": row at position ", rowNumber, " not inserted.");
	}
}

void TableOfReal_removeColumn (I, long columnNumber) {
	iam (TableOfReal);
	try {
		if (my numberOfColumns == 1)
			Melder_throw ("Cannot remove the only column.");
		if (columnNumber < 1 || columnNumber > my numberOfColumns)
			Melder_throw ("No column ", columnNumber, ".");
		autoNUMmatrix <double> data (1, my numberOfRows, 1, my numberOfColumns - 1);
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			for (long icol = 1; icol < columnNumber; icol ++)
				data [irow] [icol] = my data [irow] [icol];
			for (long icol = columnNumber; icol < my numberOfColumns; icol ++)
				data [irow] [icol] = my data [irow] [icol + 1];
		}
		/*
		 * Change without error.
		 */
		Melder_free (my columnLabels [columnNumber]);
		for (long icol = columnNumber; icol < my numberOfColumns; icol ++)
			my columnLabels [icol] = my columnLabels [icol + 1];
		NUMmatrix_free <double> (my data, 1, 1);
		my data = data.transfer();
		my numberOfColumns --;
	} catch (...) {
		rethrowm (me, ": column at position ", columnNumber, " not inserted.");
	}
}

void TableOfReal_insertColumn (I, long columnNumber) {
	iam (TableOfReal);
	try {
		if (columnNumber < 1 || columnNumber > my numberOfColumns + 1)
			Melder_throw ("Cannot create column ", columnNumber, ".");
		autoNUMmatrix <double> data (1, my numberOfRows, 1, my numberOfColumns + 1);
		autoNUMvector <wchar*> columnLabels (1, my numberOfColumns + 1);
		for (long j = 1; j < columnNumber; j ++) {
			columnLabels [j] = my columnLabels [j];
			for (long i = 1; i <= my numberOfRows; i ++) data [i] [j] = my data [i] [j];
		}
		for (long j = my numberOfColumns + 1; j > columnNumber; j --) {
			columnLabels [j] = my columnLabels [j - 1];
			for (long i = 1; i <= my numberOfRows; i ++) data [i] [j] = my data [i] [j - 1];
		}
		/*
		 * Change without error.
		 */
		NUMvector_free <wchar *> (my columnLabels, 1);
		my columnLabels = columnLabels.transfer();
		NUMmatrix_free <double> (my data, 1, 1);
		my data = data.transfer();
		my numberOfColumns ++;
	} catch (...) {
		rethrowm (me, ": column at position ", columnNumber, " not inserted.");
	}
}

void TableOfReal_setRowLabel (I, long rowNumber, const wchar_t *label) {
	iam (TableOfReal);
	try {
		if (rowNumber < 1 || rowNumber > my numberOfRows) return;
		autostring newLabel = Melder_wcsdup_e (label);
		/*
		 * Change without error.
		 */
		Melder_free (my rowLabels [rowNumber]);
		my rowLabels [rowNumber] = newLabel.transfer();
	} catch (...) {
		rethrowm (me, ": label of row ", rowNumber, " not set.");
	}
}

void TableOfReal_setColumnLabel (I, long columnNumber, const wchar_t *label) {
	iam (TableOfReal);
	try {
		if (columnNumber < 1 || columnNumber > my numberOfColumns) return;
		autostring newLabel = Melder_wcsdup_e (label);
		/*
		 * Change without error.
		 */
		Melder_free (my columnLabels [columnNumber]);
		my columnLabels [columnNumber] = newLabel.transfer();
	} catch (...) {
		rethrowm (me, ": label of column ", columnNumber, " not set.");
	}
}

/***** EXTRACT PART *****/

void TableOfReal_copyRowLabels (TableOfReal me, TableOfReal thee) {
	try {
		Melder_assert (me != thee);
		Melder_assert (my numberOfRows == thy numberOfRows);
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			thy rowLabels [irow] = Melder_wcsdup_e (my rowLabels [irow]); therror
		}
	} catch (...) {
		rethrow;
	}
}

void TableOfReal_copyColumnLabels (TableOfReal me, TableOfReal thee) {
	try {
		Melder_assert (me != thee);
		Melder_assert (my numberOfColumns == thy numberOfColumns);
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			thy columnLabels [icol] = Melder_wcsdup_e (my columnLabels [icol]); therror;
		}
	} catch (...) {
		rethrow;
	}
}

void TableOfReal_copyRow (TableOfReal me, long myRow, TableOfReal thee, long thyRow) {
	try {
		Melder_assert (me != thee);
		Melder_assert (my numberOfColumns == thy numberOfColumns);
		thy rowLabels [thyRow] = Melder_wcsdup_e (my rowLabels [myRow]); therror
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			thy data [thyRow] [icol] = my data [myRow] [icol];
		}
	} catch (...) {
		rethrow;
	}
}

void TableOfReal_copyColumn (TableOfReal me, long myCol, TableOfReal thee, long thyCol) {
	try {
		Melder_assert (me != thee);
		Melder_assert (my numberOfRows == thy numberOfRows);
		thy columnLabels [thyCol] = Melder_wcsdup_e (my columnLabels [myCol]); therror
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			thy data [irow] [thyCol] = my data [irow] [myCol];
		}
	} catch (...) {
		rethrow;
	}
}

TableOfReal TableOfReal_extractRowsWhereColumn (I, long column, int which_Melder_NUMBER, double criterion) {
	iam (TableOfReal);
	try {
		if (column < 1 || column > my numberOfColumns)
			Melder_throw ("No such column: ", column, ".");
		long n = 0;
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			if (Melder_numberMatchesCriterion (my data [irow] [column], which_Melder_NUMBER, criterion)) {
				n ++;
			}
		}
		if (n == 0) Melder_throw ("No row matches this criterion.");
		autoTableOfReal thee = TableOfReal_create (n, my numberOfColumns);
		TableOfReal_copyColumnLabels (me, thee.peek()); therror
		n = 0;
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			if (Melder_numberMatchesCriterion (my data [irow] [column], which_Melder_NUMBER, criterion)) {
				TableOfReal_copyRow (me, irow, thee.peek(), ++ n); therror
			}
		}
		return thee.transfer();
	} catch (...) {
		rethrowmzero (me, ": rows not extracted.");
	}
}

TableOfReal TableOfReal_extractRowsWhereLabel (I, int which_Melder_STRING, const wchar_t *criterion) {
	iam (TableOfReal);
	try {
		long n = 0;
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			if (Melder_stringMatchesCriterion (my rowLabels [irow], which_Melder_STRING, criterion)) {
				n ++;
			}
		}
		if (n == 0) Melder_throw (L"No row matches this criterion.");
		autoTableOfReal thee = TableOfReal_create (n, my numberOfColumns);
		TableOfReal_copyColumnLabels (me, thee.peek()); therror
		n = 0;
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			if (Melder_stringMatchesCriterion (my rowLabels [irow], which_Melder_STRING, criterion)) {
				TableOfReal_copyRow (me, irow, thee.peek(), ++ n); therror
			}
		}
		return thee.transfer();
	} catch (...) {
		rethrowmzero (me, ": rows not extracted.");
	}
}

TableOfReal TableOfReal_extractColumnsWhereRow (I, long row, int which_Melder_NUMBER, double criterion) {
	iam (TableOfReal);
	try {
		if (row < 1 || row > my numberOfRows)
			Melder_throw ("No such row: ", row, ".");
		long n = 0;
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			if (Melder_numberMatchesCriterion (my data [row] [icol], which_Melder_NUMBER, criterion)) {
				n ++;
			}
		}
		if (n == 0) Melder_throw ("No column matches this criterion.");

		autoTableOfReal thee = TableOfReal_create (my numberOfRows, n);
		TableOfReal_copyRowLabels (me, thee.peek()); therror
		n = 0;
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			if (Melder_numberMatchesCriterion (my data [row] [icol], which_Melder_NUMBER, criterion)) {
				TableOfReal_copyColumn (me, icol, thee.peek(), ++ n); therror
			}
		}
		return thee.transfer();
	} catch (...) {
		rethrowmzero (me, ": columns not extracted.");
	}
}

TableOfReal TableOfReal_extractColumnsWhereLabel (I, int which_Melder_STRING, const wchar_t *criterion) {
	iam (TableOfReal);
	try {
		long n = 0;
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			if (Melder_stringMatchesCriterion (my columnLabels [icol], which_Melder_STRING, criterion)) {
				n ++;
			}
		}
		if (n == 0) Melder_throw ("No column matches this criterion.");

		autoTableOfReal thee = TableOfReal_create (my numberOfRows, n);
		TableOfReal_copyRowLabels (me, thee.peek()); therror
		n = 0;
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			if (Melder_stringMatchesCriterion (my columnLabels [icol], which_Melder_STRING, criterion)) {
				TableOfReal_copyColumn (me, icol, thee.peek(), ++ n); therror
			}
		}
		return thee.transfer();
	} catch (...) {
		rethrowmzero (me, ": columns not extracted.");
	}
}

/*
 * Acceptable ranges e.g. "1 4 2 3:7 4:3 3:5:2" -->
 * 1, 4, 2, 3, 4, 5, 6, 7, 4, 3, 3, 4, 5, 4, 3, 2
 * Overlap is allowed. Ranges can go up and down.
 */
static long *getElementsOfRanges (const wchar_t *ranges, long maximumElement, long *numberOfElements, const wchar_t *elementType) {
	try {
		/*
		 * Count the elements.
		 */
		long previousElement = 0;
		*numberOfElements = 0;
		const wchar *p = & ranges [0];
		for (;;) {
			while (*p == ' ' || *p == '\t') p ++;
			if (*p == '\0') break;
			if (isdigit (*p)) {
				long currentElement = wcstol (p, NULL, 10);
				if (currentElement == 0)
					Melder_throw ("No such ", elementType, L": 0 (minimum is 1).");
				if (currentElement > maximumElement)
					Melder_throw ("No such ", elementType, ": ", currentElement, " (maximum is ", maximumElement, ").");
				*numberOfElements += 1;
				previousElement = currentElement;
				do { p ++; } while (isdigit (*p));
			} else if (*p == ':') {
				if (previousElement == 0)
					Melder_throw ("Cannot start range with colon.");
				do { p ++; } while (*p == ' ' || *p == '\t');
				if (*p == '\0')
					Melder_throw ("Cannot end range with colon.");
				if (! isdigit (*p))
					Melder_throw ("End of range should be a positive whole number.");
				long currentElement = wcstol (p, NULL, 10);
				if (currentElement == 0)
					Melder_throw ("No such ", elementType, ": 0 (minimum is 1).");
				if (currentElement > maximumElement)
					Melder_throw ("No such ", elementType, ": ", currentElement, " (maximum is ", maximumElement, ").");
				if (currentElement > previousElement) {
					*numberOfElements += currentElement - previousElement;
				} else {
					*numberOfElements += previousElement - currentElement;
				}
				previousElement = currentElement;
				do { p ++; } while (isdigit (*p));
			} else {
				Melder_throw ("Start of range should be a positive whole number.");
			}
		}
		/*
		 * Create room for the elements.
		 */
		autoNUMvector <long> elements (1, *numberOfElements);
		/*
		 * Store the elements.
		 */
		previousElement = 0;
		*numberOfElements = 0;
		p = & ranges [0];
		for (;;) {
			while (*p == ' ' || *p == '\t') p ++;
			if (*p == '\0') break;
			if (isdigit (*p)) {
				long currentElement = wcstol (p, NULL, 10);
				elements [++ *numberOfElements] = currentElement;
				previousElement = currentElement;
				do { p ++; } while (isdigit (*p));
			} else if (*p == ':') {
				do { p ++; } while (*p == ' ' || *p == '\t');
				long currentElement = wcstol (p, NULL, 10);
				if (currentElement > previousElement) {
					for (long ielement = previousElement + 1; ielement <= currentElement; ielement ++) {
						elements [++ *numberOfElements] = ielement;
					}
				} else {
					for (long ielement = previousElement - 1; ielement >= currentElement; ielement --) {
						elements [++ *numberOfElements] = ielement;
					}
				}
				previousElement = currentElement;
				do { p ++; } while (isdigit (*p));
			}
		}
		return elements.transfer();
	} catch (...) {
		rethrowzero;
	}
}

TableOfReal TableOfReal_extractRowRanges (I, const wchar_t *ranges) {
	iam (TableOfReal);
	try {
		long numberOfElements;
		autoNUMvector <long> elements (getElementsOfRanges (ranges, my numberOfRows, & numberOfElements, L"row"), 1);
		autoTableOfReal thee = TableOfReal_create (numberOfElements, my numberOfColumns);
		TableOfReal_copyColumnLabels (me, thee.peek()); therror
		for (long ielement = 1; ielement <= numberOfElements; ielement ++) {
			TableOfReal_copyRow (me, elements [ielement], thee.peek(), ielement); therror
		}
		return thee.transfer();
	} catch (...) {
		rethrowmzero (me, ": row ranges not extracted.");
	}
}

TableOfReal TableOfReal_extractColumnRanges (I, const wchar_t *ranges) {
	iam (TableOfReal);
	try {
		long numberOfElements;
		autoNUMvector <long> elements (getElementsOfRanges (ranges, my numberOfColumns, & numberOfElements, L"column"), 1);
		autoTableOfReal thee = TableOfReal_create (my numberOfRows, numberOfElements);
		TableOfReal_copyRowLabels (me, thee.peek()); therror
		for (long ielement = 1; ielement <= numberOfElements; ielement ++) {
			TableOfReal_copyColumn (me, elements [ielement], thee.peek(), ielement); therror
		}
		return thee.transfer();
	} catch (...) {
		rethrowmzero (me, ": column ranges not extracted.");
	}
}

/***** EXTRACT *****/

Strings TableOfReal_extractRowLabelsAsStrings (I) {
	iam (TableOfReal);
	try {
		autoStrings thee = Thing_new (Strings);
		thy strings = NUMwvector (1, my numberOfRows); therror
		thy numberOfStrings = my numberOfRows;
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			thy strings [irow] = Melder_wcsdup_e (my rowLabels [irow] ? my rowLabels [irow] : L""); therror
		}
		return thee.transfer();	
	} catch (...) {
		rethrowmzero (me, ": row labels not extracted.");
	}
}

Strings TableOfReal_extractColumnLabelsAsStrings (I) {
	iam (TableOfReal);
	try {
		autoStrings thee = Thing_new (Strings);
		thy strings = NUMwvector (1, my numberOfColumns); therror
		thy numberOfStrings = my numberOfColumns;
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			thy strings [icol] = Melder_wcsdup_e (my columnLabels [icol] ? my columnLabels [icol] : L""); therror
		}
		return thee.transfer();	
	} catch (...) {
		rethrowmzero (me, ": column labels not extracted.");
	}
}

Any TablesOfReal_append (I, thou) {
	iam (TableOfReal); thouart (TableOfReal);
	try {
		if (thy numberOfColumns != my numberOfColumns)
			Melder_throw (L"Numbers of columns are ", my numberOfColumns, " and ", thy numberOfColumns, " but should be equal.");
		autoTableOfReal him = static_cast<TableOfReal> (_Thing_new (my methods));
		TableOfReal_init (him.peek(), my numberOfRows + thy numberOfRows, my numberOfColumns); therror
		/* Unsafe: new attributes not initialized. */
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			TableOfReal_setColumnLabel (him.peek(), icol, my columnLabels [icol]); therror
		}
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			TableOfReal_setRowLabel (him.peek(), irow, my rowLabels [irow]); therror
			for (long icol = 1; icol <= my numberOfColumns; icol ++)
				his data [irow] [icol] = my data [irow] [icol];
		}
		for (long irow = 1; irow <= thy numberOfRows; irow ++) {
			long hisRow = irow + my numberOfRows;
			TableOfReal_setRowLabel (him.peek(), hisRow, thy rowLabels [irow]); therror
			for (long icol = 1; icol <= my numberOfColumns; icol ++)
				his data [hisRow] [icol] = thy data [irow] [icol];
		}
		return him.transfer();
	} catch (...) {
		rethrowmzero ("TableOfReal objects not appended.");
	}
}

Any TablesOfReal_appendMany (Collection me) {
	try {
		if (my size == 0) Melder_throw ("Cannot add zero tables.");
		TableOfReal thee = static_cast <TableOfReal> (my item [1]);
		long totalNumberOfRows = thy numberOfRows;
		long numberOfColumns = thy numberOfColumns;
		for (long itab = 2; itab <= my size; itab ++) {
			thee = static_cast <TableOfReal> (my item [itab]);
			totalNumberOfRows += thy numberOfRows;
			if (thy numberOfColumns != numberOfColumns) Melder_throw ("Numbers of columns do not match.");
		}
		autoTableOfReal him = static_cast <TableOfReal> (_Thing_new (thy methods));
		TableOfReal_init (him.peek(), totalNumberOfRows, numberOfColumns); therror
		/* Unsafe: new attributes not initialized. */
		for (long icol = 1; icol <= numberOfColumns; icol ++) {
			TableOfReal_setColumnLabel (him.peek(), icol, thy columnLabels [icol]); therror
		}
		totalNumberOfRows = 0;
		for (long itab = 1; itab <= my size; itab ++) {
			thee = static_cast <TableOfReal> (my item [itab]);
			for (long irow = 1; irow <= thy numberOfRows; irow ++) {
				totalNumberOfRows ++;
				TableOfReal_setRowLabel (him.peek(), totalNumberOfRows, thy rowLabels [irow]); therror
				for (long icol = 1; icol <= numberOfColumns; icol ++)
					his data [totalNumberOfRows] [icol] = thy data [irow] [icol];
			}
		}
		Melder_assert (totalNumberOfRows == his numberOfRows);
		return him.transfer();
	} catch (...) {
		rethrowmzero ("TableOfReal objects not appended.");
	}
}

static void TableOfReal_sort (TableOfReal me, bool useLabels, long column1, long column2) {
	for (long irow = 1; irow < my numberOfRows; irow ++) for (long jrow = irow + 1; jrow <= my numberOfRows; jrow ++) {
		wchar_t *tmpString;
		if (useLabels) {
			if (my rowLabels [irow] != NULL) {
				if (my rowLabels [jrow] != NULL) {
					int compare = wcscmp (my rowLabels [irow], my rowLabels [jrow]);
					if (compare < 0) continue;
					if (compare > 0) goto swap;
				} else goto swap;
			} else if (my rowLabels [jrow] != NULL) continue;
		}
		/*
		 * If we arrive here, the two labels are equal or both NULL (or useLabels is FALSE).
		 */
		if (column1 > 0 && column1 <= my numberOfColumns) {
			if (my data [irow] [column1] < my data [jrow] [column1]) continue;
			if (my data [irow] [column1] > my data [jrow] [column1]) goto swap;
		}
		if (column2 > 0 && column2 <= my numberOfColumns) {
			if (my data [irow] [column2] < my data [jrow] [column2]) continue;
			if (my data [irow] [column2] > my data [jrow] [column2]) goto swap;
		}
		/*
		 * If we arrive here, everything is equal.
		 */
		continue;
	swap:
		tmpString = my rowLabels [irow];
		my rowLabels [irow] = my rowLabels [jrow];
		my rowLabels [jrow] = tmpString;
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			double tmpValue = my data [irow] [icol];
			my data [irow] [icol] = my data [jrow] [icol];
			my data [jrow] [icol] = tmpValue;
		}
	}
}

void TableOfReal_sortByLabel (I, long column1, long column2) {
	iam (TableOfReal);
	TableOfReal_sort (me, true, column1, column2);
}

void TableOfReal_sortByColumn (I, long column1, long column2) {
	iam (TableOfReal);
	TableOfReal_sort (me, false, column1, column2);
}

TableOfReal Table_to_TableOfReal (Table me, long labelColumn) {
	try {
		if (labelColumn < 1 || labelColumn > my numberOfColumns) labelColumn = 0;
		autoTableOfReal thee = TableOfReal_create (my rows -> size, labelColumn ? my numberOfColumns - 1 : my numberOfColumns);
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			Table_numericize_Assert (me, icol);
		}
		if (labelColumn) {
			for (long icol = 1; icol < labelColumn; icol ++) {
				TableOfReal_setColumnLabel (thee.peek(), icol, my columnHeaders [icol]. label); therror
			}
			for (long icol = labelColumn + 1; icol <= my numberOfColumns; icol ++) {
				TableOfReal_setColumnLabel (thee.peek(), icol - 1, my columnHeaders [icol]. label); therror
			}
			for (long irow = 1; irow <= my rows -> size; irow ++) {
				TableRow row = static_cast <TableRow> (my rows -> item [irow]);
				wchar_t *string = row -> cells [labelColumn]. string;
				TableOfReal_setRowLabel (thee.peek(), irow, string ? string : L""); therror
				for (long icol = 1; icol < labelColumn; icol ++) {
					thy data [irow] [icol] = row -> cells [icol]. number;   // Optimization.
					//thy data [irow] [icol] = Table_getNumericValue_Assert (me, irow, icol);
				}
				for (long icol = labelColumn + 1; icol <= my numberOfColumns; icol ++) {
					thy data [irow] [icol - 1] = row -> cells [icol]. number;   // Optimization.
					//thy data [irow] [icol - 1] = Table_getNumericValue_Assert (me, irow, icol);
				}
			}
		} else {
			for (long icol = 1; icol <= my numberOfColumns; icol ++) {
				TableOfReal_setColumnLabel (thee.peek(), icol, my columnHeaders [icol]. label); therror
			}
			for (long irow = 1; irow <= my rows -> size; irow ++) {
				TableRow row = static_cast <TableRow> (my rows -> item [irow]);
				for (long icol = 1; icol <= my numberOfColumns; icol ++) {
					thy data [irow] [icol] = row -> cells [icol]. number;   // Optimization.
					//thy data [irow] [icol] = Table_getNumericValue_Assert (me, irow, icol);
				}
			}
		}
		return thee.transfer();
	} catch (...) {
		rethrowmzero (me, ": not converted to TableOfReal.");
	}
}

Table TableOfReal_to_Table (TableOfReal me, const wchar_t *labelOfFirstColumn) {
	try {
		autoTable thee = Table_createWithoutColumnNames (my numberOfRows, my numberOfColumns + 1);
		Table_setColumnLabel (thee.peek(), 1, labelOfFirstColumn); therror
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			wchar_t *columnLabel = my columnLabels [icol];
			thy columnHeaders [icol + 1]. label = Melder_wcsdup_e (columnLabel && columnLabel [0] ? columnLabel : L"?"); therror
		}
		for (long irow = 1; irow <= thy rows -> size; irow ++) {
			wchar_t *stringValue = my rowLabels [irow];
			TableRow row = static_cast <TableRow> (thy rows -> item [irow]);
			row -> cells [1]. string = Melder_wcsdup_e (stringValue && stringValue [0] ? stringValue : L"?"); therror
			for (long icol = 1; icol <= my numberOfColumns; icol ++) {
				double numericValue = my data [irow] [icol];
				row -> cells [icol + 1]. string = Melder_wcsdup_e (Melder_double (numericValue)); therror
			}
		}
		return thee.transfer();
	} catch (...) {
		rethrowmzero (me, ": not converted to Table.");
	}
}

int TableOfReal_writeToHeaderlessSpreadsheetFile (TableOfReal me, MelderFile file) {
	try {
		autoMelderString buffer;
		MelderString_copy (& buffer, L"rowLabel"); therror
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			MelderString_appendCharacter (& buffer, '\t'); therror
			wchar_t *s = my columnLabels [icol];
			MelderString_append (& buffer, s != NULL && s [0] != '\0' ? s : L"?"); therror
		}
		MelderString_appendCharacter (& buffer, '\n'); therror
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			wchar_t *s = my rowLabels [irow];
			MelderString_append (& buffer, s != NULL && s [0] != '\0' ? s : L"?"); therror
			for (long icol = 1; icol <= my numberOfColumns; icol ++) {
				MelderString_appendCharacter (& buffer, '\t'); therror
				double x = my data [irow] [icol];
				MelderString_append (& buffer, Melder_double (x)); therror
			}
			MelderString_appendCharacter (& buffer, '\n'); therror
		}
		MelderFile_writeText (file, buffer.string); therror
		return 1;
	} catch (...) {
		rethrowmzero (me, ": not saved to headerless spreadsheet file.");
	}
}

TableOfReal TableOfReal_readFromHeaderlessSpreadsheetFile (MelderFile file) {
	try {
		autostring string = MelderFile_readText (file);
		long nrow, ncol, nelements;

		/*
		 * Count columns.
		 */
		ncol = 0;
		wchar_t *p = & string [0];
		for (;;) {
			wchar_t kar = *p++;
			if (kar == '\n' || kar == '\0') break;
			if (kar == ' ' || kar == '\t') continue;
			ncol ++;
			do { kar = *p++; } while (kar != ' ' && kar != '\t' && kar != '\n' && kar != '\0');
			if (kar == '\n' || kar == '\0') break;
		}
		ncol --;
		if (ncol < 1) Melder_throw ("No columns.");

		/*
		 * Count elements.
		 */
		p = & string [0];
		nelements = 0;
		for (;;) {
			wchar_t kar = *p++;
			if (kar == '\0') break;
			if (kar == ' ' || kar == '\t' || kar == '\n') continue;
			nelements ++;
			do { kar = *p++; } while (kar != ' ' && kar != '\t' && kar != '\n' && kar != '\0');
			if (kar == '\0') break;
		}

		/*
		 * Check if all columns are complete.
		 */
		if (nelements == 0 || nelements % (ncol + 1) != 0)
			Melder_throw ("The number of elements (", nelements, ") is not a multiple of the number of columns plus 1 (", ncol + 1, ").");

		/*
		 * Create empty table.
		 */
		nrow = nelements / (ncol + 1) - 1;
		autoTableOfReal me = TableOfReal_create (nrow, ncol);

		/*
		 * Read elements.
		 */
		p = & string [0];
		while (*p == ' ' || *p == '\t') { Melder_assert (*p != '\0'); p ++; }
		while (*p != ' ' && *p != '\t') { Melder_assert (*p != '\0'); p ++; }   // ignore the header of the zeroth column ("rowLabel" perhaps)
		for (long icol = 1; icol <= ncol; icol ++) {
			while (*p == ' ' || *p == '\t') { Melder_assert (*p != '\0'); p ++; }
			static MelderString buffer = { 0 };
			MelderString_empty (& buffer);
			while (*p != ' ' && *p != '\t' && *p != '\n') {
				MelderString_appendCharacter (& buffer, *p); therror
				p ++;
			}
			TableOfReal_setColumnLabel (me.peek(), icol, buffer.string); therror
			MelderString_empty (& buffer);
		}
		for (long irow = 1; irow <= nrow; irow ++) {
			while (*p == ' ' || *p == '\t' || *p == '\n') { Melder_assert (*p != '\0'); p ++; }
			static MelderString buffer = { 0 };
			MelderString_empty (& buffer);
			while (*p != ' ' && *p != '\t') {
				MelderString_appendCharacter (& buffer, *p); therror
				p ++;
			}
			TableOfReal_setRowLabel (me.peek(), irow, buffer.string);
			MelderString_empty (& buffer);
			for (long icol = 1; icol <= ncol; icol ++) {
				while (*p == ' ' || *p == '\t' || *p == '\n') { Melder_assert (*p != '\0'); p ++; }
				MelderString_empty (& buffer);
				while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0') {
					MelderString_appendCharacter (& buffer, *p); therror
					p ++;
				}
				my data [irow] [icol] = Melder_atof (buffer.string);   /* If cell contains a string, this will be 0. */
				MelderString_empty (& buffer);
			}
		}
		return me.transfer();
	} catch (...) {
		rethrowmzero ("TableOfReal: headerless spreadsheet file ", MelderFile_messageName (file), " not read.");
	}
}

/* End of file TableOfReal.cpp */
