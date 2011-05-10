/* Table_extensions.h
 *
 * Copyright (C) 1997-2011 David Weenink
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
 djmw 20020619 GPL header
 djmw 20110307 Latest modification
*/

#ifndef _Table_extensions_h_
#define _Table_extensions_h_

#ifndef _Table_h_
	#include "stat/Table.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

Table Table_createFromPetersonBarneyData (void);
Table Table_createFromPolsVanNieropData (void);
Table Table_createFromWeeninkData (void);

#ifdef __cplusplus
	}
#endif

#endif /* _Table_extensions_h_ */
