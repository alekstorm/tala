#ifndef _VowelEditor_h_
#define _VowelEditor_h_
/* VowelEditor.h
 *
 * Copyright (C) 2008-2011 David Weenink
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
 djmw 20070130 First
 djmw 20110306 Latest modification.
*/

#ifndef _FormantTier_h_
	#include "FormantTier.h"
#endif
#ifndef _PitchTier_h_
	#include "PitchTier.h"
#endif

#ifndef _Graphics_h_
	#include "Graphics.h"
#endif
#ifndef _TableOfReal_h_
	#include "TableOfReal.h"
#endif
#ifndef _Editor_h_
	#include "Editor.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#define Vowel_members Function_members \
	PitchTier pt; \
	FormantTier ft;
#define Vowel_methods Function_methods
class_create (Vowel, Function);

struct structF0 
{
	double start;
	double slopeOctPerSec;
	double minimum, maximum;
	double samplingFrequency, adaptFactor, adaptTime;
	long interpolationDepth;
};

struct structF1F2Grid
{
	double df1, df2;
	int text_left, text_right, text_bottom, text_top;
	double grey;
};

#define VowelEditor__parents(Klas) Editor__parents(Klas) Thing_inherit (Klas, Editor)
Thing_declare1 (VowelEditor);
#define VowelEditor__members(Klas) Editor__members(Klas) \
	int soundFollowsMouse, shiftKeyPressed; \
	double f1min, f1max, f2min, f2max; /* Domain of graphics F1-F2 area */ \
	Matrix f3, b3, f4, b4; \
	int frequencyScale; /* 0: lin, 1: log, 2: bark, 3: mel */ \
	int axisOrientation; /* 0: origin topright + f1 down + f2 to left, 0: origin lb + f1 right +f2 up */ \
	int speakerType; /* 1 male, 2 female, 3 child */ \
	Graphics g; /* the drawing */ \
	short width, height; /* Size of drawing area in pixels. */ \
	Table marks; /* Vowel, F1, F2, Colour... */ \
	Vowel vowel; \
	double markTraceEvery; \
	struct structF0 f0; \
	double maximumDuration, extendDuration; \
	Sound source, target; \
	GuiObject drawingArea, playButton, reverseButton, publishButton; \
	GuiObject f0Label, f0TextField, f0SlopeLabel, f0SlopeTextField; \
	GuiObject durationLabel, durationTextField, extendLabel, extendTextField; \
	GuiObject startInfo, endInfo; \
	struct structF1F2Grid grid;
#define VowelEditor__methods(Klas) Editor__methods(Klas)
Thing_declare2 (VowelEditor, Editor);

VowelEditor VowelEditor_create (GuiObject parent, const wchar_t *title, Any data);

void VowelEditor_prefs (void);

#ifdef __cplusplus
	}
#endif

#endif /* _VowelEditor_h_ */
