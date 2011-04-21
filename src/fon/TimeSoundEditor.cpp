/* TimeSoundEditor.cpp
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
 * pb 2004/10/16 C++ compatible struct tags
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/09/19 info
 * pb 2007/09/22 Draw visible sound
 * pb 2010/12/08 
 * pb 2011/03/23 C++
 */

#include "TimeSoundEditor.h"
#include "sys/Preferences.h"
#include "sys/EditorM.h"

/********** PREFERENCES **********/

static struct {
	struct TimeSoundEditor_sound sound;
	struct {
		bool preserveTimes;
		double bottom, top;
		bool garnish;
	} picture;
	struct {
		enum kSound_windowShape windowShape;
		double relativeWidth;
		bool preserveTimes;
	} extract;
}
	preferences;

void TimeSoundEditor::prefs (void) {
	Preferences_addBool (L"TimeSoundEditor.sound.autoscaling", & preferences.sound.autoscaling, true);
	Preferences_addBool (L"TimeSoundEditor.picture.preserveTimes", & preferences.picture.preserveTimes, true);
	Preferences_addDouble (L"TimeSoundEditor.picture.bottom", & preferences.picture.bottom, 0.0);
	Preferences_addDouble (L"TimeSoundEditor.picture.top", & preferences.picture.top, 0.0);
	Preferences_addBool (L"TimeSoundEditor.picture.garnish", & preferences.picture.garnish, true);
	Preferences_addEnum (L"TimeSoundEditor.extract.windowShape", & preferences.extract.windowShape, kSound_windowShape, DEFAULT);
	Preferences_addDouble (L"TimeSoundEditor.extract.relativeWidth", & preferences.extract.relativeWidth, 1.0);
	Preferences_addBool (L"TimeSoundEditor.extract.preserveTimes", & preferences.extract.preserveTimes, true);
}

/********** TimeSoundEditor methods **********/

TimeSoundEditor::TimeSoundEditor (GuiObject parent, const wchar_t *title, Any data, Any sound, bool ownSound)
	: FunctionEditor (parent, title, data) {
	_ownSound = ownSound;
	if (sound != NULL) {
		if (ownSound) {
			Melder_assert (Thing_member (sound, classSound));
			_sound.data = (Sound) Data_copy (sound);   // Deep copy; ownership transferred.
			Matrix_getWindowExtrema (sound, 1, _sound.data -> nx, 1, _sound.data -> ny, & _sound.minimum, & _sound.maximum);
		} else if (Thing_member (sound, classSound)) {
			_sound.data = (Sound) sound;   // Reference copy; ownership not transferred.
			Matrix_getWindowExtrema (sound, 1, _sound.data -> nx, 1, _sound.data -> ny, & _sound.minimum, & _sound.maximum);
		} else if (Thing_member (sound, classLongSound)) {
			_longSound.data = (LongSound) sound;
			_sound.minimum = -1.0, _sound.maximum = 1.0;
		} else {
			Melder_fatal ("Invalid sound class in TimeSoundEditor_init.");
		}
	}
	_sound.autoscaling = preferences.sound.autoscaling;
}

TimeSoundEditor::~TimeSoundEditor () {
	if (_ownSound) forget (_sound.data);
}

void TimeSoundEditor::info () {
	FunctionEditor::info ();
	/* Sound flag: */
	MelderInfo_writeLine2 (L"Sound autoscaling: ", Melder_boolean (_sound.autoscaling));
}

/***** FILE MENU *****/

static int menu_cb_DrawVisibleSound (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	EDITOR_FORM (L"Draw visible sound", 0)
		editor->form_pictureWindow (cmd);
		LABEL (L"", L"Sound:")
		BOOLEAN (L"Preserve times", 1);
		REAL (L"left Vertical range", L"0.0")
		REAL (L"right Vertical range", L"0.0 (= auto)")
		editor->form_pictureMargins (cmd);
		editor->form_pictureSelection (cmd);
		BOOLEAN (L"Garnish", 1);
	EDITOR_OK
		editor->ok_pictureWindow (cmd);
		SET_INTEGER (L"Preserve times", preferences.picture.preserveTimes);
		SET_REAL (L"left Vertical range", preferences.picture.bottom);
		SET_REAL (L"right Vertical range", preferences.picture.top);
		editor->ok_pictureMargins (cmd);
		editor->ok_pictureSelection (cmd);
		SET_INTEGER (L"Garnish", preferences.picture.garnish);
	EDITOR_DO
		editor->do_pictureWindow (cmd);
		preferences.picture.preserveTimes = GET_INTEGER (L"Preserve times");
		preferences.picture.bottom = GET_REAL (L"left Vertical range");
		preferences.picture.top = GET_REAL (L"right Vertical range");
		editor->do_pictureMargins (cmd);
		editor->do_pictureSelection (cmd);
		preferences.picture.garnish = GET_INTEGER (L"Garnish");
		if (editor->_longSound.data == NULL && editor->_sound.data == NULL)
			return Melder_error1 (L"There is no sound to draw.");
		Sound publish = editor->_longSound.data ?
			LongSound_extractPart (editor->_longSound.data, editor->_startWindow, editor->_endWindow, preferences.picture.preserveTimes) :
			Sound_extractPart (editor->_sound.data, editor->_startWindow, editor->_endWindow, kSound_windowShape_RECTANGULAR, 1.0, preferences.picture.preserveTimes);
		if (! publish) return 0;
		editor->openPraatPicture ();
		Sound_draw (publish, editor->_pictureGraphics, 0.0, 0.0, preferences.picture.bottom, preferences.picture.top,
			preferences.picture.garnish, L"Curve");
		forget (publish);
		editor->garnish ();
		editor->closePraatPicture ();
	EDITOR_END
}

static int menu_cb_DrawSelectedSound (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	EDITOR_FORM (L"Draw selected sound", 0)
		editor->form_pictureWindow (cmd);
		LABEL (L"", L"Sound:")
		BOOLEAN (L"Preserve times", 1);
		REAL (L"left Vertical range", L"0.0")
		REAL (L"right Vertical range", L"0.0 (= auto)")
		editor->form_pictureMargins (cmd);
		BOOLEAN (L"Garnish", 1);
	EDITOR_OK
		editor->ok_pictureWindow (cmd);
		SET_INTEGER (L"Preserve times", preferences.picture.preserveTimes);
		SET_REAL (L"left Vertical range", preferences.picture.bottom);
		SET_REAL (L"right Vertical range", preferences.picture.top);
		editor->ok_pictureMargins (cmd);
		SET_INTEGER (L"Garnish", preferences.picture.garnish);
	EDITOR_DO
		editor->do_pictureWindow (cmd);
		preferences.picture.preserveTimes = GET_INTEGER (L"Preserve times");
		preferences.picture.bottom = GET_REAL (L"left Vertical range");
		preferences.picture.top = GET_REAL (L"right Vertical range");
		editor->do_pictureMargins (cmd);
		preferences.picture.garnish = GET_INTEGER (L"Garnish");
		if (editor->_longSound.data == NULL && editor->_sound.data == NULL)
			return Melder_error1 (L"There is no sound to draw.");
		Sound publish = editor->_longSound.data ?
			LongSound_extractPart (editor->_longSound.data, editor->_startSelection, editor->_endSelection, preferences.picture.preserveTimes) :
			Sound_extractPart (editor->_sound.data, editor->_startSelection, editor->_endSelection, kSound_windowShape_RECTANGULAR, 1.0, preferences.picture.preserveTimes);
		if (! publish) return 0;
		editor->openPraatPicture ();
		Sound_draw (publish, editor->_pictureGraphics, 0.0, 0.0, preferences.picture.bottom, preferences.picture.top,
			preferences.picture.garnish, L"Curve");
		forget (publish);
		editor->closePraatPicture ();
	EDITOR_END
}

int TimeSoundEditor::do_ExtractSelectedSound (bool preserveTimes) {
	Sound extract = NULL;
	if (_endSelection <= _startSelection) return Melder_error1 (L"No selection.");
	if (_longSound.data) {
		extract = LongSound_extractPart (_longSound.data, _startSelection, _endSelection, preserveTimes);
		iferror return 0;
	} else if (_sound.data) {
		extract = Sound_extractPart (_sound.data, _startSelection, _endSelection,
			kSound_windowShape_RECTANGULAR, 1.0, preserveTimes);
		iferror return 0;
	}
	Melder_assert (extract != NULL);
	if (_publishCallback)
		_publishCallback (this, _publishClosure, extract);
	return 1;
}

static int menu_cb_ExtractSelectedSound_timeFromZero (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	return editor->do_ExtractSelectedSound (FALSE);
}

static int menu_cb_ExtractSelectedSound_preserveTimes (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	return editor->do_ExtractSelectedSound (TRUE);
}

static int menu_cb_ExtractSelectedSound_windowed (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	EDITOR_FORM (L"Extract selected sound (windowed)", 0)
		WORD (L"Name", L"slice")
		OPTIONMENU_ENUM (L"Window shape", kSound_windowShape, HANNING)
		POSITIVE (L"Relative width", L"1.0")
		BOOLEAN (L"Preserve times", 1)
	EDITOR_OK
		SET_ENUM (L"Window shape", kSound_windowShape, preferences.extract.windowShape)
		SET_REAL (L"Relative width", preferences.extract.relativeWidth)
		SET_INTEGER (L"Preserve times", preferences.extract.preserveTimes)
	EDITOR_DO
		Sound sound = editor->_sound.data;
		Melder_assert (sound != NULL);
		preferences.extract.windowShape = GET_ENUM (kSound_windowShape, L"Window shape");
		preferences.extract.relativeWidth = GET_REAL (L"Relative width");
		preferences.extract.preserveTimes = GET_INTEGER (L"Preserve times");
		Sound extract = Sound_extractPart (sound, editor->_startSelection, editor->_endSelection, preferences.extract.windowShape,
			preferences.extract.relativeWidth, preferences.extract.preserveTimes);
		if (! extract) return 0;
		Thing_setName (extract, GET_STRING (L"Name"));
		if (editor->_publishCallback)
			editor->_publishCallback (editor, editor->_publishClosure, extract);
	EDITOR_END
}

int TimeSoundEditor::do_write (MelderFile file, int format) {
	if (_startSelection >= _endSelection)
		return Melder_error1 (L"No samples selected.");
	if (_longSound.data) {
		return LongSound_writePartToAudioFile16 (_longSound.data, format, _startSelection, _endSelection, file);
	} else if (_sound.data) {
		Sound sound = _sound.data;
		double margin = 0.0;
		long nmargin = margin / sound -> dx;
		long first, last, numberOfSamples = Sampled_getWindowSamples (sound,
			_startSelection, _endSelection, & first, & last) + nmargin * 2;
		first -= nmargin;
		last += nmargin;
		if (numberOfSamples) {
			Sound save = Sound_create (sound -> ny, 0.0, numberOfSamples * sound -> dx,
							numberOfSamples, sound -> dx, 0.5 * sound -> dx);
			if (! save) return 0;
			long offset = first - 1;
			if (first < 1) first = 1;
			if (last > sound -> nx) last = sound -> nx;
			for (long channel = 1; channel <= sound -> ny; channel ++) {
				for (long i = first; i <= last; i ++) {
					save -> z [channel] [i - offset] = sound -> z [channel] [i];
				}
			}
			int result = Sound_writeToAudioFile16 (save, file, format);
			forget (save);
			return result;
		}
	}
	return 0;
}

static int menu_cb_WriteWav (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	EDITOR_FORM_WRITE (L"Save selected sound as WAV file", 0)
		swprintf (defaultName, 300, L"%ls.wav", editor->_longSound.data ? editor->_longSound.data -> name : editor->_sound.data -> name);
	EDITOR_DO_WRITE
		if (! editor->do_write (file, Melder_WAV)) return 0;
	EDITOR_END
}

static int menu_cb_WriteAiff (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	EDITOR_FORM_WRITE (L"Save selected sound as AIFF file", 0)
		swprintf (defaultName, 300, L"%ls.aiff", editor->_longSound.data ? editor->_longSound.data -> name : editor->_sound.data -> name);
	EDITOR_DO_WRITE
		if (! editor->do_write (file, Melder_AIFF)) return 0;
	EDITOR_END
}

static int menu_cb_WriteAifc (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	EDITOR_FORM_WRITE (L"Save selected sound as AIFC file", 0)
		swprintf (defaultName, 300, L"%ls.aifc", editor->_longSound.data ? editor->_longSound.data -> name : editor->_sound.data -> name);
	EDITOR_DO_WRITE
		if (! editor->do_write (file, Melder_AIFC)) return 0;
	EDITOR_END
}

static int menu_cb_WriteNextSun (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	EDITOR_FORM_WRITE (L"Save selected sound as NeXT/Sun file", 0)
		swprintf (defaultName, 300, L"%ls.au", editor->_longSound.data ? editor->_longSound.data -> name : editor->_sound.data -> name);
	EDITOR_DO_WRITE
		if (! editor->do_write (file, Melder_NEXT_SUN)) return 0;
	EDITOR_END
}

static int menu_cb_WriteNist (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	EDITOR_FORM_WRITE (L"Save selected sound as NIST file", 0)
		swprintf (defaultName, 300, L"%ls.nist", editor->_longSound.data ? editor->_longSound.data -> name : editor->_sound.data -> name);
	EDITOR_DO_WRITE
		if (! editor->do_write (file, Melder_NIST)) return 0;
	EDITOR_END
}

static int menu_cb_WriteFlac (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	EDITOR_FORM_WRITE (L"Save selected sound as FLAC file", 0)
		swprintf (defaultName, 300, L"%ls.flac", editor->_longSound.data ? editor->_longSound.data -> name : editor->_sound.data -> name);
	EDITOR_DO_WRITE
		if (! editor->do_write (file, Melder_FLAC)) return 0;
	EDITOR_END
}

void TimeSoundEditor::createMenuItems_file_draw (EditorMenu *menu) {
	menu->addCommand (L"Draw to picture window:", GuiMenu_INSENSITIVE, menu_cb_DrawVisibleSound /* dum_*/);
	if (_sound.data || _longSound.data) {
		menu->addCommand (L"Draw visible sound...", 0, menu_cb_DrawVisibleSound);
		_drawButton = menu->addCommand (L"Draw selected sound...", 0, menu_cb_DrawSelectedSound);
	}
}

void TimeSoundEditor::createMenuItems_file_extract (EditorMenu *menu) {
	menu->addCommand (L"Extract to objects window:", GuiMenu_INSENSITIVE, menu_cb_ExtractSelectedSound_preserveTimes /* dum_*/);
	if (_sound.data || _longSound.data) {
		_publishPreserveButton = menu->addCommand (L"Extract selected sound (preserve times)", 0, menu_cb_ExtractSelectedSound_preserveTimes);
			menu->addCommand (L"Extract sound selection (preserve times)", Editor_HIDDEN, menu_cb_ExtractSelectedSound_preserveTimes);
			menu->addCommand (L"Extract selection (preserve times)", Editor_HIDDEN, menu_cb_ExtractSelectedSound_preserveTimes);
		_publishButton = menu->addCommand (L"Extract selected sound (time from 0)", 0, menu_cb_ExtractSelectedSound_timeFromZero);
			menu->addCommand (L"Extract sound selection (time from 0)", Editor_HIDDEN, menu_cb_ExtractSelectedSound_timeFromZero);
			menu->addCommand (L"Extract selection (time from 0)", Editor_HIDDEN, menu_cb_ExtractSelectedSound_timeFromZero);
			menu->addCommand (L"Extract selection", Editor_HIDDEN, menu_cb_ExtractSelectedSound_timeFromZero);
		if (_sound.data) {
			_publishWindowButton = menu->addCommand (L"Extract selected sound (windowed)...", 0, menu_cb_ExtractSelectedSound_windowed);
				menu->addCommand (L"Extract windowed sound selection...", Editor_HIDDEN, menu_cb_ExtractSelectedSound_windowed);
				menu->addCommand (L"Extract windowed selection...", Editor_HIDDEN, menu_cb_ExtractSelectedSound_windowed);
		}
	}
}

void TimeSoundEditor::createMenuItems_file_write (EditorMenu *menu) {
	menu->addCommand (L"Save to disk:", GuiMenu_INSENSITIVE, menu_cb_WriteWav /* dum_*/);
	if (_sound.data || _longSound.data) {
		_writeWavButton = menu->addCommand (L"Save selected sound as WAV file...", 0, menu_cb_WriteWav);
			menu->addCommand (L"Write selected sound to WAV file...", Editor_HIDDEN, menu_cb_WriteWav);
			menu->addCommand (L"Write sound selection to WAV file...", Editor_HIDDEN, menu_cb_WriteWav);
			menu->addCommand (L"Write selection to WAV file...", Editor_HIDDEN, menu_cb_WriteWav);
		_writeAiffButton = menu->addCommand (L"Save selected sound as AIFF file...", 0, menu_cb_WriteAiff);
			menu->addCommand (L"Write selected sound to AIFF file...", Editor_HIDDEN, menu_cb_WriteAiff);
			menu->addCommand (L"Write sound selection to AIFF file...", Editor_HIDDEN, menu_cb_WriteAiff);
			menu->addCommand (L"Write selection to AIFF file...", Editor_HIDDEN, menu_cb_WriteAiff);
		_writeAifcButton = menu->addCommand (L"Save selected sound as AIFC file...", 0, menu_cb_WriteAifc);
			menu->addCommand (L"Write selected sound to AIFC file...", Editor_HIDDEN, menu_cb_WriteAifc);
			menu->addCommand (L"Write sound selection to AIFC file...", Editor_HIDDEN, menu_cb_WriteAifc);
			menu->addCommand (L"Write selection to AIFC file...", Editor_HIDDEN, menu_cb_WriteAifc);
		_writeNextSunButton = menu->addCommand (L"Save selected sound as Next/Sun file...", 0, menu_cb_WriteNextSun);
			menu->addCommand (L"Write selected sound to Next/Sun file...", Editor_HIDDEN, menu_cb_WriteNextSun);
			menu->addCommand (L"Write sound selection to Next/Sun file...", Editor_HIDDEN, menu_cb_WriteNextSun);
			menu->addCommand (L"Write selection to Next/Sun file...", Editor_HIDDEN, menu_cb_WriteNextSun);
		_writeNistButton = menu->addCommand (L"Save selected sound as NIST file...", 0, menu_cb_WriteNist);
			menu->addCommand (L"Write selected sound to NIST file...", Editor_HIDDEN, menu_cb_WriteNist);
			menu->addCommand (L"Write sound selection to NIST file...", Editor_HIDDEN, menu_cb_WriteNist);
			menu->addCommand (L"Write selection to NIST file...", Editor_HIDDEN, menu_cb_WriteNist);
		_writeFlacButton = menu->addCommand (L"Save selected sound as FLAC file...", 0, menu_cb_WriteFlac);
			menu->addCommand (L"Write selected sound to FLAC file...", Editor_HIDDEN, menu_cb_WriteFlac);
			menu->addCommand (L"Write sound selection to FLAC file...", Editor_HIDDEN, menu_cb_WriteFlac);
	}
}

void TimeSoundEditor::createMenuItems_file (EditorMenu *menu) {
	FunctionEditor::createMenuItems_file (menu);
	createMenuItems_file_draw (menu);
	menu->addCommand (L"-- after file draw --", 0, NULL);
	createMenuItems_file_extract (menu);
	menu->addCommand (L"-- after file extract --", 0, NULL);
	createMenuItems_file_write (menu);
	menu->addCommand (L"-- after file write --", 0, NULL);
}

/********** QUERY MENU **********/

static int menu_cb_SoundInfo (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	Thing_info (editor->_sound.data);
	return 1;
}

static int menu_cb_LongSoundInfo (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	Thing_info (editor->_longSound.data);
	return 1;
}

void TimeSoundEditor::createMenuItems_query_info (EditorMenu *menu) {
	FunctionEditor::createMenuItems_query_info (menu);
	if (_sound.data != NULL && _sound.data != _data) {
		menu->addCommand (L"Sound info", 0, menu_cb_SoundInfo);
	} else if (_longSound.data != NULL && _longSound.data != _data) {
		menu->addCommand (L"LongSound info", 0, menu_cb_LongSoundInfo);
	}
}

/********** VIEW MENU **********/

static int menu_cb_autoscaling (EDITOR_ARGS) {
	TimeSoundEditor *editor = (TimeSoundEditor *)editor_me;
	preferences.sound.autoscaling = editor->_sound.autoscaling = ! editor->_sound.autoscaling;
	editor->redraw ();
	return 1;
}

void TimeSoundEditor::createMenuItems_view (EditorMenu *menu) {
	if (_sound.data || _longSound.data) createMenuItems_view_sound (menu);
	FunctionEditor::createMenuItems_view (menu);
}

void TimeSoundEditor::createMenuItems_view_sound (EditorMenu *menu) {
	menu->addCommand (L"Sound autoscaling", GuiMenu_CHECKBUTTON | (preferences.sound.autoscaling ? GuiMenu_TOGGLE_ON : 0), menu_cb_autoscaling);
	menu->addCommand (L"-- sound view --", 0, 0);
}

void TimeSoundEditor::updateMenuItems_file () {
	Any sound = _sound.data != NULL ? (Sampled) _sound.data : (Sampled) _longSound.data;
	if (sound == NULL) return;
	long first, last, selectedSamples = Sampled_getWindowSamples (sound, _startSelection, _endSelection, & first, & last);
	if (_drawButton) {
		GuiObject_setSensitive (_drawButton, selectedSamples != 0);
		GuiObject_setSensitive (_publishButton, selectedSamples != 0);
		GuiObject_setSensitive (_publishPreserveButton, selectedSamples != 0);
		if (_publishWindowButton) GuiObject_setSensitive (_publishWindowButton, selectedSamples != 0);
	}
	GuiObject_setSensitive (_writeWavButton, selectedSamples != 0);
	GuiObject_setSensitive (_writeAiffButton, selectedSamples != 0);
	GuiObject_setSensitive (_writeAifcButton, selectedSamples != 0);
	GuiObject_setSensitive (_writeNextSunButton, selectedSamples != 0);
	GuiObject_setSensitive (_writeNistButton, selectedSamples != 0);
	GuiObject_setSensitive (_writeFlacButton, selectedSamples != 0);
}

void TimeSoundEditor::draw_sound (double globalMinimum, double globalMaximum) {
	Sound sound = _sound.data;
	LongSound longSound = _longSound.data;
	Melder_assert ((sound == NULL) != (longSound == NULL));
	int fits = sound ? TRUE : LongSound_haveWindow (longSound, _startWindow, _endWindow);
	int nchan = sound ? sound -> ny : longSound -> numberOfChannels;
	int cursorVisible = _startSelection == _endSelection && _startSelection >= _startWindow && _startSelection <= _endWindow;
	Graphics_setColour (_graphics, Graphics_BLACK);
	iferror {
		int outOfMemory = wcsstr (Melder_getError (), L"memory") != NULL;
		if (Melder_debug == 9) Melder_flushError (NULL); else Melder_clearError ();
		Graphics_setWindow (_graphics, 0, 1, 0, 1);
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_text (_graphics, 0.5, 0.5, outOfMemory ? L"(out of memory)" : L"(cannot read sound file)");
		return;
	}
	if (! fits) {
		Graphics_setWindow (_graphics, 0, 1, 0, 1);
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_text (_graphics, 0.5, 0.5, L"(window too large; zoom in to see the data)");
		return;
	}
	long first, last;
	if (Sampled_getWindowSamples (sound ? (Sampled) sound : (Sampled) longSound, _startWindow, _endWindow, & first, & last) <= 1) {
		Graphics_setWindow (_graphics, 0, 1, 0, 1);
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_text (_graphics, 0.5, 0.5, L"(zoom out to see the data)");
		return;
	}
	for (int ichan = 1; ichan <= nchan; ichan ++) {
		double cursorFunctionValue = longSound ? 0.0 :
			Vector_getValueAtX (sound, 0.5 * (_startSelection + _endSelection), ichan, 70);
		/*
		 * BUG: this will only work for mono or stereo, until Graphics_function16 handles quadro.
		 */
		double ymin = (double) (nchan - ichan) / nchan;
		double ymax = (double) (nchan + 1 - ichan) / nchan;
		Graphics_Viewport vp = Graphics_insetViewport (_graphics, 0, 1, ymin, ymax);
		bool horizontal = false;
		double minimum = sound ? globalMinimum : -1.0, maximum = sound ? globalMaximum : 1.0;
		if (_sound.autoscaling) {
			if (longSound)
				LongSound_getWindowExtrema (longSound, _startWindow, _endWindow, ichan, & minimum, & maximum);
			else
				Matrix_getWindowExtrema (sound, first, last, ichan, ichan, & minimum, & maximum);
		}
		if (minimum == maximum) { horizontal = true; minimum -= 1; maximum += 1;}
		Graphics_setWindow (_graphics, _startWindow, _endWindow, minimum, maximum);
		if (horizontal) {
			Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
			double mid = 0.5 * (minimum + maximum);
			Graphics_text1 (_graphics, _startWindow, mid, Melder_half (mid));
		} else {
			if (! cursorVisible || Graphics_dyWCtoMM (_graphics, cursorFunctionValue - minimum) > 5.0) {
				Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_BOTTOM);
				Graphics_text1 (_graphics, _startWindow, minimum, Melder_half (minimum));
			}
			if (! cursorVisible || Graphics_dyWCtoMM (_graphics, maximum - cursorFunctionValue) > 5.0) {
				Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_TOP);
				Graphics_text1 (_graphics, _startWindow, maximum, Melder_half (maximum));
			}
		}
		if (minimum < 0 && maximum > 0 && ! horizontal) {
			Graphics_setWindow (_graphics, 0, 1, minimum, maximum);
			if (! cursorVisible || fabs (Graphics_dyWCtoMM (_graphics, cursorFunctionValue - 0.0)) > 3.0) {
				Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
				Graphics_text (_graphics, 0, 0, L"0");
			}
			Graphics_setColour (_graphics, Graphics_CYAN);
			Graphics_setLineType (_graphics, Graphics_DOTTED);
			Graphics_line (_graphics, 0, 0, 1, 0);
			Graphics_setLineType (_graphics, Graphics_DRAWN);
		}
		/*
		 * Garnish the drawing area of each channel.
		 */
		Graphics_setWindow (_graphics, 0, 1, 0, 1);
		Graphics_setColour (_graphics, Graphics_CYAN);
		Graphics_innerRectangle (_graphics, 0, 1, 0, 1);
		Graphics_setColour (_graphics, Graphics_BLACK);
		/*
		 * Draw a very thin separator line underneath.
		 */
		if (ichan < nchan) {
			/*Graphics_setColour (_graphics, Graphics_BLACK);*/
			Graphics_line (_graphics, 0, 0, 1, 0);
		}
		/*
		 * Draw the samples.
		 */
		/*if (ichan == 1) FunctionEditor_SoundAnalysis_drawPulses ();*/
		if (sound) {
			Graphics_setWindow (_graphics, _startWindow, _endWindow, minimum, maximum);
			if (cursorVisible)
				drawCursorFunctionValue (cursorFunctionValue, Melder_float (Melder_half (cursorFunctionValue)), L"");
			Graphics_setColour (_graphics, Graphics_BLACK);
			Graphics_function (_graphics, sound -> z [ichan], first, last,
				Sampled_indexToX (sound, first), Sampled_indexToX (sound, last));
		} else {
			Graphics_setWindow (_graphics, _startWindow, _endWindow, minimum * 32768, maximum * 32768);
			Graphics_function16 (_graphics,
				longSound -> buffer - longSound -> imin * nchan + (ichan - 1), nchan - 1, first, last,
				Sampled_indexToX (longSound, first), Sampled_indexToX (longSound, last));
		}
		Graphics_resetViewport (_graphics, vp);
	}
	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_rectangle (_graphics, 0, 1, 0, 1);
}

/* End of file FunctionEditor.cpp */
