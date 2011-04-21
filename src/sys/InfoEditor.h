#ifndef _InfoEditor_h_
#define _InfoEditor_h_
/* InfoEditor.h
 *
 * Copyright (C) 1996-2011 Paul Boersma
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

#include "TextEditor.h"

class InfoEditor : public TextEditor {
  public:
	InfoEditor (GuiObject parent, const wchar_t *initialText);

	const wchar_t * type () { return L"InfoEditor"; }

	bool isScriptable () { return false; }
	bool isFileBased () { return false; }

	void clear ();
};

/* End of file InfoEditor.h */
#endif
