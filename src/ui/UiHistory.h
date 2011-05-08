#ifndef _UiHistory_h_
#define _UiHistory_h_
/* UiHistory.h
 *
 * Copyright (C) 1992-2011 Alek Storm
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

#include "sys/melder/melder.h"

class UiHistory {
  public:
	virtual void write (const wchar_t *string) { MelderString_append (& _contents, string); }
	virtual wchar_t *get (void) { return _contents.string; }
	virtual void clear (void) { MelderString_empty (& _contents); }

  private:
	MelderString _contents;
};
#endif
