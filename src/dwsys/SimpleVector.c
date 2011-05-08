/* SimpleVector.c
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
 djmw 20071012 Added: o_CAN_WRITE_AS_ENCODING.h
*/

#include "SimpleVector.h"

#include "sys/oo/oo_DESTROY.h"
#include "SimpleVector_def.h"
#include "sys/oo/oo_COPY.h"
#include "SimpleVector_def.h"
#include "sys/oo/oo_EQUAL.h"
#include "SimpleVector_def.h"
#include "sys/oo/oo_CAN_WRITE_AS_ENCODING.h"
#include "SimpleVector_def.h"
#include "sys/oo/oo_WRITE_TEXT.h"
#include "SimpleVector_def.h"
#include "sys/oo/oo_WRITE_BINARY.h"
#include "SimpleVector_def.h"
#include "sys/oo/oo_READ_TEXT.h"
#include "SimpleVector_def.h"
#include "sys/oo/oo_READ_BINARY.h"
#include "SimpleVector_def.h"
#include "sys/oo/oo_DESCRIPTION.h"
#include "SimpleVector_def.h"

class_methods (DoubleVector, Data)
	class_method_local (DoubleVector, destroy)
	class_method_local (DoubleVector, equal)
	class_method_local (DoubleVector, canWriteAsEncoding)
	class_method_local (DoubleVector, copy)
	class_method_local (DoubleVector, readText)
	class_method_local (DoubleVector, readBinary)
	class_method_local (DoubleVector, writeText)
	class_method_local (DoubleVector, writeBinary)
	class_method_local (DoubleVector, description)
class_methods_end

int DoubleVector_init (I, long min, long max)
{
	iam (DoubleVector);
	my min = min; my max = max;
	if ((my v = NUMdvector (min, max)) == NULL) return 0;
	return 1;
}

DoubleVector DoubleVector_create (long min, long max)
{
	DoubleVector me = Thing_new (DoubleVector);
	if (! me || ! DoubleVector_init (me, min, max)) forget (me);
	return me;
}

class_methods (ComplexVector, Data)
	class_method_local (ComplexVector, destroy)
	class_method_local (ComplexVector, equal)
	class_method_local (ComplexVector, canWriteAsEncoding)
	class_method_local (ComplexVector, copy)
	class_method_local (ComplexVector, readText)
	class_method_local (ComplexVector, readBinary)
	class_method_local (ComplexVector, writeText)
	class_method_local (ComplexVector, writeBinary)
	class_method_local (ComplexVector, description)
class_methods_end

int ComplexVector_init (I, long min, long max)
{
	iam (ComplexVector);
	my min = min; my max = max;
	if ((my v = NUMdcvector (min, max)) == NULL) return 0;
	return 1;
}

ComplexVector ComplexVector_create (long min, long max)
{
	ComplexVector me = Thing_new (ComplexVector);
	if (! me || ! ComplexVector_init (me, min, max)) forget (me);
	return me;
}

/* End of file SimpleVector.c */
