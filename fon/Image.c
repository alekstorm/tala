/* Image.c
 *
 * Copyright (C) 1992-2007 Paul Boersma
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
 * pb 2003/02/17 removed source code
 * pb 2007/10/01 can write as encoding
 */

#include "Image.h"
#include "Matrix.h"

#include "oo_DESTROY.h"
#include "Image_def.h"
#include "oo_COPY.h"
#include "Image_def.h"
#include "oo_EQUAL.h"
#include "Image_def.h"
#include "oo_CAN_WRITE_AS_ENCODING.h"
#include "Image_def.h"
#include "oo_WRITE_TEXT.h"
#include "Image_def.h"
#include "oo_READ_TEXT.h"
#include "Image_def.h"
#include "oo_WRITE_BINARY.h"
#include "Image_def.h"
#include "oo_READ_BINARY.h"
#include "Image_def.h"
#include "oo_DESCRIPTION.h"
#include "Image_def.h"

class_methods (Image, Sampled) {
	class_method_local (Image, destroy)
	class_method_local (Image, copy)
	class_method_local (Image, equal)
	class_method_local (Image, canWriteAsEncoding)
	class_method_local (Image, writeText)
	class_method_local (Image, readText)
	class_method_local (Image, writeBinary)
	class_method_local (Image, readBinary)
	class_method_local (Image, description)
	class_methods_end
}

/* End of file Image.c */
