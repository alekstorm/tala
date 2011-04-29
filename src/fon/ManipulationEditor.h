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
#include "Manipulation.h"
#include "ManipulationEditor_enums.h"

class ManipulationEditor : public FunctionEditor {
  public:
	static void prefs (void);

	ManipulationEditor (GuiObject parent, const wchar_t *title, Manipulation ana);
	virtual ~ManipulationEditor ();

	virtual const wchar_t * type () { return L"ManipulationEditor"; }

	virtual void updateMenus ();
	virtual int getSoundArea (double *ymin, double *ymax);
	virtual int getPitchArea (double *ymin, double *ymax);
	virtual int getDurationArea (double *ymin, double *ymax);
	virtual void save (const wchar_t *text);
	virtual void save ();
	virtual void restore ();
	virtual void createMenus ();
	virtual void drawSoundArea (double ymin, double ymax);
	virtual void drawPitchArea (double ymin, double ymax);
	virtual void drawDurationArea (double ymin, double ymax);
	virtual void draw ();
	virtual void drawWhileDragging (double xWC, double yWC, long first, long last, double dt, double df);
	virtual int clickPitch (double xWC, double yWC, int shiftKeyPressed);
	virtual void drawDurationWhileDragging (double xWC, double yWC, long first, long last, double dt, double df);
	virtual int clickDuration (double xWC, double yWC, int shiftKeyPressed);
	virtual int click (double xWC, double yWC, int shiftKeyPressed);
	virtual void play (double tmin, double tmax);

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
};

/* End of file ManipulationEditor.h */
#endif
