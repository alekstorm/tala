#ifndef _Distributions_and_Strings_h_
#define _Distributions_and_Strings_h_
/* Distributions_and_Strings.h
 *
 * Copyright (C) 1997-2011 Paul Boersma
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
 * pb 2011/03/03
 */

#include "Distributions.h"
#include "sys/Strings.h"

#ifdef __cplusplus
	extern "C" {
#endif

Strings Distributions_to_Strings (Distributions me, long column, long numberOfStrings);
Strings Distributions_to_Strings_exact (Distributions me, long column);
Distributions Strings_to_Distributions (Strings me);

#ifdef __cplusplus
	}
#endif

/* End of file Distributions_and_Strings.h */
#endif
