/* ManipulationEditor.c
 *
 * Copyright (C) 1992-2008 Paul Boersma
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
 * pb 2003/11/20 PitchTier: Interpolate quadratically...
 * pb 2004/04/13 less flashing
 * pb 2006/01/02 removed bug in Shift Frequencies: wrong option list
 * pb 2006/12/08 better NUMundefined pitch and duration range checking
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/11/30 erased Graphics_printf
 * pb 2008/03/20 split off Help menu
 * pb 2008/03/21 new Editor API
 */

#include "ManipulationEditor.h"
#include "sys/Preferences.h"
#include "PitchTier_to_PointProcess.h"
#include "Sound_to_PointProcess.h"
#include "Sound_to_Pitch.h"
#include "Pitch_to_PitchTier.h"
#include "Pitch_to_PointProcess.h"
#include "sys/EditorM.h"

#include "sys/enums_getText.h"
#include "ManipulationEditor_enums.h"
#include "sys/enums_getValue.h"
#include "ManipulationEditor_enums.h"

/*
 * How to add a synthesis method (in an interruptable order):
 * 1. add an Manipulation_ #define in Manipulation.h;
 * 2. add a synthesize_ routine in Manipulation.c, and a reference to it in Manipulation_to_Sound;
 * 3. add a button in ManipulationEditor.h;
 * 4. add a cb_Synth_ callback.
 * 5. create the button in createMenus and update updateMenus;
 */

static const wchar_t *units_strings [] = { 0, L"Hz", L"st" };

static struct {
	struct {
		double minimum, maximum;
		enum kManipulationEditor_pitchUnits units;
		enum kManipulationEditor_draggingStrategy draggingStrategy;
		struct { double frequencyResolution; bool useSemitones; } stylize;
		struct { long numberOfPointsPerParabola; } interpolateQuadratically;
	} pitchTier;
	struct { double minimum, maximum; } duration;
} preferences;

static int prefs_synthesisMethod = Manipulation_OVERLAPADD;   /* Remembered across editor creations, not across Praat sessions. */

/* BUG: 25 should be fmin */
#define YLIN(editor, freq)  (editor->_pitchTier.units == kManipulationEditor_pitchUnits_HERTZ ? ((freq) < 25 ? 25 : (freq)) : NUMhertzToSemitones ((freq) < 25 ? 25 : (freq)))
#define YLININV(editor, freq)  (editor->_pitchTier.units == kManipulationEditor_pitchUnits_HERTZ ? (freq) : NUMsemitonesToHertz (freq))

void ManipulationEditor::prefs (void) {
	Preferences_addDouble (L"ManipulationEditor.pitch.minimum", & preferences.pitchTier.minimum, 50.0);
	Preferences_addDouble (L"ManipulationEditor.pitch.maximum", & preferences.pitchTier.maximum, 300.0);
	Preferences_addEnum (L"ManipulationEditor.pitch.units", & preferences.pitchTier.units, kManipulationEditor_pitchUnits, DEFAULT);
	Preferences_addEnum (L"ManipulationEditor.pitch.draggingStrategy", & preferences.pitchTier.draggingStrategy, kManipulationEditor_draggingStrategy, DEFAULT);
	Preferences_addDouble (L"ManipulationEditor.pitch.stylize.frequencyResolution", & preferences.pitchTier.stylize.frequencyResolution, 2.0);
	Preferences_addBool (L"ManipulationEditor.pitch.stylize.useSemitones", & preferences.pitchTier.stylize.useSemitones, true);
	Preferences_addLong (L"ManipulationEditor.pitch.interpolateQuadratically.numberOfPointsPerParabola", & preferences.pitchTier.interpolateQuadratically.numberOfPointsPerParabola, 4);
	Preferences_addDouble (L"ManipulationEditor.duration.minimum", & preferences.duration.minimum, 0.25);
	Preferences_addDouble (L"ManipulationEditor.duration.maximum", & preferences.duration.maximum, 3.0);
	/*Preferences_addInt (L"ManipulationEditor.synthesis.method.1", & prefs_synthesisMethod, Manipulation_OVERLAPADD);*/
}

ManipulationEditor::ManipulationEditor (GuiObject parent, const wchar_t *title, Manipulation ana)
	: FunctionEditor (parent, title, ana) {
	_pitchTier.draggingStrategy = preferences.pitchTier.draggingStrategy;
	_pitchTier.units = preferences.pitchTier.units;
	double maximumPitchValue = RealTier_getMaximumValue (ana -> pitch);
	if (_pitchTier.units == kManipulationEditor_pitchUnits_HERTZ) {
		_pitchTier.minimum = 25.0;
		_pitchTier.minPeriodic = 50.0;
		_pitchTier.maximum = maximumPitchValue;
		_pitchTier.cursor = _pitchTier.maximum * 0.8;
		_pitchTier.maximum *= 1.2;
	} else {
		_pitchTier.minimum = -24.0;
		_pitchTier.minPeriodic = -12.0;
		_pitchTier.maximum = NUMdefined (maximumPitchValue) ? NUMhertzToSemitones (maximumPitchValue) : NUMundefined;
		_pitchTier.cursor = _pitchTier.maximum - 4.0;
		_pitchTier.maximum += 3.0;
	}
	if (_pitchTier.maximum == NUMundefined || _pitchTier.maximum < preferences.pitchTier.maximum) _pitchTier.maximum = preferences.pitchTier.maximum;

	double minimumDurationValue = ana -> duration ? RealTier_getMinimumValue (ana -> duration) : NUMundefined;
	_duration.minimum = NUMdefined (minimumDurationValue) ? minimumDurationValue : 1.0;
	if (preferences.duration.minimum > 1) preferences.duration.minimum = 0.25;
	if (_duration.minimum > preferences.duration.minimum) _duration.minimum = preferences.duration.minimum;
	double maximumDurationValue = ana -> duration ? RealTier_getMaximumValue (ana -> duration) : NUMundefined;
	_duration.maximum = NUMdefined (maximumDurationValue) ? maximumDurationValue : 1.0;
	if (preferences.duration.maximum < 1) preferences.duration.maximum = 3.0;
	if (preferences.duration.maximum <= preferences.duration.minimum) preferences.duration.minimum = 0.25, preferences.duration.maximum = 3.0;
	if (_duration.maximum < preferences.duration.maximum) _duration.maximum = preferences.duration.maximum;
	_duration.cursor = 1.0;

	_synthesisMethod = prefs_synthesisMethod;
	if (ana -> sound)
		Matrix_getWindowExtrema (ana -> sound, 0, 0, 0, 0, & _soundmin, & _soundmax);
	if (_soundmin == _soundmax) _soundmin = -1.0, _soundmax = +1.0;
	updateMenus ();
}

ManipulationEditor::~ManipulationEditor () {
	forget (_previousPulses);
	forget (_previousPitch);
	forget (_previousDuration);
}

void ManipulationEditor::updateMenus () {
	Melder_assert (_synthPulsesButton != NULL);
	GuiMenuItem_check (_synthPulsesButton, _synthesisMethod == Manipulation_PULSES);
	Melder_assert (_synthPulsesHumButton != NULL);
	GuiMenuItem_check (_synthPulsesHumButton, _synthesisMethod == Manipulation_PULSES_HUM);
	Melder_assert (_synthPulsesLpcButton != NULL);
	GuiMenuItem_check (_synthPulsesLpcButton, _synthesisMethod == Manipulation_PULSES_LPC);
	Melder_assert (_synthPitchButton != NULL);
	GuiMenuItem_check (_synthPitchButton, _synthesisMethod == Manipulation_PITCH);
	Melder_assert (_synthPitchHumButton != NULL);
	GuiMenuItem_check (_synthPitchHumButton, _synthesisMethod == Manipulation_PITCH_HUM);
	Melder_assert (_synthPulsesPitchButton != NULL);
	GuiMenuItem_check (_synthPulsesPitchButton, _synthesisMethod == Manipulation_PULSES_PITCH);
	Melder_assert (_synthPulsesPitchHumButton != NULL);
	GuiMenuItem_check (_synthPulsesPitchHumButton, _synthesisMethod == Manipulation_PULSES_PITCH_HUM);
	Melder_assert (_synthOverlapAddButton != NULL);
	GuiMenuItem_check (_synthOverlapAddButton, _synthesisMethod == Manipulation_OVERLAPADD);
	Melder_assert (_synthPitchLpcButton != NULL);
	GuiMenuItem_check (_synthPitchLpcButton, _synthesisMethod == Manipulation_PITCH_LPC);
}

/*
 * The "sound area" contains the original sound and the pulses.
 */
int ManipulationEditor::getSoundArea (double *ymin, double *ymax) {
	Manipulation ana = (Manipulation) _data;
	*ymin = 0.66;
	*ymax = 1.00;
	return ana -> sound != NULL || ana -> pulses != NULL;
}
/*
 * The "pitch area" contains the grey pitch analysis based on the pulses, and the blue pitch tier.
 */
int ManipulationEditor::getPitchArea (double *ymin, double *ymax) {
	Manipulation ana = (Manipulation) _data;
	*ymin = ana -> duration ? 0.16 : 0.00;
	*ymax = 0.65;
	return ana -> pulses != NULL || ana -> pitch != NULL;
}
int ManipulationEditor::getDurationArea (double *ymin, double *ymax) {
	Manipulation ana = (Manipulation) _data;
	if (! ana -> duration) return FALSE;
	*ymin = 0.00;
	*ymax = 0.15;
	return TRUE;
}

/********** MENU COMMANDS **********/

/***** FILE MENU *****/

#define menu_cb_extract_common(menu_cb,obj) \
static int menu_cb (EDITOR_ARGS) { \
	ManipulationEditor *editor = (ManipulationEditor *)editor_me; \
	Manipulation ana = (Manipulation) editor->_data; \
	if (! ana -> obj) return 0; \
	if (editor->_publishCallback) { \
		Data publish = (Data) Data_copy (ana -> obj); \
		if (! publish) return 0; \
		editor->_publishCallback (editor, editor->_publishClosure, publish); \
	} \
	return 1; \
}
menu_cb_extract_common (menu_cb_extractOriginalSound, sound)
menu_cb_extract_common (menu_cb_extractPulses, pulses)
menu_cb_extract_common (menu_cb_extractPitchTier, pitch)
menu_cb_extract_common (menu_cb_extractDurationTier, duration)

static int menu_cb_extractManipulatedSound (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	Manipulation ana = (Manipulation) editor->_data;
	if (editor->_publishCallback) {
		Sound publish = Manipulation_to_Sound (ana, editor->_synthesisMethod);
		if (! publish) return 0;
		editor->_publishCallback (editor, editor->_publishClosure, publish);
	}
	return 1;
}

/***** EDIT MENU *****/

void ManipulationEditor::save (const wchar_t *text) {
	Editor::save (text);
}

void ManipulationEditor::save () {
	Manipulation ana = (Manipulation) _data;
	forget (_previousPulses);
	forget (_previousPitch);
	forget (_previousDuration);
	if (ana -> pulses) _previousPulses = (PointProcess) Data_copy (ana -> pulses);
	if (ana -> pitch) _previousPitch = (PitchTier) Data_copy (ana -> pitch);
	if (ana -> duration) _previousDuration = (DurationTier) Data_copy (ana -> duration);
}

void ManipulationEditor::restore () {
	Manipulation ana = (Manipulation) _data;
	Any dummy;
	dummy = ana -> pulses; ana -> pulses = _previousPulses; _previousPulses = (PointProcess) dummy;
	dummy = ana -> pitch; ana -> pitch = _previousPitch; _previousPitch = (PitchTier) dummy;
	dummy = ana -> duration; ana -> duration = _previousDuration; _previousDuration = (DurationTier) dummy;
}

/***** PULSES MENU *****/

static int menu_cb_removePulses (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;	
	Manipulation ana = (Manipulation) editor->_data;
	if (! ana -> pulses) return 0;
	editor->save (L"Remove pulse(s)");
	if (editor->_startSelection == editor->_endSelection)
		PointProcess_removePointNear (ana -> pulses, editor->_startSelection);
	else
		PointProcess_removePointsBetween (ana -> pulses, editor->_startSelection, editor->_endSelection);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_addPulseAtCursor (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	Manipulation ana = (Manipulation) editor->_data;
	if (! ana -> pulses) return 0;
	editor->save (L"Add pulse");
	PointProcess_addPoint (ana -> pulses, 0.5 * (editor->_startSelection + editor->_endSelection));
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_addPulseAt (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	EDITOR_FORM (L"Add pulse", 0)
		REAL (L"Position (s)", L"0.0")
	EDITOR_OK
		SET_REAL (L"Position", 0.5 * (editor->_startSelection + editor->_endSelection))
	EDITOR_DO
		Manipulation ana = (Manipulation) editor->_data;
		if (! ana -> pulses) return 0;
		editor->save (L"Add pulse");
		PointProcess_addPoint (ana -> pulses, GET_REAL (L"Position"));
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

/***** PITCH MENU *****/

static int menu_cb_removePitchPoints (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	Manipulation ana = (Manipulation) editor->_data;
	if (! ana -> pitch) return 0;
	editor->save (L"Remove pitch point(s)");
	if (editor->_startSelection == editor->_endSelection)
		AnyTier_removePointNear (ana -> pitch, editor->_startSelection);
	else
		AnyTier_removePointsBetween (ana -> pitch, editor->_startSelection, editor->_endSelection);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_addPitchPointAtCursor (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	Manipulation ana = (Manipulation) editor->_data;
	if (! ana -> pitch) return 0;
	editor->save (L"Add pitch point");
	RealTier_addPoint (ana -> pitch, 0.5 * (editor->_startSelection + editor->_endSelection), YLININV (editor, editor->_pitchTier.cursor));
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_addPitchPointAtSlice (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	Manipulation ana = (Manipulation) editor->_data;
	PointProcess pulses = ana -> pulses;
	long ileft, iright, nt;
	double *t, f;
	if (! pulses) return Melder_error1 (L"There are no pulses.");
	if (! ana -> pitch) return 0;
	ileft = PointProcess_getLowIndex (pulses, 0.5 * (editor->_startSelection + editor->_endSelection)), iright = ileft + 1, nt = pulses -> nt;
	t = pulses -> t, f = editor->_pitchTier.cursor;   /* Default. */
	editor->save (L"Add pitch point");
	if (nt <= 1) {
		/* Ignore. */
	} else if (ileft <= 0) {
		double tright = t [2] - t [1];
		if (tright > 0.0 && tright <= 0.02) f = YLIN (editor, 1.0 / tright);
	} else if (iright > nt) {
		double tleft = t [nt] - t [nt - 1];
		if (tleft > 0.0 && tleft <= 0.02) f = YLIN (editor, 1.0 / tleft);
	} else {   /* Three-period median. */
		double tmid = t [iright] - t [ileft], tleft = 0.0, tright = 0.0;
		if (ileft > 1) tleft = t [ileft] - t [ileft - 1];
		if (iright < nt) tright = t [iright + 1] - t [iright];
		if (tleft > 0.02) tleft = 0;
		if (tmid > 0.02) tmid = 0;
		if (tright > 0.02) tright = 0;
		/* Bubble-sort. */
		if (tmid < tleft) { double dum = tmid; tmid = tleft; tleft = dum; }
		if (tright < tleft)  { double dum = tright; tright = tleft; tleft = dum; }
		if (tright < tmid)  { double dum = tright; tright = tmid; tmid = dum; }
		if (tleft != 0.0) f = YLIN (editor, 1 / tmid);   /* Median of 3. */
		else if (tmid != 0.0) f = YLIN (editor, 2 / (tmid + tright));   /* Median of 2. */
		else if (tright != 0.0) f = YLIN (editor, 1 / tright);   /* Median of 1. */
	}
	RealTier_addPoint (ana -> pitch, 0.5 * (editor->_startSelection + editor->_endSelection), YLININV (editor, f));
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}	

static int menu_cb_addPitchPointAt (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	EDITOR_FORM (L"Add pitch point", 0)
		REAL (L"Time (s)", L"0.0")
		REAL (L"Frequency (Hz or st)", L"100.0")
	EDITOR_OK
		SET_REAL (L"Time", 0.5 * (editor->_startSelection + editor->_endSelection))
		SET_REAL (L"Frequency", editor->_pitchTier.cursor)
	EDITOR_DO
		Manipulation ana = (Manipulation) editor->_data;
		if (! ana -> pitch) return 0;
		editor->save (L"Add pitch point");
		RealTier_addPoint (ana -> pitch, GET_REAL (L"Time"), YLININV (editor, GET_REAL (L"Frequency")));
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

static int menu_cb_stylizePitch (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	EDITOR_FORM (L"Stylize pitch", L"PitchTier: Stylize...")
		REAL (L"Frequency resolution", L"2.0")
		RADIO (L"Units", 2)
			RADIOBUTTON (L"Hertz")
			RADIOBUTTON (L"semitones")
	EDITOR_OK
		SET_REAL (L"Frequency resolution", preferences.pitchTier.stylize.frequencyResolution)   /* Once. */
		SET_INTEGER (L"Units", preferences.pitchTier.stylize.useSemitones + 1)   /* Once. */
	EDITOR_DO
		Manipulation ana = (Manipulation) editor->_data;
		if (! ana -> pitch) return 0;
		editor->save (L"Stylize pitch");
		PitchTier_stylize (ana -> pitch, preferences.pitchTier.stylize.frequencyResolution = GET_REAL (L"Frequency resolution"),
			preferences.pitchTier.stylize.useSemitones = GET_INTEGER (L"Units") - 1);
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

static int menu_cb_stylizePitch_2st (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	Manipulation ana = (Manipulation) editor->_data;
	if (! ana -> pitch) return 0;
	editor->save (L"Stylize pitch");
	PitchTier_stylize (ana -> pitch, 2.0, TRUE);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_interpolateQuadratically (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	EDITOR_FORM (L"Interpolate quadratically", 0)
		NATURAL (L"Number of points per parabola", L"4")
	EDITOR_OK
		SET_INTEGER (L"Number of points per parabola", preferences.pitchTier.interpolateQuadratically.numberOfPointsPerParabola)   /* Once. */
	EDITOR_DO
		Manipulation ana = (Manipulation) editor->_data;
		if (! ana -> pitch) return 0;
		editor->save (L"Interpolate quadratically");
		RealTier_interpolateQuadratically (ana -> pitch,
			preferences.pitchTier.interpolateQuadratically.numberOfPointsPerParabola = GET_INTEGER (L"Number of points per parabola"),
			editor->_pitchTier.units == kManipulationEditor_pitchUnits_SEMITONES);
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

static int menu_cb_interpolateQuadratically_4pts (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	Manipulation ana = (Manipulation) editor->_data;
	if (! ana -> pitch) return 0;
	editor->save (L"Interpolate quadratically");
	RealTier_interpolateQuadratically (ana -> pitch, 4, editor->_pitchTier.units == kManipulationEditor_pitchUnits_SEMITONES);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_shiftPitchFrequencies (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	EDITOR_FORM (L"Shift pitch frequencies", 0)
		REAL (L"Frequency shift", L"-20.0")
		OPTIONMENU (L"Unit", 1)
			OPTION (L"Hertz")
			OPTION (L"mel")
			OPTION (L"logHertz")
			OPTION (L"semitones")
			OPTION (L"ERB")
	EDITOR_OK
	EDITOR_DO
		Manipulation ana = (Manipulation) editor->_data;
		int unit = GET_INTEGER (L"Unit");
		unit =
			unit == 1 ? kPitch_unit_HERTZ :
			unit == 2 ? kPitch_unit_MEL :
			unit == 3 ? kPitch_unit_LOG_HERTZ :
			unit == 4 ? kPitch_unit_SEMITONES_1 :
			kPitch_unit_ERB;
		if (! ana -> pitch) return 0;
		editor->save (L"Shift pitch frequencies");
		PitchTier_shiftFrequencies (ana -> pitch, editor->_startSelection, editor->_endSelection, GET_REAL (L"Frequency shift"), unit);
		editor->redraw ();
		editor->broadcastChange ();
		iferror return 0;
	EDITOR_END
}

static int menu_cb_multiplyPitchFrequencies (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	EDITOR_FORM (L"Multiply pitch frequencies", 0)
		POSITIVE (L"Factor", L"1.2")
		LABEL (L"", L"The multiplication is always done in Hertz.")
	EDITOR_OK
	EDITOR_DO
		Manipulation ana = (Manipulation) editor->_data;
		if (! ana -> pitch) return 0;
		editor->save (L"Multiply pitch frequencies");
		PitchTier_multiplyFrequencies (ana -> pitch, editor->_startSelection, editor->_endSelection, GET_REAL (L"Factor"));
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

static int menu_cb_setPitchRange (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	EDITOR_FORM (L"Set pitch range", 0)
		/* BUG: should include Minimum */
		REAL (L"Maximum (Hz or st)", L"300.0")
	EDITOR_OK
		SET_REAL (L"Maximum", editor->_pitchTier.maximum)
	EDITOR_DO
		double maximum = GET_REAL (L"Maximum");
		if (maximum <= editor->_pitchTier.minPeriodic)
			return Melder_error5 (L"Maximum pitch must be greater than ", Melder_half (editor->_pitchTier.minPeriodic), L" ", units_strings [editor->_pitchTier.units], L".");
		preferences.pitchTier.maximum = editor->_pitchTier.maximum = maximum;
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_setPitchUnits (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	EDITOR_FORM (L"Set pitch units", 0)
		RADIO_ENUM (L"Pitch units", kManipulationEditor_pitchUnits, DEFAULT)
	EDITOR_OK
		SET_ENUM (L"Pitch units", kManipulationEditor_pitchUnits, editor->_pitchTier.units)
	EDITOR_DO
		enum kManipulationEditor_pitchUnits newPitchUnits = GET_ENUM (kManipulationEditor_pitchUnits, L"Pitch units");
		if (editor->_pitchTier.units == newPitchUnits) return 1;
		preferences.pitchTier.units = editor->_pitchTier.units = newPitchUnits;
		if (editor->_pitchTier.units == kManipulationEditor_pitchUnits_HERTZ) {
			editor->_pitchTier.minimum = 25.0;
			editor->_pitchTier.minPeriodic = 50.0;
			preferences.pitchTier.maximum = editor->_pitchTier.maximum = NUMsemitonesToHertz (editor->_pitchTier.maximum);
			editor->_pitchTier.cursor = NUMsemitonesToHertz (editor->_pitchTier.cursor);
		} else {
			editor->_pitchTier.minimum = -24.0;
			editor->_pitchTier.minPeriodic = -12.0;
			preferences.pitchTier.maximum = editor->_pitchTier.maximum = NUMhertzToSemitones (editor->_pitchTier.maximum);
			editor->_pitchTier.cursor = NUMhertzToSemitones (editor->_pitchTier.cursor);
		}
		editor->redraw ();
	EDITOR_END
}

/***** DURATION MENU *****/

static int menu_cb_setDurationRange (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	EDITOR_FORM (L"Set duration range", 0)
		REAL (L"Minimum", L"0.25")
		REAL (L"Maximum", L"3.0")
	EDITOR_OK
		SET_REAL (L"Minimum", editor->_duration.minimum)
		SET_REAL (L"Maximum", editor->_duration.maximum)
	EDITOR_DO
		Manipulation ana = (Manipulation) editor->_data;
		double minimum = GET_REAL (L"Minimum"), maximum = GET_REAL (L"Maximum");
		double minimumValue = ana -> duration ? RealTier_getMinimumValue (ana -> duration) : NUMundefined;
		double maximumValue = ana -> duration ? RealTier_getMaximumValue (ana -> duration) : NUMundefined;
		if (minimum > 1) return Melder_error1 (L"Minimum relative duration must not be greater than 1.");
		if (maximum < 1) return Melder_error1 (L"Maximum relative duration must not be less than 1.");
		if (minimum >= maximum) return Melder_error1 (L"Maximum relative duration must be greater than minimum.");
		if (NUMdefined (minimumValue) && minimum > minimumValue)
			return Melder_error3 (L"Minimum relative duration must not be greater than the minimum value present, "
				"which is ", Melder_half (minimumValue), L".");
		if (NUMdefined (maximumValue) && maximum < maximumValue)
			return Melder_error3 (L"Maximum relative duration must not be less than the maximum value present, "
				"which is ", Melder_half (maximumValue), L".");
		preferences.duration.minimum = editor->_duration.minimum = minimum;
		preferences.duration.maximum = editor->_duration.maximum = maximum;
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_setDraggingStrategy (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	EDITOR_FORM (L"Set dragging strategy", L"ManipulationEditor")
		RADIO_ENUM (L"Dragging strategy", kManipulationEditor_draggingStrategy, DEFAULT)
	EDITOR_OK
		SET_INTEGER (L"Dragging strategy", editor->_pitchTier.draggingStrategy)
	EDITOR_DO
		preferences.pitchTier.draggingStrategy = editor->_pitchTier.draggingStrategy = GET_ENUM (kManipulationEditor_draggingStrategy, L"Dragging strategy");
	EDITOR_END
}

static int menu_cb_removeDurationPoints (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	Manipulation ana = (Manipulation) editor->_data;
	if (! ana -> duration) return 0;
	editor->save (L"Remove duration point(s)");
	if (editor->_startSelection == editor->_endSelection)
		AnyTier_removePointNear (ana -> duration, 0.5 * (editor->_startSelection + editor->_endSelection));
	else
		AnyTier_removePointsBetween (ana -> duration, editor->_startSelection, editor->_endSelection);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_addDurationPointAtCursor (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	Manipulation ana = (Manipulation) editor->_data;
	if (! ana -> duration) return 0;
	editor->save (L"Add duration point");
	RealTier_addPoint (ana -> duration, 0.5 * (editor->_startSelection + editor->_endSelection), editor->_duration.cursor);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_addDurationPointAt (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	EDITOR_FORM (L"Add duration point", 0)
		REAL (L"Time (s)", L"0.0");
		REAL (L"Relative duration", L"1.0");
	EDITOR_OK
		SET_REAL (L"Time", 0.5 * (editor->_startSelection + editor->_endSelection))
	EDITOR_DO
		Manipulation ana = (Manipulation) editor->_data;
		if (! ana -> duration) return 0;
		editor->save (L"Add duration point");
		RealTier_addPoint (ana -> duration, GET_REAL (L"Time"), GET_REAL (L"Relative duration"));
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

static int menu_cb_newDuration (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	Manipulation ana = (Manipulation) editor->_data;
	editor->save (L"New duration");
	forget (ana -> duration);
	ana -> duration = DurationTier_create (ana -> xmin, ana -> xmax);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_forgetDuration (EDITOR_ARGS) {
	ManipulationEditor *editor = (ManipulationEditor *)editor_me;
	Manipulation ana = (Manipulation) editor->_data;
	forget (ana -> duration);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}
	
static int menu_cb_ManipulationEditorHelp (EDITOR_ARGS) { Melder_help (L"ManipulationEditor"); return 1; }
static int menu_cb_ManipulationHelp (EDITOR_ARGS) { Melder_help (L"Manipulation"); return 1; }

#define menu_cb_Synth_common(menu_cb,meth) \
static int menu_cb (EDITOR_ARGS) { \
	ManipulationEditor *editor = (ManipulationEditor *)editor_me; \
	prefs_synthesisMethod = editor->_synthesisMethod = meth; \
	editor->updateMenus (); \
	return 1; \
}
menu_cb_Synth_common (menu_cb_Synth_Pulses, Manipulation_PULSES)
menu_cb_Synth_common (menu_cb_Synth_Pulses_hum, Manipulation_PULSES_HUM)
menu_cb_Synth_common (menu_cb_Synth_Pulses_Lpc, Manipulation_PULSES_LPC)
menu_cb_Synth_common (menu_cb_Synth_Pitch, Manipulation_PITCH)
menu_cb_Synth_common (menu_cb_Synth_Pitch_hum, Manipulation_PITCH_HUM)
menu_cb_Synth_common (menu_cb_Synth_Pulses_Pitch, Manipulation_PULSES_PITCH)
menu_cb_Synth_common (menu_cb_Synth_Pulses_Pitch_hum, Manipulation_PULSES_PITCH_HUM)
menu_cb_Synth_common (menu_cb_Synth_OverlapAdd_nodur, Manipulation_OVERLAPADD_NODUR)
menu_cb_Synth_common (menu_cb_Synth_OverlapAdd, Manipulation_OVERLAPADD)
menu_cb_Synth_common (menu_cb_Synth_Pitch_Lpc, Manipulation_PITCH_LPC)

void ManipulationEditor::createMenus () {
	FunctionEditor::createMenus ();

	addCommand (L"File", L"Extract original sound", 0, menu_cb_extractOriginalSound);
	addCommand (L"File", L"Extract pulses", 0, menu_cb_extractPulses);
	addCommand (L"File", L"Extract pitch tier", 0, menu_cb_extractPitchTier);
	addCommand (L"File", L"Extract duration tier", 0, menu_cb_extractDurationTier);
	addCommand (L"File", L"Publish resynthesis", 0, menu_cb_extractManipulatedSound);
	addCommand (L"File", L"-- close --", 0, NULL);

	addMenu (L"Pulse", 0);
	addCommand (L"Pulse", L"Add pulse at cursor", 'P', menu_cb_addPulseAtCursor);
	addCommand (L"Pulse", L"Add pulse at...", 0, menu_cb_addPulseAt);
	addCommand (L"Pulse", L"-- remove pulses --", 0, NULL);
	addCommand (L"Pulse", L"Remove pulse(s)", GuiMenu_OPTION + 'P', menu_cb_removePulses);

	addMenu (L"Pitch", 0);
	addCommand (L"Pitch", L"Add pitch point at cursor", 'T', menu_cb_addPitchPointAtCursor);
	addCommand (L"Pitch", L"Add pitch point at time slice", 0, menu_cb_addPitchPointAtSlice);
	addCommand (L"Pitch", L"Add pitch point at...", 0, menu_cb_addPitchPointAt);
	addCommand (L"Pitch", L"-- remove pitch --", 0, NULL);
	addCommand (L"Pitch", L"Remove pitch point(s)", GuiMenu_OPTION + 'T', menu_cb_removePitchPoints);
	addCommand (L"Pitch", L"-- pitch prefs --", 0, NULL);
	addCommand (L"Pitch", L"Set pitch range...", 0, menu_cb_setPitchRange);
	addCommand (L"Pitch", L"Set pitch units...", 0, menu_cb_setPitchUnits);
	addCommand (L"Pitch", L"Set pitch dragging strategy...", 0, menu_cb_setDraggingStrategy);
	addCommand (L"Pitch", L"-- modify pitch --", 0, NULL);
	addCommand (L"Pitch", L"Shift pitch frequencies...", 0, menu_cb_shiftPitchFrequencies);
	addCommand (L"Pitch", L"Multiply pitch frequencies...", 0, menu_cb_multiplyPitchFrequencies);
	addCommand (L"Pitch", L"All:", GuiMenu_INSENSITIVE, menu_cb_stylizePitch);
	addCommand (L"Pitch", L"Stylize pitch...", 0, menu_cb_stylizePitch);
	addCommand (L"Pitch", L"Stylize pitch (2 st)", '2', menu_cb_stylizePitch_2st);
	addCommand (L"Pitch", L"Interpolate quadratically...", 0, menu_cb_interpolateQuadratically);
	addCommand (L"Pitch", L"Interpolate quadratically (4 pts)", '4', menu_cb_interpolateQuadratically_4pts);

	addMenu (L"Dur", 0);
	addCommand (L"Dur", L"Add duration point at cursor", 'D', menu_cb_addDurationPointAtCursor);
	addCommand (L"Dur", L"Add duration point at...", 0, menu_cb_addDurationPointAt);
	addCommand (L"Dur", L"-- remove duration --", 0, NULL);
	addCommand (L"Dur", L"Remove duration point(s)", GuiMenu_OPTION + 'D', menu_cb_removeDurationPoints);
	addCommand (L"Dur", L"-- duration prefs --", 0, NULL);
	addCommand (L"Dur", L"Set duration range...", 0, menu_cb_setDurationRange);
	addCommand (L"Dur", L"-- refresh duration --", 0, NULL);
	addCommand (L"Dur", L"New duration", 0, menu_cb_newDuration);
	addCommand (L"Dur", L"Forget duration", 0, menu_cb_forgetDuration);

	addMenu (L"Synth", 0);
	_synthPulsesButton = addCommand (L"Synth", L"Pulses --", GuiMenu_RADIO_FIRST, menu_cb_Synth_Pulses);
	_synthPulsesHumButton = addCommand (L"Synth", L"Pulses (hum) --", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_hum);

	_synthPulsesLpcButton = addCommand (L"Synth", L"Pulses & LPC -- (\"LPC resynthesis\")", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_Lpc);
	addCommand (L"Synth", L"-- pitch resynth --", 0, NULL);
	_synthPitchButton = addCommand (L"Synth", L" -- Pitch", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pitch);
	_synthPitchHumButton = addCommand (L"Synth", L" -- Pitch (hum)", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pitch_hum);
	_synthPulsesPitchButton = addCommand (L"Synth", L"Pulses -- Pitch", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_Pitch);
	_synthPulsesPitchHumButton = addCommand (L"Synth", L"Pulses -- Pitch (hum)", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_Pitch_hum);
	addCommand (L"Synth", L"-- full resynth --", 0, NULL);
	_synthOverlapAddButton = addCommand (L"Synth", L"Sound & Pulses -- Pitch & Duration  (\"Overlap-add manipulation\")", GuiMenu_RADIO_NEXT | GuiMenu_TOGGLE_ON, menu_cb_Synth_OverlapAdd);
	_synthPitchLpcButton = addCommand (L"Synth", L"LPC -- Pitch  (\"LPC pitch manipulation\")", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pitch_Lpc);
}

void ManipulationEditor::createHelpMenuItems (EditorMenu *menu) {
	FunctionEditor::createHelpMenuItems (menu);
	menu->addCommand (L"ManipulationEditor help", '?', menu_cb_ManipulationEditorHelp);
	menu->addCommand (L"Manipulation help", 0, menu_cb_ManipulationHelp);
}

/********** DRAWING AREA **********/

void ManipulationEditor::drawSoundArea (double ymin, double ymax) {
	Manipulation ana = (Manipulation) _data;
	Sound sound = ana -> sound;
	PointProcess pulses = ana -> pulses;
	long first, last, i;
	Graphics_Viewport viewport = Graphics_insetViewport (_graphics, 0, 1, ymin, ymax);
	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_rectangle (_graphics, 0, 1, 0, 1);
	Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_TOP);
	Graphics_setFont (_graphics, kGraphics_font_TIMES);
	Graphics_text (_graphics, 1, 1, L"%%Sound");
	Graphics_setColour (_graphics, Graphics_BLUE);
	Graphics_text (_graphics, 1, 1 - Graphics_dyMMtoWC (_graphics, 3), L"%%Pulses");
	Graphics_setFont (_graphics, kGraphics_font_HELVETICA);

	/*
	 * Draw blue pulses.
	 */
	if (pulses) {
		Graphics_setWindow (_graphics, _startWindow, _endWindow, 0.0, 1.0);
		Graphics_setColour (_graphics, Graphics_BLUE);
		for (i = 1; i <= pulses -> nt; i ++) {
			double t = pulses -> t [i];
			if (t >= _startWindow && t <= _endWindow)
				Graphics_line (_graphics, t, 0.05, t, 0.95);
		}
	}

	/*
	 * Draw sound.
	 */
	if (sound && Sampled_getWindowSamples (sound, _startWindow, _endWindow, & first, & last) > 1) {
		double minimum, maximum, scaleMin, scaleMax;
		Matrix_getWindowExtrema (sound, first, last, 1, 1, & minimum, & maximum);
		if (minimum == maximum) minimum = -0.5, maximum = +0.5;

		/*
		 * Scaling.
		 */
		scaleMin = 0.83 * minimum + 0.17 * _soundmin;
		scaleMax = 0.83 * maximum + 0.17 * _soundmax;
		Graphics_setWindow (_graphics, _startWindow, _endWindow, scaleMin, scaleMax);
		drawRangeMark (scaleMin, Melder_float (Melder_half (scaleMin)), L"", Graphics_BOTTOM);
		drawRangeMark (scaleMax, Melder_float (Melder_half (scaleMax)), L"", Graphics_TOP);

		/*
		 * Draw dotted zero line.
		 */
		if (minimum < 0.0 && maximum > 0.0) {
			Graphics_setColour (_graphics, Graphics_CYAN);
			Graphics_setLineType (_graphics, Graphics_DOTTED);
			Graphics_line (_graphics, _startWindow, 0.0, _endWindow, 0.0);
			Graphics_setLineType (_graphics, Graphics_DRAWN);
		} 

		/*
		 * Draw samples.
		 */    
		Graphics_setColour (_graphics, Graphics_BLACK);
		Graphics_function (_graphics, sound -> z [1], first, last,
			Sampled_indexToX (sound, first), Sampled_indexToX (sound, last));
	}

	Graphics_resetViewport (_graphics, viewport);
}

void ManipulationEditor::drawPitchArea (double ymin, double ymax) {
	Manipulation ana = (Manipulation) _data;
	PointProcess pulses = ana -> pulses;
	PitchTier pitch = ana -> pitch;
	long ifirstSelected, ilastSelected, n = pitch ? pitch -> points -> size : 0, imin, imax, i;
	int cursorVisible = _startSelection == _endSelection && _startSelection >= _startWindow && _startSelection <= _endWindow;
	double minimumFrequency = YLIN (this, 50);
	int rangePrecisions [] = { 0, 1, 2 };
	const wchar_t *rangeUnits [] = { L"", L" Hz", L" st" };

	/*
	 * Pitch contours.
	 */
	Graphics_Viewport viewport = Graphics_insetViewport (_graphics, 0, 1, ymin, ymax);
	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_rectangle (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_GREEN);
	Graphics_setFont (_graphics, kGraphics_font_TIMES);
	Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_TOP);
	Graphics_text (_graphics, 1, 1, L"%%Pitch manip");
	Graphics_setGrey (_graphics, 0.7);
	Graphics_text (_graphics, 1, 1 - Graphics_dyMMtoWC (_graphics, 3), L"%%Pitch from pulses");
	Graphics_setFont (_graphics, kGraphics_font_HELVETICA);

	Graphics_setWindow (_graphics, _startWindow, _endWindow, _pitchTier.minimum, _pitchTier.maximum);

	/*
	 * Draw pitch contour based on pulses.
	 */
	Graphics_setGrey (_graphics, 0.7);
	if (pulses) for (i = 1; i < pulses -> nt; i ++) {
		double tleft = pulses -> t [i], tright = pulses -> t [i + 1], t = 0.5 * (tleft + tright);
		if (t >= _startWindow && t <= _endWindow) {
			if (tleft != tright) {
				double f = YLIN (this, 1 / (tright - tleft));
				if (f >= _pitchTier.minPeriodic && f <= _pitchTier.maximum) {
					Graphics_fillCircle_mm (_graphics, t, f, 1);
				}
			}
		}
	}
	Graphics_setGrey (_graphics, 0.0);

	FunctionEditor::drawGridLine (minimumFrequency);
	FunctionEditor::drawRangeMark (_pitchTier.maximum,
		Melder_fixed (_pitchTier.maximum, rangePrecisions [_pitchTier.units]), rangeUnits [_pitchTier.units], Graphics_TOP);
	FunctionEditor::drawRangeMark (_pitchTier.minimum,
		Melder_fixed (_pitchTier.minimum, rangePrecisions [_pitchTier.units]), rangeUnits [_pitchTier.units], Graphics_BOTTOM);
	if (_startSelection == _endSelection && _pitchTier.cursor >= _pitchTier.minimum && _pitchTier.cursor <= _pitchTier.maximum)
		FunctionEditor::drawHorizontalHair (_pitchTier.cursor,
			Melder_fixed (_pitchTier.cursor, rangePrecisions [_pitchTier.units]), rangeUnits [_pitchTier.units]);
	if (cursorVisible && n > 0) {
		double y = YLIN (this, RealTier_getValueAtTime (pitch, _startSelection));
		FunctionEditor::insertCursorFunctionValue (y,
			Melder_fixed (y, rangePrecisions [_pitchTier.units]), rangeUnits [_pitchTier.units],
			_pitchTier.minimum, _pitchTier.maximum);
	}
	if (pitch) {
		ifirstSelected = AnyTier_timeToHighIndex (pitch, _startSelection);
		ilastSelected = AnyTier_timeToLowIndex (pitch, _endSelection);
		imin = AnyTier_timeToHighIndex (pitch, _startWindow);
		imax = AnyTier_timeToLowIndex (pitch, _endWindow);
	}
	Graphics_setLineWidth (_graphics, 2);
	if (n == 0) {
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_setColour (_graphics, Graphics_BLACK);
		Graphics_text (_graphics, 0.5 * (_startWindow + _endWindow), 0.5 * (_pitchTier.minimum + _pitchTier.maximum), L"(no pitch points)");
	} else if (imax < imin) {
		double fleft = YLIN (this, RealTier_getValueAtTime (pitch, _startWindow));
		double fright = YLIN (this, RealTier_getValueAtTime (pitch, _endWindow));
		Graphics_setColour (_graphics, Graphics_GREEN);
		Graphics_line (_graphics, _startWindow, fleft, _endWindow, fright);
	} else {
		for (i = imin; i <= imax; i ++) {
			RealPoint point = (RealPoint) pitch -> points -> item [i];
			double t = point -> time, f = YLIN (this, point -> value);
			Graphics_setColour (_graphics, Graphics_GREEN);
			if (i == 1)
				Graphics_line (_graphics, _startWindow, f, t, f);
			else if (i == imin)
				Graphics_line (_graphics, t, f, _startWindow, YLIN (this, RealTier_getValueAtTime (pitch, _startWindow)));
			if (i == n)
				Graphics_line (_graphics, t, f, _endWindow, f);
			else if (i == imax)
				Graphics_line (_graphics, t, f, _endWindow, YLIN (this, RealTier_getValueAtTime (pitch, _endWindow)));
			else {
				RealPoint pointRight = (RealPoint) pitch -> points -> item [i + 1];
				Graphics_line (_graphics, t, f, pointRight -> time, YLIN (this, pointRight -> value));
			}
		}
		for (i = imin; i <= imax; i ++) {
			RealPoint point = (RealPoint) pitch -> points -> item [i];
			double t = point -> time, f = YLIN (this, point -> value);
			if (i >= ifirstSelected && i <= ilastSelected)
				Graphics_setColour (_graphics, Graphics_RED);	
			else
				Graphics_setColour (_graphics, Graphics_GREEN);
			Graphics_fillCircle_mm (_graphics, t, f, 3);
		}
	}
	Graphics_setLineWidth (_graphics, 1);

	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_resetViewport (_graphics, viewport);
}

void ManipulationEditor::drawDurationArea (double ymin, double ymax) {
	Manipulation ana = (Manipulation) _data;
	DurationTier duration = ana -> duration;
	long ifirstSelected, ilastSelected, n = duration ? duration -> points -> size : 0, imin, imax, i;
	int cursorVisible = _startSelection == _endSelection && _startSelection >= _startWindow && _startSelection <= _endWindow;

	/*
	 * Duration contours.
	 */
	Graphics_Viewport viewport = Graphics_insetViewport (_graphics, 0, 1, ymin, ymax);
	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_rectangle (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_GREEN);
	Graphics_setFont (_graphics, kGraphics_font_TIMES);
	Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_TOP);
	Graphics_text (_graphics, 1, 1, L"%%Duration manip");
	Graphics_setFont (_graphics, kGraphics_font_HELVETICA);

	Graphics_setWindow (_graphics, _startWindow, _endWindow, _duration.minimum, _duration.maximum);
	FunctionEditor::drawGridLine (1.0);
	FunctionEditor::drawRangeMark (_duration.maximum, Melder_fixed (_duration.maximum, 3), L"", Graphics_TOP);
	FunctionEditor::drawRangeMark (_duration.minimum, Melder_fixed (_duration.minimum, 3), L"", Graphics_BOTTOM);
	if (_startSelection == _endSelection && _duration.cursor >= _duration.minimum && _duration.cursor <= _duration.maximum)
		FunctionEditor::drawHorizontalHair (_duration.cursor, Melder_fixed (_duration.cursor, 3), L"");
	if (cursorVisible && n > 0) {
		double y = RealTier_getValueAtTime (duration, _startSelection);
		FunctionEditor::insertCursorFunctionValue (y, Melder_fixed (y, 3), L"", _duration.minimum, _duration.maximum);
	}

	/*
	 * Draw duration tier.
	 */
	if (duration) {
		ifirstSelected = AnyTier_timeToHighIndex (duration, _startSelection);
		ilastSelected = AnyTier_timeToLowIndex (duration, _endSelection);
		imin = AnyTier_timeToHighIndex (duration, _startWindow);
		imax = AnyTier_timeToLowIndex (duration, _endWindow);
	}
	Graphics_setLineWidth (_graphics, 2);
	if (n == 0) {
		Graphics_setColour (_graphics, Graphics_BLACK);
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_text (_graphics, 0.5 * (_startWindow + _endWindow),
			0.5 * (_duration.minimum + _duration.maximum), L"(no duration points)");
	} else if (imax < imin) {
		double fleft = RealTier_getValueAtTime (duration, _startWindow);
		double fright = RealTier_getValueAtTime (duration, _endWindow);
		Graphics_setColour (_graphics, Graphics_GREEN);
		Graphics_line (_graphics, _startWindow, fleft, _endWindow, fright);
	} else {
		for (i = imin; i <= imax; i ++) {
			RealPoint point = (RealPoint) duration -> points -> item [i];
			double t = point -> time, dur = point -> value;
			Graphics_setColour (_graphics, Graphics_GREEN);
			if (i == 1)
				Graphics_line (_graphics, _startWindow, dur, t, dur);
			else if (i == imin)
				Graphics_line (_graphics, t, dur, _startWindow, RealTier_getValueAtTime (duration, _startWindow));
			if (i == n)
				Graphics_line (_graphics, t, dur, _endWindow, dur);
			else if (i == imax)
				Graphics_line (_graphics, t, dur, _endWindow, RealTier_getValueAtTime (duration, _endWindow));
			else {
				RealPoint pointRight = (RealPoint) duration -> points -> item [i + 1];
				Graphics_line (_graphics, t, dur, pointRight -> time, pointRight -> value);
			}
		}
		for (i = imin; i <= imax; i ++) {
			RealPoint point = (RealPoint) duration -> points -> item [i];
			double t = point -> time, dur = point -> value;
			if (i >= ifirstSelected && i <= ilastSelected)
				Graphics_setColour (_graphics, Graphics_RED);	
			else
				Graphics_setColour (_graphics, Graphics_GREEN);	
			Graphics_fillCircle_mm (_graphics, t, dur, 3);
		}
	}

	Graphics_setLineWidth (_graphics, 1);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_resetViewport (_graphics, viewport);
}

void ManipulationEditor::draw () {
	double ysoundmin, ysoundmax;
	double ypitchmin, ypitchmax, ydurationmin, ydurationmax;
	int hasSoundArea = getSoundArea (& ysoundmin, & ysoundmax);
	int hasPitchArea = getPitchArea (& ypitchmin, & ypitchmax);
	int hasDurationArea = getDurationArea (& ydurationmin, & ydurationmax);

	if (hasSoundArea) drawSoundArea (ysoundmin, ysoundmax);
	if (hasPitchArea) drawPitchArea (ypitchmin, ypitchmax);
	if (hasDurationArea) drawDurationArea (ydurationmin, ydurationmax);

	Graphics_setWindow (_graphics, 0.0, 1.0, 0.0, 1.0);
	Graphics_setGrey (_graphics, 0.85);
	Graphics_fillRectangle (_graphics, -0.001, 1.001, ypitchmax, ysoundmin);
	Graphics_setGrey (_graphics, 0.00);
	Graphics_line (_graphics, 0, ysoundmin, 1, ysoundmin);
	Graphics_line (_graphics, 0, ypitchmax, 1, ypitchmax);
	if (hasDurationArea) {
		Graphics_setGrey (_graphics, 0.85);
		Graphics_fillRectangle (_graphics, -0.001, 1.001, ydurationmax, ypitchmin);
		Graphics_setGrey (_graphics, 0.00);
		Graphics_line (_graphics, 0, ypitchmin, 1, ypitchmin);
		Graphics_line (_graphics, 0, ydurationmax, 1, ydurationmax);
	}
	updateMenus ();
}

void ManipulationEditor::drawWhileDragging (double xWC, double yWC, long first, long last, double dt, double df) {
	Manipulation ana = (Manipulation) _data;
	PitchTier pitch = ana -> pitch;
	long i;
	(void) xWC;
	(void) yWC;

	/*
	 * Draw all selected pitch points as magenta empty circles, if inside the window.
	 */
	for (i = first; i <= last; i ++) {
		RealPoint point = (RealPoint) pitch -> points -> item [i];
		double t = point -> time + dt, f = YLIN (this, point -> value) + df;
		if (t >= _startWindow && t <= _endWindow)
			Graphics_circle_mm (_graphics, t,
				f < _pitchTier.minPeriodic ? _pitchTier.minPeriodic : f > _pitchTier.maximum ? _pitchTier.maximum : f, 3);
	}

	if (last == first) {
		/*
		 * Draw a crosshair with time and frequency.
		 */
		RealPoint point = (RealPoint) pitch -> points -> item [first];
		double t = point -> time + dt, fWC = YLIN (this, point -> value) + df;
		Graphics_line (_graphics, t, _pitchTier.minimum, t, _pitchTier.maximum - Graphics_dyMMtoWC (_graphics, 4.0));
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_TOP);
		Graphics_text1 (_graphics, t, _pitchTier.maximum, Melder_fixed (t, 6));
		Graphics_line (_graphics, _startWindow, fWC, _endWindow, fWC);
		Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_BOTTOM);
		Graphics_text1 (_graphics, _startWindow, fWC, Melder_fixed (fWC, 5));
	}
}

int ManipulationEditor::clickPitch (double xWC, double yWC, int shiftKeyPressed) {
	Manipulation ana = (Manipulation) _data;
	PitchTier pitch = ana -> pitch;
	long inearestPoint, ifirstSelected, ilastSelected, i;
	RealPoint nearestPoint;
	double dt = 0, df = 0;
	int draggingSelection, dragHorizontal, dragVertical;

	_pitchTier.cursor = _pitchTier.minimum + yWC * (_pitchTier.maximum - _pitchTier.minimum);
	if (! pitch) {
		Graphics_resetViewport (_graphics, _inset);
		return FunctionEditor::click (xWC, yWC, shiftKeyPressed);
	}
	Graphics_setWindow (_graphics, _startWindow, _endWindow, _pitchTier.minimum, _pitchTier.maximum);
	yWC = _pitchTier.cursor;

	/*
	 * Clicked on a pitch point?
	 */
	inearestPoint = AnyTier_timeToNearestIndex (pitch, xWC);
	if (inearestPoint == 0) {
		Graphics_resetViewport (_graphics, _inset);
		return FunctionEditor::click (xWC, yWC, shiftKeyPressed);
	}
	nearestPoint = (RealPoint) pitch -> points -> item [inearestPoint];
	if (Graphics_distanceWCtoMM (_graphics, xWC, yWC, nearestPoint -> time, YLIN (this, nearestPoint -> value)) > 1.5) {
		Graphics_resetViewport (_graphics, _inset);
		return FunctionEditor::click (xWC, yWC, shiftKeyPressed);
	}

	/*
	 * Clicked on a selected pitch point?
	 */
	draggingSelection = shiftKeyPressed &&
		nearestPoint -> time > _startSelection && nearestPoint -> time < _endSelection;
	if (draggingSelection) {
		ifirstSelected = AnyTier_timeToHighIndex (pitch, _startSelection);
		ilastSelected = AnyTier_timeToLowIndex (pitch, _endSelection);
		save (L"Drag pitch points");
	} else {
		ifirstSelected = ilastSelected = inearestPoint;
		save (L"Drag pitch point");
	}

	/*
	 * Drag.
	 */
	 /*
	  * Draw at the old location once.
	  * Since some systems do double buffering,
	  * the undrawing at the old position and redrawing at the new have to be bracketed by Graphics_mouseStillDown ().
	  */
	Graphics_xorOn (_graphics, Graphics_MAROON);
	drawWhileDragging (xWC, yWC, ifirstSelected, ilastSelected, dt, df);
	dragHorizontal = _pitchTier.draggingStrategy != kManipulationEditor_draggingStrategy_VERTICAL &&
		(! shiftKeyPressed || _pitchTier.draggingStrategy != kManipulationEditor_draggingStrategy_HYBRID);
	dragVertical = _pitchTier.draggingStrategy != kManipulationEditor_draggingStrategy_HORIZONTAL;
	while (Graphics_mouseStillDown (_graphics)) {
		double xWC_new, yWC_new;
		Graphics_getMouseLocation (_graphics, & xWC_new, & yWC_new);
		if (xWC_new != xWC || yWC_new != yWC) {
			drawWhileDragging (xWC, yWC, ifirstSelected, ilastSelected, dt, df);
			if (dragHorizontal) dt += xWC_new - xWC;
			if (dragVertical) df += yWC_new - yWC;
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
	{
		RealPoint *points = (RealPoint *) pitch -> points -> item;
		double newTime = points [ifirstSelected] -> time + dt;
		if (newTime < _tmin) return 1;   /* Outside domain. */
		if (ifirstSelected > 1 && newTime <= points [ifirstSelected - 1] -> time)
			return 1;   /* Past left neighbour. */
		newTime = points [ilastSelected] -> time + dt;
		if (newTime > _tmax) return 1;   /* Outside domain. */
		if (ilastSelected < pitch -> points -> size && newTime >= points [ilastSelected + 1] -> time)
			return 1;   /* Past right neighbour. */
	}

	/*
	 * Drop.
	 */
	for (i = ifirstSelected; i <= ilastSelected; i ++) {
		RealPoint point = (RealPoint) pitch -> points -> item [i];
		point -> time += dt;
		point -> value = YLININV (this, YLIN (this, point -> value) + df);
		if (point -> value < 50.0) point -> value = 50.0;
		if (point -> value > YLININV (this, _pitchTier.maximum)) point -> value = YLININV (this, _pitchTier.maximum);
	}

	/*
	 * Make sure that the same pitch points are still selected (a problem with Undo...).
	 */

	if (draggingSelection) _startSelection += dt, _endSelection += dt;
	if (_startSelection == _endSelection) {
		RealPoint point = (RealPoint) pitch -> points -> item [ifirstSelected];
		_startSelection = _endSelection = point -> time;
		_pitchTier.cursor = YLIN (this, point -> value);
	}

	broadcastChange ();
	return 1;   /* Update needed. */
}

void ManipulationEditor::drawDurationWhileDragging (double xWC, double yWC, long first, long last, double dt, double df) {
	Manipulation ana = (Manipulation) _data;
	DurationTier duration = ana -> duration;
	long i;
	(void) xWC;
	(void) yWC;

	/*
	 * Draw all selected duration points as magenta empty circles, if inside the window.
	 */
	for (i = first; i <= last; i ++) {
		RealPoint point = (RealPoint) duration -> points -> item [i];
		double t = point -> time + dt, dur = point -> value + df;
		if (t >= _startWindow && t <= _endWindow)
			Graphics_circle_mm (_graphics, t, dur < _duration.minimum ? _duration.minimum :
				dur > _duration.maximum ? _duration.maximum : dur, 3);
	}

	if (last == first) {
		/*
		 * Draw a crosshair with time and duration.
		 */
		RealPoint point = (RealPoint) duration -> points -> item [first];
		double t = point -> time + dt, durWC = point -> value + df;
		Graphics_line (_graphics, t, _duration.minimum, t, _duration.maximum - Graphics_dyMMtoWC (_graphics, 4.0));
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_TOP);
		Graphics_text1 (_graphics, t, _duration.maximum, Melder_fixed (t, 6));
		Graphics_line (_graphics, _startWindow, durWC, _endWindow, durWC);
		Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_BOTTOM);
		Graphics_text1 (_graphics, _startWindow, durWC, Melder_fixed (durWC, 2));
	}
}

int ManipulationEditor::clickDuration (double xWC, double yWC, int shiftKeyPressed) {
	Manipulation ana = (Manipulation) _data;
	DurationTier duration = ana -> duration;
	long inearestPoint, ifirstSelected, ilastSelected, i;
	RealPoint nearestPoint;
	double dt = 0, df = 0;
	int draggingSelection;

	/*
	 * Convert from FunctionEditor's [0, 1] coordinates to world coordinates.
	 */
	yWC = _duration.minimum + yWC * (_duration.maximum - _duration.minimum);

	/*
	 * Move horizontal hair to clicked position.
	 */
	_duration.cursor = yWC;

	if (! duration) {
		Graphics_resetViewport (_graphics, _inset);
		return FunctionEditor::click (xWC, yWC, shiftKeyPressed);
	}
	Graphics_setWindow (_graphics, _startWindow, _endWindow, _duration.minimum, _duration.maximum);

	/*
	 * Clicked on a duration point?
	 */
	inearestPoint = AnyTier_timeToNearestIndex (duration, xWC);
	if (inearestPoint == 0) {
		Graphics_resetViewport (_graphics, _inset);
		return FunctionEditor::click (xWC, yWC, shiftKeyPressed);
	}
	nearestPoint = (RealPoint) duration -> points -> item [inearestPoint];
	if (Graphics_distanceWCtoMM (_graphics, xWC, yWC, nearestPoint -> time, nearestPoint -> value) > 1.5) {
		Graphics_resetViewport (_graphics, _inset);
		return FunctionEditor::click (xWC, yWC, shiftKeyPressed);
	}

	/*
	 * Clicked on a selected duration point?
	 */
	draggingSelection = shiftKeyPressed &&
		nearestPoint -> time > _startSelection && nearestPoint -> time < _endSelection;
	if (draggingSelection) {
		ifirstSelected = AnyTier_timeToHighIndex (duration, _startSelection);
		ilastSelected = AnyTier_timeToLowIndex (duration, _endSelection);
		save (L"Drag duration points");
	} else {
		ifirstSelected = ilastSelected = inearestPoint;
		save (L"Drag duration point");
	}

	/*
	 * Drag.
	 */
	Graphics_xorOn (_graphics, Graphics_MAROON);
	drawDurationWhileDragging (xWC, yWC, ifirstSelected, ilastSelected, dt, df);
	while (Graphics_mouseStillDown (_graphics)) {
		double xWC_new, yWC_new;
		Graphics_getMouseLocation (_graphics, & xWC_new, & yWC_new);
		if (xWC_new != xWC || yWC_new != yWC) {
			drawDurationWhileDragging (xWC, yWC, ifirstSelected, ilastSelected, dt, df);
			dt += xWC_new - xWC, xWC = xWC_new;
			df += yWC_new - yWC, yWC = yWC_new;
			drawDurationWhileDragging (xWC_new, yWC_new, ifirstSelected, ilastSelected, dt, df);
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
	{
		RealPoint *points = (RealPoint *) duration -> points -> item;
		double newTime = points [ifirstSelected] -> time + dt;
		if (newTime < _tmin) return 1;   /* Outside domain. */
		if (ifirstSelected > 1 && newTime <= points [ifirstSelected - 1] -> time)
			return 1;   /* Past left neighbour. */
		newTime = points [ilastSelected] -> time + dt;
		if (newTime > _tmax) return 1;   /* Outside domain. */
		if (ilastSelected < duration -> points -> size && newTime >= points [ilastSelected + 1] -> time)
			return 1;   /* Past right neighbour. */
	}

	/*
	 * Drop.
	 */
	for (i = ifirstSelected; i <= ilastSelected; i ++) {
		RealPoint point = (RealPoint) duration -> points -> item [i];
		point -> time += dt;
		point -> value += df;
		if (point -> value < _duration.minimum) point -> value = _duration.minimum;
		if (point -> value > _duration.maximum) point -> value = _duration.maximum;
	}

	/*
	 * Make sure that the same duration points are still selected (a problem with Undo...).
	 */

	if (draggingSelection) _startSelection += dt, _endSelection += dt;
	if (_startSelection == _endSelection) {
		RealPoint point = (RealPoint) duration -> points -> item [ifirstSelected];
		_startSelection = _endSelection = point -> time;
		_duration.cursor = point -> value;
	}

	broadcastChange ();
	return 1;   /* Update needed. */
}

int ManipulationEditor::click (double xWC, double yWC, int shiftKeyPressed) {
	double ypitchmin, ypitchmax, ydurationmin, ydurationmax;
	int hasPitchArea = getPitchArea (& ypitchmin, & ypitchmax);
	int hasDurationArea = getDurationArea (& ydurationmin, & ydurationmax);

	/*
	 * Dispatch click to clicked area.
	 */
	if (hasPitchArea && yWC > ypitchmin && yWC < ypitchmax) {   /* Clicked in pitch area? */
		_inset = Graphics_insetViewport (_graphics, 0, 1, ypitchmin, ypitchmax);
		return clickPitch (xWC, (yWC - ypitchmin) / (ypitchmax - ypitchmin), shiftKeyPressed);
	} else if (hasDurationArea && yWC > ydurationmin && yWC < ydurationmax) {   /* Clicked in duration area? */
		_inset = Graphics_insetViewport (_graphics, 0, 1, ydurationmin, ydurationmax);
		return clickDuration (xWC, (yWC - ydurationmin) / (ydurationmax - ydurationmin), shiftKeyPressed);
	}
	/*
	 * Perform the default action: move cursor or drag selection.
	 */
	return FunctionEditor::click (xWC, yWC, shiftKeyPressed);
}

void ManipulationEditor::play (double tmin, double tmax) {
	Manipulation ana = (Manipulation) _data;
	if (_shiftKeyPressed) {
		if (ana -> sound) Sound_playPart (ana -> sound, tmin, tmax, playCallback, this);
	} else {
		if (! Manipulation_playPart (ana, tmin, tmax, _synthesisMethod))
			Melder_flushError (NULL);
	}
}

/* End of file ManipulationEditor.c */
