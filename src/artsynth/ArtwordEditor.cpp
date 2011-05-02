/* ArtwordEditor.cpp
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
 * pb 2003/05/19 Melder_atof
 * pb 2007/08/30 include menu bar height
 * pb 2007/12/27 Gui
 * pb 2009/03/21 modern enums
 * pb 2011/03/22 C+
 */

#include "ArtwordEditor.h"
#include "sys/machine.h"

ArtwordEditor::ArtwordEditor (GuiObject parent, const wchar_t *title, Artword data)
	: Editor (parent, 20, 40, 650, 600, title, data),
	  _feature(1) {
	createChildren ();
	//XtUnmanageChild (_menuBar);
	_graphics = Graphics_create_xmdrawingarea (_drawingArea);
	updateList ();
}

ArtwordEditor::~ArtwordEditor () {
	forget (_graphics);
}

void ArtwordEditor::updateList () {
	Artword artword = (Artword) _data;
	ArtwordData a = & artword -> data [_feature];
	GuiList_deleteAllItems (_list);
	for (int i = 1; i <= a -> numberOfTargets; i ++) {
		static MelderString itemText = { 0 };
		MelderString_empty (& itemText);
		MelderString_append3 (& itemText, Melder_single (a -> times [i]), L"  ", Melder_single (a -> targets [i]));
		GuiList_insertItem (_list, itemText.string, i);
	}
	Graphics_updateWs (_graphics);
}

void ArtwordEditor::gui_button_cb_removeTarget (I, GuiButtonEvent event) {
	(void) event;
	ArtwordEditor *editor = (ArtwordEditor *)void_me;
	Artword artword = (Artword) editor->_data;
	long numberOfSelectedPositions, *selectedPositions = GuiList_getSelectedPositions (editor->_list, & numberOfSelectedPositions);
	if (selectedPositions != NULL) {
		for (long ipos = numberOfSelectedPositions; ipos > 0; ipos --)
			Artword_removeTarget (artword, editor->_feature, selectedPositions [ipos]);
	}
	NUMlvector_free (selectedPositions, 1);
	editor->updateList ();
	editor->broadcastChange ();
}

void ArtwordEditor::gui_button_cb_addTarget (I, GuiButtonEvent event) {
	(void) event;
	ArtwordEditor *editor = (ArtwordEditor *)void_me;
	Artword artword = (Artword) editor->_data;
	wchar_t *timeText = GuiText_getString (editor->_time);
	double tim = Melder_atof (timeText);
	wchar_t *valueText = GuiText_getString (editor->_value);
	double value = Melder_atof (valueText);
	ArtwordData a = & artword -> data [editor->_feature];
	int i = 1, oldCount = a -> numberOfTargets;
	Melder_free (timeText);
	Melder_free (valueText);
	Artword_setTarget (artword, editor->_feature, tim, value);

	/* Optimization instead of "updateList ()". */

	if (tim < 0) tim = 0;
	if (tim > artword -> totalTime) tim = artword -> totalTime;
	while (tim != a -> times [i]) {
		i ++;
		Melder_assert (i <= a -> numberOfTargets);   // can fail if tim is in an extended precision register
	}
	static MelderString itemText = { 0 };
	MelderString_empty (& itemText);
	MelderString_append3 (& itemText, Melder_single (tim), L"  ", Melder_single (value));
	if (a -> numberOfTargets == oldCount) {
		GuiList_replaceItem (editor->_list, itemText.string, i);
	} else {
		GuiList_insertItem (editor->_list, itemText.string, i);
	}
	Graphics_updateWs (editor->_graphics);
	editor->broadcastChange ();
}

void ArtwordEditor::gui_radiobutton_cb_toggle (I, GuiRadioButtonEvent event) {
	ArtwordEditor *editor = (ArtwordEditor *)void_me;
	int i = 0;
	while (event -> toggle != editor->_button [i]) {
		i ++;
		Melder_assert (i <= kArt_muscle_MAX);
	}
	editor->_feature = i;
	Melder_assert (editor->_feature > 0);
	Melder_assert (editor->_feature <= kArt_muscle_MAX);
	editor->updateList ();
}

void ArtwordEditor::gui_drawingarea_cb_expose (I, GuiDrawingAreaExposeEvent event) {
	ArtwordEditor *editor = (ArtwordEditor *)void_me;
	(void) event;
	if (editor->_graphics == NULL) return;
	Artword artword = (Artword) editor->_data;
	Graphics_clearWs (editor->_graphics);
	Artword_draw (artword, editor->_graphics, editor->_feature, TRUE);
}

void ArtwordEditor::gui_drawingarea_cb_click (I, GuiDrawingAreaClickEvent event) {
	ArtwordEditor *editor = (ArtwordEditor *)void_me;
	if (editor->_graphics == NULL) return;
if (gtk && event -> type != BUTTON_PRESS) return;
	Artword artword = (Artword) editor->_data;
	Graphics_setWindow (editor->_graphics, 0, artword -> totalTime, -1.0, 1.0);
	Graphics_setInner (editor->_graphics);
	double xWC, yWC;
	Graphics_DCtoWC (editor->_graphics, event -> x, event -> y, & xWC, & yWC);
	Graphics_unsetInner (editor->_graphics);
	GuiText_setString (editor->_time, Melder_fixed (xWC, 6));
	GuiText_setString (editor->_value, Melder_fixed (yWC, 6));
}

void ArtwordEditor::dataChanged () {
	updateList ();
	Graphics_updateWs (_graphics);
}

void ArtwordEditor::createChildren () {
	int dy = Machine_getMenuBarHeight ();
	GuiLabel_createShown (_dialog, 40, 100, dy + 3, Gui_AUTOMATIC, L"Targets:", 0);
	GuiLabel_createShown (_dialog, 5, 65, dy + 20, Gui_AUTOMATIC, L"Times:", 0);
	GuiLabel_createShown (_dialog, 80, 140, dy + 20, Gui_AUTOMATIC, L"Values:", 0);
	_list = GuiList_createShown (_dialog, 0, 140, dy + 40, dy + 340, true, NULL);

	GuiButton_createShown (_dialog, 10, 130, dy + 410, Gui_AUTOMATIC, L"Remove target", gui_button_cb_removeTarget, this, 0);

	_drawingArea = GuiDrawingArea_createShown (_dialog, 170, 470, dy + 10, dy + 310,
		gui_drawingarea_cb_expose, gui_drawingarea_cb_click, NULL, NULL, this, 0);

	GuiLabel_createShown (_dialog, 220, 270, dy + 340, Gui_AUTOMATIC, L"Time:", 0);
	_time = GuiText_createShown (_dialog, 270, 370, dy + 340, Gui_AUTOMATIC, 0);

	GuiLabel_createShown (_dialog, 220, 270, dy + 370, Gui_AUTOMATIC, L"Value:", 0);
	_value = GuiText_createShown (_dialog, 270, 370, dy + 370, Gui_AUTOMATIC, 0);

	GuiButton_createShown (_dialog, 240, 360, dy + 410, Gui_AUTOMATIC, L"Add target", gui_button_cb_addTarget, this, GuiButton_DEFAULT);

	#if gtk
	_radio = _dialog;	
	#elif motif
	_radio = XtVaCreateManagedWidget
		("radioBox", xmRowColumnWidgetClass, _dialog,
		 XmNradioBehavior, True, XmNx, 470, XmNy, dy, NULL);
	#endif
	for (int i = 1; i <= kArt_muscle_MAX; i ++) {
		_button [i] = GuiRadioButton_createShown (_radio,
			0, 160, Gui_AUTOMATIC, Gui_AUTOMATIC,
			kArt_muscle_getText (i), gui_radiobutton_cb_toggle, this, 0);
	}
	GuiRadioButton_setValue (_button [1], true);
}

/* End of file ArtwordEditor.cpp */
