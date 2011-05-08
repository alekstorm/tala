#ifndef _ManipulationEditor_h_
#define _ManipulationEditor_h_
/* ManipulationEditor.h
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
 * pb 2011/03/22
 */

#include "FunctionEditor.h"
#include "fon/Manipulation.h"
#include "ManipulationEditor_enums.h"

class ManipulationEditor : public FunctionEditor {
  public:
	static void prefs (void);

	ManipulationEditor (GuiObject parent, const wchar_t *title, Manipulation ana);
	virtual ~ManipulationEditor ();

	PointProcess _previousPulses;
	PitchTier _previousPitch;
	DurationTier _previousDuration;
	double _soundmin, _soundmax;
	int _synthesisMethod;
	GuiObject _synthPulsesButton, _synthPulsesHumButton;
	GuiObject _synthPulsesLpcButton;
	GuiObject _synthPitchButton, _synthPitchHumButton;
	GuiObject _synthPulsesPitchButton, _synthPulsesPitchHumButton;
	GuiObject _synthOverlapAddNodurButton, _synthOverlapAddButton;
	GuiObject _synthPitchLpcButton;
	struct { enum kManipulationEditor_pitchUnits units; enum kManipulationEditor_draggingStrategy draggingStrategy; double minimum, minPeriodic, maximum, cursor; } _pitchTier;
	struct { double minimum, maximum, cursor;  } _duration;
	Graphics_Viewport _inset;

  protected:
	virtual void save ();
	virtual void save (const wchar_t *text);
	virtual void restore ();

	virtual void draw ();
	virtual int click (double xWC, double yWC, int shiftKeyPressed);
	virtual void play (double tmin, double tmax);

  private:
	static int menu_cb_extractOriginalSound (EDITOR_ARGS);
	static int menu_cb_extractPulses (EDITOR_ARGS);
	static int menu_cb_extractPitchTier (EDITOR_ARGS);
	static int menu_cb_extractDurationTier (EDITOR_ARGS);
	static int menu_cb_extractManipulatedSound (EDITOR_ARGS);
	static int menu_cb_removePulses (EDITOR_ARGS);
	static int menu_cb_addPulseAtCursor (EDITOR_ARGS);
	static int menu_cb_addPulseAt (EDITOR_ARGS);
	static int menu_cb_removePitchPoints (EDITOR_ARGS);
	static int menu_cb_addPitchPointAtCursor (EDITOR_ARGS);
	static int menu_cb_addPitchPointAtSlice (EDITOR_ARGS);
	static int menu_cb_addPitchPointAt (EDITOR_ARGS);
	static int menu_cb_stylizePitch (EDITOR_ARGS);
	static int menu_cb_stylizePitch_2st (EDITOR_ARGS);
	static int menu_cb_interpolateQuadratically (EDITOR_ARGS);
	static int menu_cb_interpolateQuadratically_4pts (EDITOR_ARGS);
	static int menu_cb_shiftPitchFrequencies (EDITOR_ARGS);
	static int menu_cb_multiplyPitchFrequencies (EDITOR_ARGS);
	static int menu_cb_setPitchRange (EDITOR_ARGS);
	static int menu_cb_setPitchUnits (EDITOR_ARGS);
	static int menu_cb_setDurationRange (EDITOR_ARGS);
	static int menu_cb_setDraggingStrategy (EDITOR_ARGS);
	static int menu_cb_removeDurationPoints (EDITOR_ARGS);
	static int menu_cb_addDurationPointAtCursor (EDITOR_ARGS);
	static int menu_cb_addDurationPointAt (EDITOR_ARGS);
	static int menu_cb_newDuration (EDITOR_ARGS);
	static int menu_cb_forgetDuration (EDITOR_ARGS);
	static int menu_cb_ManipulationEditorHelp (EDITOR_ARGS);
	static int menu_cb_ManipulationHelp (EDITOR_ARGS);
	static int menu_cb_Synth_Pulses (EDITOR_ARGS);
	static int menu_cb_Synth_Pulses_hum (EDITOR_ARGS);
	static int menu_cb_Synth_Pulses_Lpc (EDITOR_ARGS);
	static int menu_cb_Synth_Pitch (EDITOR_ARGS);
	static int menu_cb_Synth_Pitch_hum (EDITOR_ARGS);
	static int menu_cb_Synth_Pulses_Pitch (EDITOR_ARGS);
	static int menu_cb_Synth_Pulses_Pitch_hum (EDITOR_ARGS);
	static int menu_cb_Synth_OverlapAdd_nodur (EDITOR_ARGS);
	static int menu_cb_Synth_OverlapAdd (EDITOR_ARGS);
	static int menu_cb_Synth_Pitch_Lpc (EDITOR_ARGS);

	virtual const wchar_t * type () { return L"ManipulationEditor"; }

	void drawSoundArea (double ymin, double ymax);
	void drawPitchArea (double ymin, double ymax);
	void drawDurationArea (double ymin, double ymax);
	void drawWhileDragging (double xWC, double yWC, long first, long last, double dt, double df);
	void drawDurationWhileDragging (double xWC, double yWC, long first, long last, double dt, double df);
	int clickPitch (double xWC, double yWC, int shiftKeyPressed);
	int clickDuration (double xWC, double yWC, int shiftKeyPressed);

	void updateMenus ();
	int getSoundArea (double *ymin, double *ymax);
	int getPitchArea (double *ymin, double *ymax);
	int getDurationArea (double *ymin, double *ymax);

	void createMenus ();
};

/* End of file ManipulationEditor.h */
#endif
