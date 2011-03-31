#ifndef _DTW_and_TextGrid_h_
#define _DTW_and_TextGrid_h_

/* DTW_and_TextGrid.h
 *
 * Copyright (C) 1993-2011 David Weenink
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
 djmw 20060906
 djmw 20110307 Latest modification
*/

#ifndef _DTW_h_
	#include "DTW.h"
#endif

#ifndef _TextGrid_h_
	#include "TextGrid.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

TextTier DTW_and_TextTier_to_TextTier (DTW me, TextTier thee);
IntervalTier DTW_and_IntervalTier_to_IntervalTier (DTW me, IntervalTier thee);
TextGrid DTW_and_TextGrid_to_TextGrid (DTW me, TextGrid thee);
/*
	Purpose: Create the new TextGrid with all times determined by the DTW.
		The y-dimension of the DTW determines the new times.
*/

#ifdef __cplusplus
	}
#endif

#endif /* _DTW_and_TextGrid_h_ */
