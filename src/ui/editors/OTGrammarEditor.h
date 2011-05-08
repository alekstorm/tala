#ifndef _OTGrammarEditor_h_
#define _OTGrammarEditor_h_
/* OTGrammar.h
 *
 * Copyright (C) 1997-2011 Paul Boersma
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

#include "gram/OTGrammar.h"
#include "HyperPage.h"

class OTGrammarEditor : public HyperPage {
  public:
	OTGrammarEditor (GuiObject parent, const wchar_t *title, OTGrammar ot);

	long _selected;

  protected:
	virtual bool isEditable () { return true; }
	virtual void draw ();
	virtual int goToPage (const wchar_t *title);

  private:
	static int menu_cb_evaluate (EDITOR_ARGS);
	static int menu_cb_evaluate_noise_2_0 (EDITOR_ARGS);
	static int menu_cb_evaluate_tinyNoise (EDITOR_ARGS);
	static int menu_cb_evaluate_zeroNoise (EDITOR_ARGS);
	static int menu_cb_editConstraint (EDITOR_ARGS);
	static int menu_cb_learnOne (EDITOR_ARGS);
	static int menu_cb_learnOneFromPartialOutput (EDITOR_ARGS);
	static int menu_cb_removeConstraint (EDITOR_ARGS);
	static int menu_cb_resetAllRankings (EDITOR_ARGS);
	static int menu_cb_OTGrammarEditor_help (EDITOR_ARGS);
	static int menu_cb_OTGrammar_help (EDITOR_ARGS);
	static int menu_cb_OTLearningTutorial (EDITOR_ARGS);

	virtual const wchar_t * type () { return L"OTGrammarEditor"; }

	void createMenus ();
};

/* End of file OTGrammarEditor.h */
#endif
