/* Articulation.cpp
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
 * pb 2002/07/16 GPL
 * pb 2007/10/01 can write as encoding
 * pb 2009/03/21 modern enums
 * pb 2011/03/22 C++
 */

#include "Articulation.h"

#include "sys/oo/oo_DESTROY.h"
#include "Articulation_def.h"
#include "sys/oo/oo_COPY.h"
#include "Articulation_def.h"
#include "sys/oo/oo_EQUAL.h"
#include "Articulation_def.h"
#include "sys/oo/oo_CAN_WRITE_AS_ENCODING.h"
#include "Articulation_def.h"
#include "sys/oo/oo_WRITE_TEXT.h"
#include "Articulation_def.h"
#include "sys/oo/oo_WRITE_BINARY.h"
#include "Articulation_def.h"
#include "sys/oo/oo_WRITE_CACHE.h"
#include "Articulation_def.h"
#include "sys/oo/oo_READ_TEXT.h"
#include "Articulation_def.h"
#include "sys/oo/oo_READ_BINARY.h"
#include "Articulation_def.h"
#include "sys/oo/oo_READ_CACHE.h"
#include "Articulation_def.h"
#include "sys/oo/oo_DESCRIPTION.h"
#include "Articulation_def.h"

#include "sys/enums_getText.h"
#include "Articulation_enums.h"
#include "sys/enums_getValue.h"
#include "Articulation_enums.h"

class_methods (Art, Data) {
	class_method_local (Art, destroy)
	class_method_local (Art, copy)
	class_method_local (Art, equal)
	class_method_local (Art, canWriteAsEncoding)
	class_method_local (Art, writeText)
	class_method_local (Art, writeBinary)
	class_method_local (Art, writeCache)
	class_method_local (Art, readText)
	class_method_local (Art, readBinary)
	class_method_local (Art, readCache)
	class_method_local (Art, description)
	class_methods_end
}

Art Art_create (void) {
	return Thing_new (Art);
}

/* End of file Articulation.cpp */
