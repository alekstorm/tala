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

	const wchar_t * type () { return L"SpectrumEditor"; }

	int fixedPrecision_long () { return 2; }
	const wchar_t * format_domain () { return L"Frequency domain:"; }
	const wchar_t * format_short () { return L"%.0f"; }
	const wchar_t * format_long () { return L"%.2f"; }
	const wchar_t * format_units () { return L"Hertz"; }
	const wchar_t * format_totalDuration () { return L"Total bandwidth %.2f seconds"; }
	const wchar_t * format_window () { return L"Window %.2f Hertz"; }
	const wchar_t * format_selection () { return L"%.2f Hz"; }

	void updateRange ();
	void dataChanged ();
	void draw ();
	int click (double xWC, double yWC, int shiftKeyPressed);
	void play (double fmin, double fmax);
	void createMenus ();
	void createMenuItems_view (EditorMenu *menu);
	void createHelpMenuItems (EditorMenu *menu);

	double _minimum, _maximum, _cursorHeight;
	double _bandSmoothing, _dynamicRange;
	GuiObject _publishBandButton, _publishSoundButton;
};

/* End of file SpectrumEditor.h */
#endif
