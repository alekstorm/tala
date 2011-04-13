/* Distributions_and_Transition.h
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

#ifndef _Distributions_h_
	#include "stat/Distributions.h"
#endif
#ifndef _Transition_h_
	#include "Transition.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

Transition Distributions_to_Transition (Distributions underlying, Distributions surface,
	long environment, Transition adjacency, int greedy);

Distributions Distributions_Transition_map (Distributions me, Transition map);

Distributions Transition_to_Distributions_conflate (Transition me);

#ifdef __cplusplus
	}
#endif

/* End of file Distributions_and_Transition.h */
