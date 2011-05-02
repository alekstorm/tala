/* PointEditor.c
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
 * pb 2003/05/17 more shimmer measurements
 * pb 2003/05/21 more jitter measurements
 * pb 2004/04/16 added a fixed maximum period factor of 1.3
 * pb 2004/04/21 less flashing
 * pb 2007/01/28 made compatible with stereo sounds (by converting them to mono)
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/09/08 inherit from TimeSoundEditor
 * pb 2007/09/22 autoscaling
 * pb 2008/03/20 split off Help menu
 * pb 2008/03/21 new Editor API
 * pb 2011/02/20 better messages and info
 */

#include "PointEditor.h"

#include "PointProcess_and_Sound.h"
#include "VoiceAnalysis.h"
#include "sys/EditorM.h"

PointEditor::PointEditor (GuiObject parent, const wchar_t *title, PointProcess point, Sound sound)
	: TimeSoundEditor (parent, title, point, sound ? Sound_convertToMono (sound) : NULL, false),
	  _monoSound(NULL) {
	createMenus ();
	if (sound) {
		_monoSound = _sound.data; // FIXME should be what's passed to base class constructor
	}
}

PointEditor::~PointEditor () {
	forget (_monoSound);
}

/********** MENU COMMANDS **********/

int PointEditor::menu_cb_getJitter_local (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) return Melder_error1 (L"To measure jitter, make a selection first.");
	Melder_informationReal (PointProcess_getJitter_local ((PointProcess) editor->_data, editor->_startSelection, editor->_endSelection, 1e-4, 0.02, 1.3), NULL);
	return 1;
}

int PointEditor::menu_cb_getJitter_local_absolute (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) return Melder_error1 (L"To measure jitter, make a selection first.");
	Melder_informationReal (PointProcess_getJitter_local_absolute ((PointProcess) editor->_data, editor->_startSelection, editor->_endSelection, 1e-4, 0.02, 1.3), L"seconds");
	return 1;
}

int PointEditor::menu_cb_getJitter_rap (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) return Melder_error1 (L"To measure jitter, make a selection first.");
	Melder_informationReal (PointProcess_getJitter_rap ((PointProcess) editor->_data, editor->_startSelection, editor->_endSelection, 1e-4, 0.02, 1.3), NULL);
	return 1;
}

int PointEditor::menu_cb_getJitter_ppq5 (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) return Melder_error1 (L"To measure jitter, make a selection first.");
	Melder_informationReal (PointProcess_getJitter_ppq5 ((PointProcess) editor->_data, editor->_startSelection, editor->_endSelection, 1e-4, 0.02, 1.3), NULL);
	return 1;
}

int PointEditor::menu_cb_getJitter_ddp (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) return Melder_error1 (L"To measure jitter, make a selection first.");
	Melder_informationReal (PointProcess_getJitter_ddp ((PointProcess) editor->_data, editor->_startSelection, editor->_endSelection, 1e-4, 0.02, 1.3), NULL);
	return 1;
}

int PointEditor::menu_cb_getShimmer_local (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) return Melder_error1 (L"To measure shimmer, make a selection first.");
	Melder_informationReal (PointProcess_Sound_getShimmer_local ((PointProcess) editor->_data, editor->_sound.data, editor->_startSelection, editor->_endSelection, 1e-4, 0.02, 1.3, 1.6), NULL);
	return 1;
}

int PointEditor::menu_cb_getShimmer_local_dB (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) return Melder_error1 (L"To measure shimmer, make a selection first.");
	Melder_informationReal (PointProcess_Sound_getShimmer_local_dB ((PointProcess) editor->_data, editor->_sound.data, editor->_startSelection, editor->_endSelection, 1e-4, 0.02, 1.3, 1.6), NULL);
	return 1;
}

int PointEditor::menu_cb_getShimmer_apq3 (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) return Melder_error1 (L"To measure shimmer, make a selection first.");
	Melder_informationReal (PointProcess_Sound_getShimmer_apq3 ((PointProcess) editor->_data, editor->_sound.data, editor->_startSelection, editor->_endSelection, 1e-4, 0.02, 1.3, 1.6), NULL);
	return 1;
}

int PointEditor::menu_cb_getShimmer_apq5 (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) return Melder_error1 (L"To measure shimmer, make a selection first.");
	Melder_informationReal (PointProcess_Sound_getShimmer_apq5 ((PointProcess) editor->_data, editor->_sound.data, editor->_startSelection, editor->_endSelection, 1e-4, 0.02, 1.3, 1.6), NULL);
	return 1;
}

int PointEditor::menu_cb_getShimmer_apq11 (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) return Melder_error1 (L"To measure shimmer, make a selection first.");
	Melder_informationReal (PointProcess_Sound_getShimmer_apq11 ((PointProcess) editor->_data, editor->_sound.data, editor->_startSelection, editor->_endSelection, 1e-4, 0.02, 1.3, 1.6), NULL);
	return 1;
}

int PointEditor::menu_cb_getShimmer_dda (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) return Melder_error1 (L"To measure shimmer, make a selection first.");
	Melder_informationReal (PointProcess_Sound_getShimmer_dda ((PointProcess) editor->_data, editor->_sound.data, editor->_startSelection, editor->_endSelection, 1e-4, 0.02, 1.3, 1.6), NULL);
	return 1;
}

int PointEditor::menu_cb_removePoints (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	editor->save (L"Remove point(s)");
	if (editor->_startSelection == editor->_endSelection)
		PointProcess_removePointNear ((PointProcess) editor->_data, editor->_startSelection);
	else
		PointProcess_removePointsBetween ((PointProcess) editor->_data, editor->_startSelection, editor->_endSelection);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

int PointEditor::menu_cb_addPointAtCursor (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	editor->save (L"Add point");
	PointProcess_addPoint ((PointProcess) editor->_data, 0.5 * (editor->_startSelection + editor->_endSelection));
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

int PointEditor::menu_cb_addPointAt (EDITOR_ARGS) {
	PointEditor *editor = (PointEditor *)editor_me;
	EDITOR_FORM (L"Add point", 0)
		REAL (L"Position", L"0.0");
	EDITOR_OK
		SET_REAL (L"Position", 0.5 * (editor->_startSelection + editor->_endSelection));
	EDITOR_DO
		editor->save (L"Add point");
		PointProcess_addPoint ((PointProcess) editor->_data, GET_REAL (L"Position"));
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

int PointEditor::menu_cb_PointEditorHelp (EDITOR_ARGS) { PointEditor *editor = (PointEditor *)editor_me; Melder_help (L"PointEditor"); return 1; }

void PointEditor::createMenus () {
	EditorMenu *menu = getMenu (L"Query");
	menu->addCommand (L"-- query jitter --", 0, NULL);
	menu->addCommand (L"Get jitter (local)", 0, menu_cb_getJitter_local);
	menu->addCommand (L"Get jitter (local, absolute)", 0, menu_cb_getJitter_local_absolute);
	menu->addCommand (L"Get jitter (rap)", 0, menu_cb_getJitter_rap);
	menu->addCommand (L"Get jitter (ppq5)", 0, menu_cb_getJitter_ppq5);
	menu->addCommand (L"Get jitter (ddp)", 0, menu_cb_getJitter_ddp);
	if (_sound.data) {
		menu->addCommand (L"-- query shimmer --", 0, NULL);
		menu->addCommand (L"Get shimmer (local)", 0, menu_cb_getShimmer_local);
		menu->addCommand (L"Get shimmer (local, dB)", 0, menu_cb_getShimmer_local_dB);
		menu->addCommand (L"Get shimmer (apq3)", 0, menu_cb_getShimmer_apq3);
		menu->addCommand (L"Get shimmer (apq5)", 0, menu_cb_getShimmer_apq5);
		menu->addCommand (L"Get shimmer (apq11)", 0, menu_cb_getShimmer_apq11);
		menu->addCommand (L"Get shimmer (dda)", 0, menu_cb_getShimmer_dda);
	}

	menu = addMenu (L"Point", 0);
	menu->addCommand (L"Add point at cursor", 'P', menu_cb_addPointAtCursor);
	menu->addCommand (L"Add point at...", 0, menu_cb_addPointAt);
	menu->addCommand (L"-- remove point --", 0, NULL);
	menu->addCommand (L"Remove point(s)", GuiMenu_OPTION + 'P', menu_cb_removePoints);

	menu = getMenu (L"Help");
	menu->addCommand (L"PointEditor help", '?', menu_cb_PointEditorHelp);
}

/********** DRAWING AREA **********/

void PointEditor::draw () {
	PointProcess point = (PointProcess) _data;
	Sound sound = _sound.data;
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	double minimum = -1.0, maximum = +1.0;
	if (sound != NULL && _sound.autoscaling) {
		long first, last;
		if (Sampled_getWindowSamples (sound, _startWindow, _endWindow, & first, & last) >= 1) {
			Matrix_getWindowExtrema (sound, first, last, 1, 1, & minimum, & maximum);
		}
	}
	Graphics_setWindow (_graphics, _startWindow, _endWindow, minimum, maximum);
	Graphics_setColour (_graphics, Graphics_BLACK);
	if (sound != NULL) {
		long first, last;
		if (Sampled_getWindowSamples (sound, _startWindow, _endWindow, & first, & last) > 1) {
			Graphics_setLineType (_graphics, Graphics_DOTTED);
			Graphics_line (_graphics, _startWindow, 0.0, _endWindow, 0.0);
			Graphics_setLineType (_graphics, Graphics_DRAWN);      
			Graphics_function (_graphics, sound -> z [1], first, last,
				Sampled_indexToX (sound, first), Sampled_indexToX (sound, last));
		}
	}
	Graphics_setColour (_graphics, Graphics_BLUE);
	Graphics_setWindow (_graphics, _startWindow, _endWindow, -1.0, +1.0);
	for (long i = 1; i <= point -> nt; i ++) {
		double t = point -> t [i];
		if (t >= _startWindow && t <= _endWindow)
			Graphics_line (_graphics, t, -0.9, t, +0.9);
	}
	Graphics_setColour (_graphics, Graphics_BLACK);
	updateMenuItems_file ();
}

void PointEditor::play (double tmin, double tmax) {
	if (_sound.data) {
		Sound_playPart (_sound.data, tmin, tmax, playCallback, this);
	} else {
		if (! PointProcess_playPart ((PointProcess) _data, tmin, tmax)) Melder_flushError (NULL);
	}
}

/* End of file PointEditor.c */
