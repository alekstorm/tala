#ifndef _AmplitudeTierEditor_h_
#define _AmplitudeTierEditor_h_
/* AmplitudeTierEditor.h
 *
 * Copyright (C) 2003-2011 Paul Boersma
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
#include "AmplitudeTier.h"
#include "Sound.h"

class AmplitudeTierEditor : public RealTierEditor {
  public:
	AmplitudeTierEditor (GuiObject parent, const wchar_t *title, AmplitudeTier amplitude, Sound sound, int ownSound);
	// 'sound' may be NULL.

	const wchar_t * type () { return L"AmplitudeTierEditor"; }
	const wchar_t * quantityText () { return L"Sound pressure (Pa)"; }
	const wchar_t * quantityKey () { return L"Sound pressure"; }
	const wchar_t * rightTickUnits () { return L" Pa"; }
	double defaultYmin () { return -1.0; }
	double defaultYmax () { return +1.0; }
	const wchar_t * setRangeTitle () { return L"Set amplitude range..."; }
	const wchar_t * defaultYminText () { return L"-1.0"; }
	const wchar_t * defaultYmaxText () { return L"+1.0"; }
	const wchar_t * yminText () { return L"Minimum amplitude (Pa)"; }
	const wchar_t * ymaxText () { return L"Maximum amplitude (Pa)"; }
	const wchar_t * yminKey () { return L"Minimum amplitude"; }
	const wchar_t * ymaxKey () { return L"Maximum amplitude"; }

	void createMenus ();
	void play (double tmin, double tmax);
};

/* End of file AmplitudeTierEditor.h */
#endif
