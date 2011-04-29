#ifndef _SpectrumEditor_h_
#define _SpectrumEditor_h_
/* SpectrumEditor.h
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
 * pb 2011/03/23
 */

#include "FunctionEditor.h"
#include "Spectrum.h"

class SpectrumEditor : public FunctionEditor {
  public:
	static void prefs (void);

	SpectrumEditor (GuiObject parent, const wchar_t *title, Any data);

	virtual const wchar_t * type () { return L"SpectrumEditor"; }

	virtual int fixedPrecision_long () { return 2; }
	virtual const wchar_t * format_domain () { return L"Frequency domain:"; }
	virtual const wchar_t * format_short () { return L"%.0f"; }
	virtual const wchar_t * format_long () { return L"%.2f"; }
	virtual const wchar_t * format_units () { return L"Hertz"; }
	virtual const wchar_t * format_totalDuration () { return L"Total bandwidth %.2f seconds"; }
	virtual const wchar_t * format_window () { return L"Window %.2f Hertz"; }
	virtual const wchar_t * format_selection () { return L"%.2f Hz"; }

	virtual void updateRange ();
	virtual void dataChanged ();
	virtual void draw ();
	virtual int click (double xWC, double yWC, int shiftKeyPressed);
	virtual void play (double fmin, double fmax);
	virtual void createMenus ();

	double _minimum, _maximum, _cursorHeight;
	double _bandSmoothing, _dynamicRange;
	GuiObject _publishBandButton, _publishSoundButton;
};

/* End of file SpectrumEditor.h */
#endif
