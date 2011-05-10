/* OTGrammarEditor.cpp
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
 * pb 2002/07/16 GPL
 * pb 2003/03/31 removeConstraint
 * pb 2003/05/27 learnOne and learnOneFromPartialOutput
 * pb 2004/03/16 Evaluate (tiny noise)
 * pb 2007/06/10 wchar_t
 * pb 2008/03/20 split off Help menu
 * pb 2008/03/21 new Editor API
 * pb 2008/05/31 ExponentialMaximumEntropy
 * pb 2011/03/22 C++
 */

#include "OTGrammarEditor.h"

// FIXME
void OTGrammar_drawTableau (OTGrammar me, Graphics g, const wchar_t *input);

OTGrammarEditor::OTGrammarEditor (GuiObject parent, const wchar_t *title, OTGrammar ot)
	: HyperPage (parent, title, ot),
	  _selected(0) {
	createMenus ();
	_data = ot;
}

int OTGrammarEditor::menu_cb_evaluate (EDITOR_ARGS) {
	OTGrammarEditor *editor = (OTGrammarEditor *)editor_me;
	EDITOR_FORM (L"Evaluate", 0)
		REAL (L"Noise", L"2.0")
	EDITOR_OK
	EDITOR_DO
		editor->save (L"Evaluate");
		OTGrammar_newDisharmonies ((OTGrammar) editor->_data, GET_REAL (L"Noise"));
		Graphics_updateWs (editor->_g);
		editor->broadcastChange ();
	EDITOR_END
}

int OTGrammarEditor::menu_cb_evaluate_noise_2_0 (EDITOR_ARGS) {
	OTGrammarEditor *editor = (OTGrammarEditor *)editor_me;
	editor->save (L"Evaluate (noise 2.0)");
	OTGrammar_newDisharmonies ((OTGrammar) editor->_data, 2.0);
	Graphics_updateWs (editor->_g);
	editor->broadcastChange ();
	return 1;
}

int OTGrammarEditor::menu_cb_evaluate_tinyNoise (EDITOR_ARGS) {
	OTGrammarEditor *editor = (OTGrammarEditor *)editor_me;
	editor->save (L"Evaluate (tiny noise)");
	OTGrammar_newDisharmonies ((OTGrammar) editor->_data, 1e-9);
	Graphics_updateWs (editor->_g);
	editor->broadcastChange ();
	return 1;
}

int OTGrammarEditor::menu_cb_evaluate_zeroNoise (EDITOR_ARGS) {
	OTGrammarEditor *editor = (OTGrammarEditor *)editor_me;
	editor->save (L"Evaluate (zero noise)");
	OTGrammar_newDisharmonies ((OTGrammar) editor->_data, 0.0);
	Graphics_updateWs (editor->_g);
	editor->broadcastChange ();
	return 1;
}

int OTGrammarEditor::menu_cb_editConstraint (EDITOR_ARGS) {
	OTGrammarEditor *editor = (OTGrammarEditor *)editor_me;
	EDITOR_FORM (L"Edit constraint", 0)
		LABEL (L"constraint", L"");
		REAL (L"Ranking value", L"100.0");
		REAL (L"Disharmony", L"100.0");
		REAL (L"Plasticity", L"1.0");
	EDITOR_OK
		OTGrammar ot = (OTGrammar) editor->_data;
		OTGrammarConstraint constraint;
		if (editor->_selected < 1 || editor->_selected > ot -> numberOfConstraints) return Melder_error1 (L"Select a constraint first.");
		constraint = & ot -> constraints [ot -> index [editor->_selected]];
		SET_STRING (L"constraint", constraint -> name)
		SET_REAL (L"Ranking value", constraint -> ranking)
		SET_REAL (L"Disharmony", constraint -> disharmony)
		SET_REAL (L"Plasticity", constraint -> plasticity)
	EDITOR_DO
		OTGrammar ot = (OTGrammar) editor->_data;
		OTGrammarConstraint constraint = & ot -> constraints [ot -> index [editor->_selected]];
		editor->save (L"Edit constraint");
		constraint -> ranking = GET_REAL (L"Ranking value");
		constraint -> disharmony = GET_REAL (L"Disharmony");
		constraint -> plasticity = GET_REAL (L"Plasticity");
		OTGrammar_sort (ot);
		Graphics_updateWs (editor->_g);
		editor->broadcastChange ();
	EDITOR_END
}

int OTGrammarEditor::menu_cb_learnOne (EDITOR_ARGS) {
	OTGrammarEditor *editor = (OTGrammarEditor *)editor_me;
	EDITOR_FORM (L"Learn one", L"OTGrammar: Learn one...")
		LABEL (L"", L"Underlying form:")
		SENTENCE (L"Input string", L"")
		LABEL (L"", L"Adult surface form:")
		SENTENCE (L"Output string", L"")
		REAL (L"Evaluation noise", L"2.0")
		OPTIONMENU_ENUM (L"Update rule", kOTGrammar_rerankingStrategy, SYMMETRIC_ALL)
		REAL (L"Plasticity", L"0.1")
		REAL (L"Rel. plasticity spreading", L"0.1")
		BOOLEAN (L"Honour local rankings", 1)
	EDITOR_OK
	EDITOR_DO
		editor->save (L"Learn one");
		OTGrammar_learnOne ((OTGrammar) editor->_data, GET_STRING (L"Input string"), GET_STRING (L"Output string"),
			GET_REAL (L"Evaluation noise"), GET_ENUM (kOTGrammar_rerankingStrategy, L"Update rule"), GET_INTEGER (L"Honour local rankings"),
			GET_REAL (L"Plasticity"), GET_REAL (L"Rel. plasticity spreading"), TRUE, TRUE, NULL);
		OTGrammar_sort ((OTGrammar) editor->_data);
		Graphics_updateWs (editor->_g);
		editor->broadcastChange ();
	EDITOR_END
}

int OTGrammarEditor::menu_cb_learnOneFromPartialOutput (EDITOR_ARGS) {
	OTGrammarEditor *editor = (OTGrammarEditor *)editor_me;
	EDITOR_FORM (L"Learn one from partial adult output", 0)
		LABEL (L"", L"Partial adult surface form (e.g. overt form):")
		SENTENCE (L"Partial output", L"")
		REAL (L"Evaluation noise", L"2.0")
		OPTIONMENU_ENUM (L"Update rule", kOTGrammar_rerankingStrategy, SYMMETRIC_ALL)
		REAL (L"Plasticity", L"0.1")
		REAL (L"Rel. plasticity spreading", L"0.1")
		BOOLEAN (L"Honour local rankings", 1)
		NATURAL (L"Number of chews", L"1")
	EDITOR_OK
	EDITOR_DO
		editor->save (L"Learn one from partial output");
		OTGrammar_learnOneFromPartialOutput ((OTGrammar) editor->_data, GET_STRING (L"Partial output"),
			GET_REAL (L"Evaluation noise"), GET_ENUM (kOTGrammar_rerankingStrategy, L"Update rule"), GET_INTEGER (L"Honour local rankings"),
			GET_REAL (L"Plasticity"), GET_REAL (L"Rel. plasticity spreading"), GET_INTEGER (L"Number of chews"), TRUE);
		OTGrammar_sort ((OTGrammar) editor->_data);
		Graphics_updateWs (editor->_g);
		editor->broadcastChange ();
	EDITOR_END
}

int OTGrammarEditor::menu_cb_removeConstraint (EDITOR_ARGS) {
	OTGrammarEditor *editor = (OTGrammarEditor *)editor_me;
	OTGrammar ot = (OTGrammar) editor->_data;
	OTGrammarConstraint constraint;
	if (editor->_selected < 1 || editor->_selected > ot -> numberOfConstraints) return Melder_error ("Select a constraint first.");
	constraint = & ot -> constraints [ot -> index [editor->_selected]];
	editor->save (L"Remove constraint");
	OTGrammar_removeConstraint (ot, constraint -> name);
	Graphics_updateWs (editor->_g);
	editor->broadcastChange ();
	return 1;
}

int OTGrammarEditor::menu_cb_resetAllRankings (EDITOR_ARGS) {
	OTGrammarEditor *editor = (OTGrammarEditor *)editor_me;
	EDITOR_FORM (L"Reset all rankings", 0)
		REAL (L"Ranking", L"100.0")
	EDITOR_OK
	EDITOR_DO
		editor->save (L"Reset all rankings");
		OTGrammar_reset ((OTGrammar) editor->_data, GET_REAL (L"Ranking"));
		Graphics_updateWs (editor->_g);
		editor->broadcastChange ();
	EDITOR_END
}

int OTGrammarEditor::menu_cb_OTGrammarEditor_help (EDITOR_ARGS) { OTGrammarEditor *editor = (OTGrammarEditor *)editor_me; Melder_help (L"OTGrammarEditor"); return 1; }
int OTGrammarEditor::menu_cb_OTGrammar_help (EDITOR_ARGS) { OTGrammarEditor *editor = (OTGrammarEditor *)editor_me; Melder_help (L"OTGrammar"); return 1; }
int OTGrammarEditor::menu_cb_OTLearningTutorial (EDITOR_ARGS) { OTGrammarEditor *editor = (OTGrammarEditor *)editor_me; Melder_help (L"OT learning"); return 1; }

void OTGrammarEditor::createMenus () {
	EditorMenu *menu = getMenu (L"Edit");
	menu->addCommand (L"-- edit ot --", 0, NULL);
	menu->addCommand (L"Evaluate...", 0, menu_cb_evaluate);
	menu->addCommand (L"Evaluate (noise 2.0)", '2', menu_cb_evaluate_noise_2_0);
	menu->addCommand (L"Evaluate (zero noise)", '0', menu_cb_evaluate_zeroNoise);
	menu->addCommand (L"Evaluate (tiny noise)", '9', menu_cb_evaluate_tinyNoise);
	menu->addCommand (L"Edit constraint...", 'E', menu_cb_editConstraint);
	menu->addCommand (L"Reset all rankings...", 'R', menu_cb_resetAllRankings);
	menu->addCommand (L"Learn one...", 0, menu_cb_learnOne);
	menu->addCommand (L"Learn one from partial output...", '1', menu_cb_learnOneFromPartialOutput);
	menu->addCommand (L"-- remove ot --", 0, NULL);
	menu->addCommand (L"Remove constraint", 0, menu_cb_removeConstraint);

	menu = getMenu (L"Help");
	menu->addCommand (L"OTGrammarEditor help", '?', menu_cb_OTGrammarEditor_help);
	menu->addCommand (L"OTGrammar help", 0, menu_cb_OTGrammar_help);
	menu->addCommand (L"OT learning tutorial", 0, menu_cb_OTLearningTutorial);
}

static OTGrammar drawTableau_ot;
static const wchar_t *drawTableau_input;
static void drawTableau (Graphics g) {
	OTGrammar_drawTableau (drawTableau_ot, g, drawTableau_input);
}

void OTGrammarEditor::draw () {
	OTGrammar ot = (OTGrammar) _data;
	static wchar_t text [1000];
	Graphics_clearWs (_g);
	if (ot -> decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_HG ||
		ot -> decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_MAXIMUM_ENTROPY)
	{
		listItem (L"\t\t      %%ranking value\t      %disharmony\t      %plasticity\t   %%e^^disharmony");
	} else {
		listItem (L"\t\t      %%ranking value\t      %disharmony\t      %plasticity");
	}
	for (long icons = 1; icons <= ot -> numberOfConstraints; icons ++) {
		OTGrammarConstraint constraint = & ot -> constraints [ot -> index [icons]];
		if (ot -> decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_HG ||
			ot -> decisionStrategy == kOTGrammar_decisionStrategy_EXPONENTIAL_MAXIMUM_ENTROPY)
		{
			swprintf (text, 1000, L"\t%ls@@%ld|%ls@\t      %.3f\t      %.3f\t      %.6f\t %ls",
				icons == _selected ? L"\\sp " : L"   ", icons, constraint -> name,
				constraint -> ranking, constraint -> disharmony, constraint -> plasticity,
				Melder_float (Melder_half (exp (constraint -> disharmony))));
		} else {
			swprintf (text, 1000, L"\t%ls@@%ld|%ls@\t      %.3f\t      %.3f\t      %.6f",
				icons == _selected ? L"\\sp " : L"   ", icons, constraint -> name,
				constraint -> ranking, constraint -> disharmony, constraint -> plasticity);
		}
		listItem (text);
	}
	Graphics_setAtSignIsLink (_g, FALSE);
	for (long itab = 1; itab <= ot -> numberOfTableaus; itab ++) {
		OTGrammarTableau tableau = & ot -> tableaus [itab];
		double rowHeight = 0.25;
		double tableauHeight = rowHeight * (tableau -> numberOfCandidates + 2);
		drawTableau_ot = ot;
		drawTableau_input = tableau -> input;
		picture (20, tableauHeight, drawTableau);
	}
	Graphics_setAtSignIsLink (_g, TRUE);
}

int OTGrammarEditor::goToPage (const wchar_t *title) {
	if (title == NULL) return 1;
	_selected = wcstol (title, NULL, 10);
	return 1;
}

/* End of file OTGrammarEditor.cpp */
