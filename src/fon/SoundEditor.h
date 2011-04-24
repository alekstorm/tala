#ifndef _SoundEditor_h_
#define _SoundEditor_h_
/* SoundEditor.h
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

#include "TimeSoundAnalysisEditor.h"

class SoundEditor : public TimeSoundAnalysisEditor {
  public:
	SoundEditor (GuiObject parent, const wchar_t *title, Any data);

	const wchar_t * type () { return L"SoundEditor"; }

	void dataChanged ();
	void prepareDraw ();
	void draw ();
	void play (double tmin, double tmax);
	int click (double xWC, double yWC, int shiftKeyPressed);
	void highlightSelection (double left, double right, double bottom, double top);
	void unhighlightSelection (double left, double right, double bottom, double top);

	GuiObject _cutButton, _copyButton, _pasteButton, _zeroButton, _reverseButton;
	double _maxBuffer;

  private:
	void createMenus ();
};

/* End of file SoundEditor.h */
#endif
