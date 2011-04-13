/* Excitation_to_Formant.h
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
 * pb 2011/03/03
 */

#ifndef _Excitation_h_
	#include "Excitation.h"
#endif
#ifndef _Formant_h_
	#include "Formant.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

Formant Excitation_to_Formant (Excitation, int maxnFormants);

#ifdef __cplusplus
	}
#endif

/* End of file Excitation_to_Formant.h */
