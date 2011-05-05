#ifndef _ArtwordEditor_h_
#define _ArtwordEditor_h_
/* ArtwordEditor.h
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
 * pb 2011/03/08
 */

#include "Artword.h"
#include "sys/Editor.h"
#include "sys/Graphics.h"

class ArtwordEditor : public Editor {
  public:
	ArtwordEditor (GuiObject parent, const wchar_t *title, Artword data);
	virtual ~ArtwordEditor ();

	Graphics _graphics;
	int _feature;
	GuiObject _list, _drawingArea, _radio, _time, _value;
	GuiObject _button [1 + kArt_muscle_MAX];

  protected:
	virtual const wchar_t * type () { return L"ArtwordEditor"; }

	virtual void updateList ();
	virtual void dataChanged ();

  private:
	static void gui_button_cb_removeTarget (I, GuiButtonEvent event);
	static void gui_button_cb_addTarget (I, GuiButtonEvent event);
	static void gui_radiobutton_cb_toggle (I, GuiRadioButtonEvent event);
	static void gui_drawingarea_cb_expose (I, GuiDrawingAreaExposeEvent event);
	static void gui_drawingarea_cb_click (I, GuiDrawingAreaClickEvent event);

	void createChildren ();
};

/* End of file ArtwordEditor.h */
#endif

