#ifndef _Simple_extensions_h_
#define _Simple_extensions_h_
/* Simple_extensions.h
 *
 * Copyright (C) 1994-2002 David Weenink
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
 djmw 19950616
 djmw 20020812 GPL header
*/

#include "Data.h"
#include "Graphics.h"
#include "Simple.h"

int SimpleString_init (SimpleString me, const wchar_t *value);
/* return 0 when value == NULL */

const wchar_t *SimpleString_c (SimpleString me);
/* return pointer to the string */

int SimpleString_compare (SimpleString me, SimpleString thee);

int SimpleString_append (SimpleString me, SimpleString thee);
int SimpleString_append_c (SimpleString me, const wchar_t *str);
/* append string to me */

SimpleString SimpleString_concat (SimpleString me, SimpleString thee);
SimpleString SimpleString_concat_c (SimpleString me, const wchar_t *str);
/* concatenate two strings */

int SimpleString_replace_c (SimpleString me, const wchar_t *replacement);
/* replace my value with new string */

long SimpleString_length (SimpleString me);
/* return my length */

void SimpleString_draw (SimpleString me, Any g, double xWC, double yWC);
/* draw the string */

const wchar_t * SimpleString_nativize_c (SimpleString me, int educateQuotes);
const wchar_t * SimpleString_genericize_c (SimpleString me);
/* see longchar.h for info */

#endif /* _Simple_extensions_h_ */
