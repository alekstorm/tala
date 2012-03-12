#ifndef _Simple_h_
#define _Simple_h_
/* Simple.h
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

struct SimpleInt : public Data {
	int number;

public:
	SimpleInt(int _number) { number = _number; }
};

struct SimpleShort : public Data {
	short number;

public:
	SimpleShort(int _number) { number = _number; }
};

struct SimpleLong : public Data {
	long number;

public:
	SimpleLong(int _number) { number = _number; }
};

struct SimpleDouble : public Data {
	double number;

public:
	SimpleDouble(int _number) { number = _number; }
};

struct SimpleString : public Data {
	wchar_t *string;

public:
	SimpleString(const wchar_t *_string) { string = Melder_wcsdup_e (_string); }
};

/* End of file Simple.h */
#endif
