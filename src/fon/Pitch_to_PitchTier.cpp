/* Pitch_to_PitchTier.c
 *
 * Copyright (C) 1992-2010 Paul Boersma
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
 * pb 1998/03/19
 * pb 2002/07/16 GPL
 * pb 2010/10/19 allow drawing without speckles
 */

#include "Pitch_to_PitchTier.h"

PitchTier Pitch_to_PitchTier (Pitch me) {
	PitchTier thee = PitchTier_create (my xmin, my xmax);   /* Same domain, for synchronization. */
	long i;
	if (! thee) goto error;
	for (i = 1; i <= my nx; i ++) {
		double frequency = my frame [i]. candidate [1]. frequency;

		/* Count only voiced frames.
		 */
		if (frequency > 0.0 && frequency < my ceiling) {
			double time = Sampled_indexToX (me, i);
			if (! RealTier_addPoint (thee, time, frequency)) goto error;
		}
	}
	return thee;
error:
	forget (thee);
	return (structPitchTier *)Melder_errorp ("(Pitch_to_PitchTier: ) Not performed.");
}

Pitch Pitch_PitchTier_to_Pitch (Pitch me, PitchTier tier) {
	long iframe;
	Pitch thee = NULL;
	if (tier -> points -> size == 0) return (structPitch *)Melder_errorp ("No pitch points.");
	thee = (structPitch *)Data_copy (me);
	for (iframe = 1; iframe <= my nx; iframe ++) {
		Pitch_Frame frame = & thy frame [iframe];
		Pitch_Candidate cand = & frame -> candidate [1];
		if (cand -> frequency > 0.0 && cand -> frequency <= my ceiling)
			cand -> frequency = RealTier_getValueAtTime (tier, Sampled_indexToX (me, iframe));
		cand -> strength = 0.9;
		frame -> nCandidates = 1;
	}
	return thee;
}

/* End of file Pitch_to_PitchTier.c */
