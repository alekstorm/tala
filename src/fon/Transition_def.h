/* Transition_def.h
 *
 * Copyright (C) 1997-2007 Paul Boersma
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
 * pb 2007/08/12
 */


#define ooSTRUCT Transition
oo_DEFINE_CLASS (Transition, Data)

	oo_LONG (numberOfStates)
	oo_STRING_VECTOR (stateLabels, my numberOfStates)
	oo_DOUBLE_MATRIX (data, my numberOfStates, my numberOfStates)

oo_END_CLASS (Transition)
#undef ooSTRUCT


/* End of file Transition_def.h */
