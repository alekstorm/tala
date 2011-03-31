/* Simple_extensions.c
 *
 * Copyright (C) 1994-2007 David Weenink
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
 djmw 20020812 GPL header
 djmw & pb wchar_t
*/

#include "Simple_extensions.h"
#include "longchar.h"

int SimpleString_init (SimpleString me, const wchar_t *string)
{
    if ((my string = Melder_wcsdup_e (string))  == NULL) return 0; 
    return 1;
}

int SimpleString_compare (SimpleString me, SimpleString thee)
{
	return wcscmp (my string, thy string);
}

const wchar_t *SimpleString_c (SimpleString me)
{
    return my string;
}

int SimpleString_append (SimpleString me, SimpleString thee)
{
    return SimpleString_append_c (me, thy string); 
}

int SimpleString_append_c (SimpleString me, const wchar_t *str)
{
	long myLength; wchar_t *ptr;
	if (! str) return 1;
	myLength = wcslen (my string);
	if ((ptr = Melder_realloc_e (my string, (myLength + wcslen (str) + 1) * sizeof (wchar_t))) == NULL) return 0;
	my string = ptr;
	wcscpy (& my string[myLength], str);
	return 1;	
}

SimpleString SimpleString_concat (SimpleString me, SimpleString thee)
{
	SimpleString him = Data_copy (me);
	if (! him || ! SimpleString_append_c (him, thy string)) forget (him);
	return him; 		
}

SimpleString SimpleString_concat_c (SimpleString me, const wchar_t *str)
{
	SimpleString him = Data_copy (me);
	if (! him || ! SimpleString_append_c (him, str)) forget (him);
	return him; 		
}

int SimpleString_replace_c (SimpleString me, const wchar_t *str)
{
	wchar_t *ptr;
    if (! str || ((ptr = Melder_wcsdup_e (str)) == NULL)) return 0;
    Melder_free (my string);
    my string = ptr;
    return 1;
}

long SimpleString_length (SimpleString me)
{
    return wcslen (my string);
}

void SimpleString_draw (SimpleString me, Any g, double xWC, double yWC)
{
    Graphics_text (g, xWC, yWC, my string);
}

const wchar_t * SimpleString_nativize_c (SimpleString me, int educateQuotes)
{
	SimpleString thee = Data_copy (me);
	if (! thee) return NULL;
	(void) Longchar_nativizeW (thy string, my string, educateQuotes);
	forget (thee);
	return my string;
}

const wchar_t * SimpleString_genericize_c (SimpleString me)
{
	SimpleString thee = Data_copy (me);
	wchar_t *ptr;
	if (thee == NULL ||
		(ptr = Melder_realloc_e (my string, 3 * wcslen (my string) * sizeof (wchar_t))) == NULL)
	{
		forget (thee); return NULL;
	}
	my string = ptr;
	(void) Longchar_genericizeW (thy string, my string);
	forget (thee);
	return my string;
}

/* End of file Simple_extensions.c */
