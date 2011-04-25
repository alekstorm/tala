/* TextGridEditor.cpp
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
 * pb 2002/10/06 improved visibility of dragging
 * pb 2004/04/13 less flashing
 * pb 2005/01/11 better visibility of yellow line
 * pb 2005/03/02 green colouring for matching labels
 * pb 2005/05/05 show number of intervals
 * pb 2005/06/17 enums
 * pb 2005/09/23 interface update
 * pb 2006/12/18 better info
 * pb 2007/03/23 new Editor API
 * Erez Volk & pb 2007/05/17 FLAC support
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/09/02 direct drawing to Picture window
 * pb 2007/09/04 TimeSoundAnalysisEditor
 * pb 2007/09/05 direct drawing to picture window
 * pb 2007/11/30 erased Graphics_printf
 * pb 2007/12/07 enums
 * Erez Volk 2008/03/16 Write selected TextGrid to text file
 * pb 2008/03/17 extract selected TextGrid
 * pb 2008/03/18 renamed: "convert to backslash trigraphs/Unicode"
 * pb 2008/03/20 split off Help menu
 * pb 2008/09/23 info: selectedTier
 * pb 2010/11/10 correctedIinterval2
 * pb 2011/03/22 C++
 */

#include "TextGridEditor.h"
#include "sys/EditorM.h"
#include "SoundEditor.h"
#include "Sound_and_Spectrogram.h"
#include "TextGrid_Sound.h"

#include "sys/enums_getText.h"
#include "TextGridEditor_enums.h"
#include "sys/enums_getValue.h"
#include "TextGridEditor_enums.h"

/********** PREFERENCES **********/

/*
 * If you change any of the following, you may want to raise a version number in TextGridEditor_prefs ().
 */
#define TextGridEditor_DEFAULT_USE_TEXT_STYLES  false
#define TextGridEditor_DEFAULT_FONT_SIZE  18
	#define TextGridEditor_DEFAULT_FONT_SIZE_STRING  L"18"
#define TextGridEditor_DEFAULT_SHIFT_DRAG_MULTIPLE  true
#define TextGridEditor_DEFAULT_GREEN_STRING  L"some text here for green paint"

static struct {
		bool useTextStyles, shiftDragMultiple;
		int fontSize;
		enum kGraphics_horizontalAlignment alignment;
		enum kTextGridEditor_showNumberOf showNumberOf;
		enum kMelder_string greenMethod;
		wchar_t greenString [Preferences_STRING_BUFFER_SIZE];
		struct {
			bool showBoundaries;
			bool garnish;
			struct {
				bool speckle;
			} pitch;
		} picture;
}
	preferences;

void TextGridEditor::prefs (void) {
	Preferences_addBool (L"TextGridEditor.useTextStyles", & preferences.useTextStyles, TextGridEditor_DEFAULT_USE_TEXT_STYLES);
	Preferences_addInt (L"TextGridEditor.fontSize2", & preferences.fontSize, TextGridEditor_DEFAULT_FONT_SIZE);
	Preferences_addEnum (L"TextGridEditor.alignment", & preferences.alignment, kGraphics_horizontalAlignment, DEFAULT);
	Preferences_addBool (L"TextGridEditor.shiftDragMultiple2", & preferences.shiftDragMultiple, TextGridEditor_DEFAULT_SHIFT_DRAG_MULTIPLE);
	Preferences_addEnum (L"TextGridEditor.showNumberOf2", & preferences.showNumberOf, kTextGridEditor_showNumberOf, DEFAULT);
	Preferences_addEnum (L"TextGridEditor.greenMethod", & preferences.greenMethod, kMelder_string, DEFAULT);
	Preferences_addString (L"TextGridEditor.greenString", & preferences.greenString [0], TextGridEditor_DEFAULT_GREEN_STRING);
	Preferences_addBool (L"TextGridEditor.picture.showBoundaries", & preferences.picture.showBoundaries, true);
	Preferences_addBool (L"TextGridEditor.picture.garnish", & preferences.picture.garnish, true);
	Preferences_addBool (L"TextGridEditor.picture.pitch.speckle", & preferences.picture.pitch.speckle, false);
}

TextGridEditor::TextGridEditor (GuiObject parent, const wchar_t *title, TextGrid grid, Any sound, Any spellingChecker)
	: TimeSoundAnalysisEditor (parent, title, grid, sound, sound && Thing_member (sound, classSound)),
	  _spellingChecker((SpellingChecker) spellingChecker),   // Set in time.
	  _useTextStyles(preferences.useTextStyles),
	  _fontSize(preferences.fontSize),
	  _alignment(preferences.alignment),
	  _shiftDragMultiple(preferences.shiftDragMultiple),
	  _showNumberOf(preferences.showNumberOf),
	  _greenMethod(preferences.greenMethod),
	  _selectedTier(1) {
	createMenus ();
	createChildren ();

	/*
	 * Include a deep copy of the Sound, owned by the TextGridEditor, or a pointer to the LongSound.
	 */

	wcscpy (_greenString, preferences.greenString);
	if (_endWindow - _startWindow > 30.0) {
		_endWindow = _startWindow + 30.0;
		if (_startWindow == _tmin)
			_startSelection = _endSelection = 0.5 * (_startWindow + _endWindow);
		marksChanged ();
	}
	if (spellingChecker != NULL)
		GuiText_setSelection (_text, 0, 0);
}

/*
 * The main invariant of the TextGridEditor is that the selected interval
 * always has the cursor in it, and that the cursor always selects an interval
 * if the selected tier is an interval tier.
 */

TextGridEditor::~TextGridEditor () {
	forget (_sound.data);
}

void TextGridEditor::info () {
	TimeSoundAnalysisEditor::info ();
	MelderInfo_writeLine2 (L"Selected tier: ", Melder_integer (_selectedTier));
	MelderInfo_writeLine2 (L"TextGrid uses text styles: ", Melder_boolean (_useTextStyles));
	MelderInfo_writeLine2 (L"TextGrid font size: ", Melder_integer (_fontSize));
	MelderInfo_writeLine2 (L"TextGrid alignment: ", kGraphics_horizontalAlignment_getText (_alignment));
}

/********** UTILITIES **********/

double TextGridEditor::_computeSoundY () {
	TextGrid grid = (TextGrid) _data;
	int ntier = grid -> tiers -> size;
	int showAnalysis = (_spectrogram.show || _pitch.show || _intensity.show || _formant.show) && (_longSound.data || _sound.data);
	return _sound.data || _longSound.data ? ntier / (2.0 + ntier * (showAnalysis ? 1.8 : 1.3)) : 1.0;
}

static void _AnyTier_identifyClass (Data anyTier, IntervalTier *intervalTier, TextTier *textTier) {
	if (anyTier -> methods == (Data_Table) classIntervalTier) {
		*intervalTier = (IntervalTier) anyTier;
		*textTier = NULL;
	} else {
		*intervalTier = NULL;
		*textTier = (TextTier) anyTier;
	}
}

int TextGridEditor::_yWCtoTier (double yWC) {
	TextGrid grid = (TextGrid) _data;
	int ntier = grid -> tiers -> size;
	double soundY = _computeSoundY ();
	int itier = ntier - (int) floor (yWC / soundY * (double) ntier);
	if (itier < 1) itier = 1; if (itier > ntier) itier = ntier;
	return itier;
}

void TextGridEditor::_timeToInterval (double t, int itier, double *tmin, double *tmax) {
	TextGrid grid = (TextGrid) _data;
	IntervalTier intervalTier;
	TextTier textTier;
	_AnyTier_identifyClass ((Data) grid -> tiers -> item [itier], & intervalTier, & textTier);
	if (intervalTier) {
		long iinterval = IntervalTier_timeToIndex (intervalTier, t);
		TextInterval interval;
		if (iinterval == 0) {
			if (t < _tmin) {
				iinterval = 1;
			} else {
				iinterval = intervalTier -> intervals -> size;
			}
		}
		interval = (TextInterval) intervalTier -> intervals -> item [iinterval];
		*tmin = interval -> xmin;
		*tmax = interval -> xmax;
	} else {
		long n = textTier -> points -> size;
		if (n == 0) {
			*tmin = _tmin;
			*tmax = _tmax;
		} else {
			long ipointleft = AnyTier_timeToLowIndex (textTier, t);
			*tmin = ipointleft == 0 ? _tmin : ((TextPoint) textTier -> points -> item [ipointleft]) -> time;
			*tmax = ipointleft == n ? _tmax : ((TextPoint) textTier -> points -> item [ipointleft + 1]) -> time;
		}
	}
	if (*tmin < _tmin) *tmin = _tmin;   /* Clip by FunctionEditor's time domain. */
	if (*tmax > _tmax) *tmax = _tmax;
}

int TextGridEditor::checkTierSelection (const wchar_t *verbPhrase) {
	TextGrid grid = (TextGrid) _data;
	if (_selectedTier < 1 || _selectedTier > grid -> tiers -> size)
		return Melder_error3 (L"To ", verbPhrase, L", first select a tier by clicking anywhere inside it.");
	return 1;
}

long TextGridEditor::getSelectedInterval () {
	TextGrid grid = (TextGrid) _data;
	IntervalTier tier;
	Melder_assert (_selectedTier >= 1 || _selectedTier <= grid -> tiers -> size);
	tier = (IntervalTier) grid -> tiers -> item [_selectedTier];
	Melder_assert (tier -> methods == classIntervalTier);
	return IntervalTier_timeToIndex (tier, _startSelection);
}

long TextGridEditor::getSelectedLeftBoundary () {
	TextGrid grid = (TextGrid) _data;
	IntervalTier tier;
	Melder_assert (_selectedTier >= 1 || _selectedTier <= grid -> tiers -> size);
	tier = (IntervalTier) grid -> tiers -> item [_selectedTier];
	Melder_assert (tier -> methods == classIntervalTier);
	return IntervalTier_hasBoundary (tier, _startSelection);
}

long TextGridEditor::getSelectedPoint () {
	TextGrid grid = (TextGrid) _data;
	TextTier tier;
	Melder_assert (_selectedTier >= 1 || _selectedTier <= grid -> tiers -> size);
	tier = (TextTier) grid -> tiers -> item [_selectedTier];
	Melder_assert (tier -> methods == classTextTier);
	return AnyTier_hasPoint (tier, _startSelection);
}

void TextGridEditor::scrollToView (double t) {
	if (t <= _startWindow) {
		shift (t - _startWindow - 0.618 * (_endWindow - _startWindow));
	} else if (t >= _endWindow) {
		shift (t - _endWindow + 0.618 * (_endWindow - _startWindow));
	} else {
		marksChanged ();
	}
}

/***** FILE MENU *****/

static int menu_cb_ExtractSelectedTextGrid_preserveTimes (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	if (editor->_endSelection <= editor->_startSelection) return Melder_error1 (L"No selection.");
	TextGrid extract = TextGrid_extractPart ((TextGrid) editor->_data, editor->_startSelection, editor->_endSelection, true);
	if (! extract) return 0;
	if (editor->_publishCallback)
		editor->_publishCallback (editor, editor->_publishClosure, extract);
	return 1;
}

static int menu_cb_ExtractSelectedTextGrid_timeFromZero (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	if (editor->_endSelection <= editor->_startSelection) return Melder_error1 (L"No selection.");
	TextGrid extract = TextGrid_extractPart ((TextGrid) editor->_data, editor->_startSelection, editor->_endSelection, false);
	if (! extract) return 0;
	if (editor->_publishCallback)
		editor->_publishCallback (editor, editor->_publishClosure, extract);
	return 1;
}

static int menu_cb_WriteSelectionToTextFile (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	EDITOR_FORM_WRITE (L"Save selection as TextGrid text file", 0)
		swprintf (defaultName, 300, L"%ls.TextGrid", ((Thing) editor->_data) -> name);
	EDITOR_DO_WRITE
		TextGrid publish = TextGrid_extractPart ((TextGrid) editor->_data, editor->_startSelection, editor->_endSelection, false);
		if (! publish) return 0;
		if (! Data_writeToTextFile (publish, file)) return 0;
		forget (publish);
	EDITOR_END
}

static int menu_cb_WriteToTextFile (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	EDITOR_FORM_WRITE (L"Save as TextGrid text file", 0)
		swprintf (defaultName, 300, L"%ls.TextGrid", ((Thing) editor->_data) -> name);
	EDITOR_DO_WRITE
		if (! Data_writeToTextFile (editor->_data, file)) return 0;
	EDITOR_END
}

static int menu_cb_DrawVisibleTextGrid (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	EDITOR_FORM (L"Draw visible TextGrid", 0)
		editor->form_pictureWindow (cmd);
		editor->form_pictureMargins (cmd);
		editor->form_pictureSelection (cmd);
		BOOLEAN (L"Garnish", 1);
	EDITOR_OK
		editor->ok_pictureWindow (cmd);
		editor->ok_pictureMargins (cmd);
		editor->ok_pictureSelection (cmd);
		SET_INTEGER (L"Garnish", preferences.picture.garnish);
	EDITOR_DO
		editor->do_pictureWindow (cmd);
		editor->do_pictureMargins (cmd);
		editor->do_pictureSelection (cmd);
		preferences.picture.garnish = GET_INTEGER (L"Garnish");
		editor->openPraatPicture ();
		TextGrid_Sound_draw ((TextGrid) editor->_data, NULL, editor->_pictureGraphics, editor->_startWindow, editor->_endWindow, true, editor->_useTextStyles,
			preferences.picture.garnish);
		editor->garnish ();
		editor->closePraatPicture ();
	EDITOR_END
}

static int menu_cb_DrawVisibleSoundAndTextGrid (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	EDITOR_FORM (L"Draw visible sound and TextGrid", 0)
		editor->form_pictureWindow (cmd);
		editor->form_pictureMargins (cmd);
		editor->form_pictureSelection (cmd);
		BOOLEAN (L"Garnish", 1);
	EDITOR_OK
		editor->ok_pictureWindow (cmd);
		editor->ok_pictureMargins (cmd);
		editor->ok_pictureSelection (cmd);
		SET_INTEGER (L"Garnish", preferences.picture.garnish);
	EDITOR_DO
		editor->do_pictureWindow (cmd);
		editor->do_pictureMargins (cmd);
		editor->do_pictureSelection (cmd);
		preferences.picture.garnish = GET_INTEGER (L"Garnish");
		editor->openPraatPicture ();
		Sound publish = editor->_longSound.data ?
			LongSound_extractPart (editor->_longSound.data, editor->_startWindow, editor->_endWindow, true) :
			Sound_extractPart (editor->_sound.data, editor->_startWindow, editor->_endWindow, kSound_windowShape_RECTANGULAR, 1.0, true);
		if (! publish) return 0;
		TextGrid_Sound_draw ((TextGrid) editor->_data, publish, editor->_pictureGraphics, editor->_startWindow, editor->_endWindow, true, editor->_useTextStyles, preferences.picture.garnish);
		forget (publish);
		editor->garnish ();
		editor->closePraatPicture ();
	EDITOR_END
}

/***** EDIT MENU *****/

#ifndef macintosh
static int menu_cb_Cut (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	GuiText_cut (editor->_text);
	return 1;
}
static int menu_cb_Copy (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	GuiText_copy (editor->_text);
	return 1;
}
static int menu_cb_Paste (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	GuiText_paste (editor->_text);
	return 1;
}
static int menu_cb_Erase (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	GuiText_remove (editor->_text);
	return 1;
}
#endif

static int menu_cb_Genericize (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	editor->save (L"Convert to Backslash Trigraphs");
	TextGrid_genericize ((TextGrid) editor->_data);
	editor->updateText ();
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_Nativize (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	editor->save (L"Convert to Unicode");
	TextGrid_nativize ((TextGrid) editor->_data);
	editor->updateText ();
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

/***** QUERY MENU *****/

static int menu_cb_GetStartingPointOfInterval (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	TextGrid grid = (TextGrid) editor->_data;
	Data anyTier;
	if (! editor->checkTierSelection (L"query the starting point of an interval")) return 0;
	anyTier = (Data) grid -> tiers -> item [editor->_selectedTier];
	if (anyTier -> methods == (Data_Table) classIntervalTier) {
		IntervalTier tier = (IntervalTier) anyTier;
		long iinterval = IntervalTier_timeToIndex (tier, editor->_startSelection);
		double time = iinterval < 1 || iinterval > tier -> intervals -> size ? NUMundefined :
			((TextInterval) tier -> intervals -> item [iinterval]) -> xmin;
		Melder_informationReal (time, L"seconds");
	} else {
		return Melder_error1 (L"The selected tier is not an interval tier.");
	}
	return 1;
}

static int menu_cb_GetEndPointOfInterval (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	TextGrid grid = (TextGrid) editor->_data;
	Data anyTier;
	if (! editor->checkTierSelection (L"query the end point of an interval")) return 0;
	anyTier = (Data) grid -> tiers -> item [editor->_selectedTier];
	if (anyTier -> methods == (Data_Table) classIntervalTier) {
		IntervalTier tier = (IntervalTier) anyTier;
		long iinterval = IntervalTier_timeToIndex (tier, editor->_startSelection);
		double time = iinterval < 1 || iinterval > tier -> intervals -> size ? NUMundefined :
			((TextInterval) tier -> intervals -> item [iinterval]) -> xmax;
		Melder_informationReal (time, L"seconds");
	} else {
		return Melder_error1 (L"The selected tier is not an interval tier.");
	}
	return 1;
}

static int menu_cb_GetLabelOfInterval (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	TextGrid grid = (TextGrid) editor->_data;
	Data anyTier;
	if (! editor->checkTierSelection (L"query the label of an interval")) return 0;
	anyTier = (Data) grid -> tiers -> item [editor->_selectedTier];
	if (anyTier -> methods == (Data_Table) classIntervalTier) {
		IntervalTier tier = (IntervalTier) anyTier;
		long iinterval = IntervalTier_timeToIndex (tier, editor->_startSelection);
		const wchar_t *label = iinterval < 1 || iinterval > tier -> intervals -> size ? L"" :
			((TextInterval) tier -> intervals -> item [iinterval]) -> text;
		Melder_information1 (label);
	} else {
		return Melder_error1 (L"The selected tier is not an interval tier.");
	}
	return 1;
}

/***** VIEW MENU *****/

void TextGridEditor::do_selectAdjacentTier (int previous) {
	TextGrid grid = (TextGrid) _data;
	long n = grid -> tiers -> size;
	if (n >= 2) {
		_selectedTier = previous ?
			_selectedTier > 1 ? _selectedTier - 1 : n :
			_selectedTier < n ? _selectedTier + 1 : 1;
		_timeToInterval (_startSelection, _selectedTier, & _startSelection, & _endSelection);
		marksChanged ();
	}
}

static int menu_cb_SelectPreviousTier (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	editor->do_selectAdjacentTier (TRUE);
	return 1;
}

static int menu_cb_SelectNextTier (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	editor->do_selectAdjacentTier (FALSE);
	return 1;
}

void TextGridEditor::do_selectAdjacentInterval (int previous, int shift) {
	TextGrid grid = (TextGrid) _data;
	IntervalTier intervalTier;
	TextTier textTier;
	if (_selectedTier < 1 || _selectedTier > grid -> tiers -> size) return;
	_AnyTier_identifyClass ((Data) grid -> tiers -> item [_selectedTier], & intervalTier, & textTier);
	if (intervalTier) {
		long n = intervalTier -> intervals -> size;
		if (n >= 2) {
			TextInterval interval;
			long iinterval = IntervalTier_timeToIndex (intervalTier, _startSelection);
			if (shift) {
				long binterval = IntervalTier_timeToIndex (intervalTier, _startSelection);
				long einterval = IntervalTier_timeToIndex (intervalTier, _endSelection);
				if (_endSelection == intervalTier -> xmax) einterval ++;
				if (binterval < iinterval && einterval > iinterval + 1) {
					interval = (TextInterval) intervalTier -> intervals -> item [iinterval];
					_startSelection = interval -> xmin;
					_endSelection = interval -> xmax;
				} else if (previous) {
					if (einterval > iinterval + 1) {
						if (einterval <= n + 1) {
							interval = (TextInterval) intervalTier -> intervals -> item [einterval - 1];
							_endSelection = interval -> xmin;
						}
					} else if (binterval > 1) {
						interval = (TextInterval) intervalTier -> intervals -> item [binterval - 1];
						_startSelection = interval -> xmin;
					}
				} else {
					if (binterval < iinterval) {
						if (binterval > 0) {
							interval = (TextInterval) intervalTier -> intervals -> item [binterval];
							_startSelection = interval -> xmax;
						}
					} else if (einterval <= n) {
						interval = (TextInterval) intervalTier -> intervals -> item [einterval];
						_endSelection = interval -> xmax;
					}
				}
			} else {
				iinterval = previous ?
					iinterval > 1 ? iinterval - 1 : n :
					iinterval < n ? iinterval + 1 : 1;
				interval = (TextInterval) intervalTier -> intervals -> item [iinterval];
				_startSelection = interval -> xmin;
				_endSelection = interval -> xmax;
			}
			scrollToView (iinterval == n ? _startSelection : iinterval == 1 ? _endSelection : (_startSelection + _endSelection) / 2);
		}
	} else {
		long n = textTier -> points -> size;
		if (n >= 2) {
			TextPoint point;
			long ipoint = AnyTier_timeToHighIndex (textTier, _startSelection);
			ipoint = previous ?
				ipoint > 1 ? ipoint - 1 : n :
				ipoint < n ? ipoint + 1 : 1;
			point = (TextPoint) textTier -> points -> item [ipoint];
			_startSelection = _endSelection = point -> time;
			scrollToView (_startSelection);
		}
	}
}

static int menu_cb_SelectPreviousInterval (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	editor->do_selectAdjacentInterval (TRUE, FALSE);
	return 1;
}

static int menu_cb_SelectNextInterval (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	editor->do_selectAdjacentInterval (FALSE, FALSE);
	return 1;
}

static int menu_cb_ExtendSelectPreviousInterval (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	editor->do_selectAdjacentInterval (TRUE, TRUE);
	return 1;
}

static int menu_cb_ExtendSelectNextInterval (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	editor->do_selectAdjacentInterval (FALSE, TRUE);
	return 1;
}

static int menu_cb_MoveBtoZero (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	double zero = Sound_getNearestZeroCrossing (editor->_sound.data, editor->_startSelection, 1);   // STEREO BUG
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

static int menu_cb_MoveCursorToZero (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	double zero = Sound_getNearestZeroCrossing (editor->_sound.data, 0.5 * (editor->_startSelection + editor->_endSelection), 1);   // STEREO BUG
	if (NUMdefined (zero)) {
		editor->_startSelection = editor->_endSelection = zero;
		editor->marksChanged ();
	}
	return 1;
}

static int menu_cb_MoveEtoZero (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	double zero = Sound_getNearestZeroCrossing (editor->_sound.data, editor->_endSelection, 1);   // STEREO BUG
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

/***** PITCH MENU *****/

static int menu_cb_DrawTextGridAndPitch (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	EDITOR_FORM (L"Draw TextGrid and Pitch separately", 0)
		editor->form_pictureWindow (cmd);
		LABEL (L"", L"TextGrid:")
		BOOLEAN (L"Show boundaries and points", 1);
		LABEL (L"", L"Pitch:")
		BOOLEAN (L"Speckle", 0);
		editor->form_pictureMargins (cmd);
		editor->form_pictureSelection (cmd);
		BOOLEAN (L"Garnish", 1);
	EDITOR_OK
		editor->ok_pictureWindow (cmd);
		SET_INTEGER (L"Show boundaries and points", preferences.picture.showBoundaries);
		SET_INTEGER (L"Speckle", preferences.picture.pitch.speckle);
		editor->ok_pictureMargins (cmd);
		editor->ok_pictureSelection (cmd);
		SET_INTEGER (L"Garnish", preferences.picture.garnish);
	EDITOR_DO
		editor->do_pictureWindow (cmd);
		preferences.picture.showBoundaries = GET_INTEGER (L"Show boundaries and points");
		preferences.picture.pitch.speckle = GET_INTEGER (L"Speckle");
		editor->do_pictureMargins (cmd);
		editor->do_pictureSelection (cmd);
		preferences.picture.garnish = GET_INTEGER (L"Garnish");
		if (! editor->_pitch.show)
			return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
		if (! editor->_pitch.data) {
			editor->computePitch ();
			if (! editor->_pitch.data) return Melder_error1 (L"Cannot compute pitch.");
		}
		editor->openPraatPicture ();
		double pitchFloor_hidden = ClassFunction_convertStandardToSpecialUnit (classPitch, editor->_pitch.floor, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
		double pitchCeiling_hidden = ClassFunction_convertStandardToSpecialUnit (classPitch, editor->_pitch.ceiling, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
		double pitchFloor_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchFloor_hidden, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
		double pitchCeiling_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchCeiling_hidden, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
		double pitchViewFrom_overt = editor->_pitch.viewFrom < editor->_pitch.viewTo ? editor->_pitch.viewFrom : pitchFloor_overt;
		double pitchViewTo_overt = editor->_pitch.viewFrom < editor->_pitch.viewTo ? editor->_pitch.viewTo : pitchCeiling_overt;
		TextGrid_Pitch_drawSeparately ((TextGrid) editor->_data, editor->_pitch.data, editor->_pictureGraphics, editor->_startWindow, editor->_endWindow,
			pitchViewFrom_overt, pitchViewTo_overt, GET_INTEGER (L"Show boundaries and points"), editor->_useTextStyles, GET_INTEGER (L"Garnish"),
			GET_INTEGER (L"Speckle"), editor->_pitch.unit);
		editor->garnish ();
		editor->closePraatPicture ();
	EDITOR_END
}

/***** INTERVAL MENU *****/

int TextGridEditor::insertBoundaryOrPoint (int itier, double t1, double t2, bool insertSecond) {
	TextGrid grid = (TextGrid) _data;
	IntervalTier intervalTier;
	TextTier textTier;
	int ntiers = grid -> tiers -> size;
	if (itier < 1 || itier > ntiers) return 0;
	_AnyTier_identifyClass ((Data) grid -> tiers -> item [itier], & intervalTier, & textTier);
	Melder_assert (t1 <= t2);

	if (intervalTier) {
		TextInterval rightNewInterval = NULL, midNewInterval = NULL;
		bool t1IsABoundary = IntervalTier_hasTime (intervalTier, t1);
		bool t2IsABoundary = IntervalTier_hasTime (intervalTier, t2);
		if (t1 == t2 && t1IsABoundary) {
			Melder_error3 (L"Cannot add a boundary at ", Melder_fixed (t1, 6), L" seconds, because there is already a boundary there.");
			Melder_flushError (NULL);
			return 0;
		} else if (t1IsABoundary && t2IsABoundary) {
			Melder_error5 (L"Cannot add boundaries at ", Melder_fixed (t1, 6), L" and ", Melder_fixed (t2, 6), L" seconds, because there are already boundaries there.");
			Melder_flushError (NULL);
			return 0;
		}
		long iinterval = IntervalTier_timeToIndex (intervalTier, t1);
		//Melder_casual ("iinterval %ld, t = %f", iinterval, t1);
		long iinterval2 = t1 == t2 ? iinterval : IntervalTier_timeToIndex (intervalTier, t2);
		//Melder_casual ("iinterval2 %ld, t = %f", iinterval2, t2);
		if (iinterval == 0 || iinterval2 == 0) {
			return 0;   // selection is outside time domain of intervals
		}
		long correctedIinterval2 = t2IsABoundary && iinterval2 == intervalTier -> intervals -> size ? iinterval2 + 1 : iinterval2;
		if (correctedIinterval2 > iinterval + 1 || (correctedIinterval2 > iinterval && ! t2IsABoundary)) {
			return 0;   // selection straddles a boundary
		}
		TextInterval interval = (TextInterval) intervalTier -> intervals -> item [iinterval];

		if (t1 == t2) {
			save (L"Add boundary");
		} else {
			save (L"Add interval");
		}

		if (itier == _selectedTier) {
			/*
			 * Divide up the label text into left, mid and right, depending on where the text selection is.
			 */
			long left, right;
			wchar_t *text = GuiText_getStringAndSelectionPosition (_text, & left, & right);
			rightNewInterval = TextInterval_create (t2, interval -> xmax, text + right);
			text [right] = '\0';
			midNewInterval = TextInterval_create (t1, t2, text + left);
			text [left] = '\0';
			TextInterval_setText (interval, text);
			Melder_free (text);
		} else {
			/*
			 * Move the text to the left of the boundary.
			 */
			rightNewInterval = TextInterval_create (t2, interval -> xmax, L"");
			midNewInterval = TextInterval_create (t1, t2, L"");
		}
		if (t1IsABoundary) {
			/*
			 * Merge mid with left interval.
			 */
			if (interval -> xmin != t1)
				Melder_fatal ("Boundary unequal: %.17g versus %.17g.", interval -> xmin, t1);
			interval -> xmax = t2;
			TextInterval_setText (interval, Melder_wcscat2 (interval -> text, midNewInterval -> text));
			forget (midNewInterval);
		} else if (t2IsABoundary) {
			/*
			 * Merge mid and right interval.
			 */
			if (interval -> xmax != t2)
				Melder_fatal ("Boundary unequal: %.17g versus %.17g.", interval -> xmax, t2);
			interval -> xmax = t1;
			Melder_assert (rightNewInterval -> xmin == t2);
			Melder_assert (rightNewInterval -> xmax == t2);
			rightNewInterval -> xmin = t1;
			TextInterval_setText (rightNewInterval, Melder_wcscat2 (midNewInterval -> text, rightNewInterval -> text));
			forget (midNewInterval);
		} else {
			interval -> xmax = t1;
			if (t1 != t2) Collection_addItem (intervalTier -> intervals, midNewInterval);
		}
		Collection_addItem (intervalTier -> intervals, rightNewInterval);
		if (insertSecond && ntiers >= 2) {
			/*
			 * Find the last time before t on another tier.
			 */
			double tlast = interval -> xmin, tmin, tmax;
			for (int jtier = 1; jtier <= ntiers; jtier ++) if (jtier != itier) {
				_timeToInterval (t1, jtier, & tmin, & tmax);
				if (tmin > tlast) {
					tlast = tmin;
				}
			}
			if (tlast > interval -> xmin && tlast < t1) {
				TextInterval newInterval = TextInterval_create (tlast, t1, L"");
				interval -> xmax = tlast;
				Collection_addItem (intervalTier -> intervals, newInterval);
			}
		}
	} else {
		TextPoint newPoint;
		if (AnyTier_hasPoint (textTier, t1)) {
			Melder_flushError ("Cannot add a point at %f seconds, because there is already a point there.", t1);
			return 0;
		} 

		save (L"Add point");

		newPoint = TextPoint_create (t1, L"");
		Collection_addItem (textTier -> points, newPoint);
	}
	_startSelection = _endSelection = t1;
	return 1;
}

void TextGridEditor::do_insertIntervalOnTier (int itier) {
	if (! insertBoundaryOrPoint (itier,
		_playingCursor || _playingSelection ? _playCursor : _startSelection,
		_playingCursor || _playingSelection ? _playCursor : _endSelection,
		true)) return;
	_selectedTier = itier;
	marksChanged ();
	broadcastChange ();
}

static int menu_cb_InsertIntervalOnTier1 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertIntervalOnTier (1); return 1; }
static int menu_cb_InsertIntervalOnTier2 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertIntervalOnTier (2); return 1; }
static int menu_cb_InsertIntervalOnTier3 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertIntervalOnTier (3); return 1; }
static int menu_cb_InsertIntervalOnTier4 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertIntervalOnTier (4); return 1; }
static int menu_cb_InsertIntervalOnTier5 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertIntervalOnTier (5); return 1; }
static int menu_cb_InsertIntervalOnTier6 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertIntervalOnTier (6); return 1; }
static int menu_cb_InsertIntervalOnTier7 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertIntervalOnTier (7); return 1; }
static int menu_cb_InsertIntervalOnTier8 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertIntervalOnTier (8); return 1; }

/***** BOUNDARY/POINT MENU *****/

static int menu_cb_RemovePointOrBoundary (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	TextGrid grid = (TextGrid) editor->_data;
	Data anyTier;
	if (! editor->checkTierSelection (L"remove a point or boundary")) return 0;
	anyTier = (Data) grid -> tiers -> item [editor->_selectedTier];
	if (anyTier -> methods == (Data_Table) classIntervalTier) {
		IntervalTier tier = (IntervalTier) anyTier;
		long selectedLeftBoundary = editor->getSelectedLeftBoundary ();
		if (! selectedLeftBoundary) return Melder_error1 (L"To remove a boundary, first click on it.");

		editor->save (L"Remove boundary");
		IntervalTier_removeLeftBoundary (tier, selectedLeftBoundary); iferror return 0;
	} else {
		TextTier tier = (TextTier) anyTier;
		long selectedPoint = editor->getSelectedPoint ();
		if (! selectedPoint) return Melder_error1 (L"To remove a point, first click on it.");

		editor->save (L"Remove point");
		Collection_removeItem (tier -> points, selectedPoint);
	}
	editor->updateText ();
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

int TextGridEditor::do_movePointOrBoundary (int where) {
	double position;
	TextGrid grid = (TextGrid) _data;
	Data anyTier;
	if (where == 0 && _sound.data == NULL) return 1;
	if (! checkTierSelection (L"move a point or boundary")) return 0;
	anyTier = (Data) grid -> tiers -> item [_selectedTier];
	if (anyTier -> methods == (Data_Table) classIntervalTier) {
		IntervalTier tier = (IntervalTier) anyTier;
		static const wchar_t *boundarySaveText [3] = { L"Move boundary to zero crossing", L"Move boundary to B", L"Move boundary to E" };
		TextInterval left, right;
		long selectedLeftBoundary = getSelectedLeftBoundary ();
		if (! selectedLeftBoundary) return Melder_error1 (L"To move a boundary, first click on it.");
		left = (TextInterval) tier -> intervals -> item [selectedLeftBoundary - 1];
		right = (TextInterval) tier -> intervals -> item [selectedLeftBoundary];
		position = where == 1 ? _startSelection : where == 2 ? _endSelection :
			Sound_getNearestZeroCrossing (_sound.data, left -> xmax, 1);   // STEREO BUG
		if (position == NUMundefined) return 1;
		if (position <= left -> xmin || position >= right -> xmax)
			{ Melder_beep (); return 0; }

		save (boundarySaveText [where]);

		left -> xmax = right -> xmin = _startSelection = _endSelection = position;
	} else {
		TextTier tier = (TextTier) anyTier;
		static const wchar_t *pointSaveText [3] = { L"Move point to zero crossing", L"Move point to B", L"Move point to E" };
		TextPoint point;
		long selectedPoint = getSelectedPoint ();
		if (! selectedPoint) return Melder_error1 (L"To move a point, first click on it.");
		point = (TextPoint) tier -> points -> item [selectedPoint];
		position = where == 1 ? _startSelection : where == 2 ? _endSelection :
			Sound_getNearestZeroCrossing (_sound.data, point -> time, 1);   // STEREO BUG
		if (position == NUMundefined) return 1;

		save (pointSaveText [where]);

		point -> time = _startSelection = _endSelection = position;
	}
	marksChanged ();   /* Because cursor has moved. */
	broadcastChange ();
	return 1;
}

static int menu_cb_MoveToB (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	if (! editor->do_movePointOrBoundary (1)) return 0;
	return 1;
}

static int menu_cb_MoveToE (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	if (! editor->do_movePointOrBoundary (2)) return 0;
	return 1;
}

static int menu_cb_MoveToZero (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	if (! editor->do_movePointOrBoundary (0)) return 0;
	return 1;
}

void TextGridEditor::do_insertOnTier (int itier) {
	if (! insertBoundaryOrPoint (itier,
		_playingCursor || _playingSelection ? _playCursor : _startSelection,
		_playingCursor || _playingSelection ? _playCursor : _endSelection,
		false)) return;
	_selectedTier = itier;
	marksChanged ();
	broadcastChange ();
}

static int menu_cb_InsertOnSelectedTier (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	editor->do_insertOnTier (editor->_selectedTier);
	return 1;
}

static int menu_cb_InsertOnTier1 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertOnTier (1); return 1; }
static int menu_cb_InsertOnTier2 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertOnTier (2); return 1; }
static int menu_cb_InsertOnTier3 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertOnTier (3); return 1; }
static int menu_cb_InsertOnTier4 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertOnTier (4); return 1; }
static int menu_cb_InsertOnTier5 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertOnTier (5); return 1; }
static int menu_cb_InsertOnTier6 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertOnTier (6); return 1; }
static int menu_cb_InsertOnTier7 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertOnTier (7); return 1; }
static int menu_cb_InsertOnTier8 (EDITOR_ARGS) { TextGridEditor *editor = (TextGridEditor *)editor_me; editor->do_insertOnTier (8); return 1; }

static int menu_cb_InsertOnAllTiers (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	TextGrid grid = (TextGrid) editor->_data;
	int saveTier = editor->_selectedTier, itier;
	for (itier = 1; itier <= grid -> tiers -> size; itier ++) {
		editor->do_insertOnTier (itier);
	}
	editor->_selectedTier = saveTier;
	return 1;
}

/***** SEARCH MENU *****/

int TextGridEditor::findInTier () {
	TextGrid grid = (TextGrid) _data;
	Data anyTier;
	if (! checkTierSelection (L"find a text")) return 0;
	anyTier = (Data) grid -> tiers -> item [_selectedTier];
	if (anyTier -> methods == (Data_Table) classIntervalTier) {
		IntervalTier tier = (IntervalTier) anyTier;
		long iinterval = IntervalTier_timeToIndex (tier, _startSelection) + 1;
		while (iinterval <= tier -> intervals -> size) {
			TextInterval interval = (TextInterval) tier -> intervals -> item [iinterval];
			wchar_t *text = interval -> text;
			if (text) {
				wchar_t *position = wcsstr (text, _findString);
				if (position) {
					_startSelection = interval -> xmin;
					_endSelection = interval -> xmax;
					scrollToView (_startSelection);
					GuiText_setSelection (_text, position - text, position - text + wcslen (_findString));
					return 1;
				}
			}
			iinterval ++;
		}
		if (iinterval > tier -> intervals -> size)
			Melder_beep ();
	} else {
		TextTier tier = (TextTier) anyTier;
		long ipoint = AnyTier_timeToLowIndex (tier, _startSelection) + 1;
		while (ipoint <= tier -> points -> size) {
			TextPoint point = (TextPoint) tier -> points -> item [ipoint];
			wchar_t *text = point -> mark;
			if (text) {
				wchar_t *position = wcsstr (text, _findString);
				if (position) {
					_startSelection = _endSelection = point -> time;
					scrollToView (point -> time);
					GuiText_setSelection (_text, position - text, position - text + wcslen (_findString));
					return 1;
				}
			}
			ipoint ++;
		}
		if (ipoint > tier -> points -> size)
			Melder_beep ();
	}
	return 1;
}

void TextGridEditor::do_find () {
	if (_findString) {
		long left, right;
		wchar_t *label = GuiText_getStringAndSelectionPosition (_text, & left, & right);
		wchar_t *position = wcsstr (label + right, _findString);   /* CRLF BUG? */
		if (position) {
			GuiText_setSelection (_text, position - label, position - label + wcslen (_findString));
		} else {
			if (! findInTier ()) Melder_flushError (NULL);
		}
		Melder_free (label);
	}
}

static int menu_cb_Find (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	EDITOR_FORM (L"Find text", 0)
		LABEL (L"", L"Text:")
		TEXTFIELD (L"string", L"")
	EDITOR_OK
	EDITOR_DO
		Melder_free (editor->_findString);
		editor->_findString = Melder_wcsdup_f (GET_STRING (L"string"));
		editor->do_find ();
	EDITOR_END
}

static int menu_cb_FindAgain (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	editor->do_find ();
	return 1;
}

int TextGridEditor::checkSpellingInTier () {
	TextGrid grid = (TextGrid) _data;
	Data anyTier;
	if (! checkTierSelection (L"check spelling")) return 0;
	anyTier = (Data) grid -> tiers -> item [_selectedTier];
	if (anyTier -> methods == (Data_Table) classIntervalTier) {
		IntervalTier tier = (IntervalTier) anyTier;
		long iinterval = IntervalTier_timeToIndex (tier, _startSelection) + 1;
		while (iinterval <= tier -> intervals -> size) {
			TextInterval interval = (TextInterval) tier -> intervals -> item [iinterval];
			wchar_t *text = interval -> text;
			if (text) {
				long position = 0;
				wchar_t *notAllowed = SpellingChecker_nextNotAllowedWord (_spellingChecker, text, & position);
				if (notAllowed) {
					_startSelection = interval -> xmin;
					_endSelection = interval -> xmax;
					scrollToView (_startSelection);
					GuiText_setSelection (_text, position, position + wcslen (notAllowed));
					return 1;
				}
			}
			iinterval ++;
		}
		if (iinterval > tier -> intervals -> size)
			Melder_beep ();
	} else {
		TextTier tier = (TextTier) anyTier;
		long ipoint = AnyTier_timeToLowIndex (tier, _startSelection) + 1;
		while (ipoint <= tier -> points -> size) {
			TextPoint point = (TextPoint) tier -> points -> item [ipoint];
			wchar_t *text = point -> mark;
			if (text) {
				long position = 0;
				wchar_t *notAllowed = SpellingChecker_nextNotAllowedWord (_spellingChecker, text, & position);
				if (notAllowed) {
					_startSelection = _endSelection = point -> time;
					scrollToView (point -> time);
					GuiText_setSelection (_text, position, position + wcslen (notAllowed));
					return 1;
				}
			}
			ipoint ++;
		}
		if (ipoint > tier -> points -> size)
			Melder_beep ();
	}
	return 1;
}

static int menu_cb_CheckSpelling (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	if (editor->_spellingChecker) {
		long left, right;
		wchar_t *label = GuiText_getStringAndSelectionPosition (editor->_text, & left, & right);
		long position = right;
		wchar_t *notAllowed = SpellingChecker_nextNotAllowedWord (editor->_spellingChecker, label, & position);
		if (notAllowed) {
			GuiText_setSelection (editor->_text, position, position + wcslen (notAllowed));
		} else {
			if (! editor->checkSpellingInTier ()) Melder_flushError (NULL);
		}
		Melder_free (label);
	}
	return 1;
}

static int menu_cb_CheckSpellingInInterval (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	if (editor->_spellingChecker) {
		long left, right;
		wchar_t *label = GuiText_getStringAndSelectionPosition (editor->_text, & left, & right);
		long position = right;
		wchar_t *notAllowed = SpellingChecker_nextNotAllowedWord (editor->_spellingChecker, label, & position);
		if (notAllowed) {
			GuiText_setSelection (editor->_text, position, position + wcslen (notAllowed));
		}
		Melder_free (label);
	}
	return 1;
}

static int menu_cb_AddToUserDictionary (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	if (editor->_spellingChecker) {
		wchar_t *word = GuiText_getSelection (editor->_text);
		SpellingChecker_addNewWord (editor->_spellingChecker, word);
		Melder_free (word);
		iferror return 0;
		if (editor->_dataChangedCallback)
			editor->_dataChangedCallback (editor, editor->_dataChangedClosure, editor->_spellingChecker);
	}
	return 1;
}

/***** TIER MENU *****/

static int menu_cb_RenameTier (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	EDITOR_FORM (L"Rename tier", 0)
		SENTENCE (L"Name", L"");
	EDITOR_OK
		TextGrid grid = (TextGrid) editor->_data;
		Data tier;
		if (! editor->checkTierSelection (L"rename a tier")) return 0;
		tier = (Data) grid -> tiers -> item [editor->_selectedTier];
		SET_STRING (L"Name", tier -> name ? tier -> name : L"")
	EDITOR_DO
		wchar_t *newName = GET_STRING (L"Name");
		TextGrid grid = (TextGrid) editor->_data;
		Data tier;
		if (! editor->checkTierSelection (L"rename a tier")) return 0;
		tier = (Data) grid -> tiers -> item [editor->_selectedTier];

		editor->save (L"Rename tier");

		Thing_setName (tier, newName);

		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

static int menu_cb_PublishTier (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	TextGrid publish = NULL;
//start:
	TextGrid grid = (TextGrid) editor->_data;
	editor->checkTierSelection (L"publish a tier"); cherror
	if (editor->_publishCallback) {
		Data tier = (Data) grid -> tiers -> item [editor->_selectedTier];
		publish = TextGrid_createWithoutTiers (1e30, -1e30); cherror
		TextGrid_add (publish, tier); cherror
		Thing_setName (publish, tier -> name); cherror
		editor->_publishCallback (editor, editor->_publishClosure, publish);
	}
end:
	iferror return 0;
	return 1;
}

static int menu_cb_RemoveAllTextFromTier (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	TextGrid grid = (TextGrid) editor->_data;
	IntervalTier intervalTier;
	TextTier textTier;
	if (! editor->checkTierSelection (L"remove all text from a tier")) return 0;
	_AnyTier_identifyClass ((Data) grid -> tiers -> item [editor->_selectedTier], & intervalTier, & textTier);

	editor->save (L"Remove text from tier");
	if (intervalTier) {
		IntervalTier_removeText (intervalTier);
	} else {
		TextTier_removeText (textTier);
	}

	editor->updateText ();
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_RemoveTier (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	TextGrid grid = (TextGrid) editor->_data;
	if (grid -> tiers -> size <= 1) {
		return Melder_error1 (L"Sorry, I refuse to remove the last tier.");
	}
	if (! editor->checkTierSelection (L"remove a tier")) return 0;

	editor->save (L"Remove tier");
	Collection_removeItem (grid -> tiers, editor->_selectedTier);

	editor->_selectedTier = 1;
	editor->updateText ();
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_AddIntervalTier (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	EDITOR_FORM (L"Add interval tier", 0)
		NATURAL (L"Position", L"1 (= at top)")
		SENTENCE (L"Name", L"")
	EDITOR_OK
		TextGrid grid = (TextGrid) editor->_data;
		static MelderString text = { 0 };
		MelderString_empty (& text);
		MelderString_append2 (& text, Melder_integer (grid -> tiers -> size + 1), L" (= at bottom)");
		SET_STRING (L"Position", text.string)
		SET_STRING (L"Name", L"")
	EDITOR_DO
		TextGrid grid = (TextGrid) editor->_data;
		int position = GET_INTEGER (L"Position");
		wchar_t *name = GET_STRING (L"Name");
		IntervalTier tier = IntervalTier_create (grid -> xmin, grid -> xmax);
		if (! tier) return 0;
		if (position > grid -> tiers -> size) position = grid -> tiers -> size + 1;
		Thing_setName (tier, name);

		editor->save (L"Add interval tier");
		Ordered_addItemPos (grid -> tiers, tier, position);

		editor->_selectedTier = position;
		editor->updateText ();
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

static int menu_cb_AddPointTier (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	EDITOR_FORM (L"Add point tier", 0)
		NATURAL (L"Position", L"1 (= at top)")
		SENTENCE (L"Name", L"");
	EDITOR_OK
		TextGrid grid = (TextGrid) editor->_data;
		static MelderString text = { 0 };
		MelderString_empty (& text);
		MelderString_append2 (& text, Melder_integer (grid -> tiers -> size + 1), L" (= at bottom)");
		SET_STRING (L"Position", text.string)
		SET_STRING (L"Name", L"")
	EDITOR_DO
		TextGrid grid = (TextGrid) editor->_data;
		int position = GET_INTEGER (L"Position");
		wchar_t *name = GET_STRING (L"Name");
		TextTier tier = TextTier_create (grid -> xmin, grid -> xmax);
		if (! tier) return 0;
		if (position > grid -> tiers -> size) position = grid -> tiers -> size + 1;
		Thing_setName (tier, name);

		editor->save (L"Add point tier");
		Ordered_addItemPos (grid -> tiers, tier, position);

		editor->_selectedTier = position;
		editor->updateText ();
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

static int menu_cb_DuplicateTier (EDITOR_ARGS) {
	TextGridEditor *editor = (TextGridEditor *)editor_me;
	EDITOR_FORM (L"Duplicate tier", 0)
		NATURAL (L"Position", L"1 (= at top)")
		SENTENCE (L"Name", L"")
	EDITOR_OK
		TextGrid grid = (TextGrid) editor->_data;
		if (editor->_selectedTier) {
			SET_STRING (L"Position", Melder_integer (editor->_selectedTier + 1))
			SET_STRING (L"Name", ((AnyTier) grid -> tiers -> item [editor->_selectedTier]) -> name)
		}
	EDITOR_DO
		TextGrid grid = (TextGrid) editor->_data;
		int position = GET_INTEGER (L"Position");
		wchar_t *name = GET_STRING (L"Name");
		AnyTier tier, newTier;
		if (! editor->checkTierSelection (L"duplicate a tier")) return 0;
		tier = (AnyTier) grid -> tiers -> item [editor->_selectedTier];
		newTier = (AnyTier) Data_copy (tier);
		if (! newTier) return 0;
			if (position > grid -> tiers -> size) position = grid -> tiers -> size + 1;
		Thing_setName (newTier, name);

		editor->save (L"Duplicate tier");
		Ordered_addItemPos (grid -> tiers, newTier, position);

		editor->_selectedTier = position;
		editor->updateText ();
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

/***** HELP MENU *****/

static int menu_cb_TextGridEditorHelp (EDITOR_ARGS) { Melder_help (L"TextGridEditor"); return 1; }
static int menu_cb_AboutSpecialSymbols (EDITOR_ARGS) { Melder_help (L"Special symbols"); return 1; }
static int menu_cb_PhoneticSymbols (EDITOR_ARGS) { Melder_help (L"Phonetic symbols"); return 1; }
static int menu_cb_AboutTextStyles (EDITOR_ARGS) { Melder_help (L"Text styles"); return 1; }

void TextGridEditor::createMenus () {
	EditorMenu *menu = getMenu (L"File");
	_extractSelectedTextGridPreserveTimesButton =
		menu->addCommand (L"Extract selected TextGrid (preserve times)", 0, menu_cb_ExtractSelectedTextGrid_preserveTimes) -> _itemWidget;
	_extractSelectedTextGridTimeFromZeroButton =
		menu->addCommand (L"Extract selected TextGrid (time from zero)", 0, menu_cb_ExtractSelectedTextGrid_timeFromZero) -> _itemWidget;

	menu->addCommand (L"Save TextGrid as text file...", 'S', menu_cb_WriteToTextFile);
	_writeSelectedTextGridButton = menu->addCommand (L"Save selected TextGrid to text file...", 0, menu_cb_WriteSelectionToTextFile) -> _itemWidget;

	menu->addCommand (L"Draw visible TextGrid...", 0, menu_cb_DrawVisibleTextGrid);
	if (_sound.data || _longSound.data)
		menu->addCommand (L"Draw visible sound and TextGrid...", 0, menu_cb_DrawVisibleSoundAndTextGrid);

	menu = getMenu (L"Edit");
	#ifndef macintosh
		menu->addCommand (L"-- cut copy paste --", 0, NULL);
		menu->addCommand (L"Cut text", 'X', menu_cb_Cut);
		menu->addCommand (L"Cut", Editor_HIDDEN, menu_cb_Cut);
		menu->addCommand (L"Copy text", 'C', menu_cb_Copy);
		menu->addCommand (L"Copy", Editor_HIDDEN, menu_cb_Copy);
		menu->addCommand (L"Paste text", 'V', menu_cb_Paste);
		menu->addCommand (L"Paste", Editor_HIDDEN, menu_cb_Paste);
		menu->addCommand (L"Erase text", 0, menu_cb_Erase);
		menu->addCommand (L"Erase", Editor_HIDDEN, menu_cb_Erase);
	#endif
	menu->addCommand (L"-- encoding --", 0, NULL);
	menu->addCommand (L"Convert entire TextGrid to backslash trigraphs", 0, menu_cb_Genericize);
	menu->addCommand (L"Genericize entire TextGrid", Editor_HIDDEN, menu_cb_Genericize);
	menu->addCommand (L"Genericize", Editor_HIDDEN, menu_cb_Genericize);
	menu->addCommand (L"Convert entire TextGrid to Unicode", 0, menu_cb_Nativize);
	menu->addCommand (L"Nativize entire TextGrid", Editor_HIDDEN, menu_cb_Nativize);
	menu->addCommand (L"Nativize", Editor_HIDDEN, menu_cb_Nativize);
	menu->addCommand (L"-- search --", 0, NULL);
	menu->addCommand (L"Find...", 'F', menu_cb_Find);
	menu->addCommand (L"Find again", 'G', menu_cb_FindAgain);

	if (_sound.data) {
		menu = getMenu (L"Select");
		menu->addCommand (L"-- move to zero --", 0, 0);
		menu->addCommand (L"Move start of selection to nearest zero crossing", ',', menu_cb_MoveBtoZero);
		menu->addCommand (L"Move begin of selection to nearest zero crossing", Editor_HIDDEN, menu_cb_MoveBtoZero);
		menu->addCommand (L"Move cursor to nearest zero crossing", '0', menu_cb_MoveCursorToZero);
		menu->addCommand (L"Move end of selection to nearest zero crossing", '.', menu_cb_MoveEtoZero);
	}

	menu = getMenu (L"Query");
	menu->addCommand (L"-- query interval --", 0, NULL);
	menu->addCommand (L"Get starting point of interval", 0, menu_cb_GetStartingPointOfInterval);
	menu->addCommand (L"Get end point of interval", 0, menu_cb_GetEndPointOfInterval);
	menu->addCommand (L"Get label of interval", 0, menu_cb_GetLabelOfInterval);

	menu = addMenu (L"Interval", 0);
	menu->addCommand (L"Add interval on tier 1", GuiMenu_COMMAND | '1', menu_cb_InsertIntervalOnTier1);
	menu->addCommand (L"Add interval on tier 2", GuiMenu_COMMAND | '2', menu_cb_InsertIntervalOnTier2);
	menu->addCommand (L"Add interval on tier 3", GuiMenu_COMMAND | '3', menu_cb_InsertIntervalOnTier3);
	menu->addCommand (L"Add interval on tier 4", GuiMenu_COMMAND | '4', menu_cb_InsertIntervalOnTier4);
	menu->addCommand (L"Add interval on tier 5", GuiMenu_COMMAND | '5', menu_cb_InsertIntervalOnTier5);
	menu->addCommand (L"Add interval on tier 6", GuiMenu_COMMAND | '6', menu_cb_InsertIntervalOnTier6);
	menu->addCommand (L"Add interval on tier 7", GuiMenu_COMMAND | '7', menu_cb_InsertIntervalOnTier7);
	menu->addCommand (L"Add interval on tier 8", GuiMenu_COMMAND | '8', menu_cb_InsertIntervalOnTier8);

	menu = addMenu (L"Boundary", 0);
	/*menu->addCommand (L"Move to B", 0, menu_cb_MoveToB);
	menu->addCommand (L"Move to E", 0, menu_cb_MoveToE);*/
	if (_sound.data)
		menu->addCommand (L"Move to nearest zero crossing", 0, menu_cb_MoveToZero);
	menu->addCommand (L"-- insert boundary --", 0, NULL);
	menu->addCommand (L"Add on selected tier", GuiMenu_ENTER, menu_cb_InsertOnSelectedTier);
	menu->addCommand (L"Add on tier 1", GuiMenu_COMMAND | GuiMenu_F1, menu_cb_InsertOnTier1);
	menu->addCommand (L"Add on tier 2", GuiMenu_COMMAND | GuiMenu_F2, menu_cb_InsertOnTier2);
	menu->addCommand (L"Add on tier 3", GuiMenu_COMMAND | GuiMenu_F3, menu_cb_InsertOnTier3);
	menu->addCommand (L"Add on tier 4", GuiMenu_COMMAND | GuiMenu_F4, menu_cb_InsertOnTier4);
	menu->addCommand (L"Add on tier 5", GuiMenu_COMMAND | GuiMenu_F5, menu_cb_InsertOnTier5);
	menu->addCommand (L"Add on tier 6", GuiMenu_COMMAND | GuiMenu_F6, menu_cb_InsertOnTier6);
	menu->addCommand (L"Add on tier 7", GuiMenu_COMMAND | GuiMenu_F7, menu_cb_InsertOnTier7);
	menu->addCommand (L"Add on tier 8", GuiMenu_COMMAND | GuiMenu_F8, menu_cb_InsertOnTier8);
	menu->addCommand (L"Add on all tiers", GuiMenu_COMMAND | GuiMenu_F9, menu_cb_InsertOnAllTiers);
	menu->addCommand (L"-- remove mark --", 0, NULL);
	menu->addCommand (L"Remove", GuiMenu_OPTION | GuiMenu_BACKSPACE, menu_cb_RemovePointOrBoundary);

	menu = addMenu (L"Tier", 0);
	menu->addCommand (L"Add interval tier...", 0, menu_cb_AddIntervalTier);
	menu->addCommand (L"Add point tier...", 0, menu_cb_AddPointTier);
	menu->addCommand (L"Duplicate tier...", 0, menu_cb_DuplicateTier);
	menu->addCommand (L"Rename tier...", 0, menu_cb_RenameTier);
	menu->addCommand (L"-- remove tier --", 0, NULL);
	menu->addCommand (L"Remove all text from tier", 0, menu_cb_RemoveAllTextFromTier);
	menu->addCommand (L"Remove entire tier", 0, menu_cb_RemoveTier);
	menu->addCommand (L"-- extract tier --", 0, NULL);
	menu->addCommand (L"Extract to list of objects:", GuiMenu_INSENSITIVE, menu_cb_PublishTier /* dummy */);
	menu->addCommand (L"Extract entire selected tier", 0, menu_cb_PublishTier);

	if (_spellingChecker) {
		menu = addMenu (L"Spell", 0);
		menu->addCommand (L"Check spelling in tier", GuiMenu_COMMAND | GuiMenu_OPTION | 'L', menu_cb_CheckSpelling);
		menu->addCommand (L"Check spelling in interval", 0, menu_cb_CheckSpellingInInterval);
		menu->addCommand (L"-- edit lexicon --", 0, NULL);
		menu->addCommand (L"Add selected word to user dictionary", 0, menu_cb_AddToUserDictionary);
	}

	/*if (_sound.data || _longSound.data) { // FIXME
		createMenus_analysis ();   // Insert some of the ancestor's menus *after* the TextGrid menus.
	}*/

	menu = getMenu (L"Help");
	menu->addCommand (L"TextGridEditor help", '?', menu_cb_TextGridEditorHelp);
	menu->addCommand (L"About special symbols", 0, menu_cb_AboutSpecialSymbols);
	menu->addCommand (L"Phonetic symbols", 0, menu_cb_PhoneticSymbols);
	menu->addCommand (L"About text styles", 0, menu_cb_AboutTextStyles);
}

/***** CHILDREN *****/

static void gui_text_cb_change (I, GuiTextEvent event) {
	(void) event;
	TextGridEditor *editor = (TextGridEditor *)void_me;
	TextGrid grid = (TextGrid) editor->_data;
	if (editor->_suppressRedraw) return;   /* Prevent infinite loop if 'draw' method calls GuiText_setString. */
	if (editor->_selectedTier) {
		wchar_t *text = GuiText_getString (editor->_text);
		IntervalTier intervalTier;
		TextTier textTier;
		_AnyTier_identifyClass ((Data) grid -> tiers -> item [editor->_selectedTier], & intervalTier, & textTier);
		if (intervalTier) {
			long selectedInterval = editor->getSelectedInterval ();
			if (selectedInterval) {
				TextInterval interval = (TextInterval) intervalTier -> intervals -> item [selectedInterval];
				TextInterval_setText (interval, text);
				editor->redraw ();
				editor->broadcastChange ();
			}
		} else {
			long selectedPoint = editor->getSelectedPoint ();
			if (selectedPoint) {
				TextPoint point = (TextPoint) textTier -> points -> item [selectedPoint];
				Melder_free (point -> mark);
				if (wcsspn (text, L" \n\t") != wcslen (text))   /* Any visible characters? */
				point -> mark = Melder_wcsdup_f (text);
				editor->redraw ();
				editor->broadcastChange ();
			}
		}
		Melder_free (text);
	}
}

void TextGridEditor::createChildren () {
	#if gtk
		_text = GuiText_create (NULL, 0, 0, 0, TEXT_HEIGHT, GuiText_WORDWRAP | GuiText_MULTILINE);
		gtk_box_pack_start (GTK_BOX (_dialog), _text, FALSE, FALSE, 3);
		GuiObject_show (_text);
	#else
		_text = GuiText_createShown (_dialog, 0, 0, 0, TEXT_HEIGHT, GuiText_WORDWRAP | GuiText_MULTILINE); // FIXME motif XmCreateForm
	#endif
	/*
	 * X Toolkit 4:184,461 says: "you should never call XtSetKeyboardFocus",
	 * "since it interferes with the keyboard traversal code".
	 * That's true, we needed to switch traversal off for 'form' (see above).
	 * But does anyone know of an alternative?
	 * Our simple and natural desire is that all keyboard input shall go to the only text widget
	 * in the window (in Motif emulator, this is the automatic behaviour).
	 */
	#if gtk
		gtk_widget_grab_focus (_text);   // BUG: can hardly be correct (the text should grab the focus of the window, not the global focus)
	#elif motif && defined (UNIX)
		XtSetKeyboardFocus (form, _text);
	#endif

	int top = TEXT_HEIGHT;
	int bottom = GuiObject_getY(_drawingArea)+GuiObject_getHeight(_drawingArea);
	int left = GuiObject_getX(_drawingArea);
	int right = GuiObject_getWidth(_drawingArea);
	GuiObject_size (_drawingArea, right - left, bottom - top);
	//_GuiObject_position (_drawingArea, left, right, top, bottom); // FIXME gtk

	GuiText_setChangeCallback (_text, gui_text_cb_change, this);
}

void TextGridEditor::dataChanged () {
	TextGrid grid = (TextGrid) _data;
	/*
	 * Perform a minimal selection change.
	 * Most changes will involve intervals and boundaries; however, there may also be tier removals.
	 * Do a simple guess.
	 */
	if (grid -> tiers -> size < _selectedTier) {
		_selectedTier = grid -> tiers -> size;
	}
	TimeSoundAnalysisEditor::dataChanged ();   /* Does all the updating. */
}

/********** DRAWING AREA **********/

void TextGridEditor::prepareDraw () {
	if (_longSound.data) {
		LongSound_haveWindow (_longSound.data, _startWindow, _endWindow);
		Melder_clearError ();
	}
}

void TextGridEditor::do_drawIntervalTier (IntervalTier tier, int itier) {
	#if gtk || defined (macintosh)
		bool platformUsesAntiAliasing = true;
	#else
		bool platformUsesAntiAliasing = false;
	#endif
	long x1DC, x2DC, yDC;
	int selectedInterval = itier == _selectedTier ? getSelectedInterval () : 0, iinterval, ninterval = tier -> intervals -> size;
	Graphics_WCtoDC (_graphics, _startWindow, 0.0, & x1DC, & yDC);
	Graphics_WCtoDC (_graphics, _endWindow, 0.0, & x2DC, & yDC);
	Graphics_setPercentSignIsItalic (_graphics, _useTextStyles);
	Graphics_setNumberSignIsBold (_graphics, _useTextStyles);
	Graphics_setCircumflexIsSuperscript (_graphics, _useTextStyles);
	Graphics_setUnderscoreIsSubscript (_graphics, _useTextStyles);

	/*
	 * Highlight interval: yellow (selected) or green (matching label).
	 */
	
	for (iinterval = 1; iinterval <= ninterval; iinterval ++) {
		TextInterval interval = (TextInterval) tier -> intervals -> item [iinterval];
		double tmin = interval -> xmin, tmax = interval -> xmax;
		if (tmax > _startWindow && tmin < _endWindow) {   /* Interval visible? */
			int selected = iinterval == selectedInterval;
			int labelMatches = Melder_stringMatchesCriterion (interval -> text, _greenMethod, _greenString);
			if (tmin < _startWindow) tmin = _startWindow;
			if (tmax > _endWindow) tmax = _endWindow;
			if (labelMatches) {
				Graphics_setColour (_graphics, Graphics_LIME);
				Graphics_fillRectangle (_graphics, tmin, tmax, 0.0, 1.0);
			}
			if (selected) {
				if (labelMatches) {
					tmin = 0.85 * tmin + 0.15 * tmax;
					tmax = 0.15 * tmin + 0.85 * tmax;
				}
				Graphics_setColour (_graphics, Graphics_YELLOW);
				Graphics_fillRectangle (_graphics, tmin, tmax, labelMatches ? 0.15 : 0.0, labelMatches? 0.85: 1.0);
			}
		}
	}
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_line (_graphics, _endWindow, 0.0, _endWindow, 1.0);

	/*
	 * Draw a grey bar and a selection button at the cursor position.
	 */
	if (_startSelection == _endSelection && _startSelection >= _startWindow && _startSelection <= _endWindow) {
		int cursorAtBoundary = FALSE;
		for (iinterval = 2; iinterval <= ninterval; iinterval ++) {
			TextInterval interval = (TextInterval) tier -> intervals -> item [iinterval];
			if (interval -> xmin == _startSelection) cursorAtBoundary = TRUE;
		}
		if (! cursorAtBoundary) {
			double dy = Graphics_dyMMtoWC (_graphics, 1.5);
			Graphics_setGrey (_graphics, 0.8);
			Graphics_setLineWidth (_graphics, platformUsesAntiAliasing ? 6.0 : 5.0);
			Graphics_line (_graphics, _startSelection, 0.0, _startSelection, 1.0);
			Graphics_setLineWidth (_graphics, 1.0);
			Graphics_setColour (_graphics, Graphics_BLUE);
			Graphics_circle_mm (_graphics, _startSelection, 1.0 - dy, 3.0);
		}
	}

	Graphics_setTextAlignment (_graphics, _alignment, Graphics_HALF);
	for (iinterval = 1; iinterval <= ninterval; iinterval ++) {
		TextInterval interval = (TextInterval) tier -> intervals -> item [iinterval];
		double tmin = interval -> xmin, tmax = interval -> xmax;
		int selected;
		if (tmin < _tmin) tmin = _tmin; if (tmax > _tmax) tmax = _tmax;
		if (tmin >= tmax) continue;
		selected = selectedInterval == iinterval;

		/*
		 * Draw left boundary.
		 */
		if (tmin >= _startWindow && tmin <= _endWindow && iinterval > 1) {
			int selected = ( _selectedTier == itier && tmin == _startSelection );
			Graphics_setColour (_graphics, selected ? Graphics_RED : Graphics_BLUE);
			Graphics_setLineWidth (_graphics, platformUsesAntiAliasing ? 6.0 : 5.0);
			Graphics_line (_graphics, tmin, 0.0, tmin, 1.0);

			/*
			 * Show alignment with cursor.
			 */
			if (tmin == _startSelection) {
				Graphics_setColour (_graphics, Graphics_YELLOW);
				Graphics_setLineWidth (_graphics, platformUsesAntiAliasing ? 2.0 : 1.0);
				Graphics_line (_graphics, tmin, 0.0, tmin, 1.0);
			}
		}
		Graphics_setLineWidth (_graphics, 1.0);

		/*
		 * Draw label text.
		 */
		if (interval -> text && tmax >= _startWindow && tmin <= _endWindow) {
			double t1 = _startWindow > tmin ? _startWindow : tmin;
			double t2 = _endWindow < tmax ? _endWindow : tmax;
			Graphics_setColour (_graphics, selected ? Graphics_RED : Graphics_BLACK);
			Graphics_textRect (_graphics, t1, t2, 0.0, 1.0, interval -> text);
			Graphics_setColour (_graphics, Graphics_BLACK);
		}

	}
	Graphics_setPercentSignIsItalic (_graphics, TRUE);
	Graphics_setNumberSignIsBold (_graphics, TRUE);
	Graphics_setCircumflexIsSuperscript (_graphics, TRUE);
	Graphics_setUnderscoreIsSubscript (_graphics, TRUE);
}

void TextGridEditor::do_drawTextTier (TextTier tier, int itier) {
	#if gtk || defined (macintosh)
		bool platformUsesAntiAliasing = true;
	#else
		bool platformUsesAntiAliasing = false;
	#endif
	int ipoint, npoint = tier -> points -> size;
	Graphics_setPercentSignIsItalic (_graphics, _useTextStyles);
	Graphics_setNumberSignIsBold (_graphics, _useTextStyles);
	Graphics_setCircumflexIsSuperscript (_graphics, _useTextStyles);
	Graphics_setUnderscoreIsSubscript (_graphics, _useTextStyles);

	/*
	 * Draw a grey bar and a selection button at the cursor position.
	 */
	if (_startSelection == _endSelection && _startSelection >= _startWindow && _startSelection <= _endWindow) {
		int cursorAtPoint = FALSE;
		for (ipoint = 1; ipoint <= npoint; ipoint ++) {
			TextPoint point = (TextPoint) tier -> points -> item [ipoint];
			if (point -> time == _startSelection) cursorAtPoint = TRUE;
		}
		if (! cursorAtPoint) {
			double dy = Graphics_dyMMtoWC (_graphics, 1.5);
			Graphics_setGrey (_graphics, 0.8);
			Graphics_setLineWidth (_graphics, platformUsesAntiAliasing ? 6.0 : 5.0);
			Graphics_line (_graphics, _startSelection, 0.0, _startSelection, 1.0);
			Graphics_setLineWidth (_graphics, 1.0);
			Graphics_setColour (_graphics, Graphics_BLUE);
			Graphics_circle_mm (_graphics, _startSelection, 1.0 - dy, 3.0);
		}
	}

	Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
	for (ipoint = 1; ipoint <= npoint; ipoint ++) {
		TextPoint point = (TextPoint) tier -> points -> item [ipoint];
		double t = point -> time;
		if (t >= _startWindow && t <= _endWindow) {
			int selected = ( itier == _selectedTier && t == _startSelection );
			Graphics_setColour (_graphics, selected ? Graphics_RED : Graphics_BLUE);
			Graphics_setLineWidth (_graphics, platformUsesAntiAliasing ? 6.0 : 5.0);
			Graphics_line (_graphics, t, 0.0, t, 0.2);
			Graphics_line (_graphics, t, 0.8, t, 1);
			Graphics_setLineWidth (_graphics, 1.0);

			/*
			 * Wipe out the cursor where the text is going to be.
			 */
			Graphics_setColour (_graphics, Graphics_WHITE);
			Graphics_line (_graphics, t, 0.2, t, 0.8);

			/*
			 * Show alignment with cursor.
			 */
			if (_startSelection == _endSelection && t == _startSelection) {
				Graphics_setColour (_graphics, Graphics_YELLOW);
				Graphics_setLineWidth (_graphics, platformUsesAntiAliasing ? 2.0 : 1.0);
				Graphics_line (_graphics, t, 0.0, t, 0.2);
				Graphics_line (_graphics, t, 0.8, t, 1.0);
			}
			Graphics_setColour (_graphics, selected ? Graphics_RED : Graphics_BLUE);
			if (point -> mark) Graphics_text (_graphics, t, 0.5, point -> mark);
		}
	}
	Graphics_setPercentSignIsItalic (_graphics, TRUE);
	Graphics_setNumberSignIsBold (_graphics, TRUE);
	Graphics_setCircumflexIsSuperscript (_graphics, TRUE);
	Graphics_setUnderscoreIsSubscript (_graphics, TRUE);
}

void TextGridEditor::draw () {
	TextGrid grid = (TextGrid) _data;
	Graphics_Viewport vp1, vp2;
	long itier, ntier = grid -> tiers -> size;
	enum kGraphics_font oldFont = Graphics_inqFont (_graphics);
	int oldFontSize = Graphics_inqFontSize (_graphics);
	int showAnalysis = (_spectrogram.show || _pitch.show || _intensity.show || _formant.show) && (_longSound.data || _sound.data);
	double soundY = _computeSoundY (), soundY2 = showAnalysis ? 0.5 * (1.0 + soundY) : soundY;

	/*
	 * Draw optional sound.
	 */
	if (_longSound.data || _sound.data) {
		vp1 = Graphics_insetViewport (_graphics, 0.0, 1.0, soundY2, 1.0);
		Graphics_setColour (_graphics, Graphics_WHITE);
		Graphics_setWindow (_graphics, 0, 1, 0, 1);
		Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
		draw_sound (-1.0, 1.0);
		Graphics_flushWs (_graphics);
		Graphics_resetViewport (_graphics, vp1);
	}

	/*
	 * Draw tiers.
	 */
	if (_longSound.data || _sound.data) vp1 = Graphics_insetViewport (_graphics, 0.0, 1.0, 0.0, soundY);
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_rectangle (_graphics, 0, 1, 0, 1);
	Graphics_setWindow (_graphics, _startWindow, _endWindow, 0.0, 1.0);
	for (itier = 1; itier <= ntier; itier ++) {
		Data anyTier = (Data) grid -> tiers -> item [itier];
		int selected = itier == _selectedTier;
		int isIntervalTier = anyTier -> methods == (Data_Table) classIntervalTier;
		vp2 = Graphics_insetViewport (_graphics, 0.0, 1.0,
			1.0 - (double) itier / (double) ntier,
			1.0 - (double) (itier - 1) / (double) ntier);
		Graphics_setColour (_graphics, Graphics_BLACK);
		if (itier != 1) Graphics_line (_graphics, _startWindow, 1.0, _endWindow, 1.0);

		/*
		 * Show the number and the name of the tier.
		 */
		Graphics_setColour (_graphics, selected ? Graphics_RED : Graphics_BLACK);
		Graphics_setFont (_graphics, oldFont);
		Graphics_setFontSize (_graphics, 14);
		Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
		Graphics_text2 (_graphics, _startWindow, 0.5, selected ? L"\\pf " : L"", Melder_integer (itier));
		Graphics_setFontSize (_graphics, oldFontSize);
		if (anyTier -> name && anyTier -> name [0]) {
			Graphics_setTextAlignment (_graphics, Graphics_LEFT,
				_showNumberOf == kTextGridEditor_showNumberOf_NOTHING ? Graphics_HALF : Graphics_BOTTOM);
			Graphics_text (_graphics, _endWindow, 0.5, anyTier -> name);
		}
		if (_showNumberOf != kTextGridEditor_showNumberOf_NOTHING) {
			Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_TOP);
			if (_showNumberOf == kTextGridEditor_showNumberOf_INTERVALS_OR_POINTS) {
				long count = isIntervalTier ? ((IntervalTier) anyTier) -> intervals -> size : ((TextTier) anyTier) -> points -> size;
				long position = itier == _selectedTier ? ( isIntervalTier ? getSelectedInterval () : getSelectedPoint () ) : 0;
				if (position) {
					Graphics_text5 (_graphics, _endWindow, 0.5, L"(", Melder_integer (position), L"/", Melder_integer (count), L")");
				} else {
					Graphics_text3 (_graphics, _endWindow, 0.5, L"(", Melder_integer (count), L")");
				}
			} else {
				Melder_assert (kTextGridEditor_showNumberOf_NONEMPTY_INTERVALS_OR_POINTS);
				long count = 0;
				if (isIntervalTier) {
					IntervalTier tier = (IntervalTier) anyTier;
					long ninterval = tier -> intervals -> size, iinterval;
					for (iinterval = 1; iinterval <= ninterval; iinterval ++) {
						TextInterval interval = (TextInterval) tier -> intervals -> item [iinterval];
						if (interval -> text != NULL && interval -> text [0] != '\0') {
							count ++;
						}
					}
				} else {
					TextTier tier = (TextTier) anyTier;
					long npoint = tier -> points -> size, ipoint;
					for (ipoint = 1; ipoint <= npoint; ipoint ++) {
						TextPoint point = (TextPoint) tier -> points -> item [ipoint];
						if (point -> mark != NULL && point -> mark [0] != '\0') {
							count ++;
						}
					}
				}
				Graphics_text3 (_graphics, _endWindow, 0.5, L"(##", Melder_integer (count), L"#)");
			}
		}

		Graphics_setColour (_graphics, Graphics_BLACK);
		Graphics_setFont (_graphics, kGraphics_font_TIMES);
		Graphics_setFontSize (_graphics, _fontSize);
		if (isIntervalTier)
			do_drawIntervalTier ((IntervalTier) anyTier, itier);
		else
			do_drawTextTier ((TextTier) anyTier, itier);
		Graphics_resetViewport (_graphics, vp2);
	}
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_setFont (_graphics, oldFont);
	Graphics_setFontSize (_graphics, oldFontSize);
	if (_longSound.data || _sound.data) Graphics_resetViewport (_graphics, vp1);
	Graphics_flushWs (_graphics);

	if (showAnalysis) {
		vp1 = Graphics_insetViewport (_graphics, 0.0, 1.0, soundY, soundY2);
		draw_analysis ();
		Graphics_flushWs (_graphics);
		Graphics_resetViewport (_graphics, vp1);
		/* Draw pulses. */
		if (_pulses.show) {
			vp1 = Graphics_insetViewport (_graphics, 0.0, 1.0, soundY2, 1.0);
			draw_analysis_pulses ();
			draw_sound (-1.0, 1.0);   /* Second time, partially across the pulses. */
			Graphics_flushWs (_graphics);
			Graphics_resetViewport (_graphics, vp1);
		}
	}
	Graphics_setWindow (_graphics, _startWindow, _endWindow, 0.0, 1.0);
	if (_longSound.data || _sound.data) {
		Graphics_line (_graphics, _startWindow, soundY, _endWindow, soundY);
		if (showAnalysis) {
			Graphics_line (_graphics, _startWindow, soundY2, _endWindow, soundY2);
			Graphics_line (_graphics, _startWindow, soundY, _startWindow, soundY2);
			Graphics_line (_graphics, _endWindow, soundY, _endWindow, soundY2);
		}
	}

	/*
	 * Finally, us usual, update the menus.
	 */
	updateMenuItems_file ();
}

void TextGridEditor::do_drawWhileDragging (double numberOfTiers, int *selectedTier, double x, double soundY) {
	long itier;
	for (itier = 1; itier <= numberOfTiers; itier ++) if (selectedTier [itier]) {
		double ymin = soundY * (1.0 - (double) itier / numberOfTiers);
		double ymax = soundY * (1.0 - (double) (itier - 1) / numberOfTiers);
		Graphics_setLineWidth (_graphics, 7);
		Graphics_line (_graphics, x, ymin, x, ymax);
	}
	Graphics_setLineWidth (_graphics, 1);
	Graphics_line (_graphics, x, 0, x, 1.01);
	Graphics_text1 (_graphics, x, 1.01, Melder_fixed (x, 6));
}

void TextGridEditor::do_dragBoundary (double xbegin, int iClickedTier, int shiftKeyPressed) {
	TextGrid grid = (TextGrid) _data;
	int itier, numberOfTiers = grid -> tiers -> size, itierDrop;
	double xWC = xbegin, yWC;
	double leftDraggingBoundary = _tmin, rightDraggingBoundary = _tmax;   /* Initial dragging range. */
	int selectedTier [100];
	double soundY = _computeSoundY ();

	/*
	 * Determine the set of selected boundaries and points, and the dragging range.
	 */
	for (itier = 1; itier <= numberOfTiers; itier ++) {
		selectedTier [itier] = FALSE;   /* The default. */
		/*
		 * If she has pressed the shift key, let her drag all the boundaries and points at this time.
		 * Otherwise, let her only drag the boundary or point on the clicked tier.
		 */
		if (itier == iClickedTier || shiftKeyPressed == _shiftDragMultiple) {
			IntervalTier intervalTier;
			TextTier textTier;
			_AnyTier_identifyClass ((Data) grid -> tiers -> item [itier], & intervalTier, & textTier);
			if (intervalTier) {
				long ibound = IntervalTier_hasBoundary (intervalTier, xbegin);
				if (ibound) {
					TextInterval leftInterval = (TextInterval) intervalTier -> intervals -> item [ibound - 1];
					TextInterval rightInterval = (TextInterval) intervalTier -> intervals -> item [ibound];
					selectedTier [itier] = TRUE;
					/*
					 * Prevent her to drag the boundary past its left or right neighbours on the same tier.
					 */
					if (leftInterval -> xmin > leftDraggingBoundary) {
						leftDraggingBoundary = leftInterval -> xmin;
					}
					if (rightInterval -> xmax < rightDraggingBoundary) {
						rightDraggingBoundary = rightInterval -> xmax;
					}
				}
			} else {
				if (AnyTier_hasPoint (textTier, xbegin)) {
					/*
					 * Other than with boundaries on interval tiers,
					 * points on text tiers can be dragged past their neighbours.
					 */
					selectedTier [itier] = TRUE;
				}
			}
		}
	}

	Graphics_xorOn (_graphics, Graphics_MAROON);
	Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_BOTTOM);
	do_drawWhileDragging (numberOfTiers, selectedTier, xWC, soundY);   // draw at old position
	while (Graphics_mouseStillDown (_graphics)) {
		double xWC_new;
		Graphics_getMouseLocation (_graphics, & xWC_new, & yWC);
		if (xWC_new != xWC) {
			do_drawWhileDragging (numberOfTiers, selectedTier, xWC, soundY);   // undraw at old position
			xWC = xWC_new;
			do_drawWhileDragging (numberOfTiers, selectedTier, xWC, soundY);   // draw at new position
		}
	}
	do_drawWhileDragging (numberOfTiers, selectedTier, xWC, soundY);   // undraw at new position
	Graphics_xorOff (_graphics);

	/*
	 * The simplest way to cancel the dragging operation, is to drag outside the window.
	 */
	if (xWC <= _startWindow || xWC >= _endWindow) {
		return;
	}

	/*
	 * If she dropped near an existing boundary in an unselected tier or near the cursor, we snap to that mark.
	 */
	itierDrop = _yWCtoTier (yWC);
	if (yWC > 0.0 && yWC < soundY && ! selectedTier [itierDrop]) {   /* Dropped inside an unselected tier? */
		Data anyTierDrop = (Data) grid -> tiers -> item [itierDrop];
		if (anyTierDrop -> methods == (Data_Table) classIntervalTier) {
			IntervalTier tierDrop = (IntervalTier) anyTierDrop;
			long ibound;
			for (ibound = 1; ibound < tierDrop -> intervals -> size; ibound ++) {
				TextInterval left = (TextInterval) tierDrop -> intervals -> item [ibound];
				if (fabs (Graphics_dxWCtoMM (_graphics, xWC - left -> xmax)) < 1.5) {   /* Near a boundary? */
					/*
					 * Snap to boundary.
					 */
					xWC = left -> xmax;
				}
			}
		} else {
			TextTier tierDrop = (TextTier) anyTierDrop;
			long ipoint;
			for (ipoint = 1; ipoint <= tierDrop -> points -> size; ipoint ++) {
				TextPoint point = (TextPoint) tierDrop -> points -> item [ipoint];
				if (fabs (Graphics_dxWCtoMM (_graphics, xWC - point -> time)) < 1.5) {   /* Near a point? */
					/*
					 * Snap to point.
					 */
					xWC = point -> time;
				}
			}
		}
	} else if (xbegin != _startSelection && fabs (Graphics_dxWCtoMM (_graphics, xWC - _startSelection)) < 1.5) {   /* Near the cursor? */
		/*
		 * Snap to cursor.
		 */
		xWC = _startSelection;
	} else if (xbegin != _endSelection && fabs (Graphics_dxWCtoMM (_graphics, xWC - _endSelection)) < 1.5) {   /* Near the cursor? */
		/*
		 * Snap to cursor.
		 */
		xWC = _endSelection;
	}

	/*
	 * We cannot move a boundary out of the dragging range.
	 */
	if (xWC <= leftDraggingBoundary || xWC >= rightDraggingBoundary) {
		Melder_beep ();
		return;
	}

	save (L"Drag");

	for (itier = 1; itier <= numberOfTiers; itier ++) if (selectedTier [itier]) {
		IntervalTier intervalTier;
		TextTier textTier;
		_AnyTier_identifyClass ((Data) grid -> tiers -> item [itier], & intervalTier, & textTier);
		if (intervalTier) {
			long ibound, numberOfIntervals = intervalTier -> intervals -> size;
			Any *intervals = intervalTier -> intervals -> item;
			for (ibound = 2; ibound <= numberOfIntervals; ibound ++) {
				TextInterval left = (TextInterval) intervals [ibound - 1], right = (TextInterval) intervals [ibound];
				if (left -> xmax == xbegin) {   /* Boundary dragged? */
					left -> xmax = right -> xmin = xWC;   /* Move boundary to drop site. */
					break;
				}
			}
		} else {
			long iDraggedPoint = AnyTier_hasPoint (textTier, xbegin);
			if (iDraggedPoint) {
				long dropSiteHasPoint = AnyTier_hasPoint (textTier, xWC);
				if (dropSiteHasPoint) {
					Melder_warning1 (L"Cannot drop point on an existing point.");
				} else {
					TextPoint point = (TextPoint) textTier -> points -> item [iDraggedPoint];
					/*
					 * Move point to drop site. May have passed another point.
					 */
					TextPoint newPoint = (TextPoint) Data_copy (point);
					newPoint -> time = xWC;   /* Move point to drop site. */
					Collection_removeItem (textTier -> points, iDraggedPoint);
					Collection_addItem (textTier -> points, newPoint);
				}
			}
		}
	}

	/*
	 * Select the drop site.
	 */
	if (_startSelection == xbegin)
		_startSelection = xWC;
	if (_endSelection == xbegin)
		_endSelection = xWC;
	if (_startSelection > _endSelection) {
		double dummy = _startSelection;
		_startSelection = _endSelection;
		_endSelection = dummy;
	}
	marksChanged ();
	broadcastChange ();
}

int TextGridEditor::click (double xclick, double yWC, int shiftKeyPressed) {
	TextGrid grid = (TextGrid) _data;
	double tmin, tmax, x, y;
	long ntiers = grid -> tiers -> size, iClickedTier, iClickedInterval, iClickedPoint;
	int clickedLeftBoundary = 0, nearBoundaryOrPoint, nearCursorCircle, drag = FALSE;
	IntervalTier intervalTier;
	TextTier textTier;
	TextInterval interval = NULL;
	TextPoint point = NULL;
	double soundY = _computeSoundY ();
	double tnear;

	/*
	 * In answer to a click in the sound part,
	 * we keep the same tier selected and move the cursor or drag the "yellow" selection.
	 */
	if (yWC > soundY) {   /* Clicked in sound part? */
		if ((_spectrogram.show || _formant.show) && yWC < 0.5 * (soundY + 1.0)) {
			_spectrogram.cursor = _spectrogram.viewFrom +
				2.0 * (yWC - soundY) / (1.0 - soundY) * (_spectrogram.viewTo - _spectrogram.viewFrom);
		}
		TimeSoundAnalysisEditor::click (xclick, yWC, shiftKeyPressed);
		return FunctionEditor_UPDATE_NEEDED;
	}

	if (xclick <= _startWindow || xclick >= _endWindow) {
		return FunctionEditor_NO_UPDATE_NEEDED;
	}

	/*
	 * She clicked in the grid part.
	 * We select the tier in which she clicked.
	 */
	iClickedTier = _yWCtoTier (yWC);
	_timeToInterval (xclick, iClickedTier, & tmin, & tmax);
	_AnyTier_identifyClass ((Data) grid -> tiers -> item [iClickedTier], & intervalTier, & textTier);

	/*
	 * Get the time of the nearest boundary or point.
	 */
	tnear = NUMundefined;
	if (intervalTier) {
		iClickedInterval = IntervalTier_timeToIndex (intervalTier, xclick);
		if (iClickedInterval) {
			interval = (TextInterval) intervalTier -> intervals -> item [iClickedInterval];
			if (xclick > 0.5 * (interval -> xmin + interval -> xmax)) {
				tnear = interval -> xmax;
				clickedLeftBoundary = iClickedInterval + 1;
			} else {
				tnear = interval -> xmin;
				clickedLeftBoundary = iClickedInterval;
			}
		} else {
			/*
			 * She clicked outside time domain of intervals.
			 * This can occur when we are grouped with a longer time function.
			 */
			_selectedTier = iClickedTier;
			return FunctionEditor_UPDATE_NEEDED;
		}
	} else {
		iClickedPoint = AnyTier_timeToNearestIndex (textTier, xclick);
		if (iClickedPoint) {
			point = (TextPoint) textTier -> points -> item [iClickedPoint];
			tnear = point -> time;
		}
	}
	Melder_assert (! (intervalTier && ! clickedLeftBoundary));

	/*
	 * Where did she click?
	 */
	nearBoundaryOrPoint = tnear != NUMundefined && fabs (Graphics_dxWCtoMM (_graphics, xclick - tnear)) < 1.5;
	nearCursorCircle = _startSelection == _endSelection && Graphics_distanceWCtoMM (_graphics, xclick, yWC,
		_startSelection, (ntiers + 1 - iClickedTier) * soundY / ntiers - Graphics_dyMMtoWC (_graphics, 1.5)) < 1.5;

	/*
	 * Find out whether this is a click or a drag.
	 */
	while (Graphics_mouseStillDown (_graphics)) {
		Graphics_getMouseLocation (_graphics, & x, & y);
		if (x < _startWindow) x = _startWindow;
		if (x > _endWindow) x = _endWindow;
		if (fabs (Graphics_dxWCtoMM (_graphics, x - xclick)) > 1.5) {
			drag = TRUE;
			break;
		}
	}

	if (nearBoundaryOrPoint) {
		/*
		 * Possibility 1: she clicked near a boundary or point.
		 * Select or drag it.
		 */
		if (intervalTier && (clickedLeftBoundary < 2 || clickedLeftBoundary > intervalTier -> intervals -> size)) {		
			/*
			 * Ignore click on left edge of first interval or right edge of last interval.
			 */
			_selectedTier = iClickedTier;
		} else if (drag) {
			/*
			 * The tier that has been clicked becomes the new selected tier.
			 * This has to be done before the next Update, i.e. also before do_dragBoundary!
			 */
			_selectedTier = iClickedTier;
			do_dragBoundary (tnear, iClickedTier, shiftKeyPressed);
			return FunctionEditor_NO_UPDATE_NEEDED;
		} else {
			/*
			 * If she clicked on an unselected boundary or point, we select it.
			 */
			if (shiftKeyPressed) {
				if (tnear > 0.5 * (_startSelection + _endSelection))
					_endSelection = tnear;
				else
					_startSelection = tnear;
			} else {
				_startSelection = _endSelection = tnear;   /* Move cursor so that the boundary or point is selected. */
			}
			_selectedTier = iClickedTier;
		}
	} else if (nearCursorCircle) {
		/*
		 * Possibility 2: she clicked near the cursor circle.
		 * Insert boundary or point. There is no danger that we insert on top of an existing boundary or point,
		 * because we are not 'nearBoundaryOrPoint'.
		 */
		insertBoundaryOrPoint (iClickedTier, _startSelection, _startSelection, false);
		_selectedTier = iClickedTier;
		marksChanged ();
		broadcastChange ();
		if (drag) Graphics_waitMouseUp (_graphics);
		return FunctionEditor_NO_UPDATE_NEEDED;
	} else {
		/*
		 * Possibility 3: she clicked in empty space.
		 */
		if (intervalTier) {
			_startSelection = tmin;
			_endSelection = tmax;
		}
		_selectedTier = iClickedTier;
	}
	if (drag) Graphics_waitMouseUp (_graphics);
	return FunctionEditor_UPDATE_NEEDED;
}

int TextGridEditor::clickB (double t, double yWC) {
	int itier;
	double tmin, tmax;
	double soundY = _computeSoundY ();

	if (yWC > soundY) {   /* Clicked in sound part? */
		_startSelection = t;
		if (_startSelection > _endSelection) {
			double dummy = _startSelection;
			_startSelection = _endSelection;
			_endSelection = dummy;
		}
		return FunctionEditor_UPDATE_NEEDED;
	}
	itier = _yWCtoTier (yWC);
	_timeToInterval (t, itier, & tmin, & tmax);
	_startSelection = t - tmin < tmax - t ? tmin : tmax;   /* To nearest boundary. */
	if (_startSelection > _endSelection) {
		double dummy = _startSelection;
		_startSelection = _endSelection;
		_endSelection = dummy;
	}
	return FunctionEditor_UPDATE_NEEDED;
}

int TextGridEditor::clickE (double t, double yWC) {
	int itier;
	double tmin, tmax;
	double soundY = _computeSoundY ();

	if (yWC > soundY) {   /* Clicked in sound part? */
		_endSelection = t;
		if (_startSelection > _endSelection) {
			double dummy = _startSelection;
			_startSelection = _endSelection;
			_endSelection = dummy;
		}
		return FunctionEditor_UPDATE_NEEDED;
	}
	itier = _yWCtoTier (yWC);
	_timeToInterval (t, itier, & tmin, & tmax);
	_endSelection = t - tmin < tmax - t ? tmin : tmax;
	if (_startSelection > _endSelection) {
		double dummy = _startSelection;
		_startSelection = _endSelection;
		_endSelection = dummy;
	}
	return FunctionEditor_UPDATE_NEEDED;
}

void TextGridEditor::play (double tmin, double tmax) {
	if (_longSound.data) {
		LongSound_playPart (_longSound.data, tmin, tmax, playCallback, this);
	} else if (_sound.data) {
		Sound_playPart (_sound.data, tmin, tmax, playCallback, this);
	}
}

void TextGridEditor::updateText () {
	TextGrid grid = (TextGrid) _data;
	const wchar_t *newText = L"";
	if (_selectedTier) {
		IntervalTier intervalTier;
		TextTier textTier;
		_AnyTier_identifyClass ((Data) grid -> tiers -> item [_selectedTier], & intervalTier, & textTier);
		if (intervalTier) {
			long iinterval = IntervalTier_timeToIndex (intervalTier, _startSelection);
			if (iinterval) {
				TextInterval interval = (TextInterval) intervalTier -> intervals -> item [iinterval];
				if (interval -> text) {
					newText = interval -> text;
				}
			}
		} else {
			long ipoint = AnyTier_hasPoint (textTier, _startSelection);
			if (ipoint) {
				TextPoint point = (TextPoint) textTier -> points -> item [ipoint];
				if (point -> mark) {
					newText = point -> mark;
				}
			}
		}
	}
	_suppressRedraw = TRUE;   /* Prevent valueChangedCallback from redrawing. */
	GuiText_setString (_text, newText);
	long cursor = wcslen (newText);   // at end
	GuiText_setSelection (_text, cursor, cursor);
	_suppressRedraw = FALSE;
}

void TextGridEditor::prefs_addFields (EditorCommand *cmd) {
	UiForm::UiField *radio;
	NATURAL (L"Font size (points)", TextGridEditor_DEFAULT_FONT_SIZE_STRING)
	OPTIONMENU_ENUM (L"Text alignment in intervals", kGraphics_horizontalAlignment, DEFAULT)
	OPTIONMENU (L"The symbols %#_^ in labels", TextGridEditor_DEFAULT_USE_TEXT_STYLES + 1)
		OPTION (L"are shown as typed")
		OPTION (L"mean italic/bold/sub/super")
	OPTIONMENU (L"With the shift key, you drag", TextGridEditor_DEFAULT_SHIFT_DRAG_MULTIPLE + 1)
		OPTION (L"a single boundary")
		OPTION (L"multiple boundaries")
	OPTIONMENU_ENUM (L"Show number of", kTextGridEditor_showNumberOf, DEFAULT)
	OPTIONMENU_ENUM (L"Paint intervals green whose label...", kMelder_string, DEFAULT)
	SENTENCE (L"...the text", TextGridEditor_DEFAULT_GREEN_STRING)
}
void TextGridEditor::prefs_setValues (EditorCommand *cmd) {
	SET_INTEGER (L"The symbols %#_^ in labels", _useTextStyles + 1)
	SET_INTEGER (L"Font size", _fontSize)
	SET_ENUM (L"Text alignment in intervals", kGraphics_horizontalAlignment, _alignment)
	SET_INTEGER (L"With the shift key, you drag", _shiftDragMultiple + 1)
	SET_ENUM (L"Show number of", kTextGridEditor_showNumberOf, _showNumberOf)
	SET_ENUM (L"Paint intervals green whose label...", kMelder_string, _greenMethod)
	SET_STRING (L"...the text", _greenString)
}
void TextGridEditor::prefs_getValues (EditorCommand *cmd) {
	preferences.useTextStyles = _useTextStyles = GET_INTEGER (L"The symbols %#_^ in labels") - 1;
	preferences.fontSize = _fontSize = GET_INTEGER (L"Font size");
	preferences.alignment = _alignment = GET_ENUM (kGraphics_horizontalAlignment, L"Text alignment in intervals");
	preferences.shiftDragMultiple = _shiftDragMultiple = GET_INTEGER (L"With the shift key, you drag") - 1;
	preferences.showNumberOf = _showNumberOf = GET_ENUM (kTextGridEditor_showNumberOf, L"Show number of");
	preferences.greenMethod = _greenMethod = GET_ENUM (kMelder_string, L"Paint intervals green whose label...");
	wcsncpy (_greenString, GET_STRING (L"...the text"), Preferences_STRING_BUFFER_SIZE);
	_greenString [Preferences_STRING_BUFFER_SIZE - 1] = '\0';
	wcscpy (preferences.greenString, _greenString);
	redraw ();
}

void TextGridEditor::createMenuItems_view_timeDomain (EditorMenu *menu) {
	menu->addCommand (L"Select previous tier", GuiMenu_OPTION | GuiMenu_UP_ARROW, menu_cb_SelectPreviousTier);
	menu->addCommand (L"Select next tier", GuiMenu_OPTION | GuiMenu_DOWN_ARROW, menu_cb_SelectNextTier);
	menu->addCommand (L"Select previous interval", GuiMenu_OPTION | GuiMenu_LEFT_ARROW, menu_cb_SelectPreviousInterval);
	menu->addCommand (L"Select next interval", GuiMenu_OPTION | GuiMenu_RIGHT_ARROW, menu_cb_SelectNextInterval);
	menu->addCommand (L"Extend-select left", GuiMenu_SHIFT | GuiMenu_OPTION | GuiMenu_LEFT_ARROW, menu_cb_ExtendSelectPreviousInterval);
	menu->addCommand (L"Extend-select right", GuiMenu_SHIFT | GuiMenu_OPTION | GuiMenu_RIGHT_ARROW, menu_cb_ExtendSelectNextInterval);
}

void TextGridEditor::highlightSelection (double left, double right, double bottom, double top) {
	if (_spectrogram.show && (_longSound.data || _sound.data)) {
		TextGrid grid = (TextGrid) _data;
		double soundY = grid -> tiers -> size / (2.0 + grid -> tiers -> size * 1.8), soundY2 = 0.5 * (1.0 + soundY);
		Graphics_highlight (_graphics, left, right, bottom, soundY * top + (1 - soundY) * bottom);
		Graphics_highlight (_graphics, left, right, soundY2 * top + (1 - soundY2) * bottom, top);
	} else {
		Graphics_highlight (_graphics, left, right, bottom, top);
	}
}

void TextGridEditor::unhighlightSelection (double left, double right, double bottom, double top) {
	if (_spectrogram.show) {
		TextGrid grid = (TextGrid) _data;
		double soundY = grid -> tiers -> size / (2.0 + grid -> tiers -> size * 1.8), soundY2 = 0.5 * (1.0 + soundY);
		Graphics_unhighlight (_graphics, left, right, bottom, soundY * top + (1 - soundY) * bottom);
		Graphics_unhighlight (_graphics, left, right, soundY2 * top + (1 - soundY2) * bottom, top);
	} else {
		Graphics_unhighlight (_graphics, left, right, bottom, top);
	}
}

double TextGridEditor::getBottomOfSoundAndAnalysisArea () {
	return _computeSoundY ();
}

void TextGridEditor::createMenuItems_pitch_picture (EditorMenu *menu) {
	menu->addCommand (L"Draw visible pitch contour and TextGrid...", 0, menu_cb_DrawTextGridAndPitch);
}

void TextGridEditor::updateMenuItems_file () {
	TimeSoundAnalysisEditor::updateMenuItems_file ();
	GuiObject_setSensitive (_writeSelectedTextGridButton, _endSelection > _startSelection);
	GuiObject_setSensitive (_extractSelectedTextGridPreserveTimesButton, _endSelection > _startSelection);
	GuiObject_setSensitive (_extractSelectedTextGridTimeFromZeroButton, _endSelection > _startSelection);
}

/* End of file TextGridEditor.cpp */
