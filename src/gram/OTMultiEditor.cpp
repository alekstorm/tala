/* OTMultiEditor.cpp
 *
 * Copyright (C) 2005-2011 Paul Boersma
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
 * pb 2005/07/04 created
 * pb 2006/05/17 draw disharmonies on top of tableau
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/10/01 constraint plasticity
 * pb 2007/12/14 Gui
 * pb 2008/03/20 split off Help menu
 * pb 2011/03/01 multiple update rules
 * pb 2011/03/23 C++
 */

#include "OTMultiEditor.h"

#include "sys/machine.h"

int OTMultiEditor::menu_cb_evaluate (EDITOR_ARGS) {
	OTMultiEditor *editor = (OTMultiEditor *)editor_me;
	EDITOR_FORM (L"Evaluate", 0)
		REAL (L"Evaluation noise", L"2.0")
	EDITOR_OK
	EDITOR_DO
		editor->save (L"Evaluate");
		OTMulti_newDisharmonies ((OTMulti) editor->_data, GET_REAL (L"Evaluation noise"));
		Graphics_updateWs (editor->_g);
		editor->broadcastChange ();
	EDITOR_END
}

int OTMultiEditor::menu_cb_evaluate_noise_2_0 (EDITOR_ARGS) {
	OTMultiEditor *editor = (OTMultiEditor *)editor_me;
	editor->save (L"Evaluate (noise 2.0)");
	OTMulti_newDisharmonies ((OTMulti) editor->_data, 2.0);
	Graphics_updateWs (editor->_g);
	editor->broadcastChange ();
	return 1;
}

int OTMultiEditor::menu_cb_evaluate_tinyNoise (EDITOR_ARGS) {
	OTMultiEditor *editor = (OTMultiEditor *)editor_me;
	editor->save (L"Evaluate (tiny noise)");
	OTMulti_newDisharmonies ((OTMulti) editor->_data, 1e-9);
	Graphics_updateWs (editor->_g);
	editor->broadcastChange ();
	return 1;
}

int OTMultiEditor::menu_cb_editRanking (EDITOR_ARGS) {
	OTMultiEditor *editor = (OTMultiEditor *)editor_me;
	EDITOR_FORM (L"Edit ranking", 0)
		LABEL (L"constraint", L"");
		REAL (L"Ranking value", L"100.0");
		REAL (L"Disharmony", L"100.0");
	EDITOR_OK
		OTMulti grammar = (OTMulti) editor->_data;
		OTConstraint constraint;
		if (editor->_selectedConstraint < 1 || editor->_selectedConstraint > grammar -> numberOfConstraints) return Melder_error1 (L"Select a constraint first.");
		constraint = & grammar -> constraints [grammar -> index [editor->_selectedConstraint]];
		SET_STRING (L"constraint", constraint -> name)
		SET_REAL (L"Ranking value", constraint -> ranking)
		SET_REAL (L"Disharmony", constraint -> disharmony)
	EDITOR_DO
		OTMulti grammar = (OTMulti) editor->_data;
		OTConstraint constraint = & grammar -> constraints [grammar -> index [editor->_selectedConstraint]];
		editor->save (L"Edit ranking");
		constraint -> ranking = GET_REAL (L"Ranking value");
		constraint -> disharmony = GET_REAL (L"Disharmony");
		OTMulti_sort (grammar);
		Graphics_updateWs (editor->_g);
		editor->broadcastChange ();
	EDITOR_END
}

int OTMultiEditor::menu_cb_learnOne (EDITOR_ARGS) {
	OTMultiEditor *editor = (OTMultiEditor *)editor_me;
	EDITOR_FORM (L"Learn one", L"OTGrammar: Learn one...")
		OPTIONMENU_ENUM (L"Update rule", kOTGrammar_rerankingStrategy, SYMMETRIC_ALL)
		OPTIONMENU (L"Direction", 3)
			OPTION (L"forward")
			OPTION (L"backward")
			OPTION (L"bidirectionally")
		REAL (L"Plasticity", L"0.1")
		REAL (L"Rel. plasticity spreading", L"0.1")
	EDITOR_OK
	EDITOR_DO
		editor->save (L"Learn one");
		Melder_free (editor->_form1);
		Melder_free (editor->_form2);
		editor->_form1 = GuiText_getString (editor->_form1Text);
		editor->_form2 = GuiText_getString (editor->_form2Text);
		OTMulti_learnOne ((OTMulti) editor->_data, editor->_form1, editor->_form2,
			GET_ENUM (kOTGrammar_rerankingStrategy, L"Update rule"), GET_INTEGER (L"Direction"),
			GET_REAL (L"Plasticity"), GET_REAL (L"Rel. plasticity spreading"));
		iferror return 0;
		Graphics_updateWs (editor->_g);
		editor->broadcastChange ();
	EDITOR_END
}

int OTMultiEditor::menu_cb_removeConstraint (EDITOR_ARGS) {
	OTMultiEditor *editor = (OTMultiEditor *)editor_me;
	OTMulti grammar = (OTMulti) editor->_data;
	OTConstraint constraint;
	if (editor->_selectedConstraint < 1 || editor->_selectedConstraint > grammar -> numberOfConstraints)
		return Melder_error1 (L"Select a constraint first.");
	constraint = & grammar -> constraints [grammar -> index [editor->_selectedConstraint]];
	editor->save (L"Remove constraint");
	OTMulti_removeConstraint (grammar, constraint -> name);
	Graphics_updateWs (editor->_g);
	editor->broadcastChange ();
	return 1;
}

int OTMultiEditor::menu_cb_resetAllRankings (EDITOR_ARGS) {
	OTMultiEditor *editor = (OTMultiEditor *)editor_me;
	EDITOR_FORM (L"Reset all rankings", 0)
		REAL (L"Ranking", L"100.0")
	EDITOR_OK
	EDITOR_DO
		editor->save (L"Reset all rankings");
		OTMulti_reset ((OTMulti) editor->_data, GET_REAL (L"Ranking"));
		Graphics_updateWs (editor->_g);
		editor->broadcastChange ();
	EDITOR_END
}

int OTMultiEditor::menu_cb_OTLearningTutorial (EDITOR_ARGS) {
	OTMultiEditor *editor = (OTMultiEditor *)editor_me;
	Melder_help (L"OT learning");
	return 1;
}

void OTMultiEditor::do_limit () {
	Melder_free (_form1);
	Melder_free (_form2);
	_form1 = GuiText_getString (_form1Text);
	_form2 = GuiText_getString (_form2Text);
	Graphics_updateWs (_g);
}

void OTMultiEditor::gui_button_cb_limit (I, GuiButtonEvent event) {
	(void) event;
	OTMultiEditor *editor = (OTMultiEditor *)void_me;
	editor->do_limit ();
}

void OTMultiEditor::gui_cb_limit (GUI_ARGS) {
	OTMultiEditor *editor = (OTMultiEditor *)void_me;
	editor->do_limit ();
}

OTMultiEditor::OTMultiEditor (GuiObject parent, const wchar_t *title, OTMulti grammar)
	: HyperPage (parent, title, grammar),
	  _form1(Melder_wcsdup_e (L"")),
	  _form2(Melder_wcsdup_e (L"")),
	  _selectedConstraint(0) {
	createMenus ();
	_data = grammar;
}

void OTMultiEditor::createChildren () {
#if defined (macintosh)
	#define STRING_SPACING 8
#else
	#define STRING_SPACING 2
#endif
	int height = Machine_getTextHeight (), y = Machine_getMenuBarHeight () + 4;
	GuiButton_createShown (_dialog, 4, 124, y, y + height,
		L"Partial forms:", gui_button_cb_limit, this,
		#ifdef _WIN32
			GuiButton_DEFAULT   // BUG: clickedCallback should work for texts
		#else
			0
		#endif
		);
	_form1Text = GuiText_createShown (_dialog, 124 + STRING_SPACING, 274 + STRING_SPACING, y, Gui_AUTOMATIC, 0);
	#if motif
	/* TODO */
	XtAddCallback (_form1Text, XmNactivateCallback, gui_cb_limit, (XtPointer) me);
	#endif
	_form2Text = GuiText_createShown (_dialog, 274 + 2 * STRING_SPACING, 424 + 2 * STRING_SPACING, y, Gui_AUTOMATIC, 0);
	#if motif
	/* TODO */
	XtAddCallback (_form2Text, XmNactivateCallback, gui_cb_limit, (XtPointer) me);
	#endif
}

void OTMultiEditor::createMenus () {
	EditorMenu *menu = getMenu (L"Edit");
	menu->addCommand (L"-- edit ot --", 0, NULL);
	menu->addCommand (L"Evaluate...", 0, menu_cb_evaluate);
	menu->addCommand (L"Evaluate (noise 2.0)", '2', menu_cb_evaluate_noise_2_0);
	menu->addCommand (L"Evaluate (tiny noise)", '9', menu_cb_evaluate_tinyNoise);
	menu->addCommand (L"Edit ranking...", 'E', menu_cb_editRanking);
	menu->addCommand (L"Reset all rankings...", 'R', menu_cb_resetAllRankings);
	menu->addCommand (L"Learn one...", '1', menu_cb_learnOne);
	menu->addCommand (L"-- remove --", 0, NULL);
	menu->addCommand (L"Remove constraint", 0, menu_cb_removeConstraint);

	menu->addCommand (L"OT learning tutorial", 0, menu_cb_OTLearningTutorial);
}

static OTMulti drawTableau_grammar;
static const wchar_t *drawTableau_form1, *drawTableau_form2;
static void drawTableau (Graphics g) {
	OTMulti_drawTableau (drawTableau_grammar, g, drawTableau_form1, drawTableau_form2, TRUE);
}

void OTMultiEditor::draw () {
	OTMulti grammar = (OTMulti) _data;
	static MelderString buffer = { 0 };
	double rowHeight = 0.25, tableauHeight = 2 * rowHeight;
	Graphics_clearWs (_g);
	listItem (L"\t\t      %%ranking value\t      %disharmony\t      %plasticity");
	for (long icons = 1; icons <= grammar -> numberOfConstraints; icons ++) {
		OTConstraint constraint = & grammar -> constraints [grammar -> index [icons]];
		MelderString_empty (& buffer);
		MelderString_append8 (& buffer, L"\t", icons == _selectedConstraint ? L"\\sp " : L"   ", L"@@", Melder_integer (icons),
			L"|", constraint -> name, L"@\t      ", Melder_fixed (constraint -> ranking, 3));
		MelderString_append2 (& buffer, L"\t      ", Melder_fixed (constraint -> disharmony, 3));
		MelderString_append2 (& buffer, L"\t      ", Melder_fixed (constraint -> plasticity, 6));
		listItem (buffer.string);
	}
	Graphics_setAtSignIsLink (_g, FALSE);
	drawTableau_grammar = grammar;
	for (long icand = 1; icand <= grammar -> numberOfCandidates; icand ++) {
		if (OTMulti_candidateMatches (grammar, icand, _form1, _form2)) {
			tableauHeight += rowHeight;
		}
	}
	drawTableau_form1 = _form1;
	drawTableau_form2 = _form2;
	picture (20, tableauHeight, drawTableau);
	Graphics_setAtSignIsLink (_g, TRUE);
}

int OTMultiEditor::goToPage (const wchar_t *title) {
	if (title == NULL) return 1;
	_selectedConstraint = wcstol (title, NULL, 10);
	return 1;
}

/* End of file OTGrammarEditor.cpp */
