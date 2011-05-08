/* Index.c
 *
 * Copyright (C) 2005-2011 David Weenink
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
 djmw 20050724
 djmw 20061212 Changed info to Melder_writeLine<x> format.
 djmw 20070102
 djmw 20071012 Added: o_CAN_WRITE_AS_ENCODING.h
 djmw 20110304 Thing_new
*/

#include <time.h>
#include "Index.h"
#include "NUM2.h"

#include "sys/oo/oo_DESTROY.h"
#include "Index_def.h"
#include "sys/oo/oo_COPY.h"
#include "Index_def.h"
#include "sys/oo/oo_EQUAL.h"
#include "Index_def.h"
#include "sys/oo/oo_CAN_WRITE_AS_ENCODING.h"
#include "Index_def.h"
#include "sys/oo/oo_WRITE_TEXT.h"
#include "Index_def.h"
#include "sys/oo/oo_WRITE_BINARY.h"
#include "Index_def.h"
#include "sys/oo/oo_READ_BINARY.h"
#include "Index_def.h"
#include "sys/oo/oo_DESCRIPTION.h"
#include "Index_def.h"


static void info (I)
{
	iam (Index);
	classData -> info (me);
	MelderInfo_writeLine2 (L"Number of elements: ", Melder_integer (my numberOfElements));
}


class_methods (Index, Data)
	class_method_local (Index, destroy)
	class_method_local (Index, copy)
	class_method_local (Index, equal)
	class_method_local (Index, canWriteAsEncoding)
	class_method_local (Index, writeText)
	class_method_local (Index, writeBinary)
	class_method (info)
	class_method_local (Index, readBinary)
	class_method_local (Index, description)
class_methods_end

int Index_init (I, long numberOfElements)
{
	iam (Index);

    my classes = Ordered_create ();
    if (my classes == NULL) return 0;
    my numberOfElements = numberOfElements;
    my classIndex = NUMlvector (1, numberOfElements);
    if (my classIndex == NULL) return 0;
    return 1;
}

Index Index_extractPart (I, long from, long to)
{
	iam (Index);
	Index thee;
	long i;

	if (from == 0) from = 1;
	if (to == 0) to = my numberOfElements;
	if (to < from || from < 1 || to > my numberOfElements) return Melder_errorp
		("Index_extractPart: range should be in interval [1,%d].", my numberOfElements);
	if ((thee = Data_copy (me)) == NULL) return NULL;
	thy numberOfElements = to - from + 1;
	/* */
	for (i = 1; i <= thy numberOfElements; i++)
	{
		thy classIndex[i] = my classIndex[from + i - 1];
	}
	/* Claim excess memory */

	if (Melder_hasError ()) forget (thee);
	return thee;
}

class_methods (StringsIndex, Index)
	class_method_local (StringsIndex, destroy)
	class_method_local (StringsIndex, copy)
	class_method_local (StringsIndex, equal)
	class_method_local (StringsIndex, canWriteAsEncoding)
	class_method_local (StringsIndex, writeText)
	class_method_local (StringsIndex, writeBinary)
	class_method_local (StringsIndex, readBinary)
	class_method_local (StringsIndex, description)
class_methods_end

StringsIndex StringsIndex_create (long numberOfElements)
{
	StringsIndex me = Thing_new (StringsIndex);
	if (me == NULL || ! Index_init (me, numberOfElements)) forget (me);
	return me;
}

int StringsIndex_getClass (StringsIndex me, wchar_t *classLabel)
{
	long i;

	for (i = 1; i <= my classes -> size; i++)
	{
		SimpleString ss = my classes -> item[i];
		if (Melder_wcscmp (ss -> string, classLabel) == 0) return i;
	}
	return 0;
}

long StringsIndex_countItems (StringsIndex me, int iclass)
{
	long sum = 0;
	for (long i = 1; i <= my numberOfElements; i++) if (my classIndex[i] == iclass) sum++;
	return sum;
}

/* End of Index.c */
