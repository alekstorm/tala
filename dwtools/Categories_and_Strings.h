#ifndef _Categories_and_Strings_h_
#define _Categories_and_Strings_h_
/* Categories_and_Strings.h
 * 
 * Copyright (C) 2001-2011 David Weenink
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
 djmw 20020315 GPL header
 djmw 20110305 Latest modification
 */

#ifndef _Categories_h_
	#include "Categories.h"
#endif
#ifndef _Strings_h_
	#include "Strings.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

Strings Categories_to_Strings (Categories me);

Categories Strings_to_Categories (Strings me);

#ifdef __cplusplus
	}
#endif

#endif /* _Categories_and_Strings_h_ */
