#ifndef _PitchEditor_h_
#define _PitchEditor_h_
/* PitchEditor.h
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
#include "Pitch.h"

class PitchEditor : public FunctionEditor {
  public:
	PitchEditor (GuiObject parent, const wchar_t *title, Pitch pitch);

	const wchar_t * type () { return L"PitchEditor"; }

	void createMenus ();
	void draw ();
	void play (double tmin, double tmax);
	int click (double xWC, double yWC, int dummy);
};

/* End of file PitchEditor.h */
#endif
