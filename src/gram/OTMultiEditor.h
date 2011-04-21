#ifndef _OTMultiEditor_h_
#define _OTMultiEditor_h_
/* OTMultiEditor.h
 *
 * Copyright (C) 2005-2011 Paul Boersma
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

#include "sys/HyperPage.h"
#include "OTMulti.h"

class OTMultiEditor : public HyperPage {
  public:
	OTMultiEditor (GuiObject parent, const wchar_t *title, OTMulti grammar);

	const wchar_t * type () { return L"OTMultiEditor"; }
	bool isEditable () { return true; }
	void do_limit ();
	void createChildren ();
	void createMenus ();
	void createHelpMenuItems (EditorMenu *menu);
	void draw ();
	int goToPage (const wchar_t *title);

	const wchar_t *_form1, *_form2;
	GuiObject _form1Text, _form2Text;
	long _selectedConstraint;
};

/* End of file OTMultiEditor.h */
#endif
