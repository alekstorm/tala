/* SpectrumEditor.cpp
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
 * pb 2006/04/01 dynamic range setting
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/09/04 new FunctionEditor API
 * pb 2008/01/19 double
 * pb 2008/03/20 split off Help menu
 * pb 2008/03/21 new Editor API
 * pb 2011/03/22 C++
 */

#include "SpectrumEditor.h"

#include "fon/Sound_and_Spectrum.h"
#include "ui/Preferences.h"

static struct {
	double bandSmoothing;
	double dynamicRange;
} preferences;

void SpectrumEditor::prefs (void) {
	Preferences_addDouble (L"SpectrumEditor.bandSmoothing", & preferences.bandSmoothing, 100.0);
	Preferences_addDouble (L"SpectrumEditor.dynamicRange", & preferences.dynamicRange, 60.0);
}

SpectrumEditor::SpectrumEditor (GuiObject parent, const wchar_t *title, Any data)
	: FunctionEditor (parent, title, data),
	  _minimum(0),
	  _maximum(0),
	  _cursorHeight(-1000),
	  _bandSmoothing(preferences.bandSmoothing),
	  _dynamicRange(preferences.dynamicRange) {
	createMenus ();
	updateRange ();
}

void SpectrumEditor::updateRange () {
	if (Spectrum_getPowerDensityRange ((Spectrum) _data, & _minimum, & _maximum)) {
		_minimum = _maximum - _dynamicRange;
	} else {
		_minimum = -1000, _maximum = 1000;
	}
}

void SpectrumEditor::dataChanged () {
	updateRange ();
	FunctionEditor::dataChanged ();
}

void SpectrumEditor::draw () {
	Spectrum spectrum = (Spectrum) _data;
	long first, last, selectedSamples;

	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_rectangle (_graphics, 0, 1, 0, 1);
	Spectrum_drawInside (spectrum, _graphics, _startWindow, _endWindow, _minimum, _maximum);
	drawRangeMark (_maximum, Melder_fixed (_maximum, 1), L" dB", Graphics_TOP);
	drawRangeMark (_minimum, Melder_fixed (_minimum, 1), L" dB", Graphics_BOTTOM);
	if (_cursorHeight > _minimum && _cursorHeight < _maximum)
		drawHorizontalHair (_cursorHeight, Melder_fixed (_cursorHeight, 1), L" dB");
	Graphics_setColour (_graphics, Graphics_BLACK);

	/* Update buttons. */

	selectedSamples = Sampled_getWindowSamples (spectrum, _startSelection, _endSelection, & first, & last);
	GuiObject_setSensitive (_publishBandButton, selectedSamples != 0);
	GuiObject_setSensitive (_publishSoundButton, selectedSamples != 0);
}

int SpectrumEditor::click (double xWC, double yWC, int shiftKeyPressed) {
	_cursorHeight = _minimum + yWC * (_maximum - _minimum);
	return FunctionEditor::click (xWC, yWC, shiftKeyPressed);   /* Move cursor or drag selection. */
}

static Spectrum Spectrum_band (Spectrum me, double fmin, double fmax) {
	long i, imin, imax;
	double *re, *im;
	Spectrum band = (Spectrum) Data_copy (me);
	if (! band) return NULL;
	re = band -> z [1], im = band -> z [2];
	imin = Sampled_xToLowIndex (band, fmin), imax = Sampled_xToHighIndex (band, fmax);
	for (i = 1; i <= imin; i ++) re [i] = 0.0, im [i] = 0.0;
	for (i = imax; i <= band -> nx; i ++) re [i] = 0.0, im [i] = 0.0;
	return band;
}

static Sound Spectrum_to_Sound_part (Spectrum me, double fmin, double fmax) {
	Spectrum band = Spectrum_band (me, fmin, fmax);
	Sound sound;
	if (! band) return NULL;
	sound = Spectrum_to_Sound (band);
	forget (band);
	return sound;
}

void SpectrumEditor::play (double fmin, double fmax) {
	Sound sound = Spectrum_to_Sound_part ((Spectrum) _data, fmin, fmax);
	if (! sound) { Melder_flushError (NULL); return; }
	Sound_play (sound, NULL, NULL);
	forget (sound);
}

int SpectrumEditor::menu_cb_publishBand (EDITOR_ARGS) {
	SpectrumEditor *editor = (SpectrumEditor *)editor_me;
	Spectrum publish = Spectrum_band ((Spectrum) editor->_data, editor->_startSelection, editor->_endSelection);
	if (! publish) return 0;
	if (editor->_publishCallback)
		editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

int SpectrumEditor::menu_cb_publishSound (EDITOR_ARGS) {
	SpectrumEditor *editor = (SpectrumEditor *)editor_me;
	Sound publish = Spectrum_to_Sound_part ((Spectrum) editor->_data, editor->_startSelection, editor->_endSelection);
	if (! publish) return 0;
	if (editor->_publishCallback)
		editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

int SpectrumEditor::menu_cb_passBand (EDITOR_ARGS) {
	SpectrumEditor *editor = (SpectrumEditor *)editor_me;
	EDITOR_FORM (L"Filter (pass Hann band)", L"Spectrum: Filter (pass Hann band)...");
		REAL (L"Band smoothing (Hz)", L"100.0")
	EDITOR_OK
		SET_REAL (L"Band smoothing", editor->_bandSmoothing)
	EDITOR_DO
		preferences.bandSmoothing = editor->_bandSmoothing = GET_REAL (L"Band smoothing");
		if (editor->_endSelection <= editor->_startSelection) return Melder_error1 (L"To apply band-pass filter, first make a selection.");
		editor->save (L"Pass band");
		Spectrum_passHannBand ((Spectrum) editor->_data, editor->_startSelection, editor->_endSelection, editor->_bandSmoothing);
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

int SpectrumEditor::menu_cb_stopBand (EDITOR_ARGS) {
	SpectrumEditor *editor = (SpectrumEditor *)editor_me;
	EDITOR_FORM (L"Filter (stop Hann band)", 0)
		REAL (L"Band smoothing (Hz)", L"100.0")
	EDITOR_OK
		SET_REAL (L"Band smoothing", editor->_bandSmoothing)
	EDITOR_DO
		preferences.bandSmoothing = editor->_bandSmoothing = GET_REAL (L"Band smoothing");
		if (editor->_endSelection <= editor->_startSelection) return Melder_error1 (L"To apply band-stop filter, first make a selection.");
		editor->save (L"Stop band");
		Spectrum_stopHannBand ((Spectrum) editor->_data, editor->_startSelection, editor->_endSelection, editor->_bandSmoothing);
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

int SpectrumEditor::menu_cb_setDynamicRange (EDITOR_ARGS) {
	SpectrumEditor *editor = (SpectrumEditor *)editor_me;
	EDITOR_FORM (L"Set dynamic range", 0)
		POSITIVE (L"Dynamic range (dB)", L"60.0")
	EDITOR_OK
		SET_REAL (L"Dynamic range", editor->_dynamicRange)
	EDITOR_DO
		preferences.dynamicRange = editor->_dynamicRange = GET_REAL (L"Dynamic range");
		editor->updateRange ();
		editor->redraw ();
	EDITOR_END
}

int SpectrumEditor::menu_cb_help_SpectrumEditor (EDITOR_ARGS) { Melder_help (L"SpectrumEditor"); return 1; }
int SpectrumEditor::menu_cb_help_Spectrum (EDITOR_ARGS) { Melder_help (L"Spectrum"); return 1; }

void SpectrumEditor::createMenus () {
	EditorMenu *menu = getMenu (L"File");
	_publishBandButton = menu->addCommand (L"Publish band", 0, menu_cb_publishBand) -> _itemWidget;
	_publishSoundButton = menu->addCommand (L"Publish band-filtered sound", 0, menu_cb_publishSound) -> _itemWidget;
	menu->addCommand (L"-- close --", 0, NULL);

	menu = getMenu (L"Edit");
	menu->addCommand (L"-- edit band --", 0, NULL);
	menu->addCommand (L"Pass band...", 0, menu_cb_passBand);
	menu->addCommand (L"Stop band...", 0, menu_cb_stopBand);

	menu = getMenu (L"View");
	menu->addCommand (L"Set dynamic range...", 0, menu_cb_setDynamicRange);
	menu->addCommand (L"-- view settings --", 0, 0);

	menu = getMenu (L"Help");
	menu->addCommand (L"SpectrumEditor help", '?', menu_cb_help_SpectrumEditor);
	menu->addCommand (L"Spectrum help", 0, menu_cb_help_Spectrum);
}

/* End of file SpectrumEditor.cpp */
