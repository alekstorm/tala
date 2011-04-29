#ifndef _DurationTierEditor_h_
#define _DurationTierEditor_h_
/* DurationTierEditor.h
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
#include "DurationTier.h"
#include "Sound.h"

class DurationTierEditor : public RealTierEditor {
  public:
	DurationTierEditor (GuiObject parent, const wchar_t *title, DurationTier duration, Sound sound, int ownSound);
	// 'sound' may be NULL.

	virtual const wchar_t * type () { return L"DurationTierEditor"; }
	virtual double minimumLegalValue () { return 0.0; }
	virtual const wchar_t * quantityText () { return L"Relative duration"; }
	virtual const wchar_t * quantityKey () { return L"Relative duration"; }
	virtual const wchar_t * rightTickUnits () { return L""; }
	virtual double defaultYmin () { return 0.25; }
	virtual double defaultYmax () { return 3.0; }
	virtual const wchar_t * setRangeTitle () { return L"Set duration range..."; }
	virtual const wchar_t * defaultYminText () { return L"0.25"; }
	virtual const wchar_t * defaultYmaxText () { return L"3.0"; }
	virtual const wchar_t * yminText () { return L"Minimum duration"; }
	virtual const wchar_t * ymaxText () { return L"Maximum duration"; }
	virtual const wchar_t * yminKey () { return L"Minimum duration"; }
	virtual const wchar_t * ymaxKey () { return L"Maximum duration"; }

	virtual void createMenus ();
	virtual void play (double tmin, double tmax);
};

/* End of file DurationTierEditor.h */
#endif
