/* SpectrogramEditor.cpp
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
 * pb 2002/07/16 GPL
 * pb 2004/04/13 less flashing
 * pb 2007/06/10 wchar_t
 * pb 2007/11/30 erased Graphics_printf
 * pb 2011/03/23 C++
 */

#include "SpectrogramEditor.h"

SpectrogramEditor::SpectrogramEditor (GuiObject parent, const wchar_t *title, Any data)
	: FunctionEditor (parent, title, data) {
	//try { // FIXME exception
		_maximum = 10000;
	/*} catch (...) {
		rethrowmzero ("Spectrogram window not created.");
	}*/
}

void SpectrogramEditor::draw () {
	Spectrogram spectrogram = (Spectrogram) _data;
	long itmin, itmax;

	Graphics_setWindow (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_fillRectangle (_graphics, 0, 1, 0, 1);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_rectangle (_graphics, 0, 1, 0, 1);

	Sampled_getWindowSamples (spectrogram,
		_startWindow, _endWindow, & itmin, & itmax);

	/* Autoscale frequency axis. */
	_maximum = spectrogram -> ymax;

	Graphics_setWindow (_graphics, _startWindow, _endWindow, 0, _maximum);
	Spectrogram_paintInside (spectrogram, _graphics, _startWindow, _endWindow, 0, 0, 0.0, TRUE,
		 60, 6.0, 0);

	/* Horizontal scaling lines. */
	{
		long f, df = 1000;
		Graphics_setWindow (_graphics, 0, 1, 0, _maximum);
		Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
		Graphics_setColour (_graphics, Graphics_RED);
		for (f = df; f <= _maximum; f += df) {
			Graphics_line (_graphics, 0, f, 1, f);
			Graphics_text2 (_graphics, -0.01, f, Melder_integer (f), L" Hz");
		}
	}
	/* Vertical cursor lines. */
	Graphics_setWindow (_graphics, _startWindow, _endWindow, 0, _maximum);
	if (_startSelection > _startWindow && _startSelection < _endWindow)
		Graphics_line (_graphics, _startSelection, 0, _startSelection, _maximum);
	if (_endSelection > _startWindow && _endSelection < _endWindow)
		Graphics_line (_graphics, _endSelection, 0, _endSelection, _maximum);
	Graphics_setColour (_graphics, Graphics_BLACK);
}

int SpectrogramEditor::click (double xWC, double yWC, int shiftKeyPressed) {
	Spectrogram spectrogram = (Spectrogram) _data;
	/*double frequency = yWC * _maximum;*/
	long bestFrame;
	bestFrame = Sampled_xToNearestIndex (spectrogram, xWC);
	if (bestFrame < 1)
		bestFrame = 1;
	else if (bestFrame > spectrogram -> nx)
		bestFrame = spectrogram -> nx;
	return FunctionEditor::click (xWC, yWC, shiftKeyPressed);
}

/* End of file SpectrogramEditor.cpp */
