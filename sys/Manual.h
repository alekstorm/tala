#ifndef _Manual_h_
#define _Manual_h_
/* Manual.h
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

/*
 * pb 2011/03/02
 */

#ifndef _HyperPage_h_
	#include "HyperPage.h"
#endif
#ifndef _ManPages_h_
	#include "ManPages.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#define Manual__parents(Klas) HyperPage__parents(Klas) Thing_inherit (Klas, HyperPage)
Thing_declare1 (Manual);

#define Manual__members(Klas) HyperPage__members(Klas) \
	long path, numberOfParagraphs; \
	struct structManPage_Paragraph *paragraphs; \
	GuiObject searchText; \
	GuiObject homeButton, recordButton, playButton, publishButton; \
	int numberOfMatches; \
	long matches [1 + 20], fromPage, toPage; \
	int suppressLinksHither; \
	wchar_t *printPagesStartingWith;
#define Manual__methods(Klas) HyperPage__methods(Klas)
Thing_declare2 (Manual, HyperPage);

int Manual_init (Manual me, GuiObject parent, const wchar_t *title, Any data);
Manual Manual_create (GuiObject parent, const wchar_t *title, Any data);

void Manual_search (Manual me, const wchar_t *query);

#ifdef __cplusplus
	}
#endif

/* End of file Manual.h */
#endif

