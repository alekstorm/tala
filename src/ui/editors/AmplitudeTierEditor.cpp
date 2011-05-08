/* AmplitudeTierEditor.c
 *
 * Copyright (C) 2003-2009 Paul Boersma
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
 * pb 2003/05/31 created
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2008/03/20 split off Help menu
 * pb 2008/03/21 new Editor API
 * pb 2009/01/23 minimum and maximum legal values
 */

#include "AmplitudeTierEditor.h"

#include "EditorM.h"

AmplitudeTierEditor::AmplitudeTierEditor (GuiObject parent, const wchar_t *title, AmplitudeTier amplitude, Sound sound, int ownSound)
	: RealTierEditor (parent, title, (RealTier) amplitude, sound, ownSound) {
	createMenus ();
}

int AmplitudeTierEditor::menu_cb_AmplitudeTierHelp (EDITOR_ARGS) { Melder_help (L"AmplitudeTier"); return 1; }

void AmplitudeTierEditor::createMenus () {
	EditorMenu *menu = getMenu(L"Help");
	menu->addCommand (L"AmplitudeTier help", 0, menu_cb_AmplitudeTierHelp);
}

void AmplitudeTierEditor::play (double tmin, double tmax) {
	if (_sound.data) {
		Sound_playPart (_sound.data, tmin, tmax, playCallback, this);
	} else {
		/*if (! AmplitudeTier_playPart (_data, tmin, tmax, FALSE)) Melder_flushError (NULL);*/
	}
}

/* End of file AmplitudeTierEditor.c */
