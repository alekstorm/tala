/* Art_Speaker_Delta.h
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
 * pb 2011/03/08
 */

#include "Articulation.h"
#include "Speaker.h"
#include "Delta.h"

#ifdef __cplusplus
	extern "C" {
#endif

void Art_Speaker_intoDelta (Art art, Speaker speaker, Delta delta);

#ifdef __cplusplus
	}
#endif

/* End of file Art_Speaker_Delta.h */
