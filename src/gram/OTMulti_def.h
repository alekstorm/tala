/* OTMulti_def.h
 *
 * Copyright (C) 2005-2011 Paul Boersma
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
 * pb 2011/03/02
 */

#define ooSTRUCT OTConstraint
oo_DEFINE_STRUCT (OTConstraint)

	oo_STRING (name)
	oo_DOUBLE (ranking)
	oo_DOUBLE (disharmony)
	oo_FROM (2)
		oo_DOUBLE (plasticity)
	oo_ENDFROM
	#if oo_READING
		if (localVersion < 2) {
			my plasticity = 1.0;
		}
	#endif
	#if !oo_READING && !oo_WRITING
		oo_INT (tiedToTheLeft)
		oo_INT (tiedToTheRight)
	#endif

oo_END_STRUCT (OTConstraint)
#undef ooSTRUCT


#define ooSTRUCT OTCandidate
oo_DEFINE_STRUCT (OTCandidate)

	oo_STRING (string)
	oo_LONG (numberOfConstraints)
	oo_INT_VECTOR (marks, my numberOfConstraints)
	#if !oo_READING && !oo_WRITING
		oo_DOUBLE (harmony)
		oo_DOUBLE (probability)
	#endif

oo_END_STRUCT (OTCandidate)
#undef ooSTRUCT


#define ooSTRUCT OTMulti
oo_DEFINE_CLASS (OTMulti, Data)

	oo_FROM (1)
		oo_ENUM (kOTGrammar_decisionStrategy, decisionStrategy)
	oo_ENDFROM
	oo_FROM (2)
		oo_DOUBLE (leak)
	oo_ENDFROM
	oo_LONG (numberOfConstraints)
	oo_STRUCT_VECTOR (OTConstraint, constraints, my numberOfConstraints)
	oo_LONG_VECTOR (index, my numberOfConstraints)
	oo_LONG (numberOfCandidates)
	oo_STRUCT_VECTOR (OTCandidate, candidates, my numberOfCandidates)
	#if oo_READING
		OTMulti_sort (me);
	#endif

oo_END_CLASS (OTMulti)
#undef ooSTRUCT


/* End of file OTMulti_def.h */
