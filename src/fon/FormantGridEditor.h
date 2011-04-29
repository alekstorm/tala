#ifndef _FormantGridEditor_h_
#define _FormantGridEditor_h_
/* FormantGridEditor.h
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
 * pb 2011/03/23
 */

#include "FunctionEditor.h"
#include "FormantGrid.h"

struct FormantGridEditor_Play {
	double samplingFrequency;
};
struct FormantGridEditor_Source {
	struct { double tStart, f0Start, tMid, f0Mid, tEnd, f0End; } pitch;
	struct { double adaptFactor, maximumPeriod, openPhase, collisionPhase, power1, power2; } phonation;
};

class FormantGridEditor : public FunctionEditor {
  public:
	static void prefs (void);

	FormantGridEditor (GuiObject parent, const wchar_t *title, FormantGrid data);

	virtual const wchar_t * type () { return L"FormantGridEditor"; }
	virtual bool hasSourceMenu () { return true; }

	virtual int selectFormantOrBandwidth (long iformant);
	virtual void createMenus ();
	virtual void dataChanged ();
	virtual void draw ();
	virtual void drawWhileDragging (double xWC, double yWC, long first, long last, double dt, double dy);
	virtual int click (double xWC, double yWC, int shiftKeyPressed);
	virtual void play (double tmin, double tmax);

	bool _editingBandwidths;
	long _selectedFormant;
	double _formantFloor, _formantCeiling, _bandwidthFloor, _bandwidthCeiling, _ycursor;
	struct FormantGridEditor_Play _play;
	struct FormantGridEditor_Source _source;
};

/* End of file FormantGridEditor.h */
#endif
