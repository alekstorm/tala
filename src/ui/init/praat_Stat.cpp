/* praat_Stat.cpp
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
 * pb 2011/03/28
 */

#include "stat/Distributions_and_Strings.h"
#include "stat/LogisticRegression.h"
#include "stat/PairDistribution.h"
#include "fon/Matrix.h"
#include "dwtools/SSCP.h"
#include "kar/UnicodeData.h"
#include "stat/Table.h"
#include "ui/editors/TableEditor.h"
#include "ui/Formula.h"
#include "ui/Interpreter.h"
#include "ui/UiFile.h"

#include "ui/praat.h"

int Matrix_formula (Matrix me, const wchar_t *expression, Interpreter *interpreter, Matrix target);

void SSCP_drawConcentrationEllipse (SSCP me, Graphics g, double scale,
	int confidence, long d1, long d2, double xmin, double xmax,
	double ymin, double ymax, int garnish);

static wchar_t formatBuffer [32] [40];
static int formatIndex = 0;
static wchar_t * Table_messageColumn (Table me, long column) {
	if (++ formatIndex == 32) formatIndex = 0;
	if (my columnHeaders [column]. label != NULL && my columnHeaders [column]. label [0] != '\0')
		swprintf (formatBuffer [formatIndex], 40, L"\"%.39ls\"ui/editors/AmplitudeTierEditor.h", my columnHeaders [column]. label);
	else
		swprintf (formatBuffer [formatIndex], 40, L"%ld", column);
	return formatBuffer [formatIndex];
}

/***** DISTRIBUTIONS *****/

DIRECT (Distributionses_add)
	Collection me = Collection_create (classDistributions, 10);
	if (! me) return 0;
	WHERE (SELECTED)
		if (! Collection_addItem (me, OBJECT)) { my size = 0; forget (me); return 0; }
	if (! praat_new1 (Distributions_addMany (me), L"added")) {
		my size = 0; forget (me); return 0;   // UGLY
	}
	my size = 0; forget (me);
END

FORM (Distributionses_getMeanAbsoluteDifference, L"Get mean difference", 0)
	NATURAL (L"Column number", L"1")
	OK
DO
	Distributions me = NULL, thee = NULL;
	WHERE (SELECTED) { if (me) thee = static_cast <Distributions> OBJECT; else me = static_cast <Distributions> OBJECT; }   // UGLY
	Melder_informationReal (Distributionses_getMeanAbsoluteDifference (me, thee, GET_INTEGER (L"Column number")), NULL);
END

FORM (Distributions_getProbability, L"Get probability", 0)
	NATURAL (L"Column number", L"1")
	SENTENCE (L"String", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (Distributions);
	double probability = Distributions_getProbability (me, GET_STRING (L"String"), GET_INTEGER (L"Column number")); therror
	Melder_informationReal (probability, NULL);
END

DIRECT (Distributions_help)
	Melder_help (L"Distributions");
END

FORM (Distributions_to_Strings, L"To Strings", 0)
	NATURAL (L"Column number", L"1")
	NATURAL (L"Number of strings", L"1000")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Distributions);
		autoStrings thee = Distributions_to_Strings (me, GET_INTEGER (L"Column number"), GET_INTEGER (L"Number of strings"));
		praat_new (thee.transfer(), my name);
	}
END

FORM (Distributions_to_Strings_exact, L"To Strings (exact)", 0)
	NATURAL (L"Column number", L"1")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Distributions);
		autoStrings thee = Distributions_to_Strings_exact (me, GET_INTEGER (L"Column number"));
		praat_new (thee.transfer(), my name);
	}
END

/***** LOGISTICREGRESSION *****/

static inline double NUMmin2 (double a, double b) {
	return a < b ? a : b;
}

static inline double NUMmax2 (double a, double b) {
	return a > b ? a : b;
}

void LogisticRegression_drawBoundary (LogisticRegression me, Graphics graphics, long colx, double xleft, double xright,
	long coly, double ybottom, double ytop, bool garnish)
{
	RegressionParameter parmx = static_cast<RegressionParameter> (my parameters -> item [colx]);
	RegressionParameter parmy = static_cast<RegressionParameter> (my parameters -> item [coly]);
	if (xleft == xright) {
		xleft = parmx -> minimum;
		xright = parmx -> maximum;
	}
	if (ybottom == ytop) {
		ybottom = parmy -> minimum;
		ytop = parmy -> maximum;
	}
	double intercept = my intercept;
	for (long iparm = 1; iparm <= my parameters -> size; iparm ++) {
		if (iparm != colx && iparm != coly) {
			RegressionParameter parm = static_cast<RegressionParameter> (my parameters -> item [iparm]);
			intercept += parm -> value * (0.5 * (parm -> minimum + parm -> maximum));
		}
	}
	Graphics_setInner (graphics);
	Graphics_setWindow (graphics, xleft, xright, ybottom, ytop);
	double xbottom = (intercept + parmy -> value * ybottom) / - parmx -> value;
	double xtop = (intercept + parmy -> value * ytop) / - parmx -> value;
	double yleft = (intercept + parmx -> value * xleft) / - parmy -> value;
	double yright = (intercept + parmx -> value * xright) / - parmy -> value;
	double xmin = NUMmin2 (xleft, xright), xmax = NUMmax2 (xleft, xright);
	double ymin = NUMmin2 (ybottom, ytop), ymax = NUMmax2 (ybottom, ytop);
	//Melder_casual ("LogisticRegression_drawBoundary: %f %f %f %f %f %f %f %f",
	//	xmin, xmax, xbottom, xtop, ymin, ymax, yleft, yright);
	if (xbottom >= xmin && xbottom <= xmax) {   // line goes through bottom?
		if (xtop >= xmin && xtop <= xmax)   // line goes through top?
			Graphics_line (graphics, xbottom, ybottom, xtop, ytop);   // draw from bottom to top
		else if (yleft >= ymin && yleft <= ymax)   // line goes through left?
			Graphics_line (graphics, xbottom, ybottom, xleft, yleft);   // draw from bottom to left
		else if (yright >= ymin && yright <= ymax)   // line goes through right?
			Graphics_line (graphics, xbottom, ybottom, xright, yright);   // draw from bottom to right
	} else if (yleft >= ymin && yleft <= ymax) {   // line goes through left?
		if (yright >= ymin && yright <= ymax)   // line goes through right?
			Graphics_line (graphics, xleft, yleft, xright, yright);   // draw from left to right
		else if (xtop >= xmin && xtop <= xmax)   // line goes through top?
			Graphics_line (graphics, xleft, yleft, xtop, ytop);   // draw from left to top
	} else if (xtop >= xmin && xtop <= xmax) {   // line goes through top?
		if (yright >= ymin && yright <= ymax)   // line goes through right?
			Graphics_line (graphics, xtop, ytop, xright, yright);   // draw from top to right
	}
	Graphics_unsetInner (graphics);
	if (garnish) {
		Graphics_drawInnerBox (graphics);
		Graphics_textBottom (graphics, true, parmx -> label);
		Graphics_marksBottom (graphics, 2, true, true, false);
		Graphics_textLeft (graphics, true, parmy -> label);
		Graphics_marksLeft (graphics, 2, true, true, false);
	}
}

FORM (LogisticRegression_drawBoundary, L"LogisticRegression: Draw boundary", 0)
	WORD (L"Horizontal factor", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0 (= auto)")
	WORD (L"Vertical factor", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0 (= auto)")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (LogisticRegression);
		long xfactor = Regression_getFactorIndexFromFactorName_e (me, GET_STRING (L"Horizontal factor")); therror
		long yfactor = Regression_getFactorIndexFromFactorName_e (me, GET_STRING (L"Vertical factor")); therror
		LogisticRegression_drawBoundary (me, GRAPHICS,
			xfactor, GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
			yfactor, GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
			GET_INTEGER (L"Garnish")); therror
	}
END

/***** PAIRDISTRIBUTION *****/

DIRECT (PairDistribution_getFractionCorrect_maximumLikelihood)
	iam_ONLY (PairDistribution);
	double fractionCorrect = PairDistribution_getFractionCorrect_maximumLikelihood (me); therror
	Melder_informationReal (fractionCorrect, NULL);
END

DIRECT (PairDistribution_getFractionCorrect_probabilityMatching)
	iam_ONLY (PairDistribution);
	double fractionCorrect = PairDistribution_getFractionCorrect_probabilityMatching (me); therror
	Melder_informationReal (fractionCorrect, NULL);
END

DIRECT (PairDistribution_getNumberOfPairs)
	iam_ONLY (PairDistribution);
	Melder_information1 (Melder_integer (my pairs -> size));
END

FORM (PairDistribution_getString1, L"Get string1", 0)
	NATURAL (L"Pair number", L"1")
	OK
DO
	iam_ONLY (PairDistribution);
	const wchar *string1 = PairDistribution_getString1 (me, GET_INTEGER (L"Pair number")); therror
	Melder_information1 (string1);
END

FORM (PairDistribution_getString2, L"Get string2", 0)
	NATURAL (L"Pair number", L"1")
	OK
DO
	iam_ONLY (PairDistribution);
	const wchar *string2 = PairDistribution_getString2 (me, GET_INTEGER (L"Pair number")); therror
	Melder_information1 (string2);
END

FORM (PairDistribution_getWeight, L"Get weight", 0)
	NATURAL (L"Pair number", L"1")
	OK
DO
	iam_ONLY (PairDistribution);
	double weight = PairDistribution_getWeight (me, GET_INTEGER (L"Pair number")); therror
	Melder_information1 (Melder_double (weight));
END

DIRECT (PairDistribution_help) Melder_help (L"PairDistribution"); END

DIRECT (PairDistribution_removeZeroWeights)
	EVERY (PairDistribution_removeZeroWeights (static_cast <PairDistribution> OBJECT))
END

FORM (PairDistribution_to_Stringses, L"Generate two Strings objects", 0)
	NATURAL (L"Number", L"1000")
	SENTENCE (L"Name of first Strings", L"input")
	SENTENCE (L"Name of second Strings", L"output")
	OK
DO
	iam_ONLY (PairDistribution);
	Strings strings1_, strings2_;
	PairDistribution_to_Stringses (me, GET_INTEGER (L"Number"), & strings1_, & strings2_);
	autoStrings strings1 = strings1_, strings2 = strings2_;   // UGLY
	praat_new (strings1.transfer(), GET_STRING (L"Name of first Strings"));
	praat_new (strings2.transfer(), GET_STRING (L"Name of second Strings"));
END

DIRECT (PairDistribution_to_Table)
	EVERY_TO (PairDistribution_to_Table (static_cast <PairDistribution> OBJECT))
END

/***** PAIRDISTRIBUTION & DISTRIBUTIONS *****/

FORM (PairDistribution_Distributions_getFractionCorrect, L"PairDistribution & Distributions: Get fraction correct", 0)
	NATURAL (L"Column", L"1")
	OK
DO
	iam_ONLY (PairDistribution);
	thouart_ONLY (Distributions);
	double fractionCorrect = PairDistribution_Distributions_getFractionCorrect (me, thee, GET_INTEGER (L"Column")); therror
	Melder_informationReal (fractionCorrect, NULL);
END

/***** TABLE *****/

long Table_drawRowFromDistribution (Table me, long columnNumber) {
	try {
		Table_checkSpecifiedColumnNumberWithinRange (me, columnNumber);
		Table_numericize_checkDefined (me, columnNumber); therror
		if (my rows -> size < 1)
			Melder_throw (me, ": no rows.");
		double total = 0.0;
		for (long irow = 1; irow <= my rows -> size; irow ++) {
			TableRow row = static_cast <TableRow> (my rows -> item [irow]);
			total += row -> cells [columnNumber]. number;
		}
		if (total <= 0.0)
			Melder_throw (me, ": the total weight of column ", columnNumber, " is not positive.");
		long irow;
		do {
			double rand = NUMrandomUniform (0, total), sum = 0.0;
			for (irow = 1; irow <= my rows -> size; irow ++) {
				TableRow row = static_cast <TableRow> (my rows -> item [irow]);
				sum += row -> cells [columnNumber]. number;
				if (rand <= sum) break;
			}
		} while (irow > my rows -> size);   /* Guard against rounding errors. */
		return irow;
	} catch (...) {
		rethrowmzero (me, ": cannot draw a row from the distribution of column ", columnNumber, ".");
	}
}

void Table_drawEllipse_e (Table me, Graphics g, long xcolumn, long ycolumn,
	double xmin, double xmax, double ymin, double ymax, double numberOfSigmas, int garnish)
{
	try {
		if (xcolumn < 1 || xcolumn > my numberOfColumns || ycolumn < 1 || ycolumn > my numberOfColumns) return;
		Table_numericize_Assert (me, xcolumn);
		Table_numericize_Assert (me, ycolumn);
		if (xmin == xmax) {
			if (! Table_getExtrema (me, xcolumn, & xmin, & xmax)) return;
			if (xmin == xmax) xmin -= 0.5, xmax += 0.5;
		}
		if (ymin == ymax) {
			if (! Table_getExtrema (me, ycolumn, & ymin, & ymax)) return;
			if (ymin == ymax) ymin -= 0.5, ymax += 0.5;
		}
		autoTableOfReal tableOfReal = TableOfReal_create (my rows -> size, 2);
		for (long irow = 1; irow <= my rows -> size; irow ++) {
			tableOfReal -> data [irow] [1] = Table_getNumericValue_Assert (me, irow, xcolumn);
			tableOfReal -> data [irow] [2] = Table_getNumericValue_Assert (me, irow, ycolumn);
		}
		autoSSCP sscp = TableOfReal_to_SSCP (tableOfReal.peek(), 0, 0, 0, 0);
		SSCP_drawConcentrationEllipse (sscp.peek(), g, numberOfSigmas, 0, 1, 2, xmin, xmax, ymin, ymax, garnish);
	} catch (...) {
		Melder_clearError ();   // drawing errors shall be ignored
	}
}

void Table_scatterPlot_mark (Table me, Graphics g, long xcolumn, long ycolumn,
	double xmin, double xmax, double ymin, double ymax, double markSize_mm, const wchar *mark, int garnish)
{
	long n = my rows -> size, irow;
	if (xcolumn < 1 || xcolumn > my numberOfColumns || ycolumn < 1 || ycolumn > my numberOfColumns) return;
	Table_numericize_Assert (me, xcolumn);
	Table_numericize_Assert (me, ycolumn);
	if (xmin == xmax) {
		if (! Table_getExtrema (me, xcolumn, & xmin, & xmax)) return;
		if (xmin == xmax) xmin -= 0.5, xmax += 0.5;
	}
	if (ymin == ymax) {
		if (! Table_getExtrema (me, ycolumn, & ymin, & ymax)) return;
		if (ymin == ymax) ymin -= 0.5, ymax += 0.5;
	}
	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);

	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	for (irow = 1; irow <= n; irow ++) {
		TableRow row = static_cast <TableRow> (my rows -> item [irow]);
		Graphics_mark (g, row -> cells [xcolumn]. number, row -> cells [ycolumn]. number, markSize_mm, mark);
	}
	Graphics_unsetInner (g);
	if (garnish) {
		Graphics_drawInnerBox (g);
		Graphics_marksBottom (g, 2, TRUE, TRUE, FALSE);
		if (my columnHeaders [xcolumn]. label)
			Graphics_textBottom (g, TRUE, my columnHeaders [xcolumn]. label);
		Graphics_marksLeft (g, 2, TRUE, TRUE, FALSE);
		if (my columnHeaders [ycolumn]. label)
			Graphics_textLeft (g, TRUE, my columnHeaders [ycolumn]. label);
	}
}

void Table_scatterPlot (Table me, Graphics g, long xcolumn, long ycolumn,
	double xmin, double xmax, double ymin, double ymax, long markColumn, int fontSize, int garnish)
{
	long n = my rows -> size;
	int saveFontSize = Graphics_inqFontSize (g);
	if (xcolumn < 1 || xcolumn > my numberOfColumns || ycolumn < 1 || ycolumn > my numberOfColumns) return;
	Table_numericize_Assert (me, xcolumn);
	Table_numericize_Assert (me, ycolumn);
	if (xmin == xmax) {
		if (! Table_getExtrema (me, xcolumn, & xmin, & xmax)) return;
		if (xmin == xmax) xmin -= 0.5, xmax += 0.5;
	}
	if (ymin == ymax) {
		if (! Table_getExtrema (me, ycolumn, & ymin, & ymax)) return;
		if (ymin == ymax) ymin -= 0.5, ymax += 0.5;
	}
	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);

	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	Graphics_setFontSize (g, fontSize);
	for (long irow = 1; irow <= n; irow ++) {
		TableRow row = static_cast <TableRow> (my rows -> item [irow]);
		const wchar *mark = row -> cells [markColumn]. string;
		if (mark)
			Graphics_text (g, row -> cells [xcolumn]. number, row -> cells [ycolumn]. number, mark);
	}
	Graphics_setFontSize (g, saveFontSize);
	Graphics_unsetInner (g);
	if (garnish) {
		Graphics_drawInnerBox (g);
		Graphics_marksBottom (g, 2, TRUE, TRUE, FALSE);
		if (my columnHeaders [xcolumn]. label)
			Graphics_textBottom (g, TRUE, my columnHeaders [xcolumn]. label);
		Graphics_marksLeft (g, 2, TRUE, TRUE, FALSE);
		if (my columnHeaders [ycolumn]. label)
			Graphics_textLeft (g, TRUE, my columnHeaders [ycolumn]. label);
	}
}

DIRECT (Tables_append)
	autoCollection me = Collection_create (classTable, 10);
	try {
		WHERE (SELECTED) Collection_addItem (me.peek(), OBJECT);   // dangle (share pointers)
		autoTable thee = Tables_append (me.peek());
		praat_new (thee.transfer(), L"appended");
		my size = 0;   //undangle (UGLY)
	} catch (...) {
		my size = 0;   //undangle (UGLY)
		rethrowzero;
	}
END

FORM (Table_appendColumn, L"Table: Append column", 0)
	WORD (L"Label", L"newcolumn")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		Table_appendColumn (me, GET_STRING (L"Label")); therror
		praat_dataChanged (OBJECT);
	}
END

FORM (Table_appendDifferenceColumn, L"Table: Append difference column", 0)
	WORD (L"left Columns", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"right Columns", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"Label", L"diff")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"left Columns")); therror
		long jcol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"right Columns")); therror
		Table_appendDifferenceColumn (me, icol, jcol, GET_STRING (L"Label")); therror
		praat_dataChanged (me);
	}
END

FORM (Table_appendProductColumn, L"Table: Append product column", 0)
	WORD (L"left Columns", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"right Columns", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"Label", L"diff")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"left Columns")); therror
		long jcol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"right Columns")); therror
		Table_appendProductColumn (me, icol, jcol, GET_STRING (L"Label")); therror
		praat_dataChanged (me);
	}
END

FORM (Table_appendQuotientColumn, L"Table: Append quotient column", 0)
	WORD (L"left Columns", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"right Columns", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"Label", L"diff")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"left Columns")); therror
		long jcol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"right Columns")); therror
		Table_appendQuotientColumn (me, icol, jcol, GET_STRING (L"Label")); therror
		praat_dataChanged (me);
	}
END

FORM (Table_appendSumColumn, L"Table: Append sum column", 0)
	WORD (L"left Columns", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"right Columns", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"Label", L"diff")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"left Columns")); therror
		long jcol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"right Columns")); therror
		Table_appendSumColumn (me, icol, jcol, GET_STRING (L"Label")); therror
		praat_dataChanged (me);
	}
END

DIRECT (Table_appendRow)
	WHERE (SELECTED) {
		iam_LOOP (Table);
		Table_appendRow (me);
		praat_dataChanged (me);
	}
END

FORM (Table_collapseRows, L"Table: Collapse rows", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Columns with factors (independent variables):")
	TEXTFIELD (L"factors", L"speaker dialect age vowel")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Columns to sum:")
	TEXTFIELD (L"columnsToSum", L"number cost")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Columns to average:")
	TEXTFIELD (L"columnsToAverage", L"price")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Columns to medianize:")
	TEXTFIELD (L"columnsToMedianize", L"vot")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Columns to average logarithmically:")
	TEXTFIELD (L"columnsToAverageLogarithmically", L"duration")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Columns to medianize logarithmically:")
	TEXTFIELD (L"columnsToMedianizeLogarithmically", L"F0 F1 F2 F3")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Columns not mentioned above will be ignored.")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		autoTable thee = Table_collapseRows (me,
			GET_STRING (L"factors"), GET_STRING (L"columnsToSum"),
			GET_STRING (L"columnsToAverage"), GET_STRING (L"columnsToMedianize"),
			GET_STRING (L"columnsToAverageLogarithmically"), GET_STRING (L"columnsToMedianizeLogarithmically"));
		praat_new (thee.transfer(), my name, L"_pooled");
	}
END

FORM (Table_createWithColumnNames, L"Create Table with column names", 0)
	WORD (L"Name", L"table")
	INTEGER (L"Number of rows", L"10")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Column names:")
	TEXTFIELD (L"columnNames", L"speaker dialect age vowel F0 F1 F2")
	OK
DO
	autoTable me = Table_createWithColumnNames (GET_INTEGER (L"Number of rows"), GET_STRING (L"columnNames"));
	praat_new (me.transfer(), GET_STRING (L"Name"));
END

FORM (Table_createWithoutColumnNames, L"Create Table without column names", 0)
	WORD (L"Name", L"table")
	INTEGER (L"Number of rows", L"10")
	NATURAL (L"Number of columns", L"3")
	OK
DO
	autoTable me = Table_createWithoutColumnNames (GET_INTEGER (L"Number of rows"), GET_INTEGER (L"Number of columns"));
	praat_new (me.transfer(), GET_STRING (L"Name"));
END

FORM (Table_drawEllipse, L"Draw ellipse (standard deviation)", 0)
	WORD (L"Horizontal column", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0 (= auto)")
	WORD (L"Vertical column", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0 (= auto)")
	POSITIVE (L"Number of sigmas", L"2.0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long xcolumn = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Horizontal column")); therror
		long ycolumn = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Vertical column")); therror
		Table_drawEllipse_e (me, GRAPHICS, xcolumn, ycolumn,
			GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
			GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
			GET_REAL (L"Number of sigmas"), GET_INTEGER (L"Garnish")); therror
	}
END

FORM (Table_drawRowFromDistribution, L"Table: Draw row from distribution", 0)
	WORD (L"Column with distribution", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (Table);
	long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column with distribution")); therror
	long row = Table_drawRowFromDistribution (me, icol); therror
	Melder_information1 (Melder_integer (row));
END

DIRECT (Table_edit)
	if (theCurrentPraatApplication -> batch) Melder_throw ("Cannot edit a Table from batch.");
	WHERE (SELECTED) {
		iam_LOOP (Table);
		TableEditor *editor = new TableEditor (theCurrentPraatApplication -> topShell, ID_AND_FULL_NAME, me);
		praat_installEditor (editor, IOBJECT); therror
	}
END

FORM (Table_extractRowsWhereColumn_number, L"Table: Extract rows where column (number)", 0)
	WORD (L"Extract all rows where column...", L"ui/editors/AmplitudeTierEditor.h")
	RADIO_ENUM (L"...is...", kMelder_number, DEFAULT)
	REAL (L"...the number", L"0.0")
	OK
DO
	double value = GET_REAL (L"...the number");
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Extract all rows where column...")); therror
		autoTable thee = Table_extractRowsWhereColumn_number (me, icol, GET_ENUM (kMelder_number, L"...is..."), value);
		praat_new (thee.transfer(), my name, L"_", Table_messageColumn (static_cast <Table> OBJECT, icol), L"_", NUMdefined (value) ? Melder_integer ((long) round (value)) : L"undefined");
		praat_dataChanged (me);   // WHY?
	}
END

FORM (Table_extractRowsWhereColumn_text, L"Table: Extract rows where column (text)", 0)
	WORD (L"Extract all rows where column...", L"ui/editors/AmplitudeTierEditor.h")
	OPTIONMENU_ENUM (L"...", kMelder_string, DEFAULT)
	SENTENCE (L"...the text", L"hi")
	OK
DO
	const wchar_t *value = GET_STRING (L"...the text");
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Extract all rows where column...")); therror
		autoTable thee = Table_extractRowsWhereColumn_string (me, icol, GET_ENUM (kMelder_string, L"..."), value);
		praat_new3 (thee.transfer(), my name, L"_", value);
		praat_dataChanged (me);   // WHY?
	}
END

int Table_formula_columnRange (Table me, long fromColumn, long toColumn, const wchar *expression, Interpreter *interpreter) {
	try {
		Table_checkSpecifiedColumnNumberWithinRange (me, fromColumn);
		Table_checkSpecifiedColumnNumberWithinRange (me, toColumn);
		Formula_compile (interpreter, me, expression, kFormula_EXPRESSION_TYPE_UNKNOWN, TRUE); therror
		for (long irow = 1; irow <= my rows -> size; irow ++) {
			for (long icol = fromColumn; icol <= toColumn; icol ++) {
				struct Formula_Result result;
				Formula_run (irow, icol, & result); therror
				if (result. expressionType == kFormula_EXPRESSION_TYPE_STRING) {
					Table_setStringValue (me, irow, icol, result. result.stringResult);
					Melder_free (result. result.stringResult);
				} else if (result. expressionType == kFormula_EXPRESSION_TYPE_NUMERIC) {
					Table_setNumericValue (me, irow, icol, result. result.numericResult);
				} else if (result. expressionType == kFormula_EXPRESSION_TYPE_NUMERIC_ARRAY) {
					Melder_throw (me, ": cannot put arrays into cells.");
				} else if (result. expressionType == kFormula_EXPRESSION_TYPE_STRING_ARRAY) {
					Melder_throw (me, ": cannot put arrays into cells.");
				}
			}
		}
		return 1;
	} catch (...) {
		rethrowmzero (me, ": application of formula not completed.");
	}
}

int Table_formula (Table me, long icol, const wchar *expression, Interpreter *interpreter) {
	return Table_formula_columnRange (me, icol, icol, expression, interpreter);
}

FORM (Table_formula, L"Table: Formula", L"Table: Formula...")
	WORD (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	TEXTFIELD (L"formula", L"abs (self)")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		try {
			long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
			Table_formula (me, icol, GET_STRING (L"formula"), interpreter); therror
			praat_dataChanged (me);
		} catch (...) {
			praat_dataChanged (me);   // in case of error, the Table may have partially changed
			throw 1;
		}
	}
END

FORM (Table_formula_columnRange, L"Table: Formula (column range)", L"Table: Formula...")
	WORD (L"From column label", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"To column label", L"ui/editors/AmplitudeTierEditor.h")
	TEXTFIELD (L"formula", L"log10 (self)")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		try {
			long icol1 = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"From column label")); therror
			long icol2 = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"To column label")); therror
			Table_formula_columnRange (me, icol1, icol2, GET_STRING (L"formula"), interpreter); therror
			praat_dataChanged (me);
		} catch (...) {
			praat_dataChanged (me);   // in case of error, the Table may have partially changed
			throw 1;
		}
	}
END

FORM (Table_getColumnIndex, L"Table: Get column index", 0)
	SENTENCE (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (Table);
	Melder_information1 (Melder_integer (Table_findColumnIndexFromColumnLabel (me, GET_STRING (L"Column label"))));
END

FORM (Table_getColumnLabel, L"Table: Get column label", 0)
	NATURAL (L"Column number", L"1")
	OK
DO
	iam_ONLY (Table);
	long icol = GET_INTEGER (L"Column number");
	if (icol > my numberOfColumns) Melder_throw ("Column number must not be greater than number of columns.");
	Melder_information1 (my columnHeaders [icol]. label);
END

FORM (Table_getGroupMean, L"Table: Get group mean", 0)
	WORD (L"Column label", L"salary")
	WORD (L"Group column", L"gender")
	SENTENCE (L"Group", L"F")
	OK
DO
	iam_ONLY (Table);
	long column = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
	long groupColumn = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Group column")); therror
	Melder_information1 (Melder_double (Table_getGroupMean (static_cast <Table> ONLY_OBJECT, column, groupColumn, GET_STRING (L"Group"))));
END

FORM (Table_getMaximum, L"Table: Get maximum", 0)
	SENTENCE (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (Table);
	long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
	double maximum = Table_getMaximum (me, icol); therror
	Melder_information1 (Melder_double (maximum));
END

FORM (Table_getMean, L"Table: Get mean", 0)
	SENTENCE (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (Table);
	long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
	double mean = Table_getMean (me, icol); therror
	Melder_information1 (Melder_double (mean));
END

FORM (Table_getMinimum, L"Table: Get minimum", 0)
	SENTENCE (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (Table);
	long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
	double minimum = Table_getMinimum (me, icol); therror
	Melder_information1 (Melder_double (minimum));
END

FORM (Table_getQuantile, L"Table: Get quantile", 0)
	SENTENCE (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	POSITIVE (L"Quantile", L"0.50 (= median)")
	OK
DO
	iam_ONLY (Table);
	long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
	double quantile = Table_getQuantile (me, icol, GET_REAL (L"Quantile")); therror
	Melder_information1 (Melder_double (quantile));
END

FORM (Table_getStandardDeviation, L"Table: Get standard deviation", 0)
	SENTENCE (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (Table);
	long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
	double stdev = Table_getStdev (me, icol); therror
	Melder_information1 (Melder_double (stdev));
END

DIRECT (Table_getNumberOfColumns)
	iam_ONLY (Table);
	Melder_information1 (Melder_integer (my numberOfColumns));
END

DIRECT (Table_getNumberOfRows)
	iam_ONLY (Table);
	Melder_information1 (Melder_integer (my rows -> size));
END

FORM (Table_getValue, L"Table: Get value", 0)
	NATURAL (L"Row number", L"1")
	WORD (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (Table);
	long rowNumber = GET_INTEGER (L"Row number");
	Table_checkSpecifiedRowNumberWithinRange (me, rowNumber);
	long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
	Melder_information1 (((TableRow) my rows -> item [rowNumber]) -> cells [icol]. string);
END

DIRECT (Table_help) Melder_help (L"Table"); END

FORM (Table_insertColumn, L"Table: Insert column", 0)
	NATURAL (L"Position", L"1")
	WORD (L"Label", L"newcolumn")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		Table_insertColumn (me, GET_INTEGER (L"Position"), GET_STRING (L"Label")); therror
		praat_dataChanged (me);
	}
END

FORM (Table_insertRow, L"Table: Insert row", 0)
	NATURAL (L"Position", L"1")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		Table_insertRow (me, GET_INTEGER (L"Position")); therror
		praat_dataChanged (me);
	}
END

FORM (Table_list, L"Table: List", 0)
	BOOLEAN (L"Include row numbers", true)
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		Table_list (me, GET_INTEGER (L"Include row numbers"));
	}
END

FORM_READ (Table_readFromTableFile, L"Read Table from table file", 0, true)
	praat_newWithFile (Table_readFromTableFile (file), MelderFile_name (file), file);
END

FORM_READ (Table_readFromCommaSeparatedFile, L"Read Table from comma-separated file", 0, true)
	praat_newWithFile (Table_readFromCharacterSeparatedTextFile (file, ','), MelderFile_name (file), file);
END

FORM_READ (Table_readFromTabSeparatedFile, L"Read Table from tab-separated file", 0, true)
	praat_newWithFile (Table_readFromCharacterSeparatedTextFile (file, '\t'), MelderFile_name (file), file);
END

FORM (Table_removeColumn, L"Table: Remove column", 0)
	WORD (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
		Table_removeColumn (me, icol); therror
		praat_dataChanged (me);
	}
END

FORM (Table_removeRow, L"Table: Remove row", 0)
	NATURAL (L"Row number", L"1")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		Table_removeRow (me, GET_INTEGER (L"Row number")); therror
		praat_dataChanged (me);
	}
END

FORM (Table_reportCorrelation_kendallTau, L"Report correlation (Kendall tau)", 0)
	WORD (L"left Columns", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"right Columns", L"ui/editors/AmplitudeTierEditor.h")
	POSITIVE (L"One-tailed unconfidence", L"0.025")
	OK
DO
	iam_ONLY (Table);
	long column1 = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"left Columns")); therror
	long column2 = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"right Columns")); therror
	double unconfidence = GET_REAL (L"One-tailed unconfidence");
	double correlation, significance, lowerLimit, upperLimit;
	correlation = Table_getCorrelation_kendallTau (me, column1, column2, unconfidence,
		& significance, & lowerLimit, & upperLimit);
	MelderInfo_open ();
	MelderInfo_writeLine5 (L"Correlation between column ", Table_messageColumn (me, column1),
		L" and column ", Table_messageColumn (me, column2), L":");
	MelderInfo_writeLine3 (L"Correlation = ", Melder_double (correlation), L" (Kendall's tau-b)");
	MelderInfo_writeLine3 (L"Significance from zero = ", Melder_double (significance), L" (one-tailed)");
	MelderInfo_writeLine3 (L"Confidence interval (", Melder_double (100 * (1.0 - 2.0 * unconfidence)), L"%):");
	MelderInfo_writeLine5 (L"   Lower limit = ", Melder_double (lowerLimit),
		L" (lowest tau that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_writeLine5 (L"   Upper limit = ", Melder_double (upperLimit),
		L" (highest tau that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_close ();
END

FORM (Table_reportCorrelation_pearsonR, L"Report correlation (Pearson r)", 0)
	WORD (L"left Columns", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"right Columns", L"ui/editors/AmplitudeTierEditor.h")
	POSITIVE (L"One-tailed unconfidence", L"0.025")
	OK
DO
	iam_ONLY (Table);
	long column1 = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"left Columns")); therror
	long column2 = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"right Columns")); therror
	double unconfidence = GET_REAL (L"One-tailed unconfidence");
	double correlation, significance, lowerLimit, upperLimit;
	correlation = Table_getCorrelation_pearsonR (me, column1, column2, unconfidence,
		& significance, & lowerLimit, & upperLimit);
	MelderInfo_open ();
	MelderInfo_writeLine5 (L"Correlation between column ", Table_messageColumn (me, column1),
		L" and column ", Table_messageColumn (me, column2), L":");
	MelderInfo_writeLine3 (L"Correlation = ", Melder_double (correlation), L" (Pearson's r)");
	MelderInfo_writeLine2 (L"Number of degrees of freedom = ", Melder_integer (my rows -> size - 2));
	MelderInfo_writeLine3 (L"Significance from zero = ", Melder_double (significance), L" (one-tailed)");
	MelderInfo_writeLine3 (L"Confidence interval (", Melder_double (100 * (1.0 - 2.0 * unconfidence)), L"%):");
	MelderInfo_writeLine5 (L"   Lower limit = ", Melder_double (lowerLimit),
		L" (lowest r that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_writeLine5 (L"   Upper limit = ", Melder_double (upperLimit),
		L" (highest r that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_close ();
END
	
FORM (Table_reportDifference_studentT, L"Report difference (Student t)", 0)
	WORD (L"left Columns", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"right Columns", L"ui/editors/AmplitudeTierEditor.h")
	POSITIVE (L"One-tailed unconfidence", L"0.025")
	OK
DO
	iam_ONLY (Table);
	long column1 = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"left Columns")); therror
	long column2 = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"right Columns")); therror
	double unconfidence = GET_REAL (L"One-tailed unconfidence");
	double difference, t, numberOfDegreesOfFreedom, significance, lowerLimit, upperLimit;
	difference = Table_getDifference_studentT (me, column1, column2, unconfidence,
		& t, & numberOfDegreesOfFreedom, & significance, & lowerLimit, & upperLimit);
	MelderInfo_open ();
	MelderInfo_writeLine5 (L"Difference between column ", Table_messageColumn (me, column1),
		L" and column ", Table_messageColumn (me, column2), L":");
	MelderInfo_writeLine2 (L"Difference = ", Melder_double (difference));
	MelderInfo_writeLine2 (L"Student's t = ", Melder_double (t));
	MelderInfo_writeLine2 (L"Number of degrees of freedom = ", Melder_double (numberOfDegreesOfFreedom));
	MelderInfo_writeLine3 (L"Significance from zero = ", Melder_double (significance), L" (one-tailed)");
	MelderInfo_writeLine3 (L"Confidence interval (", Melder_double (100 * (1.0 - 2.0 * unconfidence)), L"%):");
	MelderInfo_writeLine5 (L"   Lower limit = ", Melder_double (lowerLimit),
		L" (lowest difference that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_writeLine5 (L"   Upper limit = ", Melder_double (upperLimit),
		L" (highest difference that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_close ();
END
	
FORM (Table_reportGroupDifference_studentT, L"Report group difference (Student t)", 0)
	WORD (L"Column", L"salary")
	WORD (L"Group column", L"gender")
	SENTENCE (L"Group 1", L"F")
	SENTENCE (L"Group 2", L"M")
	POSITIVE (L"One-tailed unconfidence", L"0.025")
	OK
DO
	iam_ONLY (Table);
	long column = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column")); therror
	long groupColumn = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Group column")); therror
	double unconfidence = GET_REAL (L"One-tailed unconfidence");
	wchar_t *group1 = GET_STRING (L"Group 1"), *group2 = GET_STRING (L"Group 2");
	double mean, tFromZero, numberOfDegreesOfFreedom, significanceFromZero, lowerLimit, upperLimit;
	mean = Table_getGroupDifference_studentT (me, column, groupColumn, group1, group2, unconfidence,
		& tFromZero, & numberOfDegreesOfFreedom, & significanceFromZero, & lowerLimit, & upperLimit);
	MelderInfo_open ();
	MelderInfo_write4 (L"Difference in column ", Table_messageColumn (me, column), L" between groups ", group1);
	MelderInfo_writeLine5 (L" and ", group2, L" of column ", Table_messageColumn (me, groupColumn), L":");
	MelderInfo_writeLine2 (L"Difference = ", Melder_double (mean));
	MelderInfo_writeLine2 (L"Student's t = ", Melder_double (tFromZero));
	MelderInfo_writeLine2 (L"Number of degrees of freedom = ", Melder_double (numberOfDegreesOfFreedom));
	MelderInfo_writeLine3 (L"Significance from zero = ", Melder_double (significanceFromZero), L" (one-tailed)");
	MelderInfo_writeLine3 (L"Confidence interval (", Melder_double (100 * (1.0 - 2.0 * unconfidence)), L"%):");
	MelderInfo_writeLine5 (L"   Lower limit = ", Melder_double (lowerLimit),
		L" (lowest difference that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_writeLine5 (L"   Upper limit = ", Melder_double (upperLimit),
		L" (highest difference that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_close ();
END

FORM (Table_reportGroupDifference_wilcoxonRankSum, L"Report group difference (Wilcoxon rank sum)", 0)
	WORD (L"Column", L"salary")
	WORD (L"Group column", L"gender")
	SENTENCE (L"Group 1", L"F")
	SENTENCE (L"Group 2", L"M")
	OK
DO
	iam_ONLY (Table);
	long column = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column")); therror
	long groupColumn = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Group column")); therror
	wchar_t *group1 = GET_STRING (L"Group 1"), *group2 = GET_STRING (L"Group 2");
	double areaUnderCurve, rankSum, significanceFromZero;
	areaUnderCurve = Table_getGroupDifference_wilcoxonRankSum (me, column, groupColumn, group1, group2,
		& rankSum, & significanceFromZero);
	MelderInfo_open ();
	MelderInfo_write4 (L"Difference in column ", Table_messageColumn (me, column), L" between groups ", group1);
	MelderInfo_writeLine5 (L" and ", group2, L" of column ", Table_messageColumn (me, groupColumn), L":");
	MelderInfo_writeLine2 (L"Larger: ", areaUnderCurve < 0.5 ? group1 : areaUnderCurve > 0.5 ? group2 : L"(both equal)");
	MelderInfo_writeLine2 (L"Area under curve: ", Melder_double (areaUnderCurve));
	MelderInfo_writeLine2 (L"Rank sum: ", Melder_double (rankSum));
	MelderInfo_writeLine3 (L"Significance from zero: ", Melder_double (significanceFromZero), L" (one-tailed)");
	MelderInfo_close ();
END

FORM (Table_reportGroupMean_studentT, L"Report group mean (Student t)", 0)
	WORD (L"Column", L"salary")
	WORD (L"Group column", L"gender")
	SENTENCE (L"Group", L"F")
	POSITIVE (L"One-tailed unconfidence", L"0.025")
	OK
DO
	iam_ONLY (Table);
	long column = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column")); therror
	long groupColumn = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Group column")); therror
	double unconfidence = GET_REAL (L"One-tailed unconfidence");
	wchar_t *group = GET_STRING (L"Group");
	double mean, tFromZero, numberOfDegreesOfFreedom, significanceFromZero, lowerLimit, upperLimit;
	mean = Table_getGroupMean_studentT (me, column, groupColumn, group, unconfidence,
		& tFromZero, & numberOfDegreesOfFreedom, & significanceFromZero, & lowerLimit, & upperLimit);
	MelderInfo_open ();
	MelderInfo_write4 (L"Mean in column ", Table_messageColumn (me, column), L" of group ", group);
	MelderInfo_writeLine3 (L" of column ", Table_messageColumn (me, groupColumn), L":");
	MelderInfo_writeLine2 (L"Mean = ", Melder_double (mean));
	MelderInfo_writeLine2 (L"Student's t from zero = ", Melder_double (tFromZero));
	MelderInfo_writeLine2 (L"Number of degrees of freedom = ", Melder_double (numberOfDegreesOfFreedom));
	MelderInfo_writeLine3 (L"Significance from zero = ", Melder_double (significanceFromZero), L" (one-tailed)");
	MelderInfo_writeLine3 (L"Confidence interval (", Melder_double (100 * (1.0 - 2.0 * unconfidence)), L"%):");
	MelderInfo_writeLine5 (L"   Lower limit = ", Melder_double (lowerLimit),
		L" (lowest difference that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_writeLine5 (L"   Upper limit = ", Melder_double (upperLimit),
		L" (highest difference that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_close ();
END

FORM (Table_reportMean_studentT, L"Report mean (Student t)", 0)
	WORD (L"Column", L"ui/editors/AmplitudeTierEditor.h")
	POSITIVE (L"One-tailed unconfidence", L"0.025")
	OK
DO
	iam_ONLY (Table);
	long column = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column")); therror
	double unconfidence = GET_REAL (L"One-tailed unconfidence");
	double mean, tFromZero, numberOfDegreesOfFreedom, significanceFromZero, lowerLimit, upperLimit;
	mean = Table_getMean_studentT (me, column, unconfidence,
		& tFromZero, & numberOfDegreesOfFreedom, & significanceFromZero, & lowerLimit, & upperLimit);
	MelderInfo_open ();
	MelderInfo_writeLine3 (L"Mean of column ", Table_messageColumn (me, column), L":");
	MelderInfo_writeLine2 (L"Mean = ", Melder_double (mean));
	MelderInfo_writeLine2 (L"Student's t from zero = ", Melder_double (tFromZero));
	MelderInfo_writeLine2 (L"Number of degrees of freedom = ", Melder_double (numberOfDegreesOfFreedom));
	MelderInfo_writeLine3 (L"Significance from zero = ", Melder_double (significanceFromZero), L" (one-tailed)");
	MelderInfo_writeLine3 (L"Confidence interval (", Melder_double (100 * (1.0 - 2.0 * unconfidence)), L"%):");
	MelderInfo_writeLine5 (L"   Lower limit = ", Melder_double (lowerLimit),
		L" (lowest value that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_writeLine5 (L"   Upper limit = ", Melder_double (upperLimit),
		L" (highest value that cannot be rejected with " UNITEXT_GREEK_SMALL_LETTER_ALPHA " = ", Melder_double (unconfidence), L")");
	MelderInfo_close ();
END

FORM (Table_rowsToColumns, L"Table: Rows to columns", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Columns with factors (independent variables):")
	TEXTFIELD (L"factors", L"dialect gender speaker")
	WORD (L"Column to transpose", L"vowel")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Columns to expand:")
	TEXTFIELD (L"columnsToExpand", L"duration F0 F1 F2 F3")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Columns not mentioned above will be ignored.")
	OK
DO
	const wchar_t *columnLabel = GET_STRING (L"Column to transpose");
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long icol = Table_getColumnIndexFromColumnLabel (me, columnLabel); therror
		autoTable thee = Table_rowsToColumns (me, GET_STRING (L"factors"), icol, GET_STRING (L"columnsToExpand"));
		praat_new (thee.transfer(), NAME, L"_nested");
	}
END

FORM (Table_scatterPlot, L"Scatter plot", 0)
	WORD (L"Horizontal column", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0 (= auto)")
	WORD (L"Vertical column", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0 (= auto)")
	WORD (L"Column with marks", L"ui/editors/AmplitudeTierEditor.h")
	NATURAL (L"Font size", L"12")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long xcolumn = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Horizontal column")); therror
		long ycolumn = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Vertical column")); therror
		long markColumn = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column with marks")); therror
		Table_scatterPlot (me, GRAPHICS, xcolumn, ycolumn,
			GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
			GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
			markColumn, GET_INTEGER (L"Font size"), GET_INTEGER (L"Garnish"));
	}
END

FORM (Table_scatterPlot_mark, L"Scatter plot (marks)", 0)
	WORD (L"Horizontal column", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"left Horizontal range", L"0.0")
	REAL (L"right Horizontal range", L"0.0 (= auto)")
	WORD (L"Vertical column", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"left Vertical range", L"0.0")
	REAL (L"right Vertical range", L"0.0 (= auto)")
	POSITIVE (L"Mark size (mm)", L"1.0")
	BOOLEAN (L"Garnish", 1)
	SENTENCE (L"Mark string (+xo.)", L"+")
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long xcolumn = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Horizontal column")); therror
		long ycolumn = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Vertical column")); therror
		Table_scatterPlot_mark (me, GRAPHICS, xcolumn, ycolumn,
			GET_REAL (L"left Horizontal range"), GET_REAL (L"right Horizontal range"),
			GET_REAL (L"left Vertical range"), GET_REAL (L"right Vertical range"),
			GET_REAL (L"Mark size"), GET_STRING (L"Mark string"), GET_INTEGER (L"Garnish"));
	}
END

FORM (Table_searchColumn, L"Table: Search column", 0)
	WORD (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	WORD (L"Value", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (Table);
	long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
	Melder_information1 (Melder_integer (Table_searchColumn (me, icol, GET_STRING (L"Value"))));
END
	
FORM (Table_setColumnLabel_index, L"Set column label", 0)
	NATURAL (L"Column number", L"1")
	SENTENCE (L"Label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		Table_setColumnLabel (me, GET_INTEGER (L"Column number"), GET_STRING (L"Label")); therror
		praat_dataChanged (me);
	}
END

FORM (Table_setColumnLabel_label, L"Set column label", 0)
	SENTENCE (L"Old label", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"New label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		Table_setColumnLabel (me, Table_findColumnIndexFromColumnLabel (me, GET_STRING (L"Old label")), GET_STRING (L"New label")); therror
		praat_dataChanged (me);
	}
END

FORM (Table_setNumericValue, L"Table: Set numeric value", 0)
	NATURAL (L"Row number", L"1")
	WORD (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	REAL_OR_UNDEFINED (L"Numeric value", L"1.5")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
		Table_setNumericValue (me, GET_INTEGER (L"Row number"), icol, GET_REAL (L"Numeric value")); therror
		praat_dataChanged (me);
	}
END

FORM (Table_setStringValue, L"Table: Set string value", 0)
	NATURAL (L"Row number", L"1")
	WORD (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"String value", L"xx")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long icol = Table_getColumnIndexFromColumnLabel (me, GET_STRING (L"Column label")); therror
		Table_setStringValue (me, GET_INTEGER (L"Row number"), icol, GET_STRING (L"String value")); therror
		praat_dataChanged (me);
	}
END

DIRECT (Table_randomizeRows)
	WHERE (SELECTED) {
		iam_LOOP (Table);
		Table_randomizeRows (me);
		praat_dataChanged (me);
	}
END

FORM (Table_sortRows, L"Table: Sort rows", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"One or more column labels for sorting:")
	TEXTFIELD (L"columnLabels", L"dialect gender name")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		Table_sortRows_string (me, GET_STRING (L"columnLabels")); therror
		praat_dataChanged (me);
	}
END

DIRECT (Table_to_LinearRegression)
	WHERE (SELECTED) {
		iam_LOOP (Table);
		autoLinearRegression thee = Table_to_LinearRegression (me);
		praat_new (thee.transfer(), NAME);
	}
END

FORM (Table_to_LogisticRegression, L"Table: To LogisticRegression", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Factors (column names):")
	TEXTFIELD (L"factors", L"F0 F1 duration")
	WORD (L"Dependent 1 (column name)", L"e")
	WORD (L"Dependent 2 (column name)", L"i")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		autoLogisticRegression thee = Table_to_LogisticRegression (me, GET_STRING (L"factors"), GET_STRING (L"Dependent 1"), GET_STRING (L"Dependent 2"));
		praat_new (thee.transfer(), NAME);
	}
END

FORM (Table_to_TableOfReal, L"Table: Down to TableOfReal", 0)
	WORD (L"Column for row labels", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Table);
		long icol = Table_findColumnIndexFromColumnLabel (me, GET_STRING (L"Column for row labels")); therror
		autoTableOfReal thee = Table_to_TableOfReal (me, icol);
		praat_new (thee.transfer(), NAME);
	}
END

FORM_WRITE (Table_writeToTableFile, L"Save Table as table file", 0, L"Table")
	iam_ONLY (Table);
	Table_writeToTableFile (me, file); therror
END

/***** TABLEOFREAL *****/

static void NUMrationalize (double x, long *numerator, long *denominator) {
	double epsilon = 1e-6;
	*numerator = 1;
	for (*denominator = 1; *denominator <= 100000; (*denominator) ++) {
		double numerator_d = x * *denominator, rounded = floor (numerator_d + 0.5);
		if (fabs (rounded - numerator_d) < epsilon) {
			*numerator = rounded;
			return;
		}
	}
	*denominator = 0;   /* Failure. */
}

static void print4 (wchar_t *buffer, double value, int iformat, int width, int precision) {
	wchar_t formatString [40];
	if (value == NUMundefined) wcscpy (buffer, L"undefined");
	else if (iformat == 4) {
		long numerator, denominator;
		NUMrationalize (value, & numerator, & denominator);
		if (numerator == 0)
			swprintf (buffer, 40, L"0");
		else if (denominator > 1)
			swprintf (buffer, 40, L"%ld/%ld", numerator, denominator);
		else
			swprintf (buffer, 40, L"%.7g", value);
	} else {
		swprintf (formatString, 40, L"%%%d.%d%c", width, precision, iformat == 1 ? 'f' : iformat == 2 ? 'e' : 'g');
		swprintf (buffer, 40, formatString, value);
	}
}

static void fixRows (TableOfReal me, long *rowmin, long *rowmax) {
	if (*rowmax < *rowmin) { *rowmin = 1; *rowmax = my numberOfRows; }
	else if (*rowmin < 1) *rowmin = 1;
	else if (*rowmax > my numberOfRows) *rowmax = my numberOfRows;
}
static void fixColumns (TableOfReal me, long *colmin, long *colmax) {
	if (*colmax < *colmin) { *colmin = 1; *colmax = my numberOfColumns; }
	else if (*colmin < 1) *colmin = 1;
	else if (*colmax > my numberOfColumns) *colmax = my numberOfColumns;
}

static double getMaxRowLabelWidth (TableOfReal me, Graphics graphics, long rowmin, long rowmax) {
	double maxWidth = 0.0;
	if (! my rowLabels) return 0.0;
	fixRows (me, & rowmin, & rowmax);
	for (long irow = rowmin; irow <= rowmax; irow ++) if (my rowLabels [irow] && my rowLabels [irow] [0]) {
		double textWidth = Graphics_textWidth_ps (graphics, my rowLabels [irow], TRUE);   /* SILIPA is bigger than XIPA */
		if (textWidth > maxWidth) maxWidth = textWidth;
	}
	return maxWidth;
}

static double getLeftMargin (Graphics graphics) {
	return Graphics_dxMMtoWC (graphics, 1);
}

static double getLineSpacing (Graphics graphics) {
	return Graphics_dyMMtoWC (graphics, 1.5 * Graphics_inqFontSize (graphics) * 25.4 / 72);
}

static double getMaxColumnLabelHeight (TableOfReal me, Graphics graphics, long colmin, long colmax) {
	double maxHeight = 0.0, lineSpacing = getLineSpacing (graphics);
	if (! my columnLabels) return 0.0;
	fixRows (me, & colmin, & colmax);
	for (long icol = colmin; icol <= colmax; icol ++) if (my columnLabels [icol] && my columnLabels [icol] [0]) {
		if (! maxHeight) maxHeight = lineSpacing;
	}
	return maxHeight;
}

void TableOfReal_drawAsNumbers (I, Graphics graphics, long rowmin, long rowmax, int iformat, int precision) {
	iam (TableOfReal);
	fixRows (me, & rowmin, & rowmax);
	Graphics_setInner (graphics);
	Graphics_setWindow (graphics, 0.5, my numberOfColumns + 0.5, 0, 1);
	double leftMargin = getLeftMargin (graphics);   // not earlier!
	double lineSpacing = getLineSpacing (graphics);   // not earlier!
	double maxTextWidth = getMaxRowLabelWidth (me, graphics, rowmin, rowmax);
	double maxTextHeight = getMaxColumnLabelHeight (me, graphics, 1, my numberOfColumns);

	Graphics_setTextAlignment (graphics, Graphics_CENTRE, Graphics_BOTTOM);
	for (long icol = 1; icol <= my numberOfColumns; icol ++) {
		if (my columnLabels && my columnLabels [icol] && my columnLabels [icol] [0])
			Graphics_text (graphics, icol, 1, my columnLabels [icol]);
	}
	for (long irow = rowmin; irow <= rowmax; irow ++) {
		double y = 1 - lineSpacing * (irow - rowmin + 0.6);
		Graphics_setTextAlignment (graphics, Graphics_RIGHT, Graphics_HALF);
		if (my rowLabels && my rowLabels [irow] && my rowLabels [irow] [0])
			Graphics_text (graphics, 0.5 - leftMargin, y, my rowLabels [irow]);
		Graphics_setTextAlignment (graphics, Graphics_CENTRE, Graphics_HALF);
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			wchar_t text [40];
			print4 (text, my data [irow] [icol], iformat, 0, precision);
			Graphics_text (graphics, icol, y, text);
		}
	}
	if (maxTextHeight) {
		double left = 0.5;
		if (maxTextWidth > 0.0) left -= maxTextWidth + 2 * leftMargin;
		Graphics_line (graphics, left, 1, my numberOfColumns + 0.5, 1);
	}
	Graphics_unsetInner (graphics);
}

void TableOfReal_drawAsNumbers_if (I, Graphics graphics, long rowmin, long rowmax, int iformat, int precision,
	const wchar_t *conditionFormula, Interpreter *interpreter)
{
	iam (TableOfReal);
	try {
		autoMatrix original = TableOfReal_to_Matrix (me);
		autoMatrix conditions = original.clone ();
		fixRows (me, & rowmin, & rowmax);
		Graphics_setInner (graphics);
		Graphics_setWindow (graphics, 0.5, my numberOfColumns + 0.5, 0, 1);
		double leftMargin = getLeftMargin (graphics);   // not earlier!
		double lineSpacing = getLineSpacing (graphics);   // not earlier!
		double maxTextWidth = getMaxRowLabelWidth (me, graphics, rowmin, rowmax);
		double maxTextHeight = getMaxColumnLabelHeight (me, graphics, 1, my numberOfColumns);
		Matrix_formula (original.peek(), conditionFormula, interpreter, conditions.peek()); therror

		Graphics_setTextAlignment (graphics, Graphics_CENTRE, Graphics_BOTTOM);
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			if (my columnLabels && my columnLabels [icol] && my columnLabels [icol] [0])
				Graphics_text (graphics, icol, 1, my columnLabels [icol]);
		}
		for (long irow = rowmin; irow <= rowmax; irow ++) {
			double y = 1 - lineSpacing * (irow - rowmin + 0.6);
			Graphics_setTextAlignment (graphics, Graphics_RIGHT, Graphics_HALF);
			if (my rowLabels && my rowLabels [irow] && my rowLabels [irow] [0])
				Graphics_text (graphics, 0.5 - leftMargin, y, my rowLabels [irow]);
			Graphics_setTextAlignment (graphics, Graphics_CENTRE, Graphics_HALF);
			for (long icol = 1; icol <= my numberOfColumns; icol ++) if (conditions -> z [irow] [icol] != 0.0) {
				wchar_t text [40];
				print4 (text, my data [irow] [icol], iformat, 0, precision);
				Graphics_text (graphics, icol, y, text);
			}
		}
		if (maxTextHeight) {
			double left = 0.5;
			if (maxTextWidth > 0.0) left -= maxTextWidth + 2 * leftMargin;
			Graphics_line (graphics, left, 1, my numberOfColumns + 0.5, 1);
		}
		Graphics_unsetInner (graphics);
	} catch (...) {
		rethrowm (me, ": numbers not drawn.");
	}
}

void TableOfReal_drawVerticalLines (I, Graphics graphics, long rowmin, long rowmax) {
	iam (TableOfReal);
	long colmin = 1, colmax = my numberOfColumns;
	fixRows (me, & rowmin, & rowmax);
	Graphics_setInner (graphics);
	Graphics_setWindow (graphics, colmin - 0.5, colmax + 0.5, 0, 1);
	double lineSpacing = getLineSpacing (graphics);   // not earlier!
	double maxTextWidth = getMaxRowLabelWidth (me, graphics, rowmin, rowmax);
	double maxTextHeight = getMaxColumnLabelHeight (me, graphics, 1, my numberOfColumns);

	if (maxTextWidth > 0.0) colmin -= 1;
	for (long col = colmin + 1; col <= colmax; col ++)
		Graphics_line (graphics, col - 0.5, 1 + maxTextHeight, col - 0.5, 1 - lineSpacing * (rowmax - rowmin + 1));
	Graphics_unsetInner (graphics);
}

void TableOfReal_drawLeftAndRightLines (I, Graphics graphics, long rowmin, long rowmax) {
	iam (TableOfReal);
	long colmin = 1, colmax = my numberOfColumns;
	fixRows (me, & rowmin, & rowmax);
	Graphics_setInner (graphics);
	Graphics_setWindow (graphics, colmin - 0.5, colmax + 0.5, 0, 1);
	double lineSpacing = getLineSpacing (graphics);
	double maxTextWidth = getMaxRowLabelWidth (me, graphics, rowmin, rowmax);
	double maxTextHeight = getMaxColumnLabelHeight (me, graphics, 1, my numberOfColumns);

	double left = 0.5;
	if (maxTextWidth > 0.0) left -= maxTextWidth + 2 * lineSpacing;
	double right = colmax + 0.5;
	double top = 1 + maxTextHeight;
	double bottom = 1 - lineSpacing * (rowmax - rowmin + 1);
	Graphics_line (graphics, left, top, left, bottom);
	Graphics_line (graphics, right, top, right, bottom);
	Graphics_unsetInner (graphics);
}

void TableOfReal_drawHorizontalLines (I, Graphics graphics, long rowmin, long rowmax) {
	iam (TableOfReal);
	long colmin = 1, colmax = my numberOfColumns;
	fixRows (me, & rowmin, & rowmax);
	Graphics_setInner (graphics);
	Graphics_setWindow (graphics, colmin - 0.5, colmax + 0.5, 0, 1);
	double lineSpacing = getLineSpacing (graphics);
	double maxTextWidth = getMaxRowLabelWidth (me, graphics, rowmin, rowmax);
	double maxTextHeight = getMaxColumnLabelHeight (me, graphics, 1, my numberOfColumns);

	double left = 0.5;
	double top = rowmin;
	if (maxTextWidth > 0.0) left -= maxTextWidth + 2 * lineSpacing;
	if (maxTextHeight > 0.0) rowmin -= 1;
	double right = colmax + 0.5;
	for (long irow = rowmin; irow < rowmax; irow ++) {
		double y = 1 - lineSpacing * (irow - top + 1);
		Graphics_line (graphics, left, y, right, y);
	}
	Graphics_unsetInner (graphics);
}

void TableOfReal_drawTopAndBottomLines (I, Graphics graphics, long rowmin, long rowmax) {
	iam (TableOfReal);
	long colmin = 1, colmax = my numberOfColumns;
	fixRows (me, & rowmin, & rowmax);
	Graphics_setInner (graphics);
	Graphics_setWindow (graphics, colmin - 0.5, colmax + 0.5, 0, 1);
	double lineSpacing = getLineSpacing (graphics);
	double maxTextWidth = getMaxRowLabelWidth (me, graphics, rowmin, rowmax);
	double maxTextHeight = getMaxColumnLabelHeight (me, graphics, 1, my numberOfColumns);

	double left = 0.5;
	if (maxTextWidth > 0.0) left -= maxTextWidth + 2 * lineSpacing;
	double right = colmax + 0.5;
	double top = 1 + maxTextHeight;
	double bottom = 1 - lineSpacing * (rowmax - rowmin + 1);
	Graphics_line (graphics, left, top, right, top);
	Graphics_line (graphics, left, bottom, right, bottom);
	Graphics_unsetInner (graphics);
}

void TableOfReal_drawAsSquares (I, Graphics graphics, long rowmin, long rowmax,
	long colmin, long colmax, int garnish)
{
	iam (TableOfReal);
	double dx = 1, dy = 1;
	Graphics_Colour colour = Graphics_inqColour (graphics);
	fixRows (me, & rowmin, & rowmax);
	fixColumns (me, & colmin, & colmax);
	
	Graphics_setInner (graphics);
	Graphics_setWindow (graphics, colmin - 0.5, colmax + 0.5, rowmin - 0.5, rowmax + 0.5);
	double datamax = my data [rowmin] [colmin];
	for (long irow = 1; irow <= my numberOfRows; irow ++)
		for (long icol = 1; icol <= my numberOfColumns; icol ++)
			if (fabs (my data [irow] [icol]) > datamax) datamax = fabs (my data [irow] [icol]);
	
	for (long irow = rowmin; irow <= rowmax; irow ++) {
		double y = rowmax + rowmin - irow;
		for (long icol = colmin; icol <= colmax; icol ++) {
			double x = icol;
			/* two neighbouring squares should not touch -> 0.95 */
			double d = 0.95 * sqrt (fabs (my data [irow] [icol]) / datamax);
			double x1WC = x - d * dx / 2, x2WC = x + d * dx / 2;
			double y1WC = y - d * dy / 2, y2WC = y + d * dy / 2;
			if (my data [irow] [icol] > 0) Graphics_setColour (graphics, Graphics_WHITE);
			Graphics_fillRectangle (graphics, x1WC, x2WC, y1WC, y2WC);
			Graphics_setColour (graphics, colour);
			Graphics_rectangle (graphics, x1WC, x2WC , y1WC, y2WC);
		}
	}
	Graphics_setGrey (graphics, 0.0);
	Graphics_unsetInner (graphics);
	if (garnish) {
		for (long irow = rowmin; irow <= rowmax; irow ++) if (my rowLabels [irow]) 
			Graphics_markLeft (graphics, rowmax + rowmin - irow, 0, 0, 0, my rowLabels [irow]);
		for (long icol = colmin; icol <= colmax; icol ++) if (my columnLabels [icol])
			Graphics_markTop (graphics, icol, 0, 0, 0, my columnLabels [icol]);
	}
}

DIRECT (TablesOfReal_append)
	autoCollection tables = Collection_create (classTableOfReal, 10);
	Collection_dontOwnItems (tables.peek());
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		Collection_addItem (tables.peek(), me);
	}
	autoTableOfReal thee = static_cast <TableOfReal> (TablesOfReal_appendMany (tables.peek()));
	praat_new (thee.transfer(), L"appended");
END

FORM (TableOfReal_create, L"Create TableOfReal", 0)
	WORD (L"Name", L"table")
	NATURAL (L"Number of rows", L"10")
	NATURAL (L"Number of columns", L"3")
	OK
DO
	autoTableOfReal me = TableOfReal_create (GET_INTEGER (L"Number of rows"), GET_INTEGER (L"Number of columns"));
	praat_new (me.transfer(), GET_STRING (L"Name"));
END

FORM (TableOfReal_drawAsNumbers, L"Draw as numbers", 0)
	NATURAL (L"From row", L"1")
	INTEGER (L"To row", L"0 (= all)")
	RADIO (L"Format", 3)
		RADIOBUTTON (L"decimal")
		RADIOBUTTON (L"exponential")
		RADIOBUTTON (L"free")
		RADIOBUTTON (L"rational")
	NATURAL (L"Precision", L"5")
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_drawAsNumbers (me, GRAPHICS,
			GET_INTEGER (L"From row"), GET_INTEGER (L"To row"),
			GET_INTEGER (L"Format"), GET_INTEGER (L"Precision"));
	}
END

FORM (TableOfReal_drawAsNumbers_if, L"Draw as numbers if...", 0)
	NATURAL (L"From row", L"1")
	INTEGER (L"To row", L"0 (= all)")
	RADIO (L"Format", 3)
		RADIOBUTTON (L"decimal")
		RADIOBUTTON (L"exponential")
		RADIOBUTTON (L"free")
		RADIOBUTTON (L"rational")
	NATURAL (L"Precision", L"5")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Condition:")
	TEXTFIELD (L"condition", L"self <> 0")
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_drawAsNumbers_if (me, GRAPHICS,
			GET_INTEGER (L"From row"), GET_INTEGER (L"To row"),
			GET_INTEGER (L"Format"), GET_INTEGER (L"Precision"), GET_STRING (L"condition"), interpreter);
	}
END

FORM (TableOfReal_drawAsSquares, L"Draw table as squares", 0)
	INTEGER (L"From row", L"1")
	INTEGER (L"To row", L"0")
	INTEGER (L"From column", L"1")
	INTEGER (L"To column", L"0")
	BOOLEAN (L"Garnish", 1)
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_drawAsSquares (me, GRAPHICS, 
			GET_INTEGER (L"From row"), GET_INTEGER (L"To row"),
			GET_INTEGER (L"From column"), GET_INTEGER (L"To column"),
			GET_INTEGER (L"Garnish"));
		}
END

FORM (TableOfReal_drawHorizontalLines, L"Draw horizontal lines", 0)
	NATURAL (L"From row", L"1")
	INTEGER (L"To row", L"0 (= all)")
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_drawHorizontalLines (me, GRAPHICS, GET_INTEGER (L"From row"), GET_INTEGER (L"To row"));
	}
END

FORM (TableOfReal_drawLeftAndRightLines, L"Draw left and right lines", 0)
	NATURAL (L"From row", L"1")
	INTEGER (L"To row", L"0 (= all)")
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_drawLeftAndRightLines (me, GRAPHICS, GET_INTEGER (L"From row"), GET_INTEGER (L"To row"));
	}
END

FORM (TableOfReal_drawTopAndBottomLines, L"Draw top and bottom lines", 0)
	NATURAL (L"From row", L"1")
	INTEGER (L"To row", L"0 (= all)")
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_drawTopAndBottomLines (me, GRAPHICS, GET_INTEGER (L"From row"), GET_INTEGER (L"To row"));
	}
END

FORM (TableOfReal_drawVerticalLines, L"Draw vertical lines", 0)
	NATURAL (L"From row", L"1")
	INTEGER (L"To row", L"0 (= all)")
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_drawVerticalLines (me, GRAPHICS, GET_INTEGER (L"From row"), GET_INTEGER (L"To row"));
	}
END

DIRECT (TableOfReal_extractColumnLabelsAsStrings)
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoStrings thee = TableOfReal_extractColumnLabelsAsStrings (me);
		praat_new (thee.transfer(), my name);
	}
END

FORM (TableOfReal_extractColumnRanges, L"Extract column ranges", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Create a new TableOfReal from the following columns:")
	TEXTFIELD (L"ranges", L"1 2")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"To supply rising or falling ranges, use e.g. 2:6 or 5:3.")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoTableOfReal thee = TableOfReal_extractColumnRanges (me, GET_STRING (L"ranges"));
		praat_new (thee.transfer(), my name, L"_cols");
	}
END

TableOfReal TableOfReal_extractColumnsWhere (I, const wchar_t *condition, Interpreter *interpreter) {
	iam (TableOfReal);
	try {
		Formula_compile (interpreter, me, condition, kFormula_EXPRESSION_TYPE_NUMERIC, TRUE); therror
		/*
		 * Count the new number of columns.
		 */
		long numberOfElements = 0;
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			for (long irow = 1; irow <= my numberOfRows; irow ++) {
				struct Formula_Result result;
				Formula_run (irow, icol, & result); therror
				if (result. result.numericResult != 0.0) {
					numberOfElements ++;
					break;
				}
			}
		}
		if (numberOfElements < 1) Melder_throw ("No columns match this condition.");

		/*
		 * Create room for the result.
		 */	
		autoTableOfReal thee = TableOfReal_create (my numberOfRows, numberOfElements);
		TableOfReal_copyRowLabels (me, thee.peek()); therror
		/*
		 * Store the result.
		 */
		numberOfElements = 0;
		for (long icol = 1; icol <= my numberOfColumns; icol ++) {
			for (long irow = 1; irow <= my numberOfRows; irow ++) {
				struct Formula_Result result;
				Formula_run (irow, icol, & result); therror
				if (result. result.numericResult != 0.0) {
					TableOfReal_copyColumn (me, icol, thee.peek(), ++ numberOfElements); therror
					break;
				}
			}
		}
		return thee.transfer();
	} catch (...) {
		rethrowmzero (me, ": columns not extracted.");
	}
}

FORM (TableOfReal_extractColumnsWhere, L"Extract columns where", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Extract all columns with at least one cell where:")
	TEXTFIELD (L"condition", L"col mod 3 = 0 ; this example extracts every third column")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoTableOfReal thee = TableOfReal_extractColumnsWhere (me, GET_STRING (L"condition"), interpreter);
		praat_new (thee.transfer(), my name, L"_cols");
	}
END

FORM (TableOfReal_extractColumnsWhereLabel, L"Extract column where label", 0)
	OPTIONMENU_ENUM (L"Extract all columns whose label...", kMelder_string, DEFAULT)
	SENTENCE (L"...the text", L"a")
	OK
DO
	const wchar_t *text = GET_STRING (L"...the text");
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoTableOfReal thee = TableOfReal_extractColumnsWhereLabel (me, GET_ENUM (kMelder_string, L"Extract all columns whose label..."), text);
		praat_new (thee.transfer(), my name, L"_", text);
	}
END

FORM (TableOfReal_extractColumnsWhereRow, L"Extract columns where row", 0)
	NATURAL (L"Extract all columns where row...", L"1")
	OPTIONMENU_ENUM (L"...is...", kMelder_number, DEFAULT)
	REAL (L"...the value", L"0.0")
	OK
DO
	long row = GET_INTEGER (L"Extract all columns where row...");
	double value = GET_REAL (L"...the value");
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoTableOfReal thee = TableOfReal_extractColumnsWhereRow (me, row, GET_ENUM (kMelder_number, L"...is..."), value);
		praat_new (thee.transfer(), my name, L"_", Melder_integer (row), L"_", Melder_integer (round (value)));
	}
END

DIRECT (TableOfReal_extractRowLabelsAsStrings)
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoStrings thee = TableOfReal_extractRowLabelsAsStrings (me);
		praat_new (thee.transfer(), my name);
	}
END

FORM (TableOfReal_extractRowRanges, L"Extract row ranges", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Create a new TableOfReal from the following rows:")
	TEXTFIELD (L"ranges", L"1 2")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"To supply rising or falling ranges, use e.g. 2:6 or 5:3.")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoTableOfReal thee = TableOfReal_extractRowRanges (me, GET_STRING (L"ranges"));
		praat_new (thee.transfer(), my name, L"_rows");
	}
END

TableOfReal TableOfReal_extractRowsWhere (I, const wchar_t *condition, Interpreter *interpreter) {
	iam (TableOfReal);
	try {
		Formula_compile (interpreter, me, condition, kFormula_EXPRESSION_TYPE_NUMERIC, TRUE); therror
		/*
		 * Count the new number of rows.
		 */
		long numberOfElements = 0;
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			for (long icol = 1; icol <= my numberOfColumns; icol ++) {
				struct Formula_Result result;
				Formula_run (irow, icol, & result); therror
				if (result. result.numericResult != 0.0) {
					numberOfElements ++;
					break;
				}
			}
		}
		if (numberOfElements < 1) Melder_throw ("No rows match this condition.");

		/*
		 * Create room for the result.
		 */	
		autoTableOfReal thee = TableOfReal_create (numberOfElements, my numberOfColumns);
		TableOfReal_copyColumnLabels (me, thee.peek()); therror
		/*
		 * Store the result.
		 */
		numberOfElements = 0;
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			for (long icol = 1; icol <= my numberOfColumns; icol ++) {
				struct Formula_Result result;
				Formula_run (irow, icol, & result);
				if (result. result.numericResult != 0.0) {
					TableOfReal_copyRow (me, irow, thee.peek(), ++ numberOfElements); therror
					break;
				}
			}
		}
		return thee.transfer();
	} catch (...) {
		rethrowmzero (me, ": rows not extracted.");
	}
}

FORM (TableOfReal_extractRowsWhere, L"Extract rows where", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Extract all rows with at least one cell where:")
	TEXTFIELD (L"condition", L"row mod 3 = 0 ; this example extracts every third row")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoTableOfReal thee = TableOfReal_extractRowsWhere (me, GET_STRING (L"condition"), interpreter);
		praat_new (thee.transfer(), my name, L"_rows");
	}
END

FORM (TableOfReal_extractRowsWhereColumn, L"Extract rows where column", 0)
	NATURAL (L"Extract all rows where column...", L"1")
	OPTIONMENU_ENUM (L"...is...", kMelder_number, DEFAULT)
	REAL (L"...the value", L"0.0")
	OK
DO
	long column = GET_INTEGER (L"Extract all rows where column...");
	double value = GET_REAL (L"...the value");
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoTableOfReal thee = TableOfReal_extractRowsWhereColumn (me,
			column, GET_ENUM (kMelder_number, L"...is..."), value);
		praat_new (thee.transfer(), my name, L"_", Melder_integer (column), L"_", Melder_integer (round (value)));
	}
END

FORM (TableOfReal_extractRowsWhereLabel, L"Extract rows where label", 0)
	OPTIONMENU_ENUM (L"Extract all rows whose label...", kMelder_string, DEFAULT)
	SENTENCE (L"...the text", L"a")
	OK
DO
	const wchar_t *text = GET_STRING (L"...the text");
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoTableOfReal thee = TableOfReal_extractRowsWhereLabel (me, GET_ENUM (kMelder_string, L"Extract all rows whose label..."), text);
		praat_new (thee.transfer(), my name, L"_", text);
	}
END

int TableOfReal_formula (I, const wchar_t *expression, Interpreter *interpreter, thou) {
	iam (TableOfReal);
	thouart (TableOfReal);
	try {
		Formula_compile (interpreter, me, expression, kFormula_EXPRESSION_TYPE_NUMERIC, TRUE); therror
		if (thee == NULL) thee = me;
		for (long irow = 1; irow <= my numberOfRows; irow ++) {
			for (long icol = 1; icol <= my numberOfColumns; icol ++) {
				struct Formula_Result result;
				Formula_run (irow, icol, & result); therror
				thy data [irow] [icol] = result. result.numericResult;
			}
		}
		return 1;
	} catch (...) {
		rethrowmzero (me, ": formula not completed.");
	}
}

FORM (TableOfReal_formula, L"TableOfReal: Formula", L"Formula...")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"for row from 1 to nrow do for col from 1 to ncol do self [row, col] = ...")
	TEXTFIELD (L"formula", L"if col = 5 then self + self [6] else self fi")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		try {
			TableOfReal_formula (me, GET_STRING (L"formula"), interpreter, NULL); therror
			praat_dataChanged (me);
		} catch (...) {
			praat_dataChanged (me);
			throw 1;
		}
	}
END

FORM (TableOfReal_getColumnIndex, L"Get column index", 0)
	SENTENCE (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (TableOfReal);
	long columnNumber = TableOfReal_columnLabelToIndex (me, GET_STRING (L"Column label")); therror
	Melder_information1 (Melder_integer (columnNumber));
END
	
FORM (TableOfReal_getColumnLabel, L"Get column label", 0)
	NATURAL (L"Column number", L"1")
	OK
DO
	iam_ONLY (TableOfReal);
	long columnNumber = GET_INTEGER (L"Column number");
	if (columnNumber > my numberOfColumns) Melder_throw (me, ": column number must not be greater than number of columns.");
	Melder_information1 (my columnLabels == NULL ? L"ui/editors/AmplitudeTierEditor.h" : my columnLabels [columnNumber]);
END
	
FORM (TableOfReal_getColumnMean_index, L"Get column mean", 0)
	NATURAL (L"Column number", L"1")
	OK
DO
	iam_ONLY (TableOfReal);
	long columnNumber = GET_INTEGER (L"Column number");
	if (columnNumber > my numberOfColumns) Melder_throw (me, ": column number must not be greater than number of columns.");
	double columnMean = TableOfReal_getColumnMean (me, columnNumber); therror
	Melder_informationReal (columnMean, NULL);
END
	
FORM (TableOfReal_getColumnMean_label, L"Get column mean", 0)
	SENTENCE (L"Column label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (TableOfReal);
	long columnNumber = TableOfReal_columnLabelToIndex (me, GET_STRING (L"Column label"));
	if (columnNumber == 0) Melder_throw (me, ": column label does not exist.");
	double columnMean = TableOfReal_getColumnMean (me, columnNumber); therror
	Melder_informationReal (columnMean, NULL);
END
	
FORM (TableOfReal_getColumnStdev_index, L"Get column standard deviation", 0)
	NATURAL (L"Column number", L"1")
	OK
DO
	iam_ONLY (TableOfReal);
	long columnNumber = GET_INTEGER (L"Column number");
	if (columnNumber > my numberOfColumns) Melder_throw (me, ": column number must not be greater than number of columns.");
	double stdev = TableOfReal_getColumnStdev (me, columnNumber); therror
	Melder_informationReal (stdev, NULL);
END
	
FORM (TableOfReal_getColumnStdev_label, L"Get column standard deviation", 0)
	SENTENCE (L"Column label", L"1")
	OK
DO
	iam_ONLY (TableOfReal);
	long columnNumber = TableOfReal_columnLabelToIndex (me, GET_STRING (L"Column label"));
	if (columnNumber == 0) Melder_throw (me, ": column label does not exist.");
	double stdev = TableOfReal_getColumnStdev (me, columnNumber); therror
	Melder_informationReal (stdev, NULL);
END

DIRECT (TableOfReal_getNumberOfColumns)
	iam_ONLY (TableOfReal);
	Melder_information1 (Melder_integer (my numberOfColumns));
END

DIRECT (TableOfReal_getNumberOfRows)
	iam_ONLY (TableOfReal);
	Melder_information1 (Melder_integer (my numberOfRows));
END

FORM (TableOfReal_getRowIndex, L"Get row index", 0)
	SENTENCE (L"Row label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (TableOfReal);
	long rowNumber = TableOfReal_rowLabelToIndex (me, GET_STRING (L"Row label")); therror
	Melder_information1 (Melder_integer (rowNumber));
END
	
FORM (TableOfReal_getRowLabel, L"Get row label", 0)
	NATURAL (L"Row number", L"1")
	OK
DO
	iam_ONLY (TableOfReal);
	long rowNumber = GET_INTEGER (L"Row number");
	if (rowNumber > my numberOfRows) Melder_throw (me, ": row number must not be greater than number of rows.");
	Melder_information1 (my rowLabels == NULL ? L"ui/editors/AmplitudeTierEditor.h" : my rowLabels [rowNumber]);
END

FORM (TableOfReal_getValue, L"Get value", 0)
	NATURAL (L"Row number", L"1")
	NATURAL (L"Column number", L"1")
	OK
DO
	iam_ONLY (TableOfReal);
	long rowNumber = GET_INTEGER (L"Row number"), columnNumber = GET_INTEGER (L"Column number");
	if (rowNumber > my numberOfRows) Melder_throw (me, ": row number must not exceed number of rows.");
	if (columnNumber > my numberOfColumns) Melder_throw (me, ": column number must not exceed number of columns.");
	Melder_informationReal (my data [rowNumber] [columnNumber], NULL);
END

DIRECT (TableOfReal_help) Melder_help (L"TableOfReal"); END

FORM (TableOfReal_insertColumn, L"Insert column", 0)
	NATURAL (L"Column number", L"1")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_insertColumn (me, GET_INTEGER (L"Column number")); therror
		praat_dataChanged (me);
	}
END

FORM (TableOfReal_insertRow, L"Insert row", 0)
	NATURAL (L"Row number", L"1")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_insertRow (me, GET_INTEGER (L"Row number")); therror
		praat_dataChanged (me);
	}
END

FORM_READ (TableOfReal_readFromHeaderlessSpreadsheetFile, L"Read TableOfReal from headerless spreadsheet file", 0, true)
	praat_newWithFile (TableOfReal_readFromHeaderlessSpreadsheetFile (file), MelderFile_name (file), file);
END

FORM (TableOfReal_removeColumn, L"Remove column", 0)
	NATURAL (L"Column number", L"1")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_removeColumn (me, GET_INTEGER (L"Column number"));
		praat_dataChanged (me);
	}
END

FORM (TableOfReal_removeRow, L"Remove row", 0)
	NATURAL (L"Row number", L"1")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_removeRow (me, GET_INTEGER (L"Row number"));
		praat_dataChanged (me);
	}
END

FORM (TableOfReal_setColumnLabel_index, L"Set column label", 0)
	NATURAL (L"Column number", L"1")
	SENTENCE (L"Label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_setColumnLabel (me, GET_INTEGER (L"Column number"), GET_STRING (L"Label")); therror
		praat_dataChanged (me);
	}
END

FORM (TableOfReal_setColumnLabel_label, L"Set column label", 0)
	SENTENCE (L"Old label", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"New label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		long columnNumber = TableOfReal_columnLabelToIndex (me, GET_STRING (L"Old label")); therror
		TableOfReal_setColumnLabel (me, columnNumber, GET_STRING (L"New label")); therror
		praat_dataChanged (me);
	}
END

FORM (TableOfReal_setRowLabel_index, L"Set row label", 0)
	NATURAL (L"Row number", L"1")
	SENTENCE (L"Label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_setRowLabel (me, GET_INTEGER (L"Row number"), GET_STRING (L"Label")); therror
		praat_dataChanged (me);
	}
END

FORM (TableOfReal_setValue, L"Set value", L"TableOfReal: Set value...")
	NATURAL (L"Row number", L"1")
	NATURAL (L"Column number", L"1")
	REAL_OR_UNDEFINED (L"New value", L"0.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		long rowNumber = GET_INTEGER (L"Row number"), columnNumber = GET_INTEGER (L"Column number");
		if (rowNumber > my numberOfRows) Melder_throw (me, ": row number too large.");
		if (columnNumber > my numberOfColumns) Melder_throw (me, ": column number too large.");
		my data [rowNumber] [columnNumber] = GET_REAL (L"New value");
		praat_dataChanged (me);
	}
END

FORM (TableOfReal_setRowLabel_label, L"Set row label", 0)
	SENTENCE (L"Old label", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"New label", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		long rowNumber = TableOfReal_rowLabelToIndex (me, GET_STRING (L"Old label")); therror
		TableOfReal_setRowLabel (me, rowNumber, GET_STRING (L"New label"));
		praat_dataChanged (me);
	}
END

FORM (TableOfReal_sortByColumn, L"Sort rows by column", 0)
	INTEGER (L"Column", L"1")
	INTEGER (L"Secondary column", L"0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_sortByColumn (me, GET_INTEGER (L"Column"), GET_INTEGER (L"Secondary column"));
		praat_dataChanged (me);
	}
END

FORM (TableOfReal_sortByLabel, L"Sort rows by label", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Secondary sorting keys:")
	INTEGER (L"Column1", L"1")
	INTEGER (L"Column2", L"0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		TableOfReal_sortByLabel (me, GET_INTEGER (L"Column1"), GET_INTEGER (L"Column2"));
		praat_dataChanged (me);
	}
END

DIRECT (TableOfReal_to_Matrix)
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoMatrix thee = TableOfReal_to_Matrix (me);
		praat_new (thee.transfer(), my name);
	}
END

FORM (TableOfReal_to_Table, L"TableOfReal: To Table", 0)
	SENTENCE (L"Label of first column", L"rowLabel")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (TableOfReal);
		autoTable thee = TableOfReal_to_Table (me, GET_STRING (L"Label of first column"));
		praat_new (thee.transfer(), my name);
	}
END

FORM_WRITE (TableOfReal_writeToHeaderlessSpreadsheetFile, L"Save TableOfReal as spreadsheet", 0, L"txt")
	iam_ONLY (TableOfReal);
	TableOfReal_writeToHeaderlessSpreadsheetFile (me, file);
END


DIRECT (StatisticsTutorial) Melder_help (L"Statistics"); END

static bool isTabSeparated_8bit (int nread, const char *header) {
	for (long i = 0; i < nread; i ++) {
		if (header [i] == '\t') return true;
		if (header [i] == '\n' || header [i] == '\r') return false;
	}
	return false;
}

static bool isTabSeparated_utf16be (int nread, const char *header) {
	for (long i = 2; i < nread; i += 2) {
		if (header [i] == '\0' && header [i + 1] == '\t') return true;
		if (header [i] == '\0' && (header [i + 1] == '\n' || header [i + 1] == '\r')) return false;
	}
	return false;
}

static bool isTabSeparated_utf16le (int nread, const char *header) {
	for (long i = 2; i < nread; i += 2) {
		if (header [i + 1] == '\0' && header [i] == '\t') return true;
		if (header [i + 1] == '\0' && (header [i] == '\n' || header [i] == '\r')) return false;
	}
	return false;
}

static Any tabSeparatedFileRecognizer (int nread, const char *header, MelderFile file) {
	/*
	 * A table is recognized if it has at least one tab symbol,
	 * which must be before the first newline symbol (if any).
	 */
	unsigned char *uheader = (unsigned char *) header;
	bool isTabSeparated =
		uheader [0] == 0xef && uheader [1] == 0xff ? isTabSeparated_utf16be (nread, header) :
		uheader [0] == 0xff && uheader [1] == 0xef ? isTabSeparated_utf16le (nread, header) :
		isTabSeparated_8bit (nread, header);
	if (! isTabSeparated) return NULL;
	return Table_readFromCharacterSeparatedTextFile (file, '\t');
}

extern "C" void praat_TableOfReal_init (void *klas);   /* Buttons for TableOfReal and for its subclasses. */
extern "C" void praat_TableOfReal_init (void *klas) {
	praat_addAction1 (klas, 1, L"Save as headerless spreadsheet file...", 0, 0, DO_TableOfReal_writeToHeaderlessSpreadsheetFile);
	praat_addAction1 (klas, 1, L"Write to headerless spreadsheet file...", 0, praat_HIDDEN, DO_TableOfReal_writeToHeaderlessSpreadsheetFile);
	praat_addAction1 (klas, 0, L"Draw -", 0, 0, 0);
		praat_addAction1 (klas, 0, L"Draw as numbers...", 0, 1, DO_TableOfReal_drawAsNumbers);
		praat_addAction1 (klas, 0, L"Draw as numbers if...", 0, 1, DO_TableOfReal_drawAsNumbers_if);
		praat_addAction1 (klas, 0, L"Draw as squares...", 0, 1, DO_TableOfReal_drawAsSquares);	
		praat_addAction1 (klas, 0, L"-- draw lines --", 0, 1, 0);
		praat_addAction1 (klas, 0, L"Draw vertical lines...", 0, 1, DO_TableOfReal_drawVerticalLines);
		praat_addAction1 (klas, 0, L"Draw horizontal lines...", 0, 1, DO_TableOfReal_drawHorizontalLines);
		praat_addAction1 (klas, 0, L"Draw left and right lines...", 0, 1, DO_TableOfReal_drawLeftAndRightLines);
		praat_addAction1 (klas, 0, L"Draw top and bottom lines...", 0, 1, DO_TableOfReal_drawTopAndBottomLines);
	praat_addAction1 (klas, 0, L"Query -", 0, 0, 0);
		praat_addAction1 (klas, 1, L"Get number of rows", 0, 1, DO_TableOfReal_getNumberOfRows);
		praat_addAction1 (klas, 1, L"Get number of columns", 0, 1, DO_TableOfReal_getNumberOfColumns);
		praat_addAction1 (klas, 1, L"Get row label...", 0, 1, DO_TableOfReal_getRowLabel);
		praat_addAction1 (klas, 1, L"Get column label...", 0, 1, DO_TableOfReal_getColumnLabel);
		praat_addAction1 (klas, 1, L"Get row index...", 0, 1, DO_TableOfReal_getRowIndex);
		praat_addAction1 (klas, 1, L"Get column index...", 0, 1, DO_TableOfReal_getColumnIndex);
		praat_addAction1 (klas, 1, L"-- get value --", 0, 1, 0);
		praat_addAction1 (klas, 1, L"Get value...", 0, 1, DO_TableOfReal_getValue);
		if (klas == classTableOfReal) {
			praat_addAction1 (klas, 1, L"-- get statistics --", 0, 1, 0);
			praat_addAction1 (klas, 1, L"Get column mean (index)...", 0, 1, DO_TableOfReal_getColumnMean_index);
			praat_addAction1 (klas, 1, L"Get column mean (label)...", 0, 1, DO_TableOfReal_getColumnMean_label);
			praat_addAction1 (klas, 1, L"Get column stdev (index)...", 0, 1, DO_TableOfReal_getColumnStdev_index);
			praat_addAction1 (klas, 1, L"Get column stdev (label)...", 0, 1, DO_TableOfReal_getColumnStdev_label);
		}
	praat_addAction1 (klas, 0, L"Modify -", 0, 0, 0);
		praat_addAction1 (klas, 0, L"Formula...", 0, 1, DO_TableOfReal_formula);
		praat_addAction1 (klas, 0, L"Set value...", 0, 1, DO_TableOfReal_setValue);
		praat_addAction1 (klas, 0, L"Sort by label...", 0, 1, DO_TableOfReal_sortByLabel);
		praat_addAction1 (klas, 0, L"Sort by column...", 0, 1, DO_TableOfReal_sortByColumn);
		praat_addAction1 (klas, 0, L"-- structure --", 0, 1, 0);
		praat_addAction1 (klas, 0, L"Remove row (index)...", 0, 1, DO_TableOfReal_removeRow);
		praat_addAction1 (klas, 0, L"Remove column (index)...", 0, 1, DO_TableOfReal_removeColumn);
		praat_addAction1 (klas, 0, L"Insert row (index)...", 0, 1, DO_TableOfReal_insertRow);
		praat_addAction1 (klas, 0, L"Insert column (index)...", 0, 1, DO_TableOfReal_insertColumn);
		praat_addAction1 (klas, 0, L"-- set --", 0, 1, 0);
		praat_addAction1 (klas, 0, L"Set row label (index)...", 0, 1, DO_TableOfReal_setRowLabel_index);
		praat_addAction1 (klas, 0, L"Set row label (label)...", 0, 1, DO_TableOfReal_setRowLabel_label);
		praat_addAction1 (klas, 0, L"Set column label (index)...", 0, 1, DO_TableOfReal_setColumnLabel_index);
		praat_addAction1 (klas, 0, L"Set column label (label)...", 0, 1, DO_TableOfReal_setColumnLabel_label);
	praat_addAction1 (klas, 0, L"Synthesize -", 0, 0, 0);
		praat_addAction1 (klas, 0, L"Append", 0, 1, DO_TablesOfReal_append);
	praat_addAction1 (klas, 0, L"Extract part -", 0, 0, 0);
		praat_addAction1 (klas, 0, L"Extract row ranges...", 0, 1, DO_TableOfReal_extractRowRanges);
		praat_addAction1 (klas, 0, L"Extract rows where column...", 0, 1, DO_TableOfReal_extractRowsWhereColumn);
		praat_addAction1 (klas, 0, L"Extract rows where label...", 0, 1, DO_TableOfReal_extractRowsWhereLabel);
		praat_addAction1 (klas, 0, L"Extract rows where...", 0, 1, DO_TableOfReal_extractRowsWhere);
		praat_addAction1 (klas, 0, L"Extract column ranges...", 0, 1, DO_TableOfReal_extractColumnRanges);
		praat_addAction1 (klas, 0, L"Extract columns where row...", 0, 1, DO_TableOfReal_extractColumnsWhereRow);
		praat_addAction1 (klas, 0, L"Extract columns where label...", 0, 1, DO_TableOfReal_extractColumnsWhereLabel);
		praat_addAction1 (klas, 0, L"Extract columns where...", 0, 1, DO_TableOfReal_extractColumnsWhere);
	praat_addAction1 (klas, 0, L"Extract -", 0, 0, 0);
		praat_addAction1 (klas, 0, L"Extract row labels as Strings", 0, 1, DO_TableOfReal_extractRowLabelsAsStrings);
		praat_addAction1 (klas, 0, L"Extract column labels as Strings", 0, 1, DO_TableOfReal_extractColumnLabelsAsStrings);
	praat_addAction1 (klas, 0, L"Convert -", 0, 0, 0);
		praat_addAction1 (klas, 0, L"To Table...", 0, 1, DO_TableOfReal_to_Table);
		praat_addAction1 (klas, 0, L"To Matrix", 0, 1, DO_TableOfReal_to_Matrix);
}

extern "C" void praat_uvafon_Stat_init (void);
void praat_uvafon_Stat_init (void) {

	Thing_recognizeClassesByName (classTableOfReal, classDistributions, classPairDistribution,
		classTable, classLinearRegression, classLogisticRegression, NULL);

	Data_recognizeFileType (tabSeparatedFileRecognizer);

	praat_addMenuCommand (L"Objects", L"New", L"Tables", 0, 0, 0);
		praat_addMenuCommand (L"Objects", L"New", L"Create Table with column names...", 0, 1, DO_Table_createWithColumnNames);
		praat_addMenuCommand (L"Objects", L"New", L"Create Table without column names...", 0, 1, DO_Table_createWithoutColumnNames);
		praat_addMenuCommand (L"Objects", L"New", L"Create Table...", 0, praat_DEPTH_1 + praat_HIDDEN, DO_Table_createWithoutColumnNames);
		praat_addMenuCommand (L"Objects", L"New", L"Create TableOfReal...", 0, 1, DO_TableOfReal_create);

	praat_addMenuCommand (L"Objects", L"Open", L"Read TableOfReal from headerless spreadsheet file...", 0, 0, DO_TableOfReal_readFromHeaderlessSpreadsheetFile);
	praat_addMenuCommand (L"Objects", L"Open", L"Read Table from tab-separated file...", 0, 0, DO_Table_readFromTabSeparatedFile);
	praat_addMenuCommand (L"Objects", L"Open", L"Read Table from comma-separated file...", 0, 0, DO_Table_readFromCommaSeparatedFile);
	praat_addMenuCommand (L"Objects", L"Open", L"Read Table from whitespace-separated file...", 0, 0, DO_Table_readFromTableFile);
	praat_addMenuCommand (L"Objects", L"Open", L"Read Table from table file...", 0, praat_HIDDEN, DO_Table_readFromTableFile);

	praat_addAction1 (classDistributions, 0, L"Distributions help", 0, 0, DO_Distributions_help);
	praat_TableOfReal_init (classDistributions);
	praat_addAction1 (classDistributions, 1, L"Get probability (label)...", L"Get value...", 1, DO_Distributions_getProbability);
	praat_addAction1 (classDistributions, 0, L"-- get from two --", L"Get probability (label)...", 1, 0);
	praat_addAction1 (classDistributions, 2, L"Get mean absolute difference...", L"-- get from two --", 1, DO_Distributionses_getMeanAbsoluteDifference);
	praat_addAction1 (classDistributions, 0, L"-- add --", L"Append", 1, 0);
	praat_addAction1 (classDistributions, 0, L"Add", L"-- add --", 1, DO_Distributionses_add);
	praat_addAction1 (classDistributions, 0, L"Generate", 0, 0, 0);
		praat_addAction1 (classDistributions, 0, L"To Strings...", 0, 0, DO_Distributions_to_Strings);
		praat_addAction1 (classDistributions, 0, L"To Strings (exact)...", 0, 0, DO_Distributions_to_Strings_exact);

	praat_addAction1 (classLogisticRegression, 0, L"Draw boundary...", 0, 0, DO_LogisticRegression_drawBoundary);

	praat_addAction1 (classPairDistribution, 0, L"PairDistribution help", 0, 0, DO_PairDistribution_help);
	praat_addAction1 (classPairDistribution, 0, L"To Table", 0, 0, DO_PairDistribution_to_Table);
	praat_addAction1 (classPairDistribution, 1, L"To Stringses...", 0, 0, DO_PairDistribution_to_Stringses);
	praat_addAction1 (classPairDistribution, 0, L"Query -", 0, 0, 0);
		praat_addAction1 (classPairDistribution, 1, L"Get number of pairs", 0, 1, DO_PairDistribution_getNumberOfPairs);
		praat_addAction1 (classPairDistribution, 1, L"Get string1...", 0, 1, DO_PairDistribution_getString1);
		praat_addAction1 (classPairDistribution, 1, L"Get string2...", 0, 1, DO_PairDistribution_getString2);
		praat_addAction1 (classPairDistribution, 1, L"Get weight...", 0, 1, DO_PairDistribution_getWeight);
		praat_addAction1 (classPairDistribution, 1, L"-- get fraction correct --", 0, 1, 0);
		praat_addAction1 (classPairDistribution, 1, L"Get fraction correct (maximum likelihood)", 0, 1, DO_PairDistribution_getFractionCorrect_maximumLikelihood);
		praat_addAction1 (classPairDistribution, 1, L"Get fraction correct (probability matching)", 0, 1, DO_PairDistribution_getFractionCorrect_probabilityMatching);
	praat_addAction1 (classPairDistribution, 0, L"Modify -", 0, 0, 0);
	praat_addAction1 (classPairDistribution, 1, L"Remove zero weights", 0, 0, DO_PairDistribution_removeZeroWeights);

	praat_addAction1 (classTable, 0, L"Table help", 0, 0, DO_Table_help);
	praat_addAction1 (classTable, 1, L"Save as tab-separated file...", 0, 0, DO_Table_writeToTableFile);
	praat_addAction1 (classTable, 1, L"Save as table file...", 0, praat_HIDDEN, DO_Table_writeToTableFile);
	praat_addAction1 (classTable, 1, L"Write to table file...", 0, praat_HIDDEN, DO_Table_writeToTableFile);
	praat_addAction1 (classTable, 1, L"View & Edit", 0, praat_ATTRACTIVE, DO_Table_edit);
	praat_addAction1 (classTable, 1, L"Edit", 0, praat_HIDDEN, DO_Table_edit);
	praat_addAction1 (classTable, 0, L"Draw -", 0, 0, 0);
		praat_addAction1 (classTable, 0, L"Scatter plot...", 0, 1, DO_Table_scatterPlot);
		praat_addAction1 (classTable, 0, L"Scatter plot (mark)...", 0, 1, DO_Table_scatterPlot_mark);
		praat_addAction1 (classTable, 0, L"Draw ellipse (standard deviation)...", 0, 1, DO_Table_drawEllipse);
	praat_addAction1 (classTable, 0, L"Query -", 0, 0, 0);
		praat_addAction1 (classTable, 1, L"List...", 0, 1, DO_Table_list);
		praat_addAction1 (classTable, 1, L"-- get structure --", 0, 1, 0);
		praat_addAction1 (classTable, 1, L"Get number of rows", 0, 1, DO_Table_getNumberOfRows);
		praat_addAction1 (classTable, 1, L"Get number of columns", 0, 1, DO_Table_getNumberOfColumns);
		praat_addAction1 (classTable, 1, L"Get column label...", 0, 1, DO_Table_getColumnLabel);
		praat_addAction1 (classTable, 1, L"Get column index...", 0, 1, DO_Table_getColumnIndex);
		praat_addAction1 (classTable, 1, L"-- get value --", 0, 1, 0);
		praat_addAction1 (classTable, 1, L"Get value...", 0, 1, DO_Table_getValue);
		praat_addAction1 (classTable, 1, L"Search column...", 0, 1, DO_Table_searchColumn);
		praat_addAction1 (classTable, 1, L"-- statistics --", 0, 1, 0);
		praat_addAction1 (classTable, 1, L"Statistics tutorial", 0, 1, DO_StatisticsTutorial);
		praat_addAction1 (classTable, 1, L"-- get stats --", 0, 1, 0);
		praat_addAction1 (classTable, 1, L"Get quantile...", 0, 1, DO_Table_getQuantile);
		praat_addAction1 (classTable, 1, L"Get minimum...", 0, 1, DO_Table_getMinimum);
		praat_addAction1 (classTable, 1, L"Get maximum...", 0, 1, DO_Table_getMaximum);
		praat_addAction1 (classTable, 1, L"Get mean...", 0, 1, DO_Table_getMean);
		praat_addAction1 (classTable, 1, L"Get group mean...", 0, 1, DO_Table_getGroupMean);
		praat_addAction1 (classTable, 1, L"Get standard deviation...", 0, 1, DO_Table_getStandardDeviation);
		praat_addAction1 (classTable, 1, L"-- report stats --", 0, 1, 0);
		praat_addAction1 (classTable, 1, L"Report mean (Student t)...", 0, 1, DO_Table_reportMean_studentT);
		/*praat_addAction1 (classTable, 1, L"Report standard deviation...", 0, 1, DO_Table_reportStandardDeviation);*/
		praat_addAction1 (classTable, 1, L"Report difference (Student t)...", 0, 1, DO_Table_reportDifference_studentT);
		praat_addAction1 (classTable, 1, L"Report group mean (Student t)...", 0, 1, DO_Table_reportGroupMean_studentT);
		praat_addAction1 (classTable, 1, L"Report group difference (Student t)...", 0, 1, DO_Table_reportGroupDifference_studentT);
		praat_addAction1 (classTable, 1, L"Report group difference (Wilcoxon rank sum)...", 0, 1, DO_Table_reportGroupDifference_wilcoxonRankSum);
		praat_addAction1 (classTable, 1, L"Report correlation (Pearson r)...", 0, 1, DO_Table_reportCorrelation_pearsonR);
		praat_addAction1 (classTable, 1, L"Report correlation (Kendall tau)...", 0, 1, DO_Table_reportCorrelation_kendallTau);
	praat_addAction1 (classTable, 0, L"Modify -", 0, 0, 0);
		praat_addAction1 (classTable, 0, L"Set string value...", 0, 1, DO_Table_setStringValue);
		praat_addAction1 (classTable, 0, L"Set numeric value...", 0, 1, DO_Table_setNumericValue);
		praat_addAction1 (classTable, 0, L"Formula...", 0, 1, DO_Table_formula);
		praat_addAction1 (classTable, 0, L"Formula (column range)...", 0, 1, DO_Table_formula_columnRange);
		praat_addAction1 (classTable, 0, L"Sort rows...", 0, 1, DO_Table_sortRows);
		praat_addAction1 (classTable, 0, L"Randomize rows", 0, 1, DO_Table_randomizeRows);
		praat_addAction1 (classTable, 0, L"-- structure --", 0, 1, 0);
		praat_addAction1 (classTable, 0, L"Append row", 0, 1, DO_Table_appendRow);
		praat_addAction1 (classTable, 0, L"Append column...", 0, 1, DO_Table_appendColumn);
		praat_addAction1 (classTable, 0, L"Append sum column...", 0, 1, DO_Table_appendSumColumn);
		praat_addAction1 (classTable, 0, L"Append difference column...", 0, 1, DO_Table_appendDifferenceColumn);
		praat_addAction1 (classTable, 0, L"Append product column...", 0, 1, DO_Table_appendProductColumn);
		praat_addAction1 (classTable, 0, L"Append quotient column...", 0, 1, DO_Table_appendQuotientColumn);
		praat_addAction1 (classTable, 0, L"Remove row...", 0, 1, DO_Table_removeRow);
		praat_addAction1 (classTable, 0, L"Remove column...", 0, 1, DO_Table_removeColumn);
		praat_addAction1 (classTable, 0, L"Insert row...", 0, 1, DO_Table_insertRow);
		praat_addAction1 (classTable, 0, L"Insert column...", 0, 1, DO_Table_insertColumn);
		praat_addAction1 (classTable, 0, L"-- set --", 0, 1, 0);
		praat_addAction1 (classTable, 0, L"Set column label (index)...", 0, 1, DO_Table_setColumnLabel_index);
		praat_addAction1 (classTable, 0, L"Set column label (label)...", 0, 1, DO_Table_setColumnLabel_label);
	praat_addAction1 (classTable, 0, L"Analyse -", 0, 0, 0);
		praat_addAction1 (classTable, 0, L"To linear regression", 0, 1, DO_Table_to_LinearRegression);
		praat_addAction1 (classTable, 0, L"To logistic regression...", 0, 1, DO_Table_to_LogisticRegression);
	praat_addAction1 (classTable, 0, L"Synthesize -", 0, 0, 0);
		praat_addAction1 (classTable, 0, L"Append", 0, 1, DO_Tables_append);
	praat_addAction1 (classTable, 0, L"Generate -", 0, 0, 0);
		praat_addAction1 (classTable, 1, L"Draw row from distribution...", 0, 1, DO_Table_drawRowFromDistribution);
	praat_addAction1 (classTable, 0, L"Extract -", 0, 0, 0);
		praat_addAction1 (classTable, 0, L"Extract rows where column (number)...", 0, 1, DO_Table_extractRowsWhereColumn_number);
		praat_addAction1 (classTable, 0, L"Extract rows where column...", 0, praat_DEPTH_1 + praat_HIDDEN, DO_Table_extractRowsWhereColumn_number);
		praat_addAction1 (classTable, 0, L"Select rows where column...", 0, praat_DEPTH_1 + praat_HIDDEN, DO_Table_extractRowsWhereColumn_number);
		praat_addAction1 (classTable, 0, L"Extract rows where column (text)...", 0, 1, DO_Table_extractRowsWhereColumn_text);
		praat_addAction1 (classTable, 0, L"Collapse rows...", 0, 1, DO_Table_collapseRows);
		praat_addAction1 (classTable, 0, L"Rows to columns...", 0, 1, DO_Table_rowsToColumns);
	praat_addAction1 (classTable, 0, L"Down to TableOfReal...", 0, 0, DO_Table_to_TableOfReal);

	praat_addAction1 (classTableOfReal, 0, L"TableOfReal help", 0, 0, DO_TableOfReal_help);
	praat_TableOfReal_init (classTableOfReal);

	praat_addAction2 (classPairDistribution, 1, classDistributions, 1, L"Get fraction correct...", 0, 0, DO_PairDistribution_Distributions_getFractionCorrect);
}

/* End of file praat_Stat.cpp */
