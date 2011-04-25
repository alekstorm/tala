/* SoundEditor.cpp
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
 * pb 2003/05/21 more complete settings report
 * pb 2004/02/15 highlight selection, but not the spectrogram
 * pb 2005/06/16 units
 * pb 2005/09/21 interface update
 * pb 2006/05/10 repaired memory leak in do_write
 * pb 2006/12/30 new Sound_create API
 * pb 2007/01/27 compatible with stereo sounds
 * Erez Volk 2007/05/14 FLAC support
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/09/04 TimeSoundAnalysisEditor
 * pb 2007/09/05 direct drawing to picture window
 * pb 2007/09/08 moved File menu to TimeSoundEditor.c
 * pb 2007/09/19 moved settings report to info
 * pb 2007/11/30 erased Graphics_printf
 * pb 2008/01/19 double
 * pb 2008/03/20 split off Help menu
 * pb 2011/03/23 C++
 */

#include "SoundEditor.h"
#include "Sound_and_Spectrogram.h"
#include "Pitch.h"
#include "sys/Preferences.h"
#include "sys/EditorM.h"

/********** METHODS **********/

SoundEditor::SoundEditor (GuiObject parent, const wchar_t *title, Any data)
	: TimeSoundAnalysisEditor (parent, title, data, data, false) {
	createMenus ();
	Melder_assert (data != NULL);
	//try { // FIXME exception
		/*
		 * _longSound.data or _sound.data have to be set before we call FunctionEditor_init,
		 * because createMenus expect that one of them is not NULL.
		 */
		if (_longSound.data && _endWindow - _startWindow > 30.0) {
			_endWindow = _startWindow + 30.0;
			if (_startWindow == _tmin)
				_startSelection = _endSelection = 0.5 * (_startWindow + _endWindow);
			marksChanged ();
		}
	/*} catch (...) {
		rethrowmzero ("Sound window not created.");
	}*/
}

void SoundEditor::dataChanged () {
	Sound sound = (Sound) _data;
	Melder_assert (sound != NULL);   /* LongSound objects should not get dataChanged messages. */
	Matrix_getWindowExtrema (sound, 1, sound -> nx, 1, sound -> ny, & _sound.minimum, & _sound.maximum);
	destroy_analysis ();
	TimeSoundAnalysisEditor::dataChanged ();
}

/***** EDIT MENU *****/

static int menu_cb_Copy (EDITOR_ARGS) {
	SoundEditor *editor = (SoundEditor *)editor_me;
	Sound publish = editor->_longSound.data ? LongSound_extractPart ((LongSound) editor->_data, editor->_startSelection, editor->_endSelection, FALSE) :
		Sound_extractPart ((Sound) editor->_data, editor->_startSelection, editor->_endSelection, kSound_windowShape_RECTANGULAR, 1.0, FALSE);
	iferror return 0;
	forget (Sound_clipboard);
	Sound_clipboard = publish;
	return 1;
}

static int menu_cb_Cut (EDITOR_ARGS) {
	SoundEditor *editor = (SoundEditor *)editor_me;
	Sound sound = (Sound) editor->_data;
	long first, last, selectionNumberOfSamples = Sampled_getWindowSamples (sound,
		editor->_startSelection, editor->_endSelection, & first, & last);
	long oldNumberOfSamples = sound -> nx;
	long newNumberOfSamples = oldNumberOfSamples - selectionNumberOfSamples;
	if (newNumberOfSamples < 1)
		return Melder_error1 (L"(SoundEditor_cut:) You cannot cut all of the signal away,\n"
			"because you cannot create a Sound with 0 samples.\n"
			"You could consider using Copy instead.");
	if (selectionNumberOfSamples) {
		double **newData, **oldData = sound -> z;
		forget (Sound_clipboard);
		Sound_clipboard = Sound_create (sound -> ny, 0.0, selectionNumberOfSamples * sound -> dx,
						selectionNumberOfSamples, sound -> dx, 0.5 * sound -> dx);
		if (! Sound_clipboard) return 0;
		for (long channel = 1; channel <= sound -> ny; channel ++) {
			long j = 0;
			for (long i = first; i <= last; i ++) {
				Sound_clipboard -> z [channel] [++ j] = oldData [channel] [i];
			}
		}
		newData = NUMdmatrix (1, sound -> ny, 1, newNumberOfSamples);
		for (long channel = 1; channel <= sound -> ny; channel ++) {
			long j = 0;
			for (long i = 1; i < first; i ++) {
				newData [channel] [++ j] = oldData [channel] [i];
			}
			for (long i = last + 1; i <= oldNumberOfSamples; i ++) {
				newData [channel] [++ j] = oldData [channel] [i];
			}
		}
		editor->save (L"Cut");
		NUMdmatrix_free (oldData, 1, 1);
		sound -> xmin = 0.0;
		sound -> xmax = newNumberOfSamples * sound -> dx;
		sound -> nx = newNumberOfSamples;
		sound -> x1 = 0.5 * sound -> dx;
		sound -> z = newData;

		/* Start updating the markers of the FunctionEditor, respecting the invariants. */

		editor->_tmin = sound -> xmin;
		editor->_tmax = sound -> xmax;

		/* Collapse the selection, */
		/* so that the Cut operation can immediately be undone by a Paste. */
		/* The exact position will be half-way in between two samples. */

		editor->_startSelection = editor->_endSelection = sound -> xmin + (first - 1) * sound -> dx;

		/* Update the window. */
		{
			double t1 = (first - 1) * sound -> dx;
			double t2 = last * sound -> dx;
			double windowLength = editor->_endWindow - editor->_startWindow;   /* > 0 */
			if (t1 > editor->_startWindow)
				if (t2 < editor->_endWindow)
					editor->_startWindow -= 0.5 * (t2 - t1);
				else
					(void) 0;
			else if (t2 < editor->_endWindow)
				editor->_startWindow -= t2 - t1;
			else   /* Cut overlaps entire window: centre. */
				editor->_startWindow = editor->_startSelection - 0.5 * windowLength;
			editor->_endWindow = editor->_startWindow + windowLength;   /* First try. */
			if (editor->_endWindow > editor->_tmax) {
				editor->_startWindow -= editor->_endWindow - editor->_tmax;   /* 2nd try. */
				if (editor->_startWindow < editor->_tmin)
					editor->_startWindow = editor->_tmin;   /* Third try. */
				editor->_endWindow = editor->_tmax;   /* Second try. */
			} else if (editor->_startWindow < editor->_tmin) {
				editor->_endWindow -= editor->_startWindow - editor->_tmin;   /* Second try. */
				if (editor->_endWindow > editor->_tmax)
					editor->_endWindow = editor->_tmax;   /* Third try. */
				editor->_startWindow = editor->_tmin;   /* Second try. */
			}
		}

		/* Force FunctionEditor to show changes. */

		Matrix_getWindowExtrema (sound, 1, sound -> nx, 1, sound -> ny, & editor->_sound.minimum, & editor->_sound.maximum);
		editor->destroy_analysis ();
		editor->ungroup ();
		editor->marksChanged ();
		editor->broadcastChange ();
	} else {
		Melder_warning1 (L"No samples selected.");
	}
	return 1;
}

static int menu_cb_Paste (EDITOR_ARGS) {
	SoundEditor *editor = (SoundEditor *)editor_me;
	Sound sound = (Sound) editor->_data;
	long leftSample = Sampled_xToLowIndex (sound, editor->_endSelection);
	long oldNumberOfSamples = sound -> nx, newNumberOfSamples;
	double **newData, **oldData = sound -> z;
	if (! Sound_clipboard) {
		Melder_warning1 (L"(SoundEditor_paste:) Clipboard is empty; nothing pasted.");
		return 1;
	}
	if (Sound_clipboard -> ny != sound -> ny)
		return Melder_error1 (L"(SoundEditor_paste:) Cannot paste because\n"
 			"the number of channels of the clipboard is not equal to\n"
			"the number of channels of the edited sound.");
	if (Sound_clipboard -> dx != sound -> dx)
		return Melder_error1 (L"(SoundEditor_paste:) Cannot paste because\n"
 			"the sampling frequency of the clipboard is not equal to\n"
			"the sampling frequency of the edited sound.");
	if (leftSample < 0) leftSample = 0;
	if (leftSample > oldNumberOfSamples) leftSample = oldNumberOfSamples;
	newNumberOfSamples = oldNumberOfSamples + Sound_clipboard -> nx;
	if (! (newData = NUMdmatrix (1, sound -> ny, 1, newNumberOfSamples))) return 0;
	for (long channel = 1; channel <= sound -> ny; channel ++) {
		long j = 0;
		for (long i = 1; i <= leftSample; i ++) {
			newData [channel] [++ j] = oldData [channel] [i];
		}
		for (long i = 1; i <= Sound_clipboard -> nx; i ++) {
			newData [channel] [++ j] = Sound_clipboard -> z [channel] [i];
		}
		for (long i = leftSample + 1; i <= oldNumberOfSamples; i ++) {
			newData [channel] [++ j] = oldData [channel] [i];
		}
	}
	editor->save (L"Paste");
	NUMdmatrix_free (oldData, 1, 1);
	sound -> xmin = 0.0;
	sound -> xmax = newNumberOfSamples * sound -> dx;
	sound -> nx = newNumberOfSamples;
	sound -> x1 = 0.5 * sound -> dx;
	sound -> z = newData;

	/* Start updating the markers of the FunctionEditor, respecting the invariants. */

	editor->_tmin = sound -> xmin;
 	editor->_tmax = sound -> xmax;
	editor->_startSelection = leftSample * sound -> dx;
	editor->_endSelection = (leftSample + Sound_clipboard -> nx) * sound -> dx;

	/* Force FunctionEditor to show changes. */

	Matrix_getWindowExtrema (sound, 1, sound -> nx, 1, sound -> ny, & editor->_sound.minimum, & editor->_sound.maximum);
	editor->destroy_analysis ();
	editor->ungroup ();
	editor->marksChanged ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_SetSelectionToZero (EDITOR_ARGS) {
	SoundEditor *editor = (SoundEditor *)editor_me;
	Sound sound = (Sound) editor->_data;
	long first, last;
	Sampled_getWindowSamples (sound, editor->_startSelection, editor->_endSelection, & first, & last);
	editor->save (L"Set to zero");
	for (long channel = 1; channel <= sound -> ny; channel ++) {
		for (long i = first; i <= last; i ++) {
			sound -> z [channel] [i] = 0.0;
		}
	}
	editor->destroy_analysis ();
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_ReverseSelection (EDITOR_ARGS) {
	SoundEditor *editor = (SoundEditor *)editor_me;
	editor->save (L"Reverse selection");
	Sound_reverse ((Sound) editor->_data, editor->_startSelection, editor->_endSelection);
	editor->destroy_analysis ();
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

/***** SELECT MENU *****/

static int menu_cb_MoveCursorToZero (EDITOR_ARGS) {
	SoundEditor *editor = (SoundEditor *)editor_me;
	double zero = Sound_getNearestZeroCrossing ((Sound) editor->_data, 0.5 * (editor->_startSelection + editor->_endSelection), 1);   // STEREO BUG
	if (NUMdefined (zero)) {
		editor->_startSelection = editor->_endSelection = zero;
		editor->marksChanged ();
	}
	return 1;
}

static int menu_cb_MoveBtoZero (EDITOR_ARGS) {
	SoundEditor *editor = (SoundEditor *)editor_me;
	double zero = Sound_getNearestZeroCrossing ((Sound) editor->_data, editor->_startSelection, 1);   // STEREO BUG
	if (NUMdefined (zero)) {
		editor->_startSelection = zero;
		if (editor->_startSelection > editor->_endSelection) {
			double dummy = editor->_startSelection;
			editor->_startSelection = editor->_endSelection;
			editor->_endSelection = dummy;
		}
		editor->marksChanged ();
	}
	return 1;
}

static int menu_cb_MoveEtoZero (EDITOR_ARGS) {
	SoundEditor *editor = (SoundEditor *)editor_me;
	double zero = Sound_getNearestZeroCrossing ((Sound) editor->_data, editor->_endSelection, 1);   // STEREO BUG
	if (NUMdefined (zero)) {
		editor->_endSelection = zero;
		if (editor->_startSelection > editor->_endSelection) {
			double dummy = editor->_startSelection;
			editor->_startSelection = editor->_endSelection;
			editor->_endSelection = dummy;
		}
		editor->marksChanged ();
	}
	return 1;
}

/***** HELP MENU *****/

static int menu_cb_SoundEditorHelp (EDITOR_ARGS) { Melder_help (L"SoundEditor"); return 1; }
static int menu_cb_LongSoundEditorHelp (EDITOR_ARGS) { Melder_help (L"LongSoundEditor"); return 1; }

void SoundEditor::createMenus () {
	Melder_assert (_data != NULL);
	Melder_assert (_sound.data != NULL || _longSound.data != NULL);

	EditorMenu *menu = getMenu (L"Edit");
	menu->addCommand (L"-- cut copy paste --", 0, NULL);
	if (_sound.data) _cutButton = menu->addCommand (L"Cut", 'X', menu_cb_Cut) -> _itemWidget;
	_copyButton = menu->addCommand (L"Copy selection to Sound clipboard", 'C', menu_cb_Copy) -> _itemWidget;
	if (_sound.data) _pasteButton = menu->addCommand (L"Paste after selection", 'V', menu_cb_Paste) -> _itemWidget;
	if (_sound.data) {
		menu->addCommand (L"-- zero --", 0, NULL);
		_zeroButton = menu->addCommand (L"Set selection to zero", 0, menu_cb_SetSelectionToZero) -> _itemWidget;
		_reverseButton = menu->addCommand (L"Reverse selection", 'R', menu_cb_ReverseSelection) -> _itemWidget;
	}

	if (_sound.data) {
		menu = getMenu (L"Select");
		menu->addCommand (L"-- move to zero --", 0, 0);
		menu->addCommand (L"Move start of selection to nearest zero crossing", ',', menu_cb_MoveBtoZero);
		menu->addCommand (L"Move begin of selection to nearest zero crossing", Editor_HIDDEN, menu_cb_MoveBtoZero);
		menu->addCommand (L"Move cursor to nearest zero crossing", '0', menu_cb_MoveCursorToZero);
		menu->addCommand (L"Move end of selection to nearest zero crossing", '.', menu_cb_MoveEtoZero);
	}

	menu = getMenu (L"Help");
	menu->addCommand (L"SoundEditor help", '?', menu_cb_SoundEditorHelp);
	menu->addCommand (L"LongSoundEditor help", 0, menu_cb_LongSoundEditorHelp);
}

/********** UPDATE **********/

void SoundEditor::prepareDraw () {
	if (_longSound.data) {
		LongSound_haveWindow (_longSound.data, _startWindow, _endWindow);
		Melder_clearError ();
	}
}

void SoundEditor::draw () {
	long first, last, selectedSamples;
	Graphics_Viewport viewport;
	int showAnalysis = _spectrogram.show || _pitch.show || _intensity.show || _formant.show;
	Melder_assert (_data != NULL);
	Melder_assert (_sound.data != NULL || _longSound.data != NULL);

	/*
	 * We check beforehand whether the window fits the LongSound buffer.
	 */
	if (_longSound.data && _endWindow - _startWindow > _longSound.data -> bufferLength) {
		Graphics_setColour (_graphics, Graphics_WHITE);
		Graphics_setWindow (_graphics, 0, 1, 0, 1);
		Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
		Graphics_setColour (_graphics, Graphics_BLACK);
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_BOTTOM);
		Graphics_text3 (_graphics, 0.5, 0.5, L"(window longer than ", Melder_float (Melder_single (_longSound.data -> bufferLength)), L" seconds)");
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_TOP);
		Graphics_text1 (_graphics, 0.5, 0.5, L"(zoom in to see the samples)");
		return;
	}

	/* Draw sound. */

	if (showAnalysis)
		viewport = Graphics_insetViewport (_graphics, 0, 1, 0.5, 1);
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	draw_sound (_sound.minimum, _sound.maximum);
	Graphics_flushWs (_graphics);
	if (showAnalysis)
		Graphics_resetViewport (_graphics, viewport);

	/* Draw analyses. */

	if (showAnalysis) {
		/* Draw spectrogram, pitch, formants. */
		viewport = Graphics_insetViewport (_graphics, 0, 1, 0, 0.5);
		draw_analysis ();
		Graphics_flushWs (_graphics);
		Graphics_resetViewport (_graphics, viewport);
	}

	/* Draw pulses. */

	if (_pulses.show) {
		if (showAnalysis)
			viewport = Graphics_insetViewport (_graphics, 0, 1, 0.5, 1);
		draw_analysis_pulses ();
		draw_sound (_sound.minimum, _sound.maximum);   /* Second time, partially across the pulses. */
		Graphics_flushWs (_graphics);
		if (showAnalysis)
			Graphics_resetViewport (_graphics, viewport);
	}

	/* Update buttons. */

	selectedSamples = Sampled_getWindowSamples (_data, _startSelection, _endSelection, & first, & last);
	updateMenuItems_file ();
	if (_sound.data) {
		GuiObject_setSensitive (_cutButton, selectedSamples != 0 && selectedSamples < _sound.data -> nx);
		GuiObject_setSensitive (_copyButton, selectedSamples != 0);
		GuiObject_setSensitive (_zeroButton, selectedSamples != 0);
		GuiObject_setSensitive (_reverseButton, selectedSamples != 0);
	}
}

void SoundEditor::play (double tmin, double tmax) {
	if (_longSound.data)
		LongSound_playPart ((LongSound) _data, tmin, tmax, playCallback, this);
	else
		Sound_playPart ((Sound) _data, tmin, tmax, playCallback, this);
}

int SoundEditor::click (double xWC, double yWC, int shiftKeyPressed) {
	if ((_spectrogram.show || _formant.show) && yWC < 0.5 && xWC > _startWindow && xWC < _endWindow) {
		_spectrogram.cursor = _spectrogram.viewFrom +
			2 * yWC * (_spectrogram.viewTo - _spectrogram.viewFrom);
	}
	return TimeSoundAnalysisEditor::click (xWC, yWC, shiftKeyPressed);   /* Drag & update. */
}

void SoundEditor::highlightSelection (double left, double right, double bottom, double top) {
	if (_spectrogram.show)
		Graphics_highlight (_graphics, left, right, 0.5 * (bottom + top), top);
	else
		Graphics_highlight (_graphics, left, right, bottom, top);
}

void SoundEditor::unhighlightSelection (double left, double right, double bottom, double top) {
	if (_spectrogram.show)
		Graphics_unhighlight (_graphics, left, right, 0.5 * (bottom + top), top);
	else
		Graphics_unhighlight (_graphics, left, right, bottom, top);
}

/* End of file SoundEditor.cpp */
