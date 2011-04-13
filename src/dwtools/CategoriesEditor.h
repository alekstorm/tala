#ifndef _CategoriesEditor_h_
#define _CategoriesEditor_h_
/* CategoriesEditor.h
 *
 * Copyright (C) 1993-2011 David Weenink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 djmw 19950713
 djmw 20020813 GPL header
 djmw 20110305 Latest modification.
*/

#include "dwsys/Command.h"
#include "sys/Editor.h"
#include "Categories.h"

#ifdef __cplusplus
	extern "C" {
#endif

#define CategoriesEditor__parents(Klas) Editor__parents(Klas) Thing_inherit (Klas, Editor)
Thing_declare1 (CategoriesEditor);
#define CategoriesEditor__members(Klas) Editor__members(Klas)				\
	CommandHistory history;									\
	int position;											\
	GuiObject list, text, outOfView, undo, redo;				\
	GuiObject remove, insert, insertAtEnd, replace, moveUp, moveDown;
#define CategoriesEditor__methods(Klas) Editor__methods(Klas)
Thing_declare2 (CategoriesEditor, Editor);

CategoriesEditor CategoriesEditor_create (GuiObject parent, const wchar_t *title, Any data);

#ifdef __cplusplus
	}
#endif

#endif /* _CategoriesEditor_h_ */
