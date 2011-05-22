#ifndef _Matrix_and_Pitch_h_
#define _Matrix_and_Pitch_h_
/* Matrix_and_Pitch.h
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

#include "Matrix.h"
#include "Pitch.h"

#ifdef __cplusplus
	extern "C" {
#endif

Matrix Pitch_to_Matrix (Pitch me);

Pitch Matrix_to_Pitch (Matrix me);

#ifdef __cplusplus
	}
#endif

/* End of file Matrix_and_Pitch.h */
#endif
