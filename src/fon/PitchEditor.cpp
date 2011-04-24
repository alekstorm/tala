/* PitchEditor.cpp
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
 * pb 2004/04/13 less flashing
 * pb 2004/05/10 undefined pitch is NUMundefined rather than 0.0
 * pb 2004/10/16 struct PitchCandidate -> struct structPitchCandidate
 * pb 2005/06/16 units
 * pb 2006/08/08 reduced compiler warnings
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/10/16 Get pitch: F5 shortcut, as in Sound windows
 * pb 2007/11/30 erased Graphics_printf
 * pb 2008/01/19 double
 * pb 2008/03/20 split off Help menu
 * pb 2008/03/21 new Editor API
 * pb 2009/04/04 
 * pb 2011/03/22 C++
 */

#include "Pitch_to_Sound.h"
#include "PitchEditor.h"
#include "sys/EditorM.h"

#define HEIGHT_UNV  3.0
#define HEIGHT_INTENS  6.0
#define RADIUS  2.5

PitchEditor::PitchEditor (GuiObject parent, const wchar_t *title, Pitch pitch)
	: FunctionEditor (parent, title, pitch) {
	createMenus ();
}

/********** MENU COMMANDS **********/

static int menu_cb_setCeiling (EDITOR_ARGS) {
	PitchEditor *editor = (PitchEditor *)editor_me;
	EDITOR_FORM (L"Change ceiling", 0)
		POSITIVE (L"Ceiling (Hertz)", L"600")
	EDITOR_OK
		Pitch pitch = (Pitch) editor->_data;
		SET_REAL (L"Ceiling", pitch -> ceiling)
	EDITOR_DO
		Pitch pitch = (Pitch) editor->_data;
		editor->save (L"Change ceiling");
		Pitch_setCeiling (pitch, GET_REAL (L"Ceiling"));
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

static int menu_cb_pathFinder (EDITOR_ARGS) {
	PitchEditor *editor = (PitchEditor *)editor_me;
	EDITOR_FORM (L"Path finder", 0)
		REAL (L"Silence threshold", L"0.03")
		REAL (L"Voicing threshold", L"0.45")
		REAL (L"Octave cost", L"0.01")
		REAL (L"Octave-jump cost", L"0.35")
		REAL (L"Voiced/unvoiced cost", L"0.14")
		POSITIVE (L"Ceiling (Hertz)", L"600")
		BOOLEAN (L"Pull formants", 0)
	EDITOR_OK
		Pitch pitch = (Pitch) editor->_data;
		SET_REAL (L"Ceiling", pitch -> ceiling)
	EDITOR_DO
		Pitch pitch = (Pitch) editor->_data;
		editor->save (L"Path finder");
		Pitch_pathFinder (pitch,
			GET_REAL (L"Silence threshold"), GET_REAL (L"Voicing threshold"),
			GET_REAL (L"Octave cost"), GET_REAL (L"Octave-jump cost"),
			GET_REAL (L"Voiced/unvoiced cost"), GET_REAL (L"Ceiling"), GET_INTEGER (L"Pull formants"));
		editor->redraw ();
		editor->broadcastChange ();
	EDITOR_END
}

static int menu_cb_getPitch (EDITOR_ARGS) {
	PitchEditor *editor = (PitchEditor *)editor_me;
	if (editor->_startSelection == editor->_endSelection) {
		Melder_informationReal (Pitch_getValueAtTime ((Pitch) editor->_data, editor->_startSelection, kPitch_unit_HERTZ, 1), L"Hertz");
	} else {
		Melder_informationReal (Pitch_getMean ((Pitch) editor->_data, editor->_startSelection, editor->_endSelection, kPitch_unit_HERTZ), L"Hertz");
	}
	return 1;
}

static int menu_cb_octaveUp (EDITOR_ARGS) {
	PitchEditor *editor = (PitchEditor *)editor_me;
	Pitch pitch = (Pitch) editor->_data;
	editor->save (L"Octave up");
	Pitch_step (pitch, 2.0, 0.1, editor->_startSelection, editor->_endSelection);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_fifthUp (EDITOR_ARGS) {
	PitchEditor *editor = (PitchEditor *)editor_me;
	Pitch pitch = (Pitch) editor->_data;
	editor->save (L"Fifth up");
	Pitch_step (pitch, 1.5, 0.1, editor->_startSelection, editor->_endSelection);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_fifthDown (EDITOR_ARGS) {
	PitchEditor *editor = (PitchEditor *)editor_me;
	Pitch pitch = (Pitch) editor->_data;
	editor->save (L"Fifth down");
	Pitch_step (pitch, 1 / 1.5, 0.1, editor->_startSelection, editor->_endSelection);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_octaveDown (EDITOR_ARGS) {
	PitchEditor *editor = (PitchEditor *)editor_me;
	Pitch pitch = (Pitch) editor->_data;
	editor->save (L"Octave down");
	Pitch_step (pitch, 0.5, 0.1, editor->_startSelection, editor->_endSelection);
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_voiceless (EDITOR_ARGS) {
	PitchEditor *editor = (PitchEditor *)editor_me;
	Pitch pitch = (Pitch) editor->_data;
	long ileft = Sampled_xToHighIndex (pitch, editor->_startSelection), i, cand;
	long iright = Sampled_xToLowIndex (pitch, editor->_endSelection);
	if (ileft < 1) ileft = 1;
	if (iright > pitch -> nx) iright = pitch -> nx;
	editor->save (L"Unvoice");
	for (i = ileft; i <= iright; i ++) {
		Pitch_Frame frame = & pitch -> frame [i];
		for (cand = 1; cand <= frame -> nCandidates; cand ++) {
			if (frame -> candidate [cand]. frequency == 0.0) {
				struct structPitch_Candidate help = frame -> candidate [1];
				frame -> candidate [1] = frame -> candidate [cand];
				frame -> candidate [cand] = help;
			}
		}
	}
	editor->redraw ();
	editor->broadcastChange ();
	return 1;
}

static int menu_cb_PitchEditorHelp (EDITOR_ARGS) { Melder_help (L"PitchEditor"); return 1; }
static int menu_cb_PitchHelp (EDITOR_ARGS) { Melder_help (L"Pitch"); return 1; }

void PitchEditor::createMenus () {
	addCommand (L"Edit", L"Change ceiling...", 0, menu_cb_setCeiling);
	addCommand (L"Edit", L"Path finder...", 0, menu_cb_pathFinder);

	addCommand (L"Query", L"-- pitch --", 0, NULL);
	addCommand (L"Query", L"Get pitch", GuiMenu_F5, menu_cb_getPitch);

	addMenu (L"Selection", 0);
	addCommand (L"Selection", L"Unvoice", 0, menu_cb_voiceless);
	addCommand (L"Selection", L"-- up and down --", 0, NULL);
	addCommand (L"Selection", L"Octave up", 0, menu_cb_octaveUp);
	addCommand (L"Selection", L"Fifth up", 0, menu_cb_fifthUp);
	addCommand (L"Selection", L"Fifth down", 0, menu_cb_fifthDown);
	addCommand (L"Selection", L"Octave down", 0, menu_cb_octaveDown);
}

void PitchEditor::createHelpMenuItems (EditorMenu *menu) {
	menu->addCommand (L"PitchEditor help", '?', menu_cb_PitchEditorHelp);
	menu->addCommand (L"Pitch help", 0, menu_cb_PitchHelp);
}
	
/********** DRAWING AREA **********/

void PitchEditor::draw () {
	Pitch pitch = (Pitch) _data;
	long it, it1, it2;
	double dyUnv, dyIntens;

	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_rectangle (_graphics, 0, 1, 0, 1);

	dyUnv = Graphics_dyMMtoWC (_graphics, HEIGHT_UNV);
	dyIntens = Graphics_dyMMtoWC (_graphics, HEIGHT_INTENS);

	Sampled_getWindowSamples (pitch, _startWindow, _endWindow, & it1, & it2);

	/*
	 * Show pitch.
	 */
	{
		long f, df =
			pitch -> ceiling > 10000 ? 2000 :
			pitch -> ceiling > 5000 ? 1000 :
			pitch -> ceiling > 2000 ? 500 :
			pitch -> ceiling > 800 ? 200 :
			pitch -> ceiling > 400 ? 100 :
			50;
		double radius;
		Graphics_Viewport previous;
		previous = Graphics_insetViewport (_graphics, 0, 1, dyUnv, 1 - dyIntens);
		Graphics_setWindow (_graphics, _startWindow, _endWindow, 0, pitch -> ceiling);
		radius = Graphics_dxMMtoWC (_graphics, RADIUS);

		/* Horizontal hair at current pitch. */

		if (_startSelection == _endSelection && _startSelection >= _startWindow && _startSelection <= _endWindow) {
			double f = Pitch_getValueAtTime (pitch, _startSelection, kPitch_unit_HERTZ, Pitch_LINEAR);
			if (NUMdefined (f)) {
				Graphics_setColour (_graphics, Graphics_RED);
				Graphics_line (_graphics, _startWindow - radius, f, _endWindow, f);
				Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
				Graphics_text1 (_graphics, _startWindow - radius, f, Melder_fixed (f, 2));
			}
		}

		/* Horizontal scaling lines. */

		Graphics_setColour (_graphics, Graphics_BLUE);
		Graphics_setLineType (_graphics, Graphics_DOTTED);
		Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_HALF);
		for (f = df; f <= pitch -> ceiling; f += df) {
			Graphics_line (_graphics, _startWindow, f, _endWindow, f);
			Graphics_text2 (_graphics, _endWindow + radius/2, f, Melder_integer (f), L" Hz");
		}
		Graphics_setLineType (_graphics, Graphics_DRAWN);

		/* Show candidates. */

		for (it = it1; it <= it2; it ++) {
			Pitch_Frame frame = & pitch -> frame [it];
			double t = Sampled_indexToX (pitch, it);
			int icand;
			double f = frame -> candidate [1]. frequency;
			if (f > 0.0 && f < pitch -> ceiling) {
				Graphics_setColour (_graphics, Graphics_MAGENTA);
				Graphics_fillCircle_mm (_graphics, t, f, RADIUS * 2);
			}
			Graphics_setColour (_graphics, Graphics_BLACK);
			Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
			for (icand = 1; icand <= frame -> nCandidates; icand ++) {
				int strength = floor (10 * frame -> candidate [icand]. strength + 0.5);
				f = frame -> candidate [icand]. frequency;
				if (strength > 9) strength = 9;
				if (f > 0 && f <= pitch -> ceiling) Graphics_text1 (_graphics, t, f, Melder_integer (strength));
			}
		}
		Graphics_resetViewport (_graphics, previous);
	}

	/*
	 * Show intensity.
	 */
	{
		Graphics_Viewport previous;
		previous = Graphics_insetViewport (_graphics, 0, 1, 1 - dyIntens, 1);
		Graphics_setWindow (_graphics, _startWindow, _endWindow, 0, 1);
		Graphics_setColour (_graphics, Graphics_BLACK);
		Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
		Graphics_text (_graphics, _startWindow, 0.5, L"intens");
		Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_HALF);
		Graphics_text (_graphics, _endWindow, 0.5, L"intens");
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
		for (it = it1; it <= it2; it ++) {
			Pitch_Frame frame = & pitch -> frame [it];
			double t = Sampled_indexToX (pitch, it);
			int strength = floor (10 * frame -> intensity + 0.5);   /* Map 0.0-1.0 to 0-9 */
			if (strength > 9) strength = 9;
			Graphics_text1 (_graphics, t, 0.5, Melder_integer (strength));
		}
		Graphics_resetViewport (_graphics, previous);
	}

	if (it1 > 1) it1 -= 1;
	if (it2 < pitch -> nx) it2 += 1;

	/*
	 * Show voicelessness.
	 */
	{
		Graphics_Viewport previous;
		previous = Graphics_insetViewport (_graphics, 0, 1, 0, dyUnv);
		Graphics_setColour (_graphics, Graphics_BLUE);
		Graphics_line (_graphics, _startWindow, 1, _endWindow, 1);
		Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
		Graphics_text (_graphics, _startWindow, 0.5, L"Unv");
		Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_HALF);
		Graphics_text (_graphics, _endWindow, 0.5, L"Unv");
		for (it = it1; it <= it2; it ++) {
			Pitch_Frame frame = & pitch -> frame [it];
			double t = Sampled_indexToX (pitch, it), tleft = t - 0.5 * pitch -> dx, tright = t + 0.5 * pitch -> dx;
			double f = frame -> candidate [1]. frequency;
			if ((f > 0.0 && f < pitch -> ceiling) || tright <= _startWindow || tleft >= _endWindow) continue;
			if (tleft < _startWindow) tleft = _startWindow;
			if (tright > _endWindow) tright = _endWindow;
			Graphics_fillRectangle (_graphics, tleft, tright, 0, 1);
		}
		Graphics_setColour (_graphics, Graphics_BLACK);
		Graphics_resetViewport (_graphics, previous);
	}
}

void PitchEditor::play (double tmin, double tmax) {
	if (! Pitch_hum (_data, tmin, tmax)) Melder_flushError (NULL);
}

int PitchEditor::click (double xWC, double yWC, int dummy) {
	Pitch pitch = (Pitch) _data;
	double dyUnv = Graphics_dyMMtoWC (_graphics, HEIGHT_UNV);
	double dyIntens = Graphics_dyMMtoWC (_graphics, HEIGHT_INTENS);
	double frequency = (yWC - dyUnv) / (1 - dyIntens - dyUnv) * pitch -> ceiling, tmid;
	double minimumDf = 1e30;
	int cand, bestCandidate = -1;

	long ibestFrame;
	Pitch_Frame bestFrame;
	ibestFrame = Sampled_xToNearestIndex (pitch, xWC);
	if (ibestFrame < 1) ibestFrame = 1;
	if (ibestFrame > pitch -> nx) ibestFrame = pitch -> nx;
	bestFrame = & pitch -> frame [ibestFrame];

	tmid = Sampled_indexToX (pitch, ibestFrame);
	for (cand = 1; cand <= bestFrame -> nCandidates; cand ++) {
		double df = frequency - bestFrame -> candidate [cand]. frequency;
		if (fabs (df) < minimumDf) {
			minimumDf = fabs (df);
			bestCandidate = cand;
		}
	}
	if (bestCandidate != -1) {
		double bestFrequency = bestFrame -> candidate [bestCandidate]. frequency;
		double distanceWC = (frequency - bestFrequency) / pitch -> ceiling * (1 - dyIntens - dyUnv);
		double dx_mm = Graphics_dxWCtoMM (_graphics, xWC - tmid), dy_mm = Graphics_dyWCtoMM (_graphics, distanceWC);
		if (bestFrequency < pitch -> ceiling &&   /* Above ceiling: ignore. */
		    ((bestFrequency <= 0.0 && fabs (xWC - tmid) <= 0.5 * pitch -> dx && frequency <= 0.0) ||   /* Voiceless: click within frame. */
		     (bestFrequency > 0.0 && dx_mm * dx_mm + dy_mm * dy_mm <= RADIUS * RADIUS)))   /* Voiced: click within circle. */
		{
			struct structPitch_Candidate help = bestFrame -> candidate [1];
			save (L"Change path");
			bestFrame -> candidate [1] = bestFrame -> candidate [bestCandidate];
			bestFrame -> candidate [bestCandidate] = help;
			redraw ();
			broadcastChange ();
			_startSelection = _endSelection = tmid;   /* Cursor will snap to candidate. */
			return 1;
		} else {
			return FunctionEditor::click (xWC, yWC, dummy);   /* Move cursor or drag selection. */
		}
	}
	return FunctionEditor::click (xWC, yWC, dummy);   /* Move cursor or drag selection. */
}

/* End of file PitchEditor.cpp */
