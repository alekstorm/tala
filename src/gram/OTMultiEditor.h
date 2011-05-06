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

#include "OTMulti.h"
#include "sys/HyperPage.h"

class OTMultiEditor : public HyperPage {
  public:
	OTMultiEditor (GuiObject parent, const wchar_t *title, OTMulti grammar);

	const wchar_t *_form1, *_form2;
	GuiObject _form1Text, _form2Text;
	long _selectedConstraint;

  protected:
	virtual bool isEditable () { return true; }
	virtual void draw ();
	virtual int goToPage (const wchar_t *title);

  private:
	static int menu_cb_evaluate (EDITOR_ARGS);
	static int menu_cb_evaluate_noise_2_0 (EDITOR_ARGS);
	static int menu_cb_evaluate_tinyNoise (EDITOR_ARGS);
	static int menu_cb_editRanking (EDITOR_ARGS);
	static int menu_cb_learnOne (EDITOR_ARGS);
	static int menu_cb_removeConstraint (EDITOR_ARGS);
	static int menu_cb_resetAllRankings (EDITOR_ARGS);
	static int menu_cb_OTLearningTutorial (EDITOR_ARGS);
	static void gui_button_cb_limit (I, GuiButtonEvent event);
	static void gui_cb_limit (GUI_ARGS);

	virtual const wchar_t * type () { return L"OTMultiEditor"; }

	void do_limit ();

	void createMenus ();
	void createChildren ();
};

/* End of file OTMultiEditor.h */
#endif
