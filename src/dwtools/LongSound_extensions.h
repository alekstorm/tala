#ifndef _LongSound_extensions_h_
#define _LongSound_extensions_h_
/* LongSound_extensions.h
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
 djmw 20020627
 djmw 20020813 GPL header
 djmw 20110307 Latest modification
*/

#include "fon/LongSound.h"

#ifdef __cplusplus
	extern "C" {
#endif

int LongSounds_writeToStereoAudioFile16 (LongSound me, LongSound thee,
	int audioFileType, MelderFile file);

int LongSounds_appendToExistingSoundFile (Ordered me, MelderFile file);

#ifdef __cplusplus
	}
#endif

#endif /* _LongSound_extensions_h_ */
