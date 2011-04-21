/* FormantGridEditor.cpp
 *
 * Copyright (C) 2008-2011 Paul Boersma & David Weenink
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
 * pb 2008/04/24 created
 * pb 2008/04/25 audio
 * pb 2011/03/23 C++
 */

#include "FormantGridEditor.h"
#include "sys/Preferences.h"
#include "sys/EditorM.h"
#include "PointProcess_and_Sound.h"

/********** PREFERENCES **********/

static struct {
	double formantFloor, formantCeiling, bandwidthFloor, bandwidthCeiling;
	struct FormantGridEditor_Play play;
	struct FormantGridEditor_Source source;
} preferences;

#define DEFAULT_F0_START  150.0
#define DEFAULT_T_MID  0.25
#define DEFAULT_F0_MID  180.0
#define DEFAULT_F0_END  120.0

void FormantGridEditor::prefs (void) {
	Preferences_addDouble (L"FormantGridEditor.formantFloor", & preferences.formantFloor, 0.0);   // Hertz
	Preferences_addDouble (L"FormantGridEditor.formantCeiling", & preferences.formantCeiling, 11000.0);   // Hertz
	Preferences_addDouble (L"FormantGridEditor.bandwidthFloor", & preferences.bandwidthFloor, 0.0);   // Hertz
	Preferences_addDouble (L"FormantGridEditor.bandwidthCeiling", & preferences.bandwidthCeiling, 1000.0);   // Hertz
	Preferences_addDouble (L"FormantGridEditor.play.samplingFrequency", & preferences.play.samplingFrequency, 44100.0);   // Hertz
	Preferences_addDouble (L"FormantGridEditor.source.pitch.tStart", & preferences.source.pitch.tStart, 0.0);   // relative time
	Preferences_addDouble (L"FormantGridEditor.source.pitch.f0Start", & preferences.source.pitch.f0Start, DEFAULT_F0_START);   // Hertz
	Preferences_addDouble (L"FormantGridEditor.source.pitch.tMid", & preferences.source.pitch.tMid, DEFAULT_T_MID);   // relative time
	Preferences_addDouble (L"FormantGridEditor.source.pitch.f0Mid", & preferences.source.pitch.f0Mid, DEFAULT_F0_MID);   // Hertz
	Preferences_addDouble (L"FormantGridEditor.source.pitch.tEnd", & preferences.source.pitch.tEnd, 1.0);   // relative time
	Preferences_addDouble (L"FormantGridEditor.source.pitch.f0End", & preferences.source.pitch.f0End, DEFAULT_F0_END);   // Hertz
	Preferences_addDouble (L"FormantGridEditor.source.pitch.f0End", & preferences.source.pitch.f0End, DEFAULT_F0_END);   // Hertz
	Preferences_addDouble (L"FormantGridEditor.source.phonation.adaptFactor", & preferences.source.phonation.adaptFactor, PointProcess_to_Sound_phonation_DEFAULT_ADAPT_FACTOR);
	Preferences_addDouble (L"FormantGridEditor.source.phonation.maximumPeriod", & preferences.source.phonation.maximumPeriod, PointProcess_to_Sound_phonation_DEFAULT_MAXIMUM_PERIOD);
	Preferences_addDouble (L"FormantGridEditor.source.phonation.openPhase", & preferences.source.phonation.openPhase, PointProcess_to_Sound_phonation_DEFAULT_OPEN_PHASE);
	Preferences_addDouble (L"FormantGridEditor.source.phonation.collisionPhase", & preferences.source.phonation.collisionPhase, PointProcess_to_Sound_phonation_DEFAULT_COLLISION_PHASE);
	Preferences_addDouble (L"FormantGridEditor.source.phonation.power1", & preferences.source.phonation.power1, PointProcess_to_Sound_phonation_DEFAULT_POWER_1);
	Preferences_addDouble (L"FormantGridEditor.source.phonation.power2", & preferences.source.phonation.power2, PointProcess_to_Sound_phonation_DEFAULT_POWER_2);
}

FormantGridEditor::FormantGridEditor (GuiObject parent, const wchar_t *title, FormantGrid data)
	: FunctionEditor (parent, title, data) {
	Melder_assert (data != NULL);
	Melder_assert (Thing_member (data, classFormantGrid));
	_formantFloor = preferences.formantFloor;
	_formantCeiling = preferences.formantCeiling;
	_bandwidthFloor = preferences.bandwidthFloor;
	_bandwidthCeiling = preferences.bandwidthCeiling;
	_play = preferences.play;
	_source = preferences.source;
	_ycursor = 0.382 * _formantFloor + 0.618 * _formantCeiling;
	_selectedFormant = 1;
}

/********** MENU COMMANDS **********/

static int menu_cb_removePoints (EDITOR_ARGS) {
	FormantGridEditor *editor = (FormantGridEditor *)editor_me;
	editor->save (L"Remove point(s)");
	FormantGrid grid = (FormantGrid) editor->_data;
	Ordered tiers = editor->_editingBandwidths ? grid -> bandwidths : grid -> formants;
	RealTier tier = (RealTier) tiers -> item [editor->_selectedFormant];
	if (editor->_startSelection == editor->_endSelection)
		AnyTier_removePointNear (tier, editor->_startSelection);
	else
		AnyTier_removePointsBetween (tier, editor->_startSelection, editor->_endSelection);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_addPointAtCursor (EDITOR_ARGS) {
	FormantGridEditor *editor = (FormantGridEditor *)editor_me;
	editor->save (L"Add point");
	FormantGrid grid = (FormantGrid) editor->_data;
	Ordered tiers = editor->_editingBandwidths ? grid -> bandwidths : grid -> formants;
	RealTier tier = (RealTier) tiers -> item [editor->_selectedFormant];
	RealTier_addPoint (tier, 0.5 * (editor->_startSelection + editor->_endSelection), editor->_ycursor);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_addPointAt (EDITOR_ARGS) {
	FormantGridEditor *editor = (FormantGridEditor *)editor_me;
	EDITOR_FORM (L"Add point", 0)
		REAL (L"Time (s)", L"0.0")
		POSITIVE (L"Frequency (Hz)", L"200.0")
	EDITOR_OK
		SET_REAL (L"Time", 0.5 * (editor->_startSelection + editor->_endSelection))
		SET_REAL (L"Frequency", editor->_ycursor)
	EDITOR_DO
		editor->save (L"Add point");
		FormantGrid grid = (FormantGrid) editor->_data;
		Ordered tiers = editor->_editingBandwidths ? grid -> bandwidths : grid -> formants;
		RealTier tier = (RealTier) tiers -> item [editor->_selectedFormant];
		RealTier_addPoint (tier, GET_REAL (L"Time"), GET_REAL (L"Frequency"));
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

static int menu_cb_setFormantRange (EDITOR_ARGS) {
	FormantGridEditor *editor = (FormantGridEditor *)editor_me;
	EDITOR_FORM (L"Set formant range", 0)
		REAL (L"Minimum formant (Hz)", L"0.0")
		REAL (L"Maximum formant (Hz)", L"11000.0")
	EDITOR_OK
		SET_REAL (L"Minimum formant", editor->_formantFloor)
		SET_REAL (L"Maximum formant", editor->_formantCeiling)
	EDITOR_DO
		preferences.formantFloor = editor->_formantFloor = GET_REAL (L"Minimum formant");
		preferences.formantCeiling = editor->_formantCeiling = GET_REAL (L"Maximum formant");
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_setBandwidthRange (EDITOR_ARGS) {
	FormantGridEditor *editor = (FormantGridEditor *)editor_me;
	EDITOR_FORM (L"Set bandwidth range", 0)
		REAL (L"Minimum bandwidth (Hz)", L"0.0")
		REAL (L"Maximum bandwidth (Hz)", L"1000.0")
	EDITOR_OK
		SET_REAL (L"Minimum bandwidth", editor->_bandwidthFloor)
		SET_REAL (L"Maximum bandwidth", editor->_bandwidthCeiling)
	EDITOR_DO
		preferences.bandwidthFloor = editor->_bandwidthFloor = GET_REAL (L"Minimum bandwidth");
		preferences.bandwidthCeiling = editor->_bandwidthCeiling = GET_REAL (L"Maximum bandwidth");
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_showBandwidths (EDITOR_ARGS) {
	FormantGridEditor *editor = (FormantGridEditor *)editor_me;
	editor->_editingBandwidths = ! editor->_editingBandwidths;
	editor->redraw ();
	return 1;
}

int FormantGridEditor::selectFormantOrBandwidth (long iformant) {
	FormantGrid grid = (FormantGrid) _data;
	long numberOfFormants = grid -> formants -> size;
	if (iformant > numberOfFormants) {
		return Melder_error5 (L"Cannot select formant ", Melder_integer (iformant),
			L", because the FormantGrid has only ", Melder_integer (numberOfFormants), L" formants.");
	}
	_selectedFormant = iformant;
	redraw ();
	return 1;
}

static int menu_cb_selectFirst (EDITOR_ARGS) { FormantGridEditor *editor = (FormantGridEditor *)editor_me; return editor->selectFormantOrBandwidth (1); }
static int menu_cb_selectSecond (EDITOR_ARGS) { FormantGridEditor *editor = (FormantGridEditor *)editor_me; return editor->selectFormantOrBandwidth (2); }
static int menu_cb_selectThird (EDITOR_ARGS) { FormantGridEditor *editor = (FormantGridEditor *)editor_me; return editor->selectFormantOrBandwidth (3); }
static int menu_cb_selectFourth (EDITOR_ARGS) { FormantGridEditor *editor = (FormantGridEditor *)editor_me; return editor->selectFormantOrBandwidth (4); }
static int menu_cb_selectFifth (EDITOR_ARGS) { FormantGridEditor *editor = (FormantGridEditor *)editor_me; return editor->selectFormantOrBandwidth (5); }
static int menu_cb_selectSixth (EDITOR_ARGS) { FormantGridEditor *editor = (FormantGridEditor *)editor_me; return editor->selectFormantOrBandwidth (6); }
static int menu_cb_selectSeventh (EDITOR_ARGS) { FormantGridEditor *editor = (FormantGridEditor *)editor_me; return editor->selectFormantOrBandwidth (7); }
static int menu_cb_selectEighth (EDITOR_ARGS) { FormantGridEditor *editor = (FormantGridEditor *)editor_me; return editor->selectFormantOrBandwidth (8); }
static int menu_cb_selectNinth (EDITOR_ARGS) { FormantGridEditor *editor = (FormantGridEditor *)editor_me; return editor->selectFormantOrBandwidth (9); }
static int menu_cb_selectFormantOrBandwidth (EDITOR_ARGS) {
	FormantGridEditor *editor = (FormantGridEditor *)editor_me;
	EDITOR_FORM (L"Select formant or bandwidth", 0)
		NATURAL (L"Formant number", L"1")
	EDITOR_OK
		SET_INTEGER (L"Formant number", editor->_selectedFormant)
	EDITOR_DO
		if (! editor->selectFormantOrBandwidth (GET_INTEGER (L"Formant number"))) return 0;
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_pitchSettings (EDITOR_ARGS) {
	FormantGridEditor *editor = (FormantGridEditor *)editor_me;
	EDITOR_FORM (L"Source pitch settings", 0)
		LABEL (L"", L"These settings apply to the pitch curve")
		LABEL (L"", L"that you hear when playing the FormantGrid.")
		REAL (L"Starting time", L"0.0%")
		POSITIVE (L"Starting pitch (Hz)", L"150.0")
		REAL (L"Mid time", L"25.0%")
		POSITIVE (L"Mid pitch (Hz)", L"180.0")
		REAL (L"End time", L"100.0%")
		POSITIVE (L"End pitch (Hz)", L"120")
	EDITOR_OK
		SET_REAL (L"Starting time", editor->_source.pitch.tStart)
		SET_REAL (L"Starting pitch", editor->_source.pitch.f0Start)
		SET_REAL (L"Mid time", editor->_source.pitch.tMid)
		SET_REAL (L"Mid pitch", editor->_source.pitch.f0Mid)
		SET_REAL (L"End time", editor->_source.pitch.tEnd)
		SET_REAL (L"End pitch", editor->_source.pitch.f0End)
	EDITOR_DO
		preferences.source.pitch.tStart = editor->_source.pitch.tStart = GET_REAL (L"Starting time");
		preferences.source.pitch.f0Start = editor->_source.pitch.f0Start = GET_REAL (L"Starting pitch");
		preferences.source.pitch.tMid = editor->_source.pitch.tMid = GET_REAL (L"Mid time");
		preferences.source.pitch.f0Mid = editor->_source.pitch.f0Mid = GET_REAL (L"Mid pitch");
		preferences.source.pitch.tEnd = editor->_source.pitch.tEnd = GET_REAL (L"End time");
		preferences.source.pitch.f0End = editor->_source.pitch.f0End = GET_REAL (L"End pitch");
	EDITOR_END
}

void FormantGridEditor::createMenus () {
	FunctionEditor::createMenus ();
	EditorMenu *menu = addMenu (L"Formant", 0);
	menu->addCommand (L"Show bandwidths", GuiMenu_CHECKBUTTON + 'B', menu_cb_showBandwidths);
	menu->addCommand (L"Set formant range...", 0, menu_cb_setFormantRange);
	menu->addCommand (L"Set bandwidth range...", 0, menu_cb_setBandwidthRange);
	menu->addCommand (L"-- select formant --", 0, NULL);
	menu->addCommand (L"Select first", '1', menu_cb_selectFirst);
	menu->addCommand (L"Select second", '2', menu_cb_selectSecond);
	menu->addCommand (L"Select third", '3', menu_cb_selectThird);
	menu->addCommand (L"Select fourth", '4', menu_cb_selectFourth);
	menu->addCommand (L"Select fifth", '5', menu_cb_selectFifth);
	menu->addCommand (L"Select sixth", '6', menu_cb_selectSixth);
	menu->addCommand (L"Select seventh", '7', menu_cb_selectSeventh);
	menu->addCommand (L"Select eighth", '8', menu_cb_selectEighth);
	menu->addCommand (L"Select ninth", '9', menu_cb_selectNinth);
	menu->addCommand (L"Select formant or bandwidth...", 0, menu_cb_selectFormantOrBandwidth);
	menu = addMenu (L"Point", 0);
	menu->addCommand (L"Add point at cursor", 'T', menu_cb_addPointAtCursor);
	menu->addCommand (L"Add point at...", 0, menu_cb_addPointAt);
	menu->addCommand (L"-- remove point --", 0, NULL);
	menu->addCommand (L"Remove point(s)", GuiMenu_OPTION + 'T', menu_cb_removePoints);
	if (hasSourceMenu ()) {
		menu = addMenu (L"Source", 0);
		menu->addCommand (L"Pitch settings...", 0, menu_cb_pitchSettings);
		//menu->addCommand (L"Phonation settings...", 0, menu_cb_phonationSettings);
	}
}

void FormantGridEditor::dataChanged () {
	FunctionEditor::dataChanged ();
}

/********** DRAWING AREA **********/

void FormantGridEditor::draw () {
	FormantGrid grid = (FormantGrid) _data;
	Ordered tiers = _editingBandwidths ? grid -> bandwidths : grid -> formants;
	RealTier selectedTier = (RealTier) tiers -> item [_selectedFormant];
	long ifirstSelected, ilastSelected, n = selectedTier -> points -> size, imin, imax;
	double ymin = _editingBandwidths ? _bandwidthFloor : _formantFloor;
	double ymax = _editingBandwidths ? _bandwidthCeiling : _formantCeiling;
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	Graphics_setWindow (_graphics, _startWindow, _endWindow, ymin, ymax);
	Graphics_setColour (_graphics, Graphics_RED);
	Graphics_line (_graphics, _startWindow, _ycursor, _endWindow, _ycursor);
	Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
	Graphics_text1 (_graphics, _startWindow, _ycursor, Melder_float (Melder_half (_ycursor)));
	Graphics_setColour (_graphics, Graphics_BLUE);
	Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_TOP);
	Graphics_text2 (_graphics, _endWindow, ymax, Melder_float (Melder_half (ymax)), L" Hz");
	Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_HALF);
	Graphics_text2 (_graphics, _endWindow, ymin, Melder_float (Melder_half (ymin)), L" Hz");
	Graphics_setLineWidth (_graphics, 1);
	Graphics_setColour (_graphics, Graphics_GREY);
	for (long iformant = 1; iformant <= grid -> formants -> size; iformant ++) if (iformant != _selectedFormant) {
		RealTier tier = (RealTier) tiers -> item [iformant];
		long imin = AnyTier_timeToHighIndex (tier, _startWindow);
		long imax = AnyTier_timeToLowIndex (tier, _endWindow);
		long n = tier -> points -> size;
		if (n == 0) {
		} else if (imax < imin) {
			double yleft = RealTier_getValueAtTime (tier, _startWindow);
			double yright = RealTier_getValueAtTime (tier, _endWindow);
			Graphics_line (_graphics, _startWindow, yleft, _endWindow, yright);
		} else for (long i = imin; i <= imax; i ++) {
			RealPoint point = (RealPoint) tier -> points -> item [i];
			double t = point -> time, y = point -> value;
			Graphics_fillCircle_mm (_graphics, t, y, 2);
			if (i == 1)
				Graphics_line (_graphics, _startWindow, y, t, y);
			else if (i == imin)
				Graphics_line (_graphics, t, y, _startWindow, RealTier_getValueAtTime (tier, _startWindow));
			if (i == n)
				Graphics_line (_graphics, t, y, _endWindow, y);
			else if (i == imax)
				Graphics_line (_graphics, t, y, _endWindow, RealTier_getValueAtTime (tier, _endWindow));
			else {
				RealPoint pointRight = (RealPoint) tier -> points -> item [i + 1];
				Graphics_line (_graphics, t, y, pointRight -> time, pointRight -> value);
			}
		}
	}
	Graphics_setColour (_graphics, Graphics_BLUE);
	ifirstSelected = AnyTier_timeToHighIndex (selectedTier, _startSelection);
	ilastSelected = AnyTier_timeToLowIndex (selectedTier, _endSelection);
	imin = AnyTier_timeToHighIndex (selectedTier, _startWindow);
	imax = AnyTier_timeToLowIndex (selectedTier, _endWindow);
	Graphics_setLineWidth (_graphics, 2);
	if (n == 0) {
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_text (_graphics, 0.5 * (_startWindow + _endWindow),
			0.5 * (ymin + ymax), L"(no points in selected formant tier)");
	} else if (imax < imin) {
		double yleft = RealTier_getValueAtTime (selectedTier, _startWindow);
		double yright = RealTier_getValueAtTime (selectedTier, _endWindow);
		Graphics_line (_graphics, _startWindow, yleft, _endWindow, yright);
	} else for (long i = imin; i <= imax; i ++) {
		RealPoint point = (RealPoint) selectedTier -> points -> item [i];
		double t = point -> time, y = point -> value;
		if (i >= ifirstSelected && i <= ilastSelected)
			Graphics_setColour (_graphics, Graphics_RED);	
		Graphics_fillCircle_mm (_graphics, t, y, 3);
		Graphics_setColour (_graphics, Graphics_BLUE);
		if (i == 1)
			Graphics_line (_graphics, _startWindow, y, t, y);
		else if (i == imin)
			Graphics_line (_graphics, t, y, _startWindow, RealTier_getValueAtTime (selectedTier, _startWindow));
		if (i == n)
			Graphics_line (_graphics, t, y, _endWindow, y);
		else if (i == imax)
			Graphics_line (_graphics, t, y, _endWindow, RealTier_getValueAtTime (selectedTier, _endWindow));
		else {
			RealPoint pointRight = (RealPoint) selectedTier -> points -> item [i + 1];
			Graphics_line (_graphics, t, y, pointRight -> time, pointRight -> value);
		}
	}
	Graphics_setLineWidth (_graphics, 1);
	Graphics_setColour (_graphics, Graphics_BLACK);
}

void FormantGridEditor::drawWhileDragging (double xWC, double yWC, long first, long last, double dt, double dy) {
	FormantGrid grid = (FormantGrid) _data;
	Ordered tiers = _editingBandwidths ? grid -> bandwidths : grid -> formants;
	RealTier tier = (RealTier) tiers -> item [_selectedFormant];
	double ymin = _editingBandwidths ? _bandwidthFloor : _formantFloor;
	double ymax = _editingBandwidths ? _bandwidthCeiling : _formantCeiling;
	(void) xWC;
	(void) yWC;

	/*
	 * Draw all selected points as magenta empty circles, if inside the window.
	 */
	for (long i = first; i <= last; i ++) {
		RealPoint point = (RealPoint) tier -> points -> item [i];
		double t = point -> time + dt, y = point -> value + dy;
		if (t >= _startWindow && t <= _endWindow)
			Graphics_circle_mm (_graphics, t, y, 3);
	}

	if (last == first) {
		/*
		 * Draw a crosshair with time and y.
		 */
		RealPoint point = (RealPoint) tier -> points -> item [first];
		double t = point -> time + dt, y = point -> value + dy;
		Graphics_line (_graphics, t, ymin, t, ymax - Graphics_dyMMtoWC (_graphics, 4.0));
		Graphics_setTextAlignment (_graphics, kGraphics_horizontalAlignment_CENTRE, Graphics_TOP);
		Graphics_text1 (_graphics, t, ymax, Melder_fixed (t, 6));
		Graphics_line (_graphics, _startWindow, y, _endWindow, y);
		Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_BOTTOM);
		Graphics_text1 (_graphics, _startWindow, y, Melder_fixed (y, 6));
	}
}

int FormantGridEditor::click (double xWC, double yWC, int shiftKeyPressed) {
	FormantGrid grid = (FormantGrid) _data;
	Ordered tiers = _editingBandwidths ? grid -> bandwidths : grid -> formants;
	RealTier tier = (RealTier) tiers -> item [_selectedFormant];
	double ymin = _editingBandwidths ? _bandwidthFloor : _formantFloor;
	double ymax = _editingBandwidths ? _bandwidthCeiling : _formantCeiling;
	long inearestPoint, ifirstSelected, ilastSelected;
	RealPoint nearestPoint;
	double dt = 0, df = 0;
	int draggingSelection;

	/*
	 * Perform the default action: move cursor.
	 */
	_startSelection = _endSelection = xWC;
	_ycursor = (1.0 - yWC) * ymin + yWC * ymax;
	Graphics_setWindow (_graphics, _startWindow, _endWindow, ymin, ymax);
	yWC = _ycursor;

	/*
	 * Clicked on a point?
	 */
	inearestPoint = AnyTier_timeToNearestIndex (tier, xWC);
	if (inearestPoint == 0) {
		return FunctionEditor::click (xWC, yWC, shiftKeyPressed);
	}
	nearestPoint = (RealPoint) tier -> points -> item [inearestPoint];
	if (Graphics_distanceWCtoMM (_graphics, xWC, yWC, nearestPoint -> time, nearestPoint -> value) > 1.5) {
		return FunctionEditor::click (xWC, yWC, shiftKeyPressed);
	}

	/*
	 * Clicked on a selected point?
	 */
	draggingSelection = shiftKeyPressed &&
		nearestPoint -> time > _startSelection && nearestPoint -> time < _endSelection;
	if (draggingSelection) {
		ifirstSelected = AnyTier_timeToHighIndex (tier, _startSelection);
		ilastSelected = AnyTier_timeToLowIndex (tier, _endSelection);
		save (L"Drag points");
	} else {
		ifirstSelected = ilastSelected = inearestPoint;
		save (L"Drag point");
	}

	/*
	 * Drag.
	 */
	Graphics_xorOn (_graphics, Graphics_MAROON);
	drawWhileDragging (xWC, yWC, ifirstSelected, ilastSelected, dt, df);
	while (Graphics_mouseStillDown (_graphics)) {
		double xWC_new, yWC_new;
		Graphics_getMouseLocation (_graphics, & xWC_new, & yWC_new);
		if (xWC_new != xWC || yWC_new != yWC) {
			drawWhileDragging (xWC, yWC, ifirstSelected, ilastSelected, dt, df);
			dt += xWC_new - xWC, df += yWC_new - yWC;
			xWC = xWC_new, yWC = yWC_new;
			drawWhileDragging (xWC, yWC, ifirstSelected, ilastSelected, dt, df);
		}
	}
	Graphics_xorOff (_graphics);

	/*
	 * Dragged inside window?
	 */
	if (xWC < _startWindow || xWC > _endWindow) return 1;

	/*
	 * Points not dragged past neighbours?
	 */
	RealPoint *points = (RealPoint *) tier -> points -> item;
	double newTime = points [ifirstSelected] -> time + dt;
	if (newTime < _tmin) return 1;   /* Outside domain. */
	if (ifirstSelected > 1 && newTime <= points [ifirstSelected - 1] -> time)
		return 1;   /* Past left neighbour. */
	newTime = points [ilastSelected] -> time + dt;
	if (newTime > _tmax) return 1;   /* Outside domain. */
	if (ilastSelected < tier -> points -> size && newTime >= points [ilastSelected + 1] -> time)
		return 1;   /* Past right neighbour. */

	/*
	 * Drop.
	 */
	for (long i = ifirstSelected; i <= ilastSelected; i ++) {
		RealPoint point = (RealPoint) tier -> points -> item [i];
		point -> time += dt;
		point -> value += df;
	}

	/*
	 * Make sure that the same points are still selected (a problem with Undo...).
	 */

	if (draggingSelection) _startSelection += dt, _endSelection += dt;
	if (ifirstSelected == ilastSelected) {
		/*
		 * Move crosshair to only selected formant point.
		 */
		RealPoint point = (RealPoint) tier -> points -> item [ifirstSelected];
		_startSelection = _endSelection = point -> time;
		_ycursor = point -> value;
	} else {
		/*
		 * Move crosshair to mouse location.
		 */
		/*_cursor += dt;*/
		_ycursor += df;
	}

	broadcastChange ();
	return 1;   /* Update needed. */
}

void FormantGridEditor::play (double tmin, double tmax) {
	FormantGrid_playPart ((FormantGrid) _data, tmin, tmax, _play.samplingFrequency,
		_source.pitch.tStart, _source.pitch.f0Start,
		_source.pitch.tMid, _source.pitch.f0Mid,
		_source.pitch.tEnd, _source.pitch.f0End,
		_source.phonation.adaptFactor, _source.phonation.maximumPeriod,
		_source.phonation.openPhase, _source.phonation.collisionPhase,
		_source.phonation.power1, _source.phonation.power2,
		playCallback, this);
}

/* End of file FormantGridEditor.cpp */
