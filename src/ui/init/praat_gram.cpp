/* praat_gram.c
 *
 * Copyright (C) 1997-2011 Paul Boersma
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
 * pb 2011/03/22
 */

#include "gram/Network.h"
#include "gram/OTGrammar.h"
#include "gram/OTMulti.h"
#include "ui/editors/OTGrammarEditor.h"
#include "ui/editors/OTMultiEditor.h"
#include "ui/UiFile.h"

#include "ui/praat.h"

/***** HELP *****/

DIRECT (OT_learning_tutorial) Melder_help (L"OT learning"); END

DIRECT (OTGrammar_help) Melder_help (L"OTGrammar"); END

/***** NETWORK *****/

static void UiForm_addNetworkFields (UiForm *dia) {
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Activity spreading settings:")
	REAL (L"left Activity range", L"-1.0")
	REAL (L"right Activity range", L"1.0")
	REAL (L"Spreading rate", L"1.0")
	REAL (L"Self-excitation", L"1.0")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Weight update settings:")
	REAL (L"left Weight range", L"-1.0")
	REAL (L"right Weight range", L"1.0")
	REAL (L"Learning rate", L"0.1")
	REAL (L"Leak", L"0.0")
}

FORM (Create_empty_Network, L"Create empty Network", 0)
	WORD (L"Name", L"network")
	UiForm_addNetworkFields (dia);
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"World coordinates:")
	REAL (L"left x range", L"0.0")
	REAL (L"right x range", L"10.0")
	REAL (L"left y range", L"0.0")
	REAL (L"right y range", L"10.0")
	OK
DO
	autoNetwork me = Network_create (GET_REAL (L"left Activity range"), GET_REAL (L"right Activity range"),
		GET_REAL (L"Spreading rate"), GET_REAL (L"Self-excitation"),
		GET_REAL (L"left Weight range"), GET_REAL (L"right Weight range"),
		GET_REAL (L"Learning rate"), GET_REAL (L"Leak"),
		GET_REAL (L"left x range"), GET_REAL (L"right x range"), GET_REAL (L"left y range"), GET_REAL (L"right y range"),
		0, 0);
	praat_new (me.transfer(), GET_STRING (L"Name"));
END

FORM (Create_rectangular_Network, L"Create rectangular Network", 0)
	UiForm_addNetworkFields (dia);
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Structure settings:")
	NATURAL (L"Number of rows", L"10")
	NATURAL (L"Number of columns", L"10")
	BOOLEAN (L"Bottom row clamped", 1)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Initial state settings:")
	REAL (L"left Initial weight range", L"-0.1")
	REAL (L"right Initial weight range", L"0.1")
	OK
DO
	autoNetwork me = Network_create_rectangle_e (GET_REAL (L"left Activity range"), GET_REAL (L"right Activity range"),
		GET_REAL (L"Spreading rate"), GET_REAL (L"Self-excitation"),
		GET_REAL (L"left Weight range"), GET_REAL (L"right Weight range"),
		GET_REAL (L"Learning rate"), GET_REAL (L"Leak"),
		GET_INTEGER (L"Number of rows"), GET_INTEGER (L"Number of columns"),
		GET_INTEGER (L"Bottom row clamped"),
		GET_REAL (L"left Initial weight range"), GET_REAL (L"right Initial weight range"));
	praat_new (me.transfer(),
			L"rectangle_", Melder_integer (GET_INTEGER (L"Number of rows")),
			L"_", Melder_integer (GET_INTEGER (L"Number of columns")));
END

FORM (Create_rectangular_Network_vertical, L"Create rectangular Network (vertical)", 0)
	UiForm_addNetworkFields (dia);
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Structure settings:")
	NATURAL (L"Number of rows", L"10")
	NATURAL (L"Number of columns", L"10")
	BOOLEAN (L"Bottom row clamped", 1)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Initial state settings:")
	REAL (L"left Initial weight range", L"-0.1")
	REAL (L"right Initial weight range", L"0.1")
	OK
DO
	autoNetwork me = Network_create_rectangle_vertical_e (GET_REAL (L"left Activity range"), GET_REAL (L"right Activity range"),
		GET_REAL (L"Spreading rate"), GET_REAL (L"Self-excitation"),
		GET_REAL (L"left Weight range"), GET_REAL (L"right Weight range"),
		GET_REAL (L"Learning rate"), GET_REAL (L"Leak"),
		GET_INTEGER (L"Number of rows"), GET_INTEGER (L"Number of columns"),
		GET_INTEGER (L"Bottom row clamped"),
		GET_REAL (L"left Initial weight range"), GET_REAL (L"right Initial weight range"));
	praat_new (me.transfer(),
			L"rectangle_", Melder_integer (GET_INTEGER (L"Number of rows")),
			L"_", Melder_integer (GET_INTEGER (L"Number of columns")));
END

void Network_draw (Network me, Graphics graphics, bool colour) {
	double saveLineWidth = Graphics_inqLineWidth (graphics);
	Graphics_setInner (graphics);
	Graphics_setWindow (graphics, my xmin, my xmax, my ymin, my ymax);
	Graphics_setColour (graphics, Graphics_SILVER);
	Graphics_fillRectangle (graphics, my xmin, my xmax, my ymin, my ymax);
	/*
	 * Draw connections.
	 */
	for (long iconn = 1; iconn <= my numberOfConnections; iconn ++) {
		NetworkConnection conn = & my connections [iconn];
		if (conn -> weight != 0.0) {
			NetworkNode nodeFrom = & my nodes [conn -> nodeFrom];
			NetworkNode nodeTo = & my nodes [conn -> nodeTo];
			Graphics_setLineWidth (graphics, fabs (conn -> weight) * 6.0);
			Graphics_setColour (graphics, conn -> weight < 0.0 ? Graphics_WHITE : Graphics_BLACK);
			Graphics_line (graphics, nodeFrom -> x, nodeFrom -> y, nodeTo -> x, nodeTo -> y);
		}
	}
	Graphics_setLineWidth (graphics, 1.0);
	/*
	 * Draw nodes.
	 */
	for (long inode = 1; inode <= my numberOfNodes; inode ++) {
		NetworkNode node = & my nodes [inode];
		double diameter = fabs (node -> activity) * 5.0;
		if (diameter != 0.0) {
			Graphics_setColour (graphics,
				colour ? ( node -> activity < 0.0 ? Graphics_BLUE : Graphics_RED )
				: ( node -> activity < 0.0 ? Graphics_WHITE : Graphics_BLACK));
			Graphics_fillCircle_mm (graphics, node -> x, node -> y, diameter);
		}
		if (node -> clamped) {
			Graphics_setColour (graphics, Graphics_BLACK);
			Graphics_setLineWidth (graphics, 2.0);
			Graphics_circle_mm (graphics, node -> x, node -> y, 5.0);
		}
	}
	Graphics_setColour (graphics, Graphics_BLACK);
	Graphics_setLineWidth (graphics, saveLineWidth);
	Graphics_unsetInner (graphics);
}

FORM (Network_addConnection, L"Network: Add connection", 0)
	NATURAL (L"From node", L"1")
	NATURAL (L"To node", L"2")
	REAL (L"Weight", L"0.0")
	REAL (L"Plasticity", L"1.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Network);
		Network_addConnection_e (me, GET_INTEGER (L"From node"), GET_INTEGER (L"To node"), GET_REAL (L"Weight"), GET_REAL (L"Plasticity")); therror
		praat_dataChanged (me);
	}
END

FORM (Network_addNode, L"Network: Add node", 0)
	REAL (L"x", L"5.0")
	REAL (L"y", L"5.0")
	REAL (L"Activity", L"0.0")
	BOOLEAN (L"Clamping", 0)
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Network);
		Network_addNode_e (me, GET_REAL (L"x"), GET_REAL (L"y"), GET_REAL (L"Activity"), GET_INTEGER (L"Clamping")); therror
		praat_dataChanged (me);
	}
END

FORM (Network_draw, L"Draw Network", 0)
	BOOLEAN (L"Colour", 1)
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (Network);
		Network_draw (me, GRAPHICS, GET_INTEGER (L"Colour"));
	}
END

FORM (Network_getActivity, L"Network: Get activity", 0)
	NATURAL (L"Node", L"1")
	OK
DO
	iam_ONLY (Network);
	double activity = Network_getActivity_e (me, GET_INTEGER (L"Node")); therror
	Melder_information1 (Melder_double (activity));
END

FORM (Network_getWeight, L"Network: Get weight", 0)
	NATURAL (L"Connection", L"1")
	OK
DO
	iam_ONLY (Network);
	double weight = Network_getWeight_e (me, GET_INTEGER (L"Connection")); therror
	Melder_information1 (Melder_double (weight));
END

FORM (Network_normalizeActivities, L"Network: Normalize activities", 0)
	INTEGER (L"From node", L"1")
	INTEGER (L"To node", L"0 (= all)")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Network);
		Network_normalizeActivities (me, GET_INTEGER (L"From node"), GET_INTEGER (L"To node")); therror
		praat_dataChanged (me);
	}
END

FORM (Network_setActivity, L"Network: Set activity", 0)
	NATURAL (L"Node", L"1")
	REAL (L"Activity", L"1.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Network);
		Network_setActivity_e (me, GET_INTEGER (L"Node"), GET_REAL (L"Activity")); therror
		praat_dataChanged (me);
	}
END

FORM (Network_setWeight, L"Network: Set weight", 0)
	NATURAL (L"Connection", L"1")
	REAL (L"Weight", L"1.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Network);
		Network_setWeight_e (me, GET_INTEGER (L"Connection"), GET_REAL (L"Weight")); therror
		praat_dataChanged (me);
	}
END

FORM (Network_setClamping, L"Network: Set clamping", 0)
	NATURAL (L"Node", L"1")
	BOOLEAN (L"Clamping", 1)
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Network);
		Network_setClamping_e (me, GET_INTEGER (L"Node"), GET_INTEGER (L"Clamping")); therror
		praat_dataChanged (me);
	}
END

FORM (Network_spreadActivities, L"Network: Spread activities", 0)
	NATURAL (L"Number of steps", L"20")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Network);
		Network_spreadActivities (me, GET_INTEGER (L"Number of steps"));
		praat_dataChanged (me);
	}
END

DIRECT (Network_updateWeights)
	WHERE (SELECTED) {
		iam_LOOP (Network);
		Network_updateWeights (me);
		praat_dataChanged (me);
	}
END

FORM (Network_zeroActivities, L"Network: Zero activities", 0)
	INTEGER (L"From node", L"1")
	INTEGER (L"To node", L"0 (= all)")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Network);
		Network_zeroActivities (me, GET_INTEGER (L"From node"), GET_INTEGER (L"To node")); therror
		praat_dataChanged (me);
	}
END


/***** OTGRAMMAR *****/

FORM (Create_metrics_grammar, L"Create metrics grammar", 0)
	OPTIONMENU (L"Initial ranking", 1)
		OPTION (L"Equal")
		OPTION (L"Foot form high")
		OPTION (L"WSP high")
	OPTIONMENU (L"Trochaicity constraint", 1)
		OPTION (L"FtNonfinal")
		OPTION (L"Trochaic")
	BOOLEAN (L"Include FootBimoraic", 0)
	BOOLEAN (L"Include FootBisyllabic", 0)
	BOOLEAN (L"Include Peripheral", 0)
	OPTIONMENU (L"Nonfinality constraint", 1)
		OPTION (L"Nonfinal")
		OPTION (L"MainNonfinal")
		OPTION (L"HeadNonfinal")
	BOOLEAN (L"Overt forms have secondary stress", 1)
	BOOLEAN (L"Include *Clash and *Lapse", 0)
	BOOLEAN (L"Include codas", 0)
	OK
DO
	if (! praat_new1 (OTGrammar_create_metrics (GET_INTEGER (L"Initial ranking"), GET_INTEGER (L"Trochaicity constraint"),
		GET_INTEGER (L"Include FootBimoraic"), GET_INTEGER (L"Include FootBisyllabic"),
		GET_INTEGER (L"Include Peripheral"), GET_INTEGER (L"Nonfinality constraint"),
		GET_INTEGER (L"Overt forms have secondary stress"), GET_INTEGER (L"Include *Clash and *Lapse"), GET_INTEGER (L"Include codas")),
		GET_STRING (L"Initial ranking"))) return 0;
END

DIRECT (Create_NoCoda_grammar)
	autoOTGrammar me = OTGrammar_create_NoCoda_grammar ();
	praat_new (me.transfer(), L"NoCoda");
END

DIRECT (Create_NPA_grammar)
	autoOTGrammar me = OTGrammar_create_NPA_grammar ();
	praat_new (me.transfer(), L"assimilation");
END

DIRECT (Create_NPA_distribution)
	autoPairDistribution me = OTGrammar_create_NPA_distribution ();
	praat_new (me.transfer(), L"assimilation");
END

FORM (Create_tongue_root_grammar, L"Create tongue-root grammar", L"Create tongue-root grammar...")
	RADIO (L"Constraint set", 1)
		RADIOBUTTON (L"Five")
		RADIOBUTTON (L"Nine")
	RADIO (L"Ranking", 3)
		RADIOBUTTON (L"Equal")
		RADIOBUTTON (L"Random")
		RADIOBUTTON (L"Infant")
		RADIOBUTTON (L"Wolof")
	OK
DO
	autoOTGrammar me = OTGrammar_create_tongueRoot_grammar (GET_INTEGER (L"Constraint set"), GET_INTEGER (L"Ranking"));
	praat_new (me.transfer(), GET_STRING (L"Ranking"));
END

static double OTGrammar_constraintWidth (Graphics g, const wchar_t *name) {
	wchar_t text [100], *newLine;
	wcscpy (text, name);
	newLine = wcschr (text, '\n');
	if (newLine) {
		double firstWidth, secondWidth;
		*newLine = '\0';
		firstWidth = Graphics_textWidth (g, text);
		secondWidth = Graphics_textWidth (g, newLine + 1);
		return firstWidth > secondWidth ? firstWidth : secondWidth;
	}
	return Graphics_textWidth (g, text);
}

void OTGrammar_drawTableau (OTGrammar me, Graphics g, const wchar_t *input) {
	long itab, winner, icons, icand, numberOfOptimalCandidates, imark;
	OTGrammarTableau tableau;
	double candWidth, margin, fingerWidth, doubleLineDx, doubleLineDy;
	double tableauWidth, rowHeight, headerHeight, descent, x, y, fontSize = Graphics_inqFontSize (g);
	Graphics_Colour colour = Graphics_inqColour (g);
	wchar_t text [200];
	itab = OTGrammar_getTableau (me, input);
	if (! itab) {
		Melder_error3 (L"This grammar accepts no input \"", input, L"\".");
		Melder_flushError (NULL);
		return;
	}
	OTGrammar_fillInHarmonies (me, itab);
	winner = OTGrammar_getWinner (me, itab);
	
	Graphics_setWindow (g, 0.0, 1.0, 0.0, 1.0);
	margin = Graphics_dxMMtoWC (g, 1.0);
	fingerWidth = Graphics_dxMMtoWC (g, 7.0) * fontSize / 12.0;
	doubleLineDx = Graphics_dxMMtoWC (g, 0.9);
	doubleLineDy = Graphics_dyMMtoWC (g, 0.9);
	rowHeight = Graphics_dyMMtoWC (g, 1.5 * fontSize * 25.4 / 72);
	descent = rowHeight * 0.5;
	/*
	 * Compute height of header row.
	 */
	headerHeight = rowHeight;
	for (icons = 1; icons <= my numberOfConstraints; icons ++) {
		OTGrammarConstraint constraint = & my constraints [icons];
		if (wcschr (constraint -> name, '\n')) {
			headerHeight *= 1.6;
			break;
		}
	}
	/*
	 * Compute longest candidate string.
	 * Also count the number of optimal candidates (if there are more than one, the fingers will be drawn in red).
	 */
	candWidth = Graphics_textWidth (g, input);
	tableau = & my tableaus [itab];
	numberOfOptimalCandidates = 0;
	for (icand = 1; icand <= tableau -> numberOfCandidates; icand ++) {
		double width = Graphics_textWidth (g, tableau -> candidates [icand]. output);
		if (OTGrammar_compareCandidates (me, itab, icand, itab, winner) == 0) {
			width += fingerWidth;
			numberOfOptimalCandidates ++;
		}
		if (width > candWidth) candWidth = width;
	}
	candWidth += margin * 3;
	/*
	 * Compute tableau width.
	 */
	tableauWidth = candWidth + doubleLineDx;
	for (icons = 1; icons <= my numberOfConstraints; icons ++) {
		OTGrammarConstraint constraint = & my constraints [icons];
		tableauWidth += OTGrammar_constraintWidth (g, constraint -> name);
	}
	tableauWidth += margin * 2 * my numberOfConstraints;
	/*
	 * Draw box.
	 */
	x = doubleLineDx;   /* Left side of tableau. */
	y = 1.0 - doubleLineDy;
	Graphics_rectangle (g, x, x + tableauWidth,
		y - headerHeight - tableau -> numberOfCandidates * rowHeight - doubleLineDy, y);
	/*
	 * Draw input.
	 */
	y -= headerHeight;
	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	Graphics_text (g, x + 0.5 * candWidth, y + 0.5 * headerHeight, input);
	Graphics_rectangle (g, x, x + candWidth, y, y + headerHeight);
	/*
	 * Draw constraint names.
	 */
	x += candWidth + doubleLineDx;
	for (icons = 1; icons <= my numberOfConstraints; icons ++) {
		OTGrammarConstraint constraint = & my constraints [my index [icons]];
		double width = OTGrammar_constraintWidth (g, constraint -> name) + margin * 2;
		if (wcschr (constraint -> name, '\n')) {
			wchar_t *newLine;
			wcscpy (text, constraint -> name);
			newLine = wcschr (text, '\n');
			*newLine = '\0';
			Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_TOP);
			Graphics_text (g, x + 0.5 * width, y + headerHeight, text);
			Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_BOTTOM);
			Graphics_text (g, x + 0.5 * width, y, newLine + 1);
		} else {
			Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
			Graphics_text (g, x + 0.5 * width, y + 0.5 * headerHeight, constraint -> name);
		}
		if (constraint -> tiedToTheLeft)
			Graphics_setLineType (g, Graphics_DOTTED);
		Graphics_line (g, x, y, x, y + headerHeight);
		Graphics_setLineType (g, Graphics_DRAWN);
		Graphics_line (g, x, y, x + width, y);
		x += width;
	}
	/*
	 * Draw candidates.
	 */
	y -= doubleLineDy;
	for (icand = 1; icand <= tableau -> numberOfCandidates; icand ++) {
		long crucialCell = OTGrammar_crucialCell (me, itab, icand, winner, numberOfOptimalCandidates);
		int candidateIsOptimal = OTGrammar_compareCandidates (me, itab, icand, itab, winner) == 0;
		/*
		 * Draw candidate transcription.
		 */
		x = doubleLineDx;
		y -= rowHeight;
		Graphics_setTextAlignment (g, Graphics_RIGHT, Graphics_HALF);
		Graphics_text (g, x + candWidth - margin, y + descent, tableau -> candidates [icand]. output);
		if (candidateIsOptimal) {
			Graphics_setTextAlignment (g, Graphics_LEFT, Graphics_HALF);
			Graphics_setFontSize (g, (int) (1.5 * fontSize));
			if (numberOfOptimalCandidates > 1) Graphics_setColour (g, Graphics_RED);
			Graphics_text (g, x + margin, y + descent - Graphics_dyMMtoWC (g, 1.0) * fontSize / 12.0, L"\\pf");
			Graphics_setColour (g, colour);
			Graphics_setFontSize (g, (int) fontSize);
		}
		Graphics_rectangle (g, x, x + candWidth, y, y + rowHeight);
		/*
		 * Draw grey cell backgrounds.
		 */
		x = candWidth + 2 * doubleLineDx;
		Graphics_setGrey (g, 0.9);
		for (icons = 1; icons <= my numberOfConstraints; icons ++) {
			int index = my index [icons];
			OTGrammarConstraint constraint = & my constraints [index];
			double width = OTGrammar_constraintWidth (g, constraint -> name) + margin * 2;
			if (icons > crucialCell)
				Graphics_fillRectangle (g, x, x + width, y, y + rowHeight);
			x += width;
		}
		Graphics_setColour (g, colour);
		/*
		 * Draw cell marks.
		 */
		x = candWidth + 2 * doubleLineDx;
		Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
		for (icons = 1; icons <= my numberOfConstraints; icons ++) {
			int index = my index [icons];
			OTGrammarConstraint constraint = & my constraints [index];
			double width = OTGrammar_constraintWidth (g, constraint -> name) + margin * 2;
			wchar_t markString [40];
			markString [0] = '\0';
			if (my decisionStrategy == kOTGrammar_decisionStrategy_OPTIMALITY_THEORY) {
				/*
				 * An exclamation mark can be drawn in this cell only if all of the following conditions are met:
				 * 1. the candidate is not optimal;
				 * 2. the constraint is not tied;
				 * 3. this is the crucial cell, i.e. the cells after it are drawn in grey.
				 */
				if (icons == crucialCell && ! candidateIsOptimal && ! constraint -> tiedToTheLeft && ! constraint -> tiedToTheRight) {
					int winnerMarks = tableau -> candidates [winner]. marks [index];
					for (imark = 1; imark <= winnerMarks + 1; imark ++)
						wcscat (markString, L"*");
					for (imark = tableau -> candidates [icand]. marks [index]; imark < 0; imark ++)
						wcscat (markString, L"+");
					wcscat (markString, L"!");
					for (imark = winnerMarks + 2; imark <= tableau -> candidates [icand]. marks [index]; imark ++)
						wcscat (markString, L"*");
				} else {
					if (! candidateIsOptimal && (constraint -> tiedToTheLeft || constraint -> tiedToTheRight) &&
					    crucialCell >= 1 && constraint -> disharmony == my constraints [my index [crucialCell]]. disharmony)
					{
						Graphics_setColour (g, Graphics_RED);
					}
					for (imark = 1; imark <= tableau -> candidates [icand]. marks [index]; imark ++)
						wcscat (markString, L"*");
					for (imark = tableau -> candidates [icand]. marks [index]; imark < 0; imark ++)
						wcscat (markString, L"+");
				}
			} else {
				for (imark = 1; imark <= tableau -> candidates [icand]. marks [index]; imark ++)
					wcscat (markString, L"*");
				for (imark = tableau -> candidates [icand]. marks [index]; imark < 0; imark ++)
					wcscat (markString, L"+");
			}
			Graphics_text (g, x + 0.5 * width, y + descent, markString);
			Graphics_setColour (g, colour);
			if (constraint -> tiedToTheLeft)
				Graphics_setLineType (g, Graphics_DOTTED);
			Graphics_line (g, x, y, x, y + rowHeight);
			Graphics_setLineType (g, Graphics_DRAWN);
			Graphics_line (g, x, y + rowHeight, x + width, y + rowHeight);
			x += width;
		}
		/*
		 * Draw harmony.
		 */
		if (my decisionStrategy != kOTGrammar_decisionStrategy_OPTIMALITY_THEORY) {
			Graphics_setTextAlignment (g, Graphics_LEFT, Graphics_HALF);
			double value = tableau -> candidates [icand]. harmony;
			if (my decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_HG ||
				my decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_MAXIMUM_ENTROPY)
			{
				//value = value > -1e-300 ? 1000 : value < -1e300 ? -1000 : - log (- value);
				Graphics_text (g, x, y + descent, Melder_float (Melder_half (value)));
			} else {
				Graphics_text (g, x, y + descent, Melder_fixed (value, 3));
			}
		}
	}
	/*
	 * Draw box.
	 */
	x = doubleLineDx;   /* Left side of tableau. */
	y = 1.0 - doubleLineDy;
	Graphics_rectangle (g, x, x + tableauWidth,
		y - headerHeight - tableau -> numberOfCandidates * rowHeight - doubleLineDy, y);
}

FORM (OTGrammar_drawTableau, L"Draw tableau", L"OT learning")
	SENTENCE (L"Input string", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (OTGrammar);
		OTGrammar_drawTableau (me, GRAPHICS, GET_STRING (L"Input string"));
	}
END

DIRECT (OTGrammar_edit)
	if (theCurrentPraatApplication -> batch) Melder_throw ("Cannot edit from batch.");
	WHERE (SELECTED) {
		iam_LOOP (OTGrammar);
		OTGrammarEditor *editor = new OTGrammarEditor (theCurrentPraatApplication -> topShell, ID_AND_FULL_NAME, me);
		praat_installEditor (editor, IOBJECT); therror
	}
END

FORM (OTGrammar_evaluate, L"OTGrammar: Evaluate", 0)
	REAL (L"Evaluation noise", L"2.0")
	OK
DO
	iam_ONLY (OTGrammar);
	OTGrammar_newDisharmonies (me, GET_REAL (L"Evaluation noise"));
	praat_dataChanged (me);
END

FORM (OTGrammar_generateInputs, L"Generate inputs", L"OTGrammar: Generate inputs...")
	NATURAL (L"Number of trials", L"1000")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTGrammar);
		autoStrings thee = OTGrammar_generateInputs (me, GET_INTEGER (L"Number of trials"));
		praat_new (thee.transfer(), my name, L"_in");
	}
END

FORM (OTGrammar_getCandidate, L"Get candidate", 0)
	NATURAL (L"Tableau number", L"1")
	NATURAL (L"Candidate number", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	OTGrammarTableau tableau;
	long itab = GET_INTEGER (L"Tableau"), icand = GET_INTEGER (L"Candidate");
	if (itab > my numberOfTableaus)
		Melder_throw ("The specified tableau number should not exceed the number of tableaus.");
	tableau = & my tableaus [itab];
	if (icand > tableau -> numberOfCandidates)
		Melder_throw ("The specified candidate should not exceed the number of candidates.");
	Melder_information1 (tableau -> candidates [icand]. output);
END

FORM (OTGrammar_getConstraint, L"Get constraint name", 0)
	NATURAL (L"Constraint number", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	long icons = GET_INTEGER (L"Constraint number");
	if (icons > my numberOfConstraints)
		Melder_throw ("The specified constraint number should not exceed the number of constraints.");
	Melder_information1 (my constraints [icons]. name);
END

FORM (OTGrammar_getDisharmony, L"Get disharmony", 0)
	NATURAL (L"Constraint number", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	long icons = GET_INTEGER (L"Constraint number");
	if (icons > my numberOfConstraints)
		Melder_throw ("The specified constraint number should not exceed the number of constraints.");
	Melder_information1 (Melder_double (my constraints [icons]. disharmony));
END

FORM (OTGrammar_getInput, L"Get input", 0)
	NATURAL (L"Tableau number", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	long itab = GET_INTEGER (L"Tableau number");
	if (itab > my numberOfTableaus)
		Melder_throw ("The specified tableau number should not exceed the number of tableaus.");
	Melder_information1 (my tableaus [itab]. input);
END

DIRECT (OTGrammar_getInputs)
	WHERE (SELECTED) {
		iam_LOOP (OTGrammar);
		autoStrings thee = OTGrammar_getInputs (me);
		praat_new (thee.transfer(), my name, L"_in");
	}
END

FORM (OTGrammar_getInterpretiveParse, L"OTGrammar: Interpretive parse", 0)
	SENTENCE (L"Partial output", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (OTGrammar);
	long bestInput, bestOutput;
	OTGrammar_getInterpretiveParse (me, GET_STRING (L"Partial output"), & bestInput, & bestOutput); therror
	Melder_information8 (L"Best input = ", Melder_integer (bestInput), L": ", my tableaus [bestInput]. input,
		L"\nBest output = ", Melder_integer (bestOutput), L": ", my tableaus [bestInput]. candidates [bestOutput]. output);
END

FORM (OTGrammar_getNumberOfCandidates, L"Get number of candidates", 0)
	NATURAL (L"Tableau number", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	long itab = GET_INTEGER (L"Tableau number");
	if (itab > my numberOfTableaus)
		Melder_throw ("The specified tableau number should not exceed the number of tableaus.");
	Melder_information1 (Melder_integer (my tableaus [itab]. numberOfCandidates));
END

DIRECT (OTGrammar_getNumberOfConstraints)
	iam_ONLY (OTGrammar);
	Melder_information1 (Melder_integer (my numberOfConstraints));
END

FORM (OTGrammar_getNumberOfOptimalCandidates, L"Get number of optimal candidates", 0)
	NATURAL (L"Tableau number", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	long itab = GET_INTEGER (L"Tableau number");
	if (itab > my numberOfTableaus)
		Melder_throw ("The specified tableau number should not exceed the number of tableaus.");
	Melder_information1 (Melder_integer (OTGrammar_getNumberOfOptimalCandidates (me, itab)));
END

DIRECT (OTGrammar_getNumberOfTableaus)
	iam_ONLY (OTGrammar);
	Melder_information1 (Melder_integer (my numberOfTableaus));
END

FORM (OTGrammar_getNumberOfViolations, L"Get number of violations", 0)
	NATURAL (L"Tableau number", L"1")
	NATURAL (L"Candidate number", L"1")
	NATURAL (L"Constraint number", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	long itab = GET_INTEGER (L"Tableau number"), icand = GET_INTEGER (L"Candidate number"), icons = GET_INTEGER (L"Constraint number");
	if (itab > my numberOfTableaus)
		Melder_throw ("The specified tableau number should not exceed the number of tableaus.");
	if (icand > my tableaus [itab]. numberOfCandidates)
		Melder_throw ("The specified candidate should not exceed the number of candidates.");
	if (icons > my numberOfConstraints)
		Melder_throw ("The specified constraint number should not exceed the number of constraints.");
	Melder_information1 (Melder_integer (my tableaus [itab]. candidates [icand]. marks [icons]));
END

FORM (OTGrammar_compareCandidates, L"Compare candidates", 0)
	NATURAL (L"Tableau number 1", L"1")
	NATURAL (L"Candidate number 1", L"1")
	NATURAL (L"Tableau number 2", L"1")
	NATURAL (L"Candidate number 2", L"2")
	OK
DO
	iam_ONLY (OTGrammar);
	long itab1 = GET_INTEGER (L"Tableau number 1"), icand1 = GET_INTEGER (L"Candidate number 1");
	long itab2 = GET_INTEGER (L"Tableau number 2"), icand2 = GET_INTEGER (L"Candidate number 2");
	if (itab1 > my numberOfTableaus)
		Melder_throw ("The specified tableau (number 1) should not exceed the number of tableaus.");
	if (itab2 > my numberOfTableaus)
		Melder_throw ("The specified tableau (number 2) should not exceed the number of tableaus.");
	if (icand1 > my tableaus [itab1]. numberOfCandidates)
		Melder_throw ("The specified candidate (number 1) should not exceed the number of candidates for this tableau.");
	if (icand2 > my tableaus [itab1]. numberOfCandidates)
		Melder_throw ("The specified candidate (number 2) should not exceed the number of candidates for this tableau.");
	Melder_information1 (Melder_integer (OTGrammar_compareCandidates (me, itab1, icand1, itab2, icand2)));
END

FORM (OTGrammar_getRankingValue, L"Get ranking value", 0)
	NATURAL (L"Constraint number", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	long icons = GET_INTEGER (L"Constraint number");
	if (icons > my numberOfConstraints)
		Melder_throw ("The specified constraint number should not exceed the number of constraints.");
	Melder_information1 (Melder_double (my constraints [icons]. ranking));
END

FORM (OTGrammar_getWinner, L"Get winner", 0)
	NATURAL (L"Tableau", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	long itab = GET_INTEGER (L"Tableau");
	if (itab > my numberOfTableaus)
		Melder_throw ("The specified tableau number should not exceed the number of tableaus.");
	Melder_information1 (Melder_integer (OTGrammar_getWinner (me, itab)));
END

FORM (OTGrammar_inputToOutput, L"OTGrammar: Input to output", L"OTGrammar: Input to output...")
	SENTENCE (L"Input form", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"Evaluation noise", L"2.0")
	OK
DO
	iam_ONLY (OTGrammar);
	wchar_t output [100];
	OTGrammar_inputToOutput (me, GET_STRING (L"Input form"), output, GET_REAL (L"Evaluation noise"));
	Melder_information1 (output);
	praat_dataChanged (me);
END

FORM (OTGrammar_inputToOutputs, L"OTGrammar: Input to outputs", L"OTGrammar: Input to outputs...")
	NATURAL (L"Trials", L"1000")
	REAL (L"Evaluation noise", L"2.0")
	SENTENCE (L"Input form", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (OTGrammar);
	autoStrings thee = OTGrammar_inputToOutputs (me, GET_STRING (L"Input form"), GET_INTEGER (L"Trials"), GET_REAL (L"Evaluation noise"));
	praat_new (thee.transfer(), my name, L"_out");
	praat_dataChanged (me);
END

FORM (OTGrammar_inputsToOutputs, L"OTGrammar: Inputs to outputs", L"OTGrammar: Inputs to outputs...")
	REAL (L"Evaluation noise", L"2.0")
	OK
DO
	iam_ONLY (OTGrammar);
	thouart_ONLY (Strings);
	autoStrings him = OTGrammar_inputsToOutputs (me, thee, GET_REAL (L"Evaluation noise"));
	praat_new2 (him.transfer(), my name, L"_out");
	praat_dataChanged (me);
END

FORM (OTGrammar_isCandidateGrammatical, L"Is candidate grammatical?", 0)
	NATURAL (L"Tableau", L"1")
	NATURAL (L"Candidate", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	long itab = GET_INTEGER (L"Tableau");
	if (itab > my numberOfTableaus)
		Melder_throw ("The specified tableau number should not exceed the number of tableaus.");
	long icand = GET_INTEGER (L"Candidate");
	if (icand > my tableaus [itab]. numberOfCandidates)
		Melder_throw ("The specified candidate should not exceed the number of candidates.");
	Melder_information1 (Melder_integer (OTGrammar_isCandidateGrammatical (me, itab, icand)));
END

FORM (OTGrammar_isCandidateSinglyGrammatical, L"Is candidate singly grammatical?", 0)
	NATURAL (L"Tableau", L"1")
	NATURAL (L"Candidate", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	long itab = GET_INTEGER (L"Tableau");
	if (itab > my numberOfTableaus)
		Melder_throw ("The specified tableau number should not exceed the number of tableaus.");
	long icand = GET_INTEGER (L"Candidate");
	if (icand > my tableaus [itab]. numberOfCandidates)
		Melder_throw ("The specified candidate should not exceed the number of candidates.");
	Melder_information1 (Melder_integer (OTGrammar_isCandidateSinglyGrammatical (me, itab, icand)));
END

FORM (OTGrammar_isPartialOutputGrammatical, L"Is partial output grammatical?", 0)
	SENTENCE (L"Partial output", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (OTGrammar);
	Melder_information1 (Melder_integer (OTGrammar_isPartialOutputGrammatical (me, GET_STRING (L"Partial output"))));
END

FORM (OTGrammar_isPartialOutputSinglyGrammatical, L"Is partial output singly grammatical?", 0)
	SENTENCE (L"Partial output", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (OTGrammar);
	Melder_information1 (Melder_integer (OTGrammar_isPartialOutputSinglyGrammatical (me, GET_STRING (L"Partial output"))));
END

FORM (OTGrammar_learn, L"OTGrammar: Learn", L"OTGrammar & 2 Strings: Learn...")
	REAL (L"Evaluation noise", L"2.0")
	OPTIONMENU_ENUM (L"Update rule", kOTGrammar_rerankingStrategy, SYMMETRIC_ALL)
	REAL (L"Plasticity", L"0.1")
	REAL (L"Rel. plasticity spreading", L"0.1")
	BOOLEAN (L"Honour local rankings", 1)
	NATURAL (L"Number of chews", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	Strings inputs = NULL, outputs = NULL;
	WHERE (SELECTED && CLASS == classStrings) { if (! inputs) inputs = (Strings) OBJECT; else outputs = (Strings) OBJECT; }
	try {
		OTGrammar_learn (me, inputs, outputs,
			GET_REAL (L"Evaluation noise"),
			GET_ENUM (kOTGrammar_rerankingStrategy, L"Update rule"),
			GET_INTEGER (L"Honour local rankings"),
			GET_REAL (L"Plasticity"), GET_REAL (L"Rel. plasticity spreading"), GET_INTEGER (L"Number of chews")); therror
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (me);   // partial change
	}
END

FORM (OTGrammar_learnFromPartialOutputs, L"OTGrammar: Learn from partial adult outputs", 0)
	REAL (L"Evaluation noise", L"2.0")
	OPTIONMENU_ENUM (L"Update rule", kOTGrammar_rerankingStrategy, SYMMETRIC_ALL)
	REAL (L"Plasticity", L"0.1")
	REAL (L"Rel. plasticity spreading", L"0.1")
	BOOLEAN (L"Honour local rankings", 1)
	NATURAL (L"Number of chews", L"1")
	INTEGER (L"Store history every", L"0")
	OK
DO
	iam_ONLY (OTGrammar);
	thouart_ONLY (Strings);
	OTHistory history = NULL;
	try {
		OTGrammar_learnFromPartialOutputs (me, thee,
			GET_REAL (L"Evaluation noise"),
			GET_ENUM (kOTGrammar_rerankingStrategy, L"Update rule"),
			GET_INTEGER (L"Honour local rankings"),
			GET_REAL (L"Plasticity"), GET_REAL (L"Rel. plasticity spreading"), GET_INTEGER (L"Number of chews"),
			GET_INTEGER (L"Store history every"), & history); therror
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (me);   // e.g. in case of partial learning
		// trickle down to save history
	}
	if (history) praat_new (history, my name);
	iferror {
		if (history) praat_updateSelection ();
		return 0;
	}
END

FORM (OTGrammar_learnOne, L"OTGrammar: Learn one", L"OTGrammar: Learn one...")
	SENTENCE (L"Input string", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"Output string", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"Evaluation noise", L"2.0")
	OPTIONMENU_ENUM (L"Update rule", kOTGrammar_rerankingStrategy, SYMMETRIC_ALL)
	REAL (L"Plasticity", L"0.1")
	REAL (L"Rel. plasticity spreading", L"0.1")
	BOOLEAN (L"Honour local rankings", 1)
	OK
DO
	WHERE (SELECTED) try {
		iam_LOOP (OTGrammar);
		OTGrammar_learnOne (me, GET_STRING (L"Input string"), GET_STRING (L"Output string"),
			GET_REAL (L"Evaluation noise"),
			GET_ENUM (kOTGrammar_rerankingStrategy, L"Update rule"),
			GET_INTEGER (L"Honour local rankings"),
			GET_REAL (L"Plasticity"), GET_REAL (L"Rel. plasticity spreading"), TRUE, TRUE, NULL);
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (OBJECT);
		throw 1;
	}
END

FORM (OTGrammar_learnOneFromPartialOutput, L"OTGrammar: Learn one from partial adult output", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Partial adult surface form (e.g. overt form):")
	SENTENCE (L"Partial output", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"Evaluation noise", L"2.0")
	OPTIONMENU_ENUM (L"Update rule", kOTGrammar_rerankingStrategy, SYMMETRIC_ALL)
	REAL (L"Plasticity", L"0.1")
	REAL (L"Rel. plasticity spreading", L"0.1")
	BOOLEAN (L"Honour local rankings", 1)
	NATURAL (L"Number of chews", L"1")
	OK
DO
	WHERE (SELECTED) try {
		iam_LOOP (OTGrammar);
		OTGrammar_learnOneFromPartialOutput (me, GET_STRING (L"Partial output"),
			GET_REAL (L"Evaluation noise"),
			GET_ENUM (kOTGrammar_rerankingStrategy, L"Update rule"),
			GET_INTEGER (L"Honour local rankings"),
			GET_REAL (L"Plasticity"), GET_REAL (L"Rel. plasticity spreading"), GET_INTEGER (L"Number of chews"), TRUE);
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (OBJECT);
		throw 1;
	}
END

FORM (OTGrammar_removeConstraint, L"OTGrammar: Remove constraint", 0)
	SENTENCE (L"Constraint name", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTGrammar);
		OTGrammar_removeConstraint (me, GET_STRING (L"Constraint name")); therror
		praat_dataChanged (me);
	}
END

FORM (OTGrammar_removeHarmonicallyBoundedCandidates, L"OTGrammar: Remove harmonically bounded candidates", 0)
	BOOLEAN (L"Singly", 0)
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTGrammar);
		OTGrammar_removeHarmonicallyBoundedCandidates (me, GET_INTEGER (L"Singly")); therror
		praat_dataChanged (me);
	}
END

FORM (OTGrammar_resetAllRankings, L"OTGrammar: Reset all rankings", 0)
	REAL (L"Ranking", L"100.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTGrammar);
		OTGrammar_reset (me, GET_REAL (L"Ranking"));
		praat_dataChanged (me);
	}
END

FORM (OTGrammar_resetToRandomTotalRanking, L"OTGrammar: Reset to random total ranking", 0)
	REAL (L"Maximum ranking", L"100.0")
	POSITIVE (L"Ranking distance", L"1.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTGrammar);
		OTGrammar_resetToRandomTotalRanking (me, GET_REAL (L"Maximum ranking"), GET_REAL (L"Ranking distance"));
		praat_dataChanged (me);
	}
END

FORM (OTGrammar_setConstraintPlasticity, L"OTGrammar: Set constraint plasticity", 0)
	NATURAL (L"Constraint", L"1")
	REAL (L"Plasticity", L"1.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTGrammar);
		OTGrammar_setConstraintPlasticity (me, GET_INTEGER (L"Constraint"), GET_REAL (L"Plasticity"));
		praat_dataChanged (OBJECT);
	}
END

FORM (OTGrammar_setDecisionStrategy, L"OTGrammar: Set decision strategy", 0)
	RADIO_ENUM (L"Decision strategy", kOTGrammar_decisionStrategy, DEFAULT)
	OK
iam_ONLY (OTGrammar);
SET_ENUM (L"Decision strategy", kOTGrammar_decisionStrategy, my decisionStrategy);
DO
	iam_ONLY (OTGrammar);
	my decisionStrategy = GET_ENUM (kOTGrammar_decisionStrategy, L"Decision strategy");
	praat_dataChanged (me);
END

FORM (OTGrammar_setLeak, L"OTGrammar: Set leak", 0)
	REAL (L"Leak", L"0.0")
	OK
iam_ONLY (OTGrammar);
SET_REAL (L"Leak", my leak);
DO
	iam_ONLY (OTGrammar);
	my leak = GET_REAL (L"Leak");
	praat_dataChanged (me);
END

FORM (OTGrammar_setRanking, L"OTGrammar: Set ranking", 0)
	NATURAL (L"Constraint", L"1")
	REAL (L"Ranking", L"100.0")
	REAL (L"Disharmony", L"100.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTGrammar);
		OTGrammar_setRanking (me, GET_INTEGER (L"Constraint"), GET_REAL (L"Ranking"), GET_REAL (L"Disharmony")); therror
		praat_dataChanged (me);
	}
END

FORM (OTGrammar_Distributions_getFractionCorrect, L"OTGrammar & Distributions: Get fraction correct...", 0)
	NATURAL (L"Column number", L"1")
	REAL (L"Evaluation noise", L"2.0")
	INTEGER (L"Replications", L"100000")
	OK
DO
	iam_ONLY (OTGrammar);
	thouart_ONLY (Distributions);
	double result;
	OTGrammar_Distributions_getFractionCorrect (me, thee, GET_INTEGER (L"Column number"),
		GET_REAL (L"Evaluation noise"), GET_INTEGER (L"Replications"), & result); therror
	praat_dataChanged (me);
	Melder_informationReal (result, NULL);
END

int OTGrammar_Distributions_learnFromPartialOutputs (OTGrammar me, Distributions thee, long columnNumber,
	double evaluationNoise, enum kOTGrammar_rerankingStrategy updateRule, int honourLocalRankings,
	double initialPlasticity, long replicationsPerPlasticity, double plasticityDecrement,
	long numberOfPlasticities, double relativePlasticityNoise, long numberOfChews,
	long storeHistoryEvery, OTHistory *history_out)
{
	const long numberOfData = numberOfPlasticities * replicationsPerPlasticity;
	OTHistory history = NULL;

	OTGrammar_Distributions_opt_createOutputMatching (me, thee, columnNumber);
	const Graphics graphics = (Graphics) Melder_monitor1 (0.0, L"Learning with limited knowledge...");
	if (graphics) {
		Graphics_clearWs (graphics);
	}
	if (storeHistoryEvery) {
		history = OTGrammar_createHistory (me, storeHistoryEvery, numberOfData); cherror
	}
	{
		long idatum = 0;
		double plasticity = initialPlasticity;
		for (long iplasticity = 1; iplasticity <= numberOfPlasticities; iplasticity ++) {
			for (long ireplication = 1; ireplication <= replicationsPerPlasticity; ireplication ++) {
				long ipartialOutput;
				if (! Distributions_peek_opt (thee, columnNumber, & ipartialOutput)) goto end;
				++ idatum;
				if (graphics && idatum % (numberOfData / 400 + 1) == 0) {
					Graphics_setWindow (graphics, 0, numberOfData, 50, 150);
					for (long icons = 1; icons <= 14 && icons <= my numberOfConstraints; icons ++) {
						Graphics_setGrey (graphics, (double) icons / 14);
						Graphics_line (graphics, idatum, my constraints [icons]. ranking,
							idatum, my constraints [icons]. ranking+1);
					}
					Graphics_flushWs (graphics);   /* Because drawing is faster than progress loop. */
				}
				if (! Melder_monitor6 ((double) idatum / numberOfData,
					L"Processing partial output ", Melder_integer (idatum), L" out of ", Melder_integer (numberOfData), L": ",
					thy rowLabels [ipartialOutput]))
				{
					Melder_flushError ("Only %ld partial outputs out of %ld were processed.", idatum - 1, numberOfData);
					goto end;
				}
				OTGrammar_learnOneFromPartialOutput_opt (me, ipartialOutput,
					evaluationNoise, updateRule, honourLocalRankings,
					plasticity, relativePlasticityNoise, numberOfChews, FALSE);   // No warning if stalled: RIP form is allowed to be harmonically bounded.
				if (history) {
					OTGrammar_updateHistory (me, history, storeHistoryEvery, idatum, thy rowLabels [ipartialOutput]);
				}
				cherror
			}
			plasticity *= plasticityDecrement;
		}
	}
end:
	Melder_monitor1 (1.0, NULL);
	OTGrammar_opt_deleteOutputMatching (me);
	if (history_out) *history_out = history;   /* Even (or especially) in case of error, so that we can inspect. */
	iferror return Melder_error1 (L"OTGrammar did not complete learning from partial outputs.");
	if (history) {
		OTGrammar_finalizeHistory (me, history, numberOfData);
	}
	return 1;
}

FORM (OTGrammar_Distributions_learnFromPartialOutputs, L"OTGrammar & Distributions: Learn from partial outputs", L"OT learning 6. Shortcut to OT learning")
	NATURAL (L"Column number", L"1")
	REAL (L"Evaluation noise", L"2.0")
	OPTIONMENU_ENUM (L"Update rule", kOTGrammar_rerankingStrategy, SYMMETRIC_ALL)
	REAL (L"Initial plasticity", L"1.0")
	NATURAL (L"Replications per plasticity", L"100000")
	REAL (L"Plasticity decrement", L"0.1")
	NATURAL (L"Number of plasticities", L"4")
	REAL (L"Rel. plasticity spreading", L"0.1")
	BOOLEAN (L"Honour local rankings", 1)
	NATURAL (L"Number of chews", L"1")
	INTEGER (L"Store history every", L"0")
	OK
DO
	iam_ONLY (OTGrammar);
	thouart_ONLY (Distributions);
	OTHistory history = NULL;
	try {
		OTGrammar_Distributions_learnFromPartialOutputs (me, thee, GET_INTEGER (L"Column number"),
			GET_REAL (L"Evaluation noise"),
			GET_ENUM (kOTGrammar_rerankingStrategy, L"Update rule"),
			GET_INTEGER (L"Honour local rankings"),
			GET_REAL (L"Initial plasticity"), GET_INTEGER (L"Replications per plasticity"),
			GET_REAL (L"Plasticity decrement"), GET_INTEGER (L"Number of plasticities"),
			GET_REAL (L"Rel. plasticity spreading"), GET_INTEGER (L"Number of chews"),
			GET_INTEGER (L"Store history every"), & history);
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (me);
	}
	if (history) praat_new1 (history, my name);
	iferror {
		if (history) praat_updateSelection ();
		return 0;
	}
END

int OTGrammar_Distributions_listObligatoryRankings (OTGrammar me, Distributions thee, long columnNumber) {
	OTGrammarFixedRanking savedFixedRankings;
	long ifixedRanking, icons, jcons, kcons, ipair = 0, npair = my numberOfConstraints * (my numberOfConstraints - 1);
	/*
	 * Save.
	 */
	savedFixedRankings = my fixedRankings;
	OTGrammar_save (me);
	/*
	 * Add room for one more fixed ranking.
	 */
	my numberOfFixedRankings ++;
	my fixedRankings = NUMstructvector (OTGrammarFixedRanking, 1, my numberOfFixedRankings);
	for (ifixedRanking = 1; ifixedRanking < my numberOfFixedRankings; ifixedRanking ++) {
		my fixedRankings [ifixedRanking]. higher = savedFixedRankings [ifixedRanking]. higher;
		my fixedRankings [ifixedRanking]. lower = savedFixedRankings [ifixedRanking]. lower;
	}
	/*
	 * Test learnability of every possible ranked pair.
	 */
	MelderInfo_open ();
	Melder_progress1 (0.0, L"");
	for (icons = 1; icons <= my numberOfConstraints; icons ++) {
		for (jcons = 1; jcons <= my numberOfConstraints; jcons ++) if (icons != jcons) {
			my fixedRankings [my numberOfFixedRankings]. higher = icons;
			my fixedRankings [my numberOfFixedRankings]. lower = jcons;
			OTGrammar_reset (me, 100.0);
			Melder_progress7 ((double) ipair / npair, Melder_integer (ipair + 1), L"/", Melder_integer (npair), L": Trying ranking ",
				my constraints [icons]. name, L" >> ", my constraints [jcons]. name);
			ipair ++;
			Melder_progressOff ();
			OTGrammar_Distributions_learnFromPartialOutputs (me, thee, columnNumber,
				1e-9, kOTGrammar_rerankingStrategy_EDCD, TRUE /* honour fixed rankings; very important */,
				1.0, 1000, 0.0, 1, 0.0, 1, 0, NULL); cherror
			Melder_progressOn ();
			for (kcons = 1; kcons <= my numberOfConstraints; kcons ++) {
				if (my constraints [kcons]. ranking < 0.0) {
					MelderInfo_writeLine3 (my constraints [jcons]. name, L" >> ", my constraints [icons]. name);
					break;
				}
			}
		}
	}
end:
	Melder_progress1 (1.0, L"");
	MelderInfo_close ();
	/*
	 * Remove room.
	 */
	my numberOfFixedRankings --;
	NUMstructvector_free (OTGrammarFixedRanking, my fixedRankings, 1);   // dangle
	/*
	 * Restore.
	 */
	my fixedRankings = savedFixedRankings;   // undangle
	OTGrammar_restore (me);
	iferror return 0;
	return 1;
}

FORM (OTGrammar_Distributions_listObligatoryRankings, L"OTGrammar & Distributions: Get fraction correct...", 0)
	NATURAL (L"Column number", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	thouart_ONLY (Distributions);
	OTGrammar_Distributions_listObligatoryRankings (me, thee, GET_INTEGER (L"Column number")); therror
END

FORM (OTGrammar_PairDistribution_findPositiveWeights, L"OTGrammar & PairDistribution: Find positive weights", L"OTGrammar & PairDistribution: Find positive weights...")
	POSITIVE (L"Weight floor", L"1.0")
	POSITIVE (L"Margin of separation", L"1.0")
	OK
DO
	iam_ONLY (OTGrammar);
	thouart_ONLY (PairDistribution);
	OTGrammar_PairDistribution_findPositiveWeights_e (me, thee,
		GET_REAL (L"Weight floor"), GET_REAL (L"Margin of separation")); therror
	praat_dataChanged (me);
END

FORM (OTGrammar_PairDistribution_getFractionCorrect, L"OTGrammar & PairDistribution: Get fraction correct...", 0)
	REAL (L"Evaluation noise", L"2.0")
	INTEGER (L"Replications", L"100000")
	OK
DO
	iam_ONLY (OTGrammar);
	thouart_ONLY (PairDistribution);
	double result;
	try {
		OTGrammar_PairDistribution_getFractionCorrect (me, thee,
			GET_REAL (L"Evaluation noise"), GET_INTEGER (L"Replications"), & result); therror
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (me);
		throw 1;
	}
	Melder_informationReal (result, NULL);
END

FORM (OTGrammar_PairDistribution_getMinimumNumberCorrect, L"OTGrammar & PairDistribution: Get minimum number correct...", 0)
	REAL (L"Evaluation noise", L"2.0")
	INTEGER (L"Replications per input", L"1000")
	OK
DO
	iam_ONLY (OTGrammar);
	thouart_ONLY (PairDistribution);
	long result;
	try {
		OTGrammar_PairDistribution_getMinimumNumberCorrect (me, thee,
			GET_REAL (L"Evaluation noise"), GET_INTEGER (L"Replications per input"), & result);
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (me);
		throw 1;
	}
	Melder_information1 (Melder_integer (result));
END

int OTGrammar_PairDistribution_learn (OTGrammar me, PairDistribution thee,
	double evaluationNoise, enum kOTGrammar_rerankingStrategy updateRule, int honourLocalRankings,
	double initialPlasticity, long replicationsPerPlasticity, double plasticityDecrement,
	long numberOfPlasticities, double relativePlasticityNoise, long numberOfChews)
{
	long iplasticity, ireplication, ichew, idatum = 0, numberOfData = numberOfPlasticities * replicationsPerPlasticity;
	double plasticity = initialPlasticity;
	Graphics graphics = (Graphics) Melder_monitor1 (0.0, L"Learning with full knowledge...");
	if (graphics) {
		Graphics_clearWs (graphics);
	}
	for (iplasticity = 1; iplasticity <= numberOfPlasticities; iplasticity ++) {
		for (ireplication = 1; ireplication <= replicationsPerPlasticity; ireplication ++) {
			wchar_t *input, *output;
			PairDistribution_peekPair (thee, & input, & output); cherror
			++ idatum;
			if (graphics && idatum % (numberOfData / 400 + 1) == 0) {
				long icons;
				Graphics_setWindow (graphics, 0, numberOfData, 50, 150);
				for (icons = 1; icons <= 14 && icons <= my numberOfConstraints; icons ++) {
					Graphics_setGrey (graphics, (double) icons / 14);
					Graphics_line (graphics, idatum, my constraints [icons]. ranking,
						idatum, my constraints [icons]. ranking+1);
				}
				Graphics_flushWs (graphics);   /* Because drawing is faster than progress loop. */
			}
			if (! Melder_monitor8 ((double) idatum / numberOfData,
				L"Processing input-output pair ", Melder_integer (idatum), L" out of ", Melder_integer (numberOfData), L": ", input, L" -> ", output))
			{
				Melder_flushError ("Only %ld input-output pairs out of %ld were processed.", idatum - 1, numberOfData);
				goto end;
			}
			for (ichew = 1; ichew <= numberOfChews; ichew ++) {
				if (! OTGrammar_learnOne (me, input, output,
					evaluationNoise, updateRule, honourLocalRankings,
					plasticity, relativePlasticityNoise, TRUE, TRUE, NULL)) goto end;
			}
		}
		plasticity *= plasticityDecrement;
	}
end:
	Melder_monitor1 (1.0, NULL);
	iferror return Melder_error1 (L"OTGrammar did not complete learning from input-output pairs.");
	return 1;
}

FORM (OTGrammar_PairDistribution_learn, L"OTGrammar & PairDistribution: Learn", L"OT learning 6. Shortcut to OT learning")
	REAL (L"Evaluation noise", L"2.0")
	OPTIONMENU_ENUM (L"Update rule", kOTGrammar_rerankingStrategy, SYMMETRIC_ALL)
	POSITIVE (L"Initial plasticity", L"1.0")
	NATURAL (L"Replications per plasticity", L"100000")
	REAL (L"Plasticity decrement", L"0.1")
	NATURAL (L"Number of plasticities", L"4")
	REAL (L"Rel. plasticity spreading", L"0.1")
	BOOLEAN (L"Honour local rankings", 1)
	NATURAL (L"Number of chews", L"1")
	OK
DO
	iam_ONLY (OTGrammar);
	thouart_ONLY (PairDistribution);
	try {
		OTGrammar_PairDistribution_learn (me, thee,
			GET_REAL (L"Evaluation noise"), GET_ENUM (kOTGrammar_rerankingStrategy, L"Update rule"), GET_INTEGER (L"Honour local rankings"),
			GET_REAL (L"Initial plasticity"), GET_INTEGER (L"Replications per plasticity"),
			GET_REAL (L"Plasticity decrement"), GET_INTEGER (L"Number of plasticities"),
			GET_REAL (L"Rel. plasticity spreading"), GET_INTEGER (L"Number of chews")); therror
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (me);
		throw 1;
	}
END

DIRECT (OTGrammar_PairDistribution_listObligatoryRankings)
	iam_ONLY (OTGrammar);
	thouart_ONLY (PairDistribution);
	OTGrammar_PairDistribution_listObligatoryRankings (me, thee); therror
END

FORM (OTGrammar_to_Distributions, L"OTGrammar: Compute output distributions", L"OTGrammar: To output Distributions...")
	NATURAL (L"Trials per input", L"100000")
	REAL (L"Evaluation noise", L"2.0")
	OK
DO
	WHERE (SELECTED) try {
		iam_LOOP (OTGrammar);
		autoDistributions thee = OTGrammar_to_Distribution (me, GET_INTEGER (L"Trials per input"), GET_REAL (L"Evaluation noise"));
		praat_new (thee.transfer(), my name, L"_out");
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (OBJECT);
		throw 1;
	}
END

FORM (OTGrammar_to_PairDistribution, L"OTGrammar: Compute output distributions", 0)
	NATURAL (L"Trials per input", L"100000")
	REAL (L"Evaluation noise", L"2.0")
	OK
DO
	WHERE (SELECTED) try {
		iam_LOOP (OTGrammar);
		autoPairDistribution thee = OTGrammar_to_PairDistribution (me, GET_INTEGER (L"Trials per input"), GET_REAL (L"Evaluation noise"));
		praat_new (thee.transfer(), my name, L"_out");
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (OBJECT);
		throw 1;
	}
END

DIRECT (OTGrammar_measureTypology)
	WHERE (SELECTED) try {
		iam_LOOP (OTGrammar);
		autoDistributions thee = OTGrammar_measureTypology (me);
		praat_new (thee.transfer(), my name, L"_out");
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (OBJECT);
		throw 1;
	}
END

FORM_WRITE (OTGrammar_writeToHeaderlessSpreadsheetFile, L"Write OTGrammar to spreadsheet", 0, L"txt")
	iam_ONLY (OTGrammar);
	OTGrammar_writeToHeaderlessSpreadsheetFile (me, file); therror
END

static long OTMulti_crucialCell (OTMulti me, long icand, long iwinner, long numberOfOptimalCandidates, const wchar_t *form1, const wchar_t *form2)
{
	long icons;
	if (my numberOfCandidates < 2) return 0;   // if there is only one candidate, all cells can be greyed
	if (OTMulti_compareCandidates (me, icand, iwinner) == 0)   // candidate equally good as winner?
	{
		if (numberOfOptimalCandidates > 1)
		{
			/* All cells are important. */
		}
		else
		{
			long jcand, secondBest = 0;
			for (jcand = 1; jcand <= my numberOfCandidates; jcand ++) {
				if (OTMulti_candidateMatches (me, jcand, form1, form2) && OTMulti_compareCandidates (me, jcand, iwinner) != 0)   // a non-optimal candidate?
				{
					if (secondBest == 0)
					{
						secondBest = jcand;   // first guess
					}
					else if (OTMulti_compareCandidates (me, jcand, secondBest) < 0)
					{
						secondBest = jcand;   // better guess
					}
				}
			}
			if (secondBest == 0) return 0;   // if all candidates are equally good, all cells can be greyed
			return OTMulti_crucialCell (me, secondBest, iwinner, 1, form1, form2);
		}
	}
	else
	{
		int *candidateMarks = my candidates [icand]. marks;
		int *winnerMarks = my candidates [iwinner]. marks;
		for (icons = 1; icons <= my numberOfConstraints; icons ++)
		{
			int numberOfCandidateMarks = candidateMarks [my index [icons]];
			int numberOfWinnerMarks = winnerMarks [my index [icons]];
			if (numberOfCandidateMarks > numberOfWinnerMarks)
			{
				return icons;
			}
		}
	}
	return my numberOfConstraints;   /* Nothing grey. */
}

static double OTMulti_constraintWidth (Graphics g, OTConstraint constraint, int showDisharmony) {
	wchar_t text [100], *newLine;
	double maximumWidth = showDisharmony ? 0.8 * Graphics_textWidth_ps (g, Melder_fixed (constraint -> disharmony, 1), TRUE) : 0.0,
		firstWidth, secondWidth;
	wcscpy (text, constraint -> name);
	newLine = wcschr (text, '\n');
	if (newLine) {
		*newLine = '\0';
		firstWidth = Graphics_textWidth_ps (g, text, true);
		if (firstWidth > maximumWidth) maximumWidth = firstWidth;
		secondWidth = Graphics_textWidth_ps (g, newLine + 1, true);
		if (secondWidth > maximumWidth) maximumWidth = secondWidth;
		return maximumWidth;
	}
	firstWidth = Graphics_textWidth_ps (g, text, true);
	if (firstWidth > maximumWidth) maximumWidth = firstWidth;
	return maximumWidth;
}

void OTMulti_drawTableau (OTMulti me, Graphics g, const wchar_t *form1, const wchar_t *form2, int showDisharmonies) {
	long winner, winner1 = 0, winner2 = 0, numberOfMatchingCandidates;
	long numberOfOptimalCandidates, numberOfOptimalCandidates1, numberOfOptimalCandidates2;
	double candWidth, margin, fingerWidth, doubleLineDx, doubleLineDy;
	double tableauWidth, rowHeight, headerHeight, descent, x, y, fontSize = Graphics_inqFontSize (g);
	Graphics_Colour colour = Graphics_inqColour (g);
	wchar_t text [200];
	int bidirectional = form1 [0] != '\0' && form2 [0] != '\0';
	winner = OTMulti_getWinner (me, form1, form2);
	if (winner == 0) {
		Melder_clearError ();
		Graphics_setWindow (g, 0.0, 1.0, 0.0, 1.0);
		Graphics_setTextAlignment (g, Graphics_LEFT, Graphics_HALF);
		Graphics_rectangle (g, 0, 1, 0, 1);
		Graphics_text (g, 0.0, 0.5, L"(no matching candidates)");
		return;
	}

	if (bidirectional) {
		winner1 = OTMulti_getWinner (me, form1, L"");
		winner2 = OTMulti_getWinner (me, form2, L"");
	}
	Graphics_setWindow (g, 0.0, 1.0, 0.0, 1.0);
	margin = Graphics_dxMMtoWC (g, 1.0);
	fingerWidth = Graphics_dxMMtoWC (g, 7.0) * fontSize / 12.0;
	doubleLineDx = Graphics_dxMMtoWC (g, 0.9);
	doubleLineDy = Graphics_dyMMtoWC (g, 0.9);
	rowHeight = Graphics_dyMMtoWC (g, 1.5 * fontSize * 25.4 / 72);
	descent = rowHeight * 0.5;
	/*
	 * Compute height of header row.
	 */
	headerHeight = rowHeight;
	for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
		OTConstraint constraint = & my constraints [icons];
		if (wcschr (constraint -> name, '\n')) {
			headerHeight += 0.7 * rowHeight;
			break;
		}
	}
	/*
	 * Compute longest candidate string.
	 * Also count the number of optimal candidates (if there are more than one, the fingers will be drawn in red).
	 */
	candWidth = Graphics_textWidth_ps (g, form1, TRUE) + Graphics_textWidth_ps (g, form2, true);
	numberOfMatchingCandidates = 0;
	numberOfOptimalCandidates = numberOfOptimalCandidates1 = numberOfOptimalCandidates2 = 0;
	for (long icand = 1; icand <= my numberOfCandidates; icand ++) {
		if ((form1 [0] != '\0' && OTMulti_candidateMatches (me, icand, form1, L"")) ||
		    (form2 [0] != '\0' && OTMulti_candidateMatches (me, icand, form2, L"")) ||
		    (form1 [0] == '\0' && form2 [0] == '\0'))
		{
			double width = Graphics_textWidth_ps (g, my candidates [icand]. string, true);
			if (width > candWidth) candWidth = width;
			numberOfMatchingCandidates ++;
			if (OTMulti_compareCandidates (me, icand, winner) == 0) {
				numberOfOptimalCandidates ++;
			}
			if (winner1 != 0 && OTMulti_compareCandidates (me, icand, winner1) == 0) {
				numberOfOptimalCandidates1 ++;
			}
			if (winner2 != 0 && OTMulti_compareCandidates (me, icand, winner2) == 0) {
				numberOfOptimalCandidates2 ++;
			}
		}
	}
	candWidth += fingerWidth * (bidirectional ? 3 : 1) + margin * 3;
	/*
	 * Compute tableau width.
	 */
	tableauWidth = candWidth + doubleLineDx;
	for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
		OTConstraint constraint = & my constraints [icons];
		tableauWidth += OTMulti_constraintWidth (g, constraint, showDisharmonies);
	}
	tableauWidth += margin * 2 * my numberOfConstraints;
	/*
	 * Draw box.
	 */
	x = doubleLineDx;   /* Left side of tableau. */
	y = 1.0 - doubleLineDy;
	if (showDisharmonies) y -= 0.6 * rowHeight;
	Graphics_rectangle (g, x, x + tableauWidth,
		y - headerHeight - numberOfMatchingCandidates * rowHeight - doubleLineDy, y);
	/*
	 * Draw input.
	 */
	y -= headerHeight;
	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	Graphics_text2 (g, x + 0.5 * candWidth, y + 0.5 * headerHeight, form1, form2);
	Graphics_rectangle (g, x, x + candWidth, y, y + headerHeight);
	/*
	 * Draw constraint names.
	 */
	x += candWidth + doubleLineDx;
	for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
		OTConstraint constraint = & my constraints [my index [icons]];
		double width = OTMulti_constraintWidth (g, constraint, showDisharmonies) + margin * 2;
		if (wcschr (constraint -> name, '\n')) {
			wchar_t *newLine;
			wcscpy (text, constraint -> name);
			newLine = wcschr (text, '\n');
			*newLine = '\0';
			Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_TOP);
			Graphics_text (g, x + 0.5 * width, y + headerHeight, text);
			Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_BOTTOM);
			Graphics_text (g, x + 0.5 * width, y, newLine + 1);
		} else {
			Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
			Graphics_text (g, x + 0.5 * width, y + 0.5 * headerHeight, constraint -> name);
		}
		if (showDisharmonies) {
			Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_BOTTOM);
			Graphics_setFontSize (g, 0.8 * fontSize);
			Graphics_text (g, x + 0.5 * width, y + headerHeight, Melder_fixed (constraint -> disharmony, 1));
			Graphics_setFontSize (g, fontSize);
		}
		Graphics_line (g, x, y, x, y + headerHeight);
		Graphics_line (g, x, y, x + width, y);
		x += width;
	}
	/*
	 * Draw candidates.
	 */
	y -= doubleLineDy;
	for (long icand = 1; icand <= my numberOfCandidates; icand ++)
		if ((form1 [0] != '\0' && OTMulti_candidateMatches (me, icand, form1, L"")) ||
		    (form2 [0] != '\0' && OTMulti_candidateMatches (me, icand, form2, L"")) ||
		    (form1 [0] == '\0' && form2 [0] == '\0'))
	{
		long crucialCell = OTMulti_crucialCell (me, icand, winner, numberOfOptimalCandidates, form1, form2);
		int candidateIsOptimal = OTMulti_compareCandidates (me, icand, winner) == 0;
		int candidateIsOptimal1 = winner1 != 0 && OTMulti_compareCandidates (me, icand, winner1) == 0;
		int candidateIsOptimal2 = winner2 != 0 && OTMulti_compareCandidates (me, icand, winner2) == 0;
		/*
		 * Draw candidate transcription.
		 */
		x = doubleLineDx;
		y -= rowHeight;
		Graphics_setTextAlignment (g, Graphics_RIGHT, Graphics_HALF);
		Graphics_text (g, x + candWidth - margin, y + descent, my candidates [icand]. string);
		if (candidateIsOptimal) {
			Graphics_setTextAlignment (g, Graphics_LEFT, Graphics_HALF);
			Graphics_setFontSize (g, (int) ((bidirectional ? 1.2 : 1.5) * fontSize));
			if (numberOfOptimalCandidates > 1) Graphics_setColour (g, Graphics_RED);
			Graphics_text (g, x + margin, y + descent - Graphics_dyMMtoWC (g, 0.5) * fontSize / 12.0, bidirectional ? L"\\Vr" : L"\\pf");
			Graphics_setColour (g, colour);
			Graphics_setFontSize (g, (int) fontSize);
		}
		if (candidateIsOptimal1) {
			Graphics_setTextAlignment (g, Graphics_LEFT, Graphics_HALF);
			Graphics_setFontSize (g, (int) (1.5 * fontSize));
			if (numberOfOptimalCandidates1 > 1) Graphics_setColour (g, Graphics_RED);
			Graphics_text (g, x + margin + fingerWidth, y + descent - Graphics_dyMMtoWC (g, 0.5) * fontSize / 12.0, L"\\pf");
			Graphics_setColour (g, colour);
			Graphics_setFontSize (g, (int) fontSize);
		}
		if (candidateIsOptimal2) {
			Graphics_setTextAlignment (g, Graphics_RIGHT, Graphics_HALF);
			Graphics_setFontSize (g, (int) (1.5 * fontSize));
			if (numberOfOptimalCandidates2 > 1) Graphics_setColour (g, Graphics_RED);
			Graphics_setTextRotation (g, 180);
			Graphics_text (g, x + margin + fingerWidth * 2, y + descent - Graphics_dyMMtoWC (g, 0.0) * fontSize / 12.0, L"\\pf");
			Graphics_setTextRotation (g, 0);
			Graphics_setColour (g, colour);
			Graphics_setFontSize (g, (int) fontSize);
		}
		Graphics_rectangle (g, x, x + candWidth, y, y + rowHeight);
		/*
		 * Draw grey cell backgrounds.
		 */
		if (! bidirectional && my decisionStrategy == kOTGrammar_decisionStrategy_OPTIMALITY_THEORY) {
			x = candWidth + 2 * doubleLineDx;
			Graphics_setGrey (g, 0.9);
			for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
				int index = my index [icons];
				OTConstraint constraint = & my constraints [index];
				double width = OTMulti_constraintWidth (g, constraint, showDisharmonies) + margin * 2;
				if (icons > crucialCell)
					Graphics_fillRectangle (g, x, x + width, y, y + rowHeight);
				x += width;
			}
			Graphics_setColour (g, colour);
		}
		/*
		 * Draw cell marks.
		 */
		x = candWidth + 2 * doubleLineDx;
		Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
		for (long icons = 1; icons <= my numberOfConstraints; icons ++) {
			int index = my index [icons];
			OTConstraint constraint = & my constraints [index];
			double width = OTMulti_constraintWidth (g, constraint, showDisharmonies) + margin * 2;
			wchar_t markString [40];
			markString [0] = '\0';
			if (bidirectional && my candidates [icand]. marks [index] > 0) {
				if ((candidateIsOptimal1 || candidateIsOptimal2) && ! candidateIsOptimal) {
					wcscat (markString, L"\\<-");
				}
			}
			if (bidirectional && my candidates [icand]. marks [index] < 0) {
				if (candidateIsOptimal && ! candidateIsOptimal1) {
					wcscat (markString, L"\\<-");
				}
				if (candidateIsOptimal && ! candidateIsOptimal2) {
					wcscat (markString, L"\\<-");
				}
			}
			/*
			 * An exclamation mark can be drawn in this cell only if both of the following conditions are met:
			 * 1. the candidate is not optimal;
			 * 2. this is the crucial cell, i.e. the cells after it are drawn in grey.
			 */
			if (! bidirectional && icons == crucialCell && ! candidateIsOptimal &&
			    my decisionStrategy == kOTGrammar_decisionStrategy_OPTIMALITY_THEORY)
			{
				int winnerMarks = my candidates [winner]. marks [index];
				if (winnerMarks + 1 > 5) {
					wcscat (markString, Melder_integer (winnerMarks + 1));
				} else {
					for (long imark = 1; imark <= winnerMarks + 1; imark ++)
						wcscat (markString, L"*");
				}
				for (long imark = my candidates [icand]. marks [index]; imark < 0; imark ++)
					wcscat (markString, L"+");
				wcscat (markString, L"!");
				if (my candidates [icand]. marks [index] - (winnerMarks + 2) + 1 > 5) {
					wcscat (markString, Melder_integer (my candidates [icand]. marks [index] - (winnerMarks + 2) + 1));
				} else {
					for (long imark = winnerMarks + 2; imark <= my candidates [icand]. marks [index]; imark ++)
						wcscat (markString, L"*");
				}
			} else {
				if (my candidates [icand]. marks [index] > 5) {
					wcscat (markString, Melder_integer (my candidates [icand]. marks [index]));
				} else {
					for (long imark = 1; imark <= my candidates [icand]. marks [index]; imark ++)
						wcscat (markString, L"*");
					for (long imark = my candidates [icand]. marks [index]; imark < 0; imark ++)
						wcscat (markString, L"+");
				}
			}
			if (bidirectional && my candidates [icand]. marks [index] > 0) {
				if (candidateIsOptimal && ! candidateIsOptimal1) {
					wcscat (markString, L"\\->");
				}
				if (candidateIsOptimal && ! candidateIsOptimal2) {
					wcscat (markString, L"\\->");
				}
			}
			if (bidirectional && my candidates [icand]. marks [index] < 0) {
				if ((candidateIsOptimal1 || candidateIsOptimal2) && ! candidateIsOptimal) {
					wcscat (markString, L"\\->");
				}
			}
			Graphics_text (g, x + 0.5 * width, y + descent, markString);
			Graphics_setColour (g, colour);
			Graphics_line (g, x, y, x, y + rowHeight);
			Graphics_line (g, x, y + rowHeight, x + width, y + rowHeight);
			x += width;
		}
	}
	/*
	 * Draw box.
	 */
	x = doubleLineDx;   /* Left side of tableau. */
	y = 1.0 - doubleLineDy;
	if (showDisharmonies) y -= 0.6 * rowHeight;
	Graphics_rectangle (g, x, x + tableauWidth,
		y - headerHeight - numberOfMatchingCandidates * rowHeight - doubleLineDy, y);
}

FORM (OTMulti_drawTableau, L"Draw tableau", L"OT learning")
	SENTENCE (L"Partial form 1", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"Partial form 2", L"ui/editors/AmplitudeTierEditor.h")
	BOOLEAN (L"Show disharmonies", 1)
	OK
DO
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (OTMulti);
		OTMulti_drawTableau (me, GRAPHICS, GET_STRING (L"Partial form 1"), GET_STRING (L"Partial form 2"),
			GET_INTEGER (L"Show disharmonies"));
	}
END

DIRECT (OTMulti_edit)
	if (theCurrentPraatApplication -> batch) Melder_throw ("Cannot edit an OTMulti from batch.");
	WHERE (SELECTED) {
		iam_LOOP (OTMulti);
		OTMultiEditor *editor = new OTMultiEditor (theCurrentPraatApplication -> topShell, ID_AND_FULL_NAME, me);
		praat_installEditor (editor, IOBJECT); therror
	}
END

FORM (OTMulti_evaluate, L"OTMulti: Evaluate", 0)
	REAL (L"Evaluation noise", L"2.0")
	OK
DO
	iam_ONLY (OTMulti);
	OTMulti_newDisharmonies (me, GET_REAL (L"Evaluation noise"));
	praat_dataChanged (me);
END

FORM (OTMulti_generateOptimalForms, L"OTMulti: Generate optimal forms", 0)
	SENTENCE (L"Partial form 1", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"Partial form 2", L"ui/editors/AmplitudeTierEditor.h")
	NATURAL (L"Number of trials", L"1000")
	REAL (L"Evaluation noise", L"2.0")
	OK
DO
	iam_ONLY (OTMulti);
	autoStrings thee = OTMulti_generateOptimalForms (me, GET_STRING (L"Partial form 1"), GET_STRING (L"Partial form 2"),
		GET_INTEGER (L"Number of trials"), GET_REAL (L"Evaluation noise"));
	praat_new (thee.transfer(), my name, L"_out");
	praat_dataChanged (me);
END

FORM (OTMulti_getCandidate, L"Get candidate", 0)
	NATURAL (L"Candidate", L"1")
	OK
DO
	iam_ONLY (OTMulti);
	long icand = GET_INTEGER (L"Candidate");
	if (icand > my numberOfCandidates)
		Melder_throw ("The specified candidate number should not exceed the number of candidates.");
	Melder_information1 (my candidates [icand]. string);
END

FORM (OTMulti_getConstraint, L"Get constraint name", 0)
	NATURAL (L"Constraint number", L"1")
	OK
DO
	iam_ONLY (OTMulti);
	long icons = GET_INTEGER (L"Constraint number");
	if (icons > my numberOfConstraints)
		Melder_throw ("The specified constraint number should not exceed the number of constraints.");
	Melder_information1 (my constraints [icons]. name);
END

FORM (OTMulti_getConstraintIndexFromName, L"OTMulti: Get constraint number", 0)
	SENTENCE (L"Constraint name", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (OTMulti);
	Melder_information1 (Melder_integer (OTMulti_getConstraintIndexFromName (me, GET_STRING (L"Constraint name"))));
END

FORM (OTMulti_getDisharmony, L"Get disharmony", 0)
	NATURAL (L"Constraint number", L"1")
	OK
DO
	iam_ONLY (OTMulti);
	long icons = GET_INTEGER (L"Constraint number");
	if (icons > my numberOfConstraints)
		Melder_throw ("The specified constraint number should not exceed the number of constraints.");
	Melder_information1 (Melder_double (my constraints [icons]. disharmony));
END

DIRECT (OTMulti_getNumberOfCandidates)
	iam_ONLY (OTMulti);
	Melder_information1 (Melder_integer (my numberOfCandidates));
END

DIRECT (OTMulti_getNumberOfConstraints)
	iam_ONLY (OTMulti);
	Melder_information1 (Melder_integer (my numberOfConstraints));
END

FORM (OTMulti_getNumberOfViolations, L"Get number of violations", 0)
	NATURAL (L"Candidate number", L"1")
	NATURAL (L"Constraint number", L"1")
	OK
DO
	iam_ONLY (OTMulti);
	long icand = GET_INTEGER (L"Candidate number");
	if (icand > my numberOfCandidates)
		Melder_throw ("The specified candidate number should not exceed the number of candidates.");
	long icons = GET_INTEGER (L"Constraint number");
	if (icons > my numberOfConstraints)
		Melder_throw ("The specified constraint number should not exceed the number of constraints.");
	Melder_information1 (Melder_integer (my candidates [icand]. marks [icons]));
END

FORM (OTMulti_getRankingValue, L"Get ranking value", 0)
	NATURAL (L"Constraint number", L"1")
	OK
DO
	iam_ONLY (OTMulti);
	long icons = GET_INTEGER (L"Constraint number");
	if (icons > my numberOfConstraints)
		Melder_throw ("The specified constraint number should not exceed the number of constraints.");
	Melder_information1 (Melder_double (my constraints [icons]. ranking));
END

FORM (OTMulti_getWinner, L"OTMulti: Get winner", 0)
	SENTENCE (L"Partial form 1", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"Partial form 2", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	iam_ONLY (OTMulti);
	Melder_information1 (Melder_integer (OTMulti_getWinner (me, GET_STRING (L"Partial form 1"), GET_STRING (L"Partial form 2"))));
END

FORM (OTMulti_generateOptimalForm, L"OTMulti: Generate optimal form", 0)
	SENTENCE (L"Partial form 1", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"Partial form 2", L"ui/editors/AmplitudeTierEditor.h")
	REAL (L"Evaluation noise", L"2.0")
	OK
DO
	iam_ONLY (OTMulti);
	wchar_t output [100];
	OTMulti_generateOptimalForm (me, GET_STRING (L"Partial form 1"), GET_STRING (L"Partial form 2"),
		output, GET_REAL (L"Evaluation noise")); therror
	Melder_information1 (output);
	praat_dataChanged (me);
END

FORM (OTMulti_learnOne, L"OTMulti: Learn one", 0)
	SENTENCE (L"Partial form 1", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"Partial form 2", L"ui/editors/AmplitudeTierEditor.h")
	OPTIONMENU_ENUM (L"Update rule", kOTGrammar_rerankingStrategy, SYMMETRIC_ALL)
	OPTIONMENU (L"Direction", 3)
		OPTION (L"forward")
		OPTION (L"backward")
		OPTION (L"bidirectionally")
	POSITIVE (L"Plasticity", L"0.1")
	REAL (L"Rel. plasticity spreading", L"0.1")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTMulti);
		try {
			OTMulti_learnOne (me, GET_STRING (L"Partial form 1"), GET_STRING (L"Partial form 2"),
				GET_ENUM (kOTGrammar_rerankingStrategy, L"Update rule"),
				GET_INTEGER (L"Direction"), GET_REAL (L"Plasticity"), GET_REAL (L"Rel. plasticity spreading")); therror
			praat_dataChanged (me);
		} catch (...) {
			praat_dataChanged (me);
		}
	}
END

FORM (OTMulti_removeConstraint, L"OTMulti: Remove constraint", 0)
	SENTENCE (L"Constraint name", L"ui/editors/AmplitudeTierEditor.h")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTMulti);
		OTMulti_removeConstraint (me, GET_STRING (L"Constraint name")); therror
		praat_dataChanged (me);
	}
END

FORM (OTMulti_resetAllRankings, L"OTMulti: Reset all rankings", 0)
	REAL (L"Ranking", L"100.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTMulti);
		OTMulti_reset (me, GET_REAL (L"Ranking")); therror
		praat_dataChanged (me);
	}
END

FORM (OTMulti_setConstraintPlasticity, L"OTMulti: Set constraint plasticity", 0)
	NATURAL (L"Constraint", L"1")
	REAL (L"Plasticity", L"1.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTMulti);
		OTMulti_setConstraintPlasticity (me, GET_INTEGER (L"Constraint"), GET_REAL (L"Plasticity")); therror
		praat_dataChanged (me);
	}
END

FORM (OTMulti_setDecisionStrategy, L"OTMulti: Set decision strategy", 0)
	RADIO_ENUM (L"Decision strategy", kOTGrammar_decisionStrategy, DEFAULT)
	OK
iam_ONLY (OTMulti);
SET_ENUM (L"Decision strategy", kOTGrammar_decisionStrategy, my decisionStrategy);
DO
	iam_ONLY (OTMulti);
	my decisionStrategy = GET_ENUM (kOTGrammar_decisionStrategy, L"Decision strategy");
	praat_dataChanged (me);
END

FORM (OTMulti_setLeak, L"OTGrammar: Set leak", 0)
	REAL (L"Leak", L"0.0")
	OK
iam_ONLY (OTMulti);
SET_REAL (L"Leak", my leak);
DO
	iam_ONLY (OTMulti);
	my leak = GET_REAL (L"Leak");
	praat_dataChanged (me);
END

FORM (OTMulti_setRanking, L"OTMulti: Set ranking", 0)
	NATURAL (L"Constraint", L"1")
	REAL (L"Ranking", L"100.0")
	REAL (L"Disharmony", L"100.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTMulti);
		OTMulti_setRanking (me, GET_INTEGER (L"Constraint"), GET_REAL (L"Ranking"), GET_REAL (L"Disharmony")); therror
		praat_dataChanged (me);
	}
END

FORM (OTMulti_to_Distribution, L"OTMulti: Compute output distribution", 0)
	SENTENCE (L"Partial form 1", L"ui/editors/AmplitudeTierEditor.h")
	SENTENCE (L"Partial form 2", L"ui/editors/AmplitudeTierEditor.h")
	NATURAL (L"Number of trials", L"100000")
	POSITIVE (L"Evaluation noise", L"2.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (OTMulti);
		try {
			autoDistributions thee = OTMulti_to_Distribution (me, GET_STRING (L"Partial form 1"), GET_STRING (L"Partial form 2"),
			GET_INTEGER (L"Number of trials"), GET_REAL (L"Evaluation noise"));
			praat_new (thee.transfer(), my name, L"_out");
			praat_dataChanged (me);
		} catch (...) {
			praat_dataChanged (me);
			throw 1;
		}
	}
END

int OTMulti_PairDistribution_learn (OTMulti me, PairDistribution thee, double evaluationNoise, enum kOTGrammar_rerankingStrategy updateRule, int direction,
	double initialPlasticity, long replicationsPerPlasticity, double plasticityDecrement,
	long numberOfPlasticities, double relativePlasticityNoise, long storeHistoryEvery, Table *history_out)
{
	long idatum = 0, numberOfData = numberOfPlasticities * replicationsPerPlasticity;
	double plasticity = initialPlasticity;
	Table history = NULL;
	Graphics graphics = (Graphics) Melder_monitor1 (0.0, L"Learning with full knowledge...");
	if (graphics) {
		Graphics_clearWs (graphics);
	}
	if (storeHistoryEvery) {
		history = OTMulti_createHistory (me, storeHistoryEvery, numberOfData); cherror
	}
	for (long iplasticity = 1; iplasticity <= numberOfPlasticities; iplasticity ++) {
		for (long ireplication = 1; ireplication <= replicationsPerPlasticity; ireplication ++) {
			wchar_t *form1, *form2;
			PairDistribution_peekPair (thee, & form1, & form2); cherror
			++ idatum;
			if (graphics && idatum % (numberOfData / 400 + 1) == 0)
			{
				long numberOfDrawnConstraints = my numberOfConstraints < 14 ? my numberOfConstraints : 14;
				if (numberOfDrawnConstraints > 0)
				{
					double sumOfRankings = 0.0;
					for (long icons = 1; icons <= numberOfDrawnConstraints; icons ++)
					{
						sumOfRankings += my constraints [icons]. ranking;
					}
					double meanRanking = sumOfRankings / numberOfDrawnConstraints;
					Graphics_setWindow (graphics, 0, numberOfData, meanRanking - 50, meanRanking + 50);
					for (long icons = 1; icons <= numberOfDrawnConstraints; icons ++)
					{
						Graphics_setGrey (graphics, (double) icons / numberOfDrawnConstraints);
						Graphics_line (graphics, idatum, my constraints [icons]. ranking,
							idatum, my constraints [icons]. ranking+1);
					}
					Graphics_flushWs (graphics);   /* Because drawing is faster than progress loop. */
				}
			}
			if (! Melder_monitor8 ((double) idatum / numberOfData,
				L"Processing partial pair ", Melder_integer (idatum), L" out of ", Melder_integer (numberOfData),
					L":\n      ", form1, L"     ", form2))
			{
				Melder_flushError ("Only %ld partial pairs out of %ld were processed.", idatum - 1, numberOfData);
				goto end;
			}
			OTMulti_newDisharmonies (me, evaluationNoise);
			if (! OTMulti_learnOne (me, form1, form2, updateRule, direction, plasticity, relativePlasticityNoise)) goto end;
			if (history)
			{
				OTMulti_updateHistory (me, history, storeHistoryEvery, idatum, form1, form2);
			}
		}
		plasticity *= plasticityDecrement;
	}
end:
	Melder_monitor1 (1.0, NULL);
	iferror return Melder_error1 (L"OTMulti did not complete learning from partial pairs.");
	*history_out = history;
	return 1;
}

FORM (OTMulti_PairDistribution_learn, L"OTMulti & PairDistribution: Learn", 0)
	REAL (L"Evaluation noise", L"2.0")
	OPTIONMENU_ENUM (L"Update rule", kOTGrammar_rerankingStrategy, SYMMETRIC_ALL)
	OPTIONMENU (L"Direction", 3)
		OPTION (L"forward")
		OPTION (L"backward")
		OPTION (L"bidirectionally")
	POSITIVE (L"Initial plasticity", L"1.0")
	NATURAL (L"Replications per plasticity", L"100000")
	REAL (L"Plasticity decrement", L"0.1")
	NATURAL (L"Number of plasticities", L"4")
	REAL (L"Rel. plasticity spreading", L"0.1")
	INTEGER (L"Store history every", L"0")
	OK
DO
	iam_ONLY (OTMulti);
	thouart_ONLY (PairDistribution);
	Table history = NULL;
	try {
		OTMulti_PairDistribution_learn (me, thee,
			GET_REAL (L"Evaluation noise"),
			GET_ENUM (kOTGrammar_rerankingStrategy, L"Update rule"),
			GET_INTEGER (L"Direction"),
			GET_REAL (L"Initial plasticity"), GET_INTEGER (L"Replications per plasticity"),
			GET_REAL (L"Plasticity decrement"), GET_INTEGER (L"Number of plasticities"),
			GET_REAL (L"Rel. plasticity spreading"),
			GET_INTEGER (L"Store history every"), & history); therror
		praat_dataChanged (me);
	} catch (...) {
		praat_dataChanged (me);   // e.g. in case of partial learning
		// trickle down to save history
	}
	if (history) praat_new (history, my name);
END

FORM (OTMulti_Strings_generateOptimalForms, L"OTGrammar: Inputs to outputs", L"OTGrammar: Inputs to outputs...")
	REAL (L"Evaluation noise", L"2.0")
	OK
DO
	iam_ONLY (OTMulti);
	thouart_ONLY (Strings);
	autoStrings him = OTMulti_Strings_generateOptimalForms (me, thee, GET_REAL (L"Evaluation noise"));
	praat_new (him.transfer(), my name, L"_out");
	praat_dataChanged (me);
END

/***** buttons *****/

extern "C" void praat_TableOfReal_init (void *klas);

extern "C" void praat_uvafon_gram_init (void);
void praat_uvafon_gram_init (void) {
	Thing_recognizeClassesByName (classNetwork, classOTGrammar, classOTHistory, classOTMulti, NULL);
	Thing_recognizeClassByOtherName (classOTGrammar, L"OTCase");

	praat_addMenuCommand (L"Objects", L"New", L"Constraint grammars", 0, 0, 0);
		praat_addMenuCommand (L"Objects", L"New", L"OT learning tutorial", 0, 1, DO_OT_learning_tutorial);
		praat_addMenuCommand (L"Objects", L"New", L"-- tableau grammars --", 0, 1, 0);
		praat_addMenuCommand (L"Objects", L"New", L"Create NoCoda grammar", 0, 1, DO_Create_NoCoda_grammar);
		praat_addMenuCommand (L"Objects", L"New", L"Create place assimilation grammar", 0, 1, DO_Create_NPA_grammar);
		praat_addMenuCommand (L"Objects", L"New", L"Create place assimilation distribution", 0, 1, DO_Create_NPA_distribution);
		praat_addMenuCommand (L"Objects", L"New", L"Create tongue-root grammar...", 0, 1, DO_Create_tongue_root_grammar);
		praat_addMenuCommand (L"Objects", L"New", L"Create metrics grammar...", 0, 1, DO_Create_metrics_grammar);

	praat_addAction1 (classOTGrammar, 0, L"OTGrammar help", 0, 0, DO_OTGrammar_help);
	praat_addAction1 (classOTGrammar, 0, L"View & Edit", 0, praat_ATTRACTIVE, DO_OTGrammar_edit);
	praat_addAction1 (classOTGrammar, 0, L"Edit", 0, praat_HIDDEN, DO_OTGrammar_edit);
	praat_addAction1 (classOTGrammar, 0, L"Draw tableau...", 0, 0, DO_OTGrammar_drawTableau);
	praat_addAction1 (classOTGrammar, 1, L"Save as headerless spreadsheet file...", 0, 0, DO_OTGrammar_writeToHeaderlessSpreadsheetFile);
	praat_addAction1 (classOTGrammar, 1, L"Write to headerless spreadsheet file...", 0, praat_HIDDEN, DO_OTGrammar_writeToHeaderlessSpreadsheetFile);
	praat_addAction1 (classOTGrammar, 0, L"Query -", 0, 0, 0);
	praat_addAction1 (classOTGrammar, 1, L"Get number of constraints", 0, 1, DO_OTGrammar_getNumberOfConstraints);
	praat_addAction1 (classOTGrammar, 1, L"Get constraint...", 0, 1, DO_OTGrammar_getConstraint);
	praat_addAction1 (classOTGrammar, 1, L"Get ranking value...", 0, 1, DO_OTGrammar_getRankingValue);
	praat_addAction1 (classOTGrammar, 1, L"Get disharmony...", 0, 1, DO_OTGrammar_getDisharmony);
	praat_addAction1 (classOTGrammar, 1, L"Get number of tableaus", 0, 1, DO_OTGrammar_getNumberOfTableaus);
	praat_addAction1 (classOTGrammar, 1, L"Get input...", 0, 1, DO_OTGrammar_getInput);
	praat_addAction1 (classOTGrammar, 1, L"Get number of candidates...", 0, 1, DO_OTGrammar_getNumberOfCandidates);
	praat_addAction1 (classOTGrammar, 1, L"Get candidate...", 0, 1, DO_OTGrammar_getCandidate);
	praat_addAction1 (classOTGrammar, 1, L"Get number of violations...", 0, 1, DO_OTGrammar_getNumberOfViolations);
	praat_addAction1 (classOTGrammar, 1, L"-- parse --", 0, 1, 0);
	praat_addAction1 (classOTGrammar, 1, L"Get winner...", 0, 1, DO_OTGrammar_getWinner);
	praat_addAction1 (classOTGrammar, 1, L"Compare candidates...", 0, 1, DO_OTGrammar_compareCandidates);
	praat_addAction1 (classOTGrammar, 1, L"Get number of optimal candidates...", 0, 1, DO_OTGrammar_getNumberOfOptimalCandidates);
	praat_addAction1 (classOTGrammar, 1, L"Is candidate grammatical...", 0, 1, DO_OTGrammar_isCandidateGrammatical);
	praat_addAction1 (classOTGrammar, 1, L"Is candidate singly grammatical...", 0, 1, DO_OTGrammar_isCandidateSinglyGrammatical);
	praat_addAction1 (classOTGrammar, 1, L"Get interpretive parse...", 0, 1, DO_OTGrammar_getInterpretiveParse);
	praat_addAction1 (classOTGrammar, 1, L"Is partial output grammatical...", 0, 1, DO_OTGrammar_isPartialOutputGrammatical);
	praat_addAction1 (classOTGrammar, 1, L"Is partial output singly grammatical...", 0, 1, DO_OTGrammar_isPartialOutputSinglyGrammatical);
	praat_addAction1 (classOTGrammar, 0, L"Generate inputs...", 0, 0, DO_OTGrammar_generateInputs);
	praat_addAction1 (classOTGrammar, 0, L"Get inputs", 0, 0, DO_OTGrammar_getInputs);
	praat_addAction1 (classOTGrammar, 0, L"Measure typology", 0, 0, DO_OTGrammar_measureTypology);
	praat_addAction1 (classOTGrammar, 0, L"Evaluate", 0, 0, 0);
	praat_addAction1 (classOTGrammar, 0, L"Evaluate...", 0, 0, DO_OTGrammar_evaluate);
	praat_addAction1 (classOTGrammar, 0, L"Input to output...", 0, 0, DO_OTGrammar_inputToOutput);
	praat_addAction1 (classOTGrammar, 0, L"Input to outputs...", 0, 0, DO_OTGrammar_inputToOutputs);
	praat_addAction1 (classOTGrammar, 0, L"To output Distributions...", 0, 0, DO_OTGrammar_to_Distributions);
	praat_addAction1 (classOTGrammar, 0, L"To PairDistribution...", 0, 0, DO_OTGrammar_to_PairDistribution);
	praat_addAction1 (classOTGrammar, 0, L"Modify ranking -", 0, 0, 0);
	praat_addAction1 (classOTGrammar, 0, L"Set ranking...", 0, 1, DO_OTGrammar_setRanking);
	praat_addAction1 (classOTGrammar, 0, L"Reset all rankings...", 0, 1, DO_OTGrammar_resetAllRankings);
	praat_addAction1 (classOTGrammar, 0, L"Reset to random total ranking...", 0, 1, DO_OTGrammar_resetToRandomTotalRanking);
	praat_addAction1 (classOTGrammar, 0, L"Learn one...", 0, 1, DO_OTGrammar_learnOne);
	praat_addAction1 (classOTGrammar, 0, L"Learn one from partial output...", 0, 1, DO_OTGrammar_learnOneFromPartialOutput);
	praat_addAction1 (classOTGrammar, 0, L"Modify behaviour -", 0, 0, 0);
	praat_addAction1 (classOTGrammar, 1, L"Set harmony computation method...", 0, praat_DEPTH_1 + praat_HIDDEN, DO_OTGrammar_setDecisionStrategy);
	praat_addAction1 (classOTGrammar, 1, L"Set decision strategy...", 0, 1, DO_OTGrammar_setDecisionStrategy);
	praat_addAction1 (classOTGrammar, 1, L"Set leak...", 0, 1, DO_OTGrammar_setLeak);
	praat_addAction1 (classOTGrammar, 1, L"Set constraint plasticity...", 0, 1, DO_OTGrammar_setConstraintPlasticity);
	praat_addAction1 (classOTGrammar, 0, L"Modify structure -", 0, 0, 0);
	praat_addAction1 (classOTGrammar, 0, L"Remove constraint...", 0, 1, DO_OTGrammar_removeConstraint);
	praat_addAction1 (classOTGrammar, 0, L"Remove harmonically bounded candidates...", 0, 1, DO_OTGrammar_removeHarmonicallyBoundedCandidates);

	praat_TableOfReal_init (classOTHistory);

	praat_addAction1 (classOTMulti, 0, L"View & Edit", 0, praat_ATTRACTIVE, DO_OTMulti_edit);
	praat_addAction1 (classOTMulti, 0, L"Edit", 0, praat_HIDDEN, DO_OTMulti_edit);
	praat_addAction1 (classOTMulti, 0, L"Draw tableau...", 0, 0, DO_OTMulti_drawTableau);
	praat_addAction1 (classOTMulti, 0, L"Query -", 0, 0, 0);
	praat_addAction1 (classOTMulti, 1, L"Get number of constraints", 0, 1, DO_OTMulti_getNumberOfConstraints);
	praat_addAction1 (classOTMulti, 1, L"Get constraint...", 0, 1, DO_OTMulti_getConstraint);
	praat_addAction1 (classOTMulti, 1, L"Get constraint number...", 0, 1, DO_OTMulti_getConstraintIndexFromName);
	praat_addAction1 (classOTMulti, 1, L"Get ranking value...", 0, 1, DO_OTMulti_getRankingValue);
	praat_addAction1 (classOTMulti, 1, L"Get disharmony...", 0, 1, DO_OTMulti_getDisharmony);
	praat_addAction1 (classOTMulti, 1, L"Get number of candidates", 0, 1, DO_OTMulti_getNumberOfCandidates);
	praat_addAction1 (classOTMulti, 1, L"Get candidate...", 0, 1, DO_OTMulti_getCandidate);
	praat_addAction1 (classOTMulti, 1, L"Get number of violations...", 0, 1, DO_OTMulti_getNumberOfViolations);
	praat_addAction1 (classOTMulti, 1, L"-- parse --", 0, 1, 0);
	praat_addAction1 (classOTMulti, 1, L"Get winner...", 0, 1, DO_OTMulti_getWinner);
	praat_addAction1 (classOTMulti, 0, L"Evaluate", 0, 0, 0);
	praat_addAction1 (classOTMulti, 0, L"Evaluate...", 0, 0, DO_OTMulti_evaluate);
	praat_addAction1 (classOTMulti, 0, L"Get output...", 0, 0, DO_OTMulti_generateOptimalForm);
	praat_addAction1 (classOTMulti, 0, L"Get outputs...", 0, 0, DO_OTMulti_generateOptimalForms);
	praat_addAction1 (classOTMulti, 0, L"To output Distribution...", 0, 0, DO_OTMulti_to_Distribution);
	praat_addAction1 (classOTMulti, 0, L"Modify ranking", 0, 0, 0);
	praat_addAction1 (classOTMulti, 0, L"Set ranking...", 0, 0, DO_OTMulti_setRanking);
	praat_addAction1 (classOTMulti, 0, L"Reset all rankings...", 0, 0, DO_OTMulti_resetAllRankings);
	praat_addAction1 (classOTMulti, 0, L"Learn one...", 0, 0, DO_OTMulti_learnOne);
	praat_addAction1 (classOTMulti, 0, L"Modify behaviour -", 0, 0, 0);
	praat_addAction1 (classOTMulti, 1, L"Set decision strategy...", 0, 1, DO_OTMulti_setDecisionStrategy);
	praat_addAction1 (classOTMulti, 1, L"Set leak...", 0, 1, DO_OTMulti_setLeak);
	praat_addAction1 (classOTMulti, 1, L"Set constraint plasticity...", 0, 1, DO_OTMulti_setConstraintPlasticity);
	praat_addAction1 (classOTMulti, 0, L"Modify structure -", 0, 0, 0);
	praat_addAction1 (classOTMulti, 0, L"Remove constraint...", 0, 1, DO_OTMulti_removeConstraint);

	praat_addAction2 (classOTGrammar, 1, classDistributions, 1, L"Learn from partial outputs...", 0, 0, DO_OTGrammar_Distributions_learnFromPartialOutputs);
	praat_addAction2 (classOTGrammar, 1, classDistributions, 1, L"Get fraction correct...", 0, 0, DO_OTGrammar_Distributions_getFractionCorrect);
	praat_addAction2 (classOTGrammar, 1, classDistributions, 1, L"List obligatory rankings...", 0, praat_HIDDEN, DO_OTGrammar_Distributions_listObligatoryRankings);
	praat_addAction2 (classOTGrammar, 1, classPairDistribution, 1, L"Learn...", 0, 0, DO_OTGrammar_PairDistribution_learn);
	praat_addAction2 (classOTGrammar, 1, classPairDistribution, 1, L"Find positive weights...", 0, 0, DO_OTGrammar_PairDistribution_findPositiveWeights);
	praat_addAction2 (classOTGrammar, 1, classPairDistribution, 1, L"Get fraction correct...", 0, 0, DO_OTGrammar_PairDistribution_getFractionCorrect);
	praat_addAction2 (classOTGrammar, 1, classPairDistribution, 1, L"Get minimum number correct...", 0, 0, DO_OTGrammar_PairDistribution_getMinimumNumberCorrect);
	praat_addAction2 (classOTGrammar, 1, classPairDistribution, 1, L"List obligatory rankings", 0, 0, DO_OTGrammar_PairDistribution_listObligatoryRankings);
	praat_addAction2 (classOTGrammar, 1, classStrings, 1, L"Inputs to outputs...", 0, 0, DO_OTGrammar_inputsToOutputs);
	praat_addAction2 (classOTGrammar, 1, classStrings, 1, L"Learn from partial outputs...", 0, 0, DO_OTGrammar_learnFromPartialOutputs);
	praat_addAction2 (classOTGrammar, 1, classStrings, 2, L"Learn...", 0, 0, DO_OTGrammar_learn);
	praat_addAction2 (classOTMulti, 1, classPairDistribution, 1, L"Learn...", 0, 0, DO_OTMulti_PairDistribution_learn);
	praat_addAction2 (classOTMulti, 1, classStrings, 1, L"Get outputs...", 0, 0, DO_OTMulti_Strings_generateOptimalForms);

	praat_addMenuCommand (L"Objects", L"New", L"Symmetric networks", 0, 0, 0);
		praat_addMenuCommand (L"Objects", L"New", L"Create empty Network...", 0, 1, DO_Create_empty_Network);
		praat_addMenuCommand (L"Objects", L"New", L"Create rectangular Network...", 0, 1, DO_Create_rectangular_Network);
		praat_addMenuCommand (L"Objects", L"New", L"Create rectangular Network (vertical)...", 0, 1, DO_Create_rectangular_Network_vertical);

	praat_addAction1 (classNetwork, 0, L"Draw...", 0, 0, DO_Network_draw);
	praat_addAction1 (classNetwork, 0, L"Query -", 0, 0, 0);
	praat_addAction1 (classNetwork, 1, L"Get activity...", 0, 0, DO_Network_getActivity);
	praat_addAction1 (classNetwork, 1, L"Get weight...", 0, 0, DO_Network_getWeight);
	praat_addAction1 (classNetwork, 0, L"Modify -", 0, 0, 0);
	praat_addAction1 (classNetwork, 0, L"Add node...", 0, 0, DO_Network_addNode);
	praat_addAction1 (classNetwork, 0, L"Add connection...", 0, 0, DO_Network_addConnection);
	praat_addAction1 (classNetwork, 0, L"Set activity...", 0, 0, DO_Network_setActivity);
	praat_addAction1 (classNetwork, 0, L"Set clamping...", 0, 0, DO_Network_setClamping);
	praat_addAction1 (classNetwork, 0, L"Zero activities...", 0, 0, DO_Network_zeroActivities);
	praat_addAction1 (classNetwork, 0, L"Normalize activities...", 0, 0, DO_Network_normalizeActivities);
	praat_addAction1 (classNetwork, 0, L"Spread activities...", 0, 0, DO_Network_spreadActivities);
	praat_addAction1 (classNetwork, 0, L"Set weight...", 0, 0, DO_Network_setWeight);
	praat_addAction1 (classNetwork, 0, L"Update weights", 0, 0, DO_Network_updateWeights);
}

/* End of file praat_gram.c */
