#ifndef _PitchTierEditor_h_
#define _PitchTierEditor_h_
/* PitchTierEditor.h
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

#include "RealTierEditor.h"
#include "PitchTier.h"
#include "Sound.h"

class PitchTierEditor : public RealTierEditor {
  public:
	PitchTierEditor (GuiObject parent, const wchar_t *title, PitchTier pitch, Sound sound, int ownSound);
	//'sound' may be NULL.

	const wchar_t * type () { return L"PitchTierEditor"; }
	double minimumLegalValue () { return 0.0; }
	const wchar_t * quantityText () { return L"Frequency (Hz)"; }
	const wchar_t * quantityKey () { return L"Frequency"; }
	const wchar_t * rightTickUnits () { return L" Hz"; }
	double defaultYmin () { return 50.0; }
	double defaultYmax () { return 600.0; }
	const wchar_t * setRangeTitle () { return L"Set frequency range..."; }
	const wchar_t * defaultYminText () { return L"50.0"; }
	const wchar_t * defaultYmaxText () { return L"600.0"; }
	const wchar_t * yminText () { return L"Minimum frequency (Hz)"; }
	const wchar_t * ymaxText () { return L"Maximum frequency (Hz)"; }
	const wchar_t * yminKey () { return L"Minimum frequency"; }
	const wchar_t * ymaxKey () { return L"Maximum frequency"; }

	void createHelpMenuItems (EditorMenu *menu);
	void play (double tmin, double tmax);
};

/* End of file PitchTierEditor.h */
#endif
