#ifndef _RealTierEditor_h_
#define _RealTierEditor_h_
/* RealTierEditor.h
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

#include "RealTier.h"
#include "TimeSoundEditor.h"

class RealTierEditor : public TimeSoundEditor {
  public:
	RealTierEditor (GuiObject parent, const wchar_t *title, RealTier data, Sound sound, int ownSound);
/*	'Sound' may be NULL;
	if 'ownSound' is TRUE, the editor will contain a deep copy of the Sound,
	which the editor will destroy when the editor is destroyed. */

	virtual const wchar_t * type () { return L"RealTierEditor"; }
	virtual double minimumLegalValue () { return NUMundefined; }
	virtual double maximumLegalValue () { return NUMundefined; }
	virtual const wchar_t * quantityText () { return L"Y"; }   /* Normally includes units. */
	virtual const wchar_t * quantityKey () { return L"Y"; }   /* Without units. */
	virtual const wchar_t * rightTickUnits () { return L""; }
	virtual double defaultYmin () { return 0.0; }
	virtual double defaultYmax () { return 1.0; }
	virtual const wchar_t * setRangeTitle () { return L"Set range..."; }
	virtual const wchar_t * defaultYminText () { return L"0.0"; }
	virtual const wchar_t * defaultYmaxText () { return L"1.0"; }
	virtual const wchar_t * yminText () { return L"Minimum"; }   /* Normally includes units. */
	virtual const wchar_t * ymaxText () { return L"Maximum"; }   /* Normally includes units. */
	virtual const wchar_t * yminKey () { return L"Minimum"; }   /* Without units. */
	virtual const wchar_t * ymaxKey () { return L"Maximum"; }   /* Without units. */

	virtual void updateScaling ();
/*	Computes the ymin and ymax values on the basis of the data.
	Call after every change in the data. */

	virtual void createMenus ();
	virtual void dataChanged ();
	virtual void draw ();
	virtual void drawWhileDragging (double xWC, double yWC, long first, long last, double dt, double dy);
	virtual int click (double xWC, double yWC, int shiftKeyPressed);
	virtual void play (double tmin, double tmax);

	double _ymin, _ymax, _ycursor;

  private:
	static int menu_cb_removePoints (EDITOR_ARGS);
	static int menu_cb_addPointAtCursor (EDITOR_ARGS);
	static int menu_cb_addPointAt (EDITOR_ARGS);
	static int menu_cb_setRange (EDITOR_ARGS);
};

/* End of file RealTierEditor.h */
#endif
