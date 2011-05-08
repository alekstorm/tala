/* melder_debug.c
 *
 * Copyright (C) 2000-2011 Paul Boersma
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
 * pb 2011/03/05
 */

#include "melder.h"

int Melder_debug = 0;

/*

If Melder_debug is set to the following values in Praat,
the behaviour of that program changes in the following way:

1: Windows: use C-clock instead of multimedia-clock in melder_audio.c.
2: Windows: always reset waveOut, even when played to end, in melder_audio.c.
3: Windows: reset waveOut if unprepareHeader fails, and retry, in melder_audio.c. STAY: 20010214
4: Windows: fewer callbacks during sound play, in melder_audio.c. STAY: 20010214
6: Windows: info on metafile properties in Picture.c.
8: Windows: don't reset waveIn, in SoundRecorder.c.
9: flush Error in FunctionEditor_Sound_draw
10: geometric pens
11: clicked item in option menu in Ui.c.
12: no forced update event in XmUpdateDisplay on Mac
14: switches off the progress window in melder.c
15: don't use TrueType IPA fonts, but always bitmaps instead
16: Linux: open /dev/dsp without O_NDELAY
17: debugging on in Formula.c
18: no endian assumptions in abcio.c
19: show path name in UiOutfile_do
20: trace PortAudio
21: Mac: list supported document formats when printing
22: UTF-8 tests in logo
23: recognize special chunks in WAV files
24: measure buttons in drawing area
25: read crooked Manipulation files (January 2008)
26: force OT-GLA
27: force HG-GLA
28: don't use GSL in NUMfisherQ
29: use GSL in NUMinvFisherQ
30: pitch path finder: use octave jump cost across voiceless parts
31: Pitch analysis: formant pulling on
32: show info on file names in ExperimentMFC
33: trace the Pitch path finder
34: trace memory allocation and deallocation
35: debugging on for QuickTime movie file opening
36: debugging on for images
37: debug autoThing
38: debug autoNUMvector
1264: Mac: Sound_recordFixedTime uses microphone "FW Solo (1264)"

(negative values are for David)

*/

/* End of file melder_debug.c */
