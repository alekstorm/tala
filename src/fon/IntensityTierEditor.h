#ifndef _IntensityTierEditor_h_
#define _IntensityTierEditor_h_
/* IntensityTierEditor.h
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

#include "IntensityTier.h"
#include "RealTierEditor.h"
#include "Sound.h"

class IntensityTierEditor : public RealTierEditor {
  public:
	IntensityTierEditor (GuiObject parent, const wchar_t *title, IntensityTier intensity, Sound sound, int ownSound);
	// 'sound' may be NULL.

  protected:
	virtual const wchar_t * type () { return L"IntensityTierEditor"; }

	virtual const wchar_t * quantityText () { return L"Intensity (dB)"; }
	virtual const wchar_t * quantityKey () { return L"Intensity"; }
	virtual const wchar_t * rightTickUnits () { return L" dB"; }
	virtual double defaultYmin () { return 50.0; }
	virtual double defaultYmax () { return 100.0; }
	virtual const wchar_t * setRangeTitle () { return L"Set intensity range..."; }
	virtual const wchar_t * defaultYminText () { return L"50.0"; }
	virtual const wchar_t * defaultYmaxText () { return L"100.0"; }
	virtual const wchar_t * yminText () { return L"Minimum intensity (dB)"; }
	virtual const wchar_t * ymaxText () { return L"Maximum intensity (dB)"; }
	virtual const wchar_t * yminKey () { return L"Minimum intensity"; }
	virtual const wchar_t * ymaxKey () { return L"Maximum intensity"; }

	virtual void createMenus ();
	virtual void play (double tmin, double tmax);

  private:
	static int menu_cb_IntensityTierHelp (EDITOR_ARGS);
};

/* End of file IntensityTierEditor.h */
#endif
