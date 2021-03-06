#ifndef _Sound_and_Cepstrum_h_
#define _Sound_and_Cepstrum_h_
/* Sound_and_Cepstrum.h
 *
 * Copyright (C) 1994-2011 David Weenink
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
 djmw 20110307 Latest modification
*/

#include "Cepstrum.h"
#include "fon/Sound.h"

#ifdef __cplusplus
	extern "C" {
#endif

Cepstrum Sound_to_Cepstrum (Sound me);

Cepstrum Sound_to_Cepstrum_bw (Sound me);

Sound Cepstrum_to_Sound (Cepstrum me);

#ifdef __cplusplus
	}
#endif

#endif /* _Sound_and_Cepstrum_h_ */
