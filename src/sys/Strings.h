#ifndef _Strings_h_
#define _Strings_h_
/* Strings.h
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
 * pb 2011/03/02
 */

#include "Data.h"

class Strings : public Data {
public:
	Strings (const wchar_t *path, bool fileList);
	Strings (MelderFile file);

	void info ();
	const wchar_t * getVectorStr(long icol);

	int writeToRawTextFile (MelderFile file);

	void randomize ();
	int genericize ();
	int nativize ();
	void sort ();

	void remove (long position);
	int replace (long position, const wchar_t *text);
	int insert (long position, const wchar_t *text);

protected:
	long _numberOfStrings;
	wchar_t **_strings;

	long totalLength ();
	long maximumLength ();
};

/* End of file Strings.h */
#endif
