/* VowelEditor.c
 *
 * Copyright (C) 2008-2010 David Weenink
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
 djmw 20080202, 20080330
 djmw 20090114 FormantTier_to_FormantGrid.
 djmw 20090613 Extract KlattGrid
*/

/*
trajectory --> path ????
 The main part of the VowelEditor is a drawing area.
 In this drawing area a cursor can be moved around by a mouse.
 The position of the cursor is related to the F1 and F2 frequencies.
 On_mouse_down the position of the cursor is sampled (Not a fixed intervals!).
 This results in a series of (x,y) values that will be transformed to (F1,F2) values in Hertz
 The corresponding sound wil be made audible until the mouse is released.

 Graphics area is F1-F2 plane. Origin top-right with log(F2) horizontal and log(F1) vertical).
 Axis orientation from topright: F1 down, F2 to the left.
 F1, F2 are always evaluated to Hz;
 In the Graphics part, the Graphics_window (0, 1, 0, 1), i.e. origin is bottom-left.
 log(fmin) -> 1; log(fmax)-> 0
 Transformations XY <=> F1F2: getXYFromF1F2(...) and getF1F2FromXY(...)
  For x direction F2 from right to left
   1 = a * log(f2min) +b
   0 = a * log(f2max) +b
   x' = a (log(f2)-log(f2max))

   1 = a * log(f1min) +b
   0 = a * log(f1max) +b
   y' = a (log(f1)-log(f1max))
 TO DO:
 The third and higher formant frequencies can also be set indirectly by defining them as functions on the f1,f2 plane
 (for example, by an object of type Matrix).
 Make sound-follows-mouse real time!
*/

#include "VowelEditor.h"

#include "dwtools/KlattGrid.h"
#include "dwtools/TableOfReal_extensions.h"
#include "dwtools/Table_extensions.h"
#include "fon/FormantGrid.h"
#include "fon/PitchTier_to_PointProcess.h"
#include "fon/PitchTier_to_Sound.h"
#include "fon/PointProcess_and_Sound.h"
#include "fon/Polygon.h"
#include "EditorM.h"
#include "ui/Interpreter.h"
#include "ui/machine.h"
#include "ui/Preferences.h"

#include "ui/praat.h"
#include "EditorM.h"

#include <portaudio.h>
#include <time.h>

#if defined (macintosh)
	#include <sys/time.h>
#elif defined (linux)
	#include <sys/time.h>
	#include <signal.h>
#endif

// Male, Female, Child speaker
#define VG_SPEAKER_M 0
#define VG_SPEAKER_F 1
#define VG_SPEAKER_C 2

// STATUS_INFO >=Gui_LABEL_HEIGHT !!
#define STATUS_INFO (1.5*Gui_LABEL_HEIGHT)
#define MARGIN_RIGHT 10
#define MARGIN_LEFT 50
#define MARGIN_TOP 50
#define MARGIN_BOTTOM (60+STATUS_INFO)
#define BUFFER_SIZE_SEC 4
#define SAMPLING_FREQUENCY 44100

#define STATUSINFO_STARTINTR0 L"Start (F1,F2,F0) = ("
#define STATUSINFO_ENDINTR0 L"End (F1,F2,F0) = ("
#define STATUSINFO_ENDING L")"
#define MICROSECPRECISION(x) (round((x)*1000000)/1000000)

// Too prevent the generation of inaudible short Sounds we set a minimum duration
#define MINIMUM_SOUND_DURATION 0.01

// helpers
static double getRealFromTextWidget (GuiObject me);
static double getCoordinate (double fmin, double fmax, double f);
static double getF0 (struct structF0 *f0p, double time);
static void checkF0 (struct structF0 *f0p, double *f0);
static void	checkXY (double *x, double *y);
static void Sound_fadeIn (Sound me, double duration, int fromFirstNonZeroSample);
static void Sound_fadeOut (Sound me, double duration);
static void PitchTier_newDuration (PitchTier me, struct structF0 *f0p, double newDuration);
static void FormantTier_newDuration (FormantTier me, double newDuration);
static void FormantTier_drawF1F2Trajectory (FormantTier me, Graphics g, double f1min, double f1max, double f2min, double f2max, double markTraceEvery, double width);
static FormantGrid FormantTier_to_FormantGrid (FormantTier me);
static double Matrix_getValue (Matrix me, double x, double y);

static Vowel Vowel_create (double duration);
static Vowel Vowel_create_twoFormantSchwa (double duration);
static void Vowel_newDuration (Vowel me, struct structF0 *f0p, double newDuration);
static Sound Vowel_to_Sound_pulses (Vowel me, double samplingFrequency, double adaptFactor, double adaptTime, long interpolationDepth);
// forward declarations end

static struct structF0 f0default = { 140.0, 0.0, 40.0, 2000.0, SAMPLING_FREQUENCY, 1, 0.0, 2000 };
static struct structF1F2Grid griddefault = { 200, 500, 0, 1, 0, 1, 0.5 };

#define VOWEL_def_h \
oo_DEFINE_CLASS (Vowel, Function)\
	oo_OBJECT (PitchTier, 0, pt)\
	oo_OBJECT (FormantTier, 0, ft)\
oo_END_CLASS (Vowel)

#include "sys/oo/oo_DESTROY.h"
#define ooSTRUCT Vowel
VOWEL_def_h
#undef ooSTRUCT
#include "sys/oo/oo_COPY.h"
#define ooSTRUCT Vowel
VOWEL_def_h
#undef ooSTRUCT

static struct {
	int soundFollowsMouse;
	double f1min, f1max, f2min, f2max;
	double f3, b3, f4, b4;
	int frequencyScale;
	int axisOrientation;
	int speakerType;
} preferences;

void VowelEditor::prefs (void)
{
	Preferences_addInt (L"VowelEditor.soundFollowsMouse", &preferences.soundFollowsMouse, 1);
	Preferences_addDouble (L"VowelEditor.f1min", &preferences.f1min, 200);
	Preferences_addDouble (L"VowelEditor.f1max", &preferences.f1max, 1200);
	Preferences_addDouble (L"VowelEditor.f2min", &preferences.f2min, 500);
	Preferences_addDouble (L"VowelEditor.f2max", &preferences.f2max, 3500);
	Preferences_addDouble (L"VowelEditor.f3", &preferences.f3, 2500);
	Preferences_addDouble (L"VowelEditor.b3", &preferences.b3, 250);
	Preferences_addDouble (L"VowelEditor.f4", &preferences.f4, 3500);
	Preferences_addDouble (L"VowelEditor.b4", &preferences.b4, 350);
	Preferences_addInt (L"VowelEditor.frequencyScale", &preferences.frequencyScale, 0);
	Preferences_addInt (L"VowelEditor.axisOrientation", &preferences.axisOrientation, 0);
	Preferences_addInt (L"VowelEditor.speakerType", &preferences.speakerType, 1);
}

void VowelEditor::cb_publish (Editor *editor, void *closure, Any publish)
{
	(void) editor;
	(void) closure;
	if (! praat_new1 (publish, NULL)) { Melder_flushError (NULL); return; }
	praat_updateSelection ();
}

VowelEditor::VowelEditor (GuiObject parent, const wchar_t *title, Any data)
	: Editor (parent, 20, 40, 650, 650, title, data),
	  _f1min(preferences.f1min),
	  _f1max(preferences.f1max),
	  _f2min(preferences.f2min),
	  _f2max(preferences.f2max),
	  _frequencyScale(preferences.frequencyScale),
	  _axisOrientation(preferences.axisOrientation),
	  _speakerType(preferences.speakerType),
	  _soundFollowsMouse(preferences.soundFollowsMouse),
	  _maximumDuration(BUFFER_SIZE_SEC),
	  _extendDuration(0.05),
	  _markTraceEvery(0.05),
	  _f0(f0default),
	  _target(Sound_createSimple (1, _maximumDuration, _f0.samplingFrequency)),
	  _grid(griddefault) {
	createMenus ();
	createChildren ();
	_g = Graphics_create_xmdrawingarea (_drawingArea);
	Graphics_setFontSize (_g, 10);
	prefs ();
	setPublishCallback (cb_publish, NULL);

	if (! setMarks (2, _speakerType, 14)) return;
	if (! setF3F4 (preferences.f3, preferences.b3, preferences.f4, preferences.b4)) return;
	if (_data != NULL)
	{
		_vowel = (structVowel *)Data_copy (data);
		if (_vowel == NULL) return;
	}
	else
	{
		_vowel = Vowel_create_twoFormantSchwa (0.2);
	}
	if (! setSource ()) return;
	GuiText_setString (_f0TextField, Melder_double (_f0.start));
	GuiText_setString (_f0SlopeTextField, Melder_double (_f0.slopeOctPerSec));
	GuiText_setString (_durationTextField, L"0.2"); // Source has been created
	GuiText_setString (_extendTextField, Melder_double (_extendDuration));
	struct structGuiDrawingAreaResizeEvent event = { 0 };
	event.widget = _drawingArea;
	gui_drawingarea_cb_resize (this, & event);
	updateWidgets ();
}

VowelEditor::~VowelEditor () {
	forget (_g);
	forget (_marks);
	forget (_source);
	forget (_target);
	forget (_vowel);
	forget (_f3); forget (_b3);
	forget (_f4); forget (_b4);
}

class_methods (Vowel, Function)
{
	class_method_local (Vowel, destroy)
	class_method_local (Vowel, copy)
	class_methods_end
}

static Vowel Vowel_create (double duration)
{
	Vowel me = Thing_new (Vowel);

	if (me == NULL || ! Function_init (me, 0, duration)) return NULL;
	my ft = FormantTier_create (0, duration);
	my pt = PitchTier_create (0, duration);
	if (Melder_hasError ()) forget ();
	return me;
}

static Vowel Vowel_create_twoFormantSchwa (double duration)
{
	FormantPoint fp = NULL;
	Vowel me = Vowel_create (duration);
	if (me == NULL) return NULL;

	fp =  FormantPoint_create (0);
	fp -> formant [0] = 500;
	fp -> bandwidth[0] = 50;
	fp -> formant [1] = 1500;
	fp -> bandwidth[1] = 150;
	fp -> numberOfFormants = 2;
	if (! Collection_addItem (my ft -> points, fp) || ! RealTier_addPoint (my pt, 0, 140)) goto end;

	fp =  FormantPoint_create (duration);
	if (fp == NULL) goto end;
	fp -> formant [0] = 500;
	fp -> bandwidth[0] = 50;
	fp -> formant [1] = 1500;
	fp -> bandwidth[1] = 150;
	fp -> numberOfFormants = 2;
	if (Collection_addItem (my ft -> points, fp)) RealTier_addPoint (my pt, duration, 140);
end:
	if (Melder_hasError ()) forget ();
	return me;
}

static Sound Vowel_to_Sound_pulses (Vowel me, double samplingFrequency, double adaptFactor, double adaptTime, long interpolationDepth)
{
	Sound thee = NULL;
	PointProcess pp = PitchTier_to_PointProcess (my pt);
	if (pp != NULL)
	{
		thee = PointProcess_to_Sound_pulseTrain (pp, samplingFrequency, adaptFactor, adaptTime, interpolationDepth);
		Sound_FormantTier_filter_inline (thee, my ft);
		forget (pp);
	}
	return thee;
}

static FormantGrid FormantTier_to_FormantGrid (FormantTier me)
{
	int numberOfFormants = FormantTier_getMaxNumFormants (me);
	FormantGrid thee = FormantGrid_createEmpty (my xmin, my xmax, numberOfFormants);
	if (thee == NULL) return NULL;
	for (long ipoint = 1; ipoint <= my points -> size; ipoint++)
	{
		FormantPoint fp = (structFormantPoint *)my points -> item[ipoint];
		double t = fp -> time;
		for (long iformant = 1; iformant <= fp -> numberOfFormants; iformant++)
		{
			if (! FormantGrid_addFormantPoint (thee, iformant, t, fp -> formant[iformant - 1]) ||
				! FormantGrid_addBandwidthPoint (thee, iformant, t, fp -> bandwidth[iformant -1])) goto end;
		}
	}
end:
	if (Melder_hasError ()) forget (thee);
	return thee;
}

void VowelEditor::getXYFromF1F2 (double f1, double f2, double *x, double *y)
{
	*x = log (f2 / _f2max) / log (_f2min / _f2max);
	*y = log (f1 / _f1max) / log (_f1min / _f1max);
}

//Graphics_DCtoWC ????
void VowelEditor::getF1F2FromXY (double x, double y, double *f1, double *f2)
{
	*f2 = _f2min * pow (_f2max / _f2min, 1 - x);
	*f1 = _f1min * pow (_f1max / _f1min, 1 - y);
}

#define REPRESENTNUMBER(x,i) (((x) == NUMundefined) ? L" undef" : ((swprintf(buffer[i], 7, L"%6.1f",x)), buffer[i]))
static void appendF1F2F0 (MelderString *statusInfo, wchar_t *intro, double f1, double f2, double f0, wchar_t *ending)
{
	wchar_t *komma = L", ";
	wchar_t buffer[4][10];
	MelderString_append7 (statusInfo, intro, REPRESENTNUMBER(f1,1), komma, REPRESENTNUMBER(f2,2), komma, REPRESENTNUMBER(f0,3), ending);
}

static double getRealFromTextWidget (GuiObject me)
{
	double value = NUMundefined;
	wchar_t *dirty = GuiText_getString (me);
	if (! Interpreter_numericExpression_FIXME (dirty, & value))
	{
		Melder_clearError (); value = NUMundefined;
	}
	Melder_free (dirty);
	return value;
}

void VowelEditor::updateF0Info ()
{
	double f0 = getRealFromTextWidget (_f0TextField);
	checkF0 (&_f0, &f0);
	GuiText_setString (_f0TextField, Melder_double (f0));
	_f0.start = f0;
	double slopeOctPerSec = getRealFromTextWidget (_f0SlopeTextField);
	if (slopeOctPerSec == NUMundefined) slopeOctPerSec = f0default.slopeOctPerSec;
    _f0.slopeOctPerSec = slopeOctPerSec;
	GuiText_setString (_f0SlopeTextField, Melder_double (_f0.slopeOctPerSec));
}

void VowelEditor::updateExtendDuration ()
{
	double extend = getRealFromTextWidget (_extendTextField);
	if (extend == NUMundefined || extend <= MINIMUM_SOUND_DURATION || extend > _maximumDuration) extend = MINIMUM_SOUND_DURATION;
	GuiText_setString (_extendTextField, Melder_double (extend));
	_extendDuration = extend;
}

double VowelEditor::updateDurationInfo ()
{
	double duration = getRealFromTextWidget (_durationTextField);
	if (duration == NUMundefined || duration < MINIMUM_SOUND_DURATION) duration = MINIMUM_SOUND_DURATION;
	GuiText_setString (_durationTextField, Melder_double (MICROSECPRECISION(duration)));
	return duration;
}

static void Sound_fadeIn (Sound me, double duration, int fromFirstNonZeroSample)
{
	long istart = 1, numberOfSamples = duration / my dx;

	if (numberOfSamples < 2) return;
	if (fromFirstNonZeroSample != 0)
	{
		// If the first part of the sound is very low level we put sample values to zero and
		// start windowing from the position where the amplitude is above the minimum level.
		// WARNING: this part is special for the artificial vowels because
		// 1. They have no offset
		// 2. They are already scaled to a maximum amplitude of 0.99
		// 3. For 16 bit precision
		double zmin = 0.5 / pow(2, 16);
		while (fabs (my z[1][istart]) < zmin && istart < my nx)
		{
			my z[1][istart] = 0; // To make sure
			istart++;
		}
	}
	if (numberOfSamples > my nx - istart + 1) numberOfSamples = my nx - istart + 1;

	for (long i = 1; i <= numberOfSamples; i++)
	{
		double phase = NUMpi * (i - 1) / (numberOfSamples - 1);

		my z[1][istart + i -1] *= 0.5 * (1 - cos (phase));
	}
}

static void Sound_fadeOut (Sound me, double duration)
{
	long istart, numberOfSamples = duration / my dx;

	if (numberOfSamples < 2) return;
	if (numberOfSamples > my nx) numberOfSamples = my nx;
	istart = my nx - numberOfSamples;
	// only one channel
	for (long i = 1; i <= numberOfSamples; i++)
	{
		double phase = NUMpi * (i - 1)/ (numberOfSamples - 1);

		my z[1][istart + i] *= 0.5 * (1 + cos (phase));
	}
}

static double getF0 (struct structF0 *f0p, double time)
{
	double f0 = f0p -> start * pow (2, f0p -> slopeOctPerSec * time);
	if (f0 < f0p -> minimum) { f0 = f0p -> minimum; } else if (f0 > f0p -> maximum) { f0 = f0p -> maximum; }
	return f0;
}

void VowelEditor::Vowel_reverseFormantTier ()
{
	FormantTier ft = _vowel -> ft;
	FormantPoint fpt;
	double duration = ft -> xmax;
	long np = ft -> points -> size, np_2 = np / 2;

	for (long i = 1; i <= np_2; i++)
	{
		fpt = (structFormantPoint *)ft -> points -> item[i];
		ft -> points -> item[i] =  ft -> points -> item[np - i + 1];
		ft -> points -> item[np - i + 1] = fpt;
		fpt = (structFormantPoint *)ft -> points -> item[i];
		fpt -> time = duration - fpt -> time;
		fpt = (structFormantPoint *)ft -> points -> item[np - i + 1];
		fpt -> time = duration - fpt -> time;
	}
	if (np % 2 == 1)
	{
		fpt = (structFormantPoint *)ft -> points -> item[np_2+1];
		fpt -> time = duration - fpt -> time;
	}
}

void VowelEditor::shiftF1F2 (double f1_st, double f2_st)
{
	FormantTier ft = _vowel -> ft;
	for (long i = 1; i <= ft -> points -> size; i++)
	{
		FormantPoint fp = (structFormantPoint *)ft -> points -> item[i];
		double f1 = fp -> formant[0], f2 = fp -> formant[1];
		double f3, b3, f4, b4;

		f1 *= pow (2, f1_st / 12);
		if (f1 < _f1min) f1 = _f1min;
		if (f1 > _f1max) f1 = _f1max;
		fp -> formant[0] = f1;
		fp -> bandwidth[0] = f1 / 10;

		f2 *= pow (2, f2_st / 12);
		if (f2 < _f2min) f2 = _f2min;
		if (f2 > _f2max) f2 = _f2max;
		fp -> formant[1] = f2;
		fp -> bandwidth[1] = f2 / 10;
		getF3F4 (f1, f2, &f3, &b3, &f4, &b4);
		fp -> formant[2] = f3;
		fp -> bandwidth[2] = b3;
		fp -> formant[3] = f4;
		fp -> bandwidth[3] = b4;
	}
}

static void Vowel_newDuration (Vowel me, struct structF0 *f0p, double newDuration)
{
	if (newDuration != my xmax)
	{
		double multiplier = newDuration /my xmax;
		FormantTier_newDuration (my ft, newDuration);
		my xmax *= multiplier;
	}
	PitchTier_newDuration (my pt, f0p, newDuration); // always update

}

static void FormantTier_newDuration (FormantTier me, double newDuration)
{
	if (newDuration != my xmax)
	{
		double multiplier = newDuration / my xmax;

		for (long i = 1; i <= my points -> size; i++)
		{
			FormantPoint fp = (structFormantPoint *)my points -> item[i];
			fp -> time *= multiplier;
		}
		my xmax *= multiplier;
	}
}

static void PitchTier_newDuration (PitchTier me, struct structF0 *f0p, double newDuration)
{
	// Always update; GuiObject text might have changed
	double multiplier = newDuration / my xmax;
	for (long i = 1; i <= my points -> size; i++)
	{
		RealPoint pp = (structRealPoint *)my points -> item[i];
		pp -> time *= multiplier;
		pp -> value = getF0 (f0p, pp -> time);
	}
	my xmax *= multiplier;
}

void VowelEditor::updateVowel ()
{
	double newDuration = updateDurationInfo (); // Get new duration from TextWidget
	updateF0Info (); // Get new pitch and slope values from TextWidgets
	Vowel_newDuration (_vowel, & _f0, newDuration);
}

static double getCoordinate (double fmin, double fmax, double f)
{
	return log (f / fmax) / log (fmin / fmax);
}

#define GETX(x) (getCoordinate (f2min, f2max, x))
#define GETY(y) (getCoordinate (f1min, f1max, y))
// Our FormantTiers always have a FormantPoint at t=xmin and t=xmax;
static void FormantTier_drawF1F2Trajectory (FormantTier me, Graphics g, double f1min, double f1max, double f2min, double f2max, double markTraceEvery, double width)
{
	int it, imark = 1, glt = Graphics_inqLineType (g);
	double glw = Graphics_inqLineWidth (g), x1, y1, x2, y2, t1, t2;
	Graphics_Colour colour = Graphics_inqColour (g);
	long nfp = my points -> size;
	FormantPoint fp = (structFormantPoint *)my points -> item[1], fpn = (structFormantPoint *)my points -> item[nfp];
	double tm, markLength = 0.01;

	Graphics_setInner (g);
	Graphics_setWindow (g, 0, 1, 0, 1);
	Graphics_setLineType (g, Graphics_DRAWN);
	// Too short too hear ?
	if ((my xmax - my xmin) < 0.005) Graphics_setColour (g, Graphics_RED);
	x1 = GETX(fp->formant[1]); y1 = GETY(fp->formant[0]); t1 = fp->time;
	for (it = 2; it <= nfp; it++)
	{
		fp = (structFormantPoint *)my points -> item[it];
		x2 = GETX(fp->formant[1]); y2 = GETY(fp->formant[0]); t2 = fp->time;
		Graphics_setLineWidth (g, 3);
		Graphics_line (g, x1, y1, x2, y2);
		while (markTraceEvery > 0 && (tm = imark * markTraceEvery) < t2)
		{
			// line orthogonal to y = (y1/x1)*x is y = -(x1/y1)*x
			double fraction = (tm - t1) / (t2 - t1);
			double dx = x2 - x1, dy = y2 - y1;
			double xm = x1 + fraction * dx, ym = y1 + fraction * dy;
			double xl1 = dy * markLength / sqrt (dx * dx + dy * dy), xl2 = - xl1;
			double yl1 = dx * markLength / sqrt (dx * dx + dy * dy), yl2 = - yl1;

			if (dx * dy > 0)
			{
				xl1 = -fabs (xl1); yl1 = fabs(yl1);
				xl2 = fabs (xl1); yl2 = -fabs(yl1);
			}
			else if (dx * dy < 0)
			{
				xl1 = -fabs (xl1); yl1 = -fabs(yl1);
				xl2 = fabs (xl1); yl2 = fabs(yl1);
			}
			Graphics_setLineWidth (g, 1);
			Graphics_line (g, xm + xl1, ym + yl1, xm + xl2, ym + yl2);

			imark++;
		}
		x1 = x2; y1 = y2; t1 = t2;
	}
	// Arrow at end
	{
		double gas = Graphics_inqArrowSize (g), arrowSize = 1;
		double size = 10.0 * arrowSize * Graphics_getResolution (g) / 75.0 / width, size2 = size * size;
		Graphics_setArrowSize (g, arrowSize);
		it = 1;
		while (it <= (nfp -1))
		{
			fp = (structFormantPoint *)my points -> item[nfp - it];
			double dx = GETX(fpn->formant[1]) - GETX(fp->formant[1]);
			double dy = GETY(fpn->formant[0]) - GETY(fp->formant[0]);
			double d2 = dx * dx + dy * dy;
			if (d2 > size2) break;
			it++;
		}
		Graphics_arrow (g, GETX(fp->formant[1]), GETY(fp->formant[0]), GETX(fpn->formant[1]), GETY(fpn->formant[0]));
		Graphics_setArrowSize (g, gas);
	}
	Graphics_unsetInner (g);
	Graphics_setColour (g, colour);
	Graphics_setLineType (g, glt);
	Graphics_setLineWidth (g, glw);
}
#undef GETX
#undef GETY

PitchTier VowelEditor::to_PitchTier (double duration)
{
	double t_end = duration;
	double f0_end = _f0.start * pow (2, _f0.slopeOctPerSec * t_end);
	PitchTier thee = PitchTier_create (0, t_end);

	if (thee == NULL) return NULL;
	if (! RealTier_addPoint (thee, 0, _f0.start)) goto end;
	if (_f0.slopeOctPerSec < 0)
	{
		if (f0_end < _f0.minimum)
		{
			t_end = log2 (_f0.minimum / _f0.start) / _f0.slopeOctPerSec;
			f0_end = _f0.minimum;
		}
	}
	else if (_f0.slopeOctPerSec > 0)
	{
		if (f0_end > _f0.maximum)
		{
			t_end = log2 (_f0.maximum / _f0.start) / _f0.slopeOctPerSec;
			f0_end = _f0.maximum;
		}
	}
	RealTier_addPoint (thee, t_end, f0_end);
end:
	if (Melder_hasError ()) forget (thee);
	return thee;
}

int VowelEditor::setMarks (int dataset, int speakerType, int fontSize)
{
	Table thee = NULL, te = NULL;
	wchar_t *Type[4] = { L"", L"m", L"w", L"c" };
	wchar_t *Sex[3] = { L"", L"m", L"f"};

	if (dataset == 1) // American-English
	{
		thee = Table_createFromPetersonBarneyData ();
		if (thee == NULL) return 0;
		te = Table_extractRowsWhereColumn_string (thee, 1, kMelder_string_EQUAL_TO, Type[speakerType]);
	}
	else if (dataset == 2) // Dutch
	{
		if (speakerType == 1 || speakerType == 2) // male + female from Pols van Nierop
		{
			thee = Table_createFromPolsVanNieropData ();
			if (thee == NULL) return 0;
			te = Table_extractRowsWhereColumn_string (thee, 1, kMelder_string_EQUAL_TO, Sex[speakerType]);
		}
		else
		{
			thee = Table_createFromWeeninkData ();
			if (thee == NULL) return 0;
			te = Table_extractRowsWhereColumn_string (thee, 1, kMelder_string_EQUAL_TO, Type[speakerType]);
		}
	}
	else
	{
		forget (_marks);
		return 1;
	}
	forget (thee);
	if (te == NULL) return 0;
	thee = Table_collapseRows (te, L"IPA", L"", L"F1 F2", L"", L"", L"");
	forget (te);
	if (thee == NULL) return 0;
	if (Table_appendColumn (thee, L"fontSize"))
	{
		for (long i = 1; i <= thee->rows -> size; i++)
		{
			if (! Table_setNumericValue (thee, i, thee->numberOfColumns, fontSize)) goto end;
		}
		forget (_marks);
		_marks = thee;
		return 1;
	}
end:
	forget (thee);
	return 0;
}

int VowelEditor::setF3F4 (double f3, double b3, double f4, double b4)
{
	double xmin = _f2min, xmax = _f2max, dx = _f2max - _f2min, x1 = dx / 2;
	double dy = _f1max - _f1min, y1 = dy / 2;

	if (_f3 == NULL)
	{
		_f3 = Matrix_create (xmin, xmax, 1, dx, x1, _f1min, _f1max, 1, dy, y1);
		if (_f3 == NULL) goto end;
	}
	if (_b3 == NULL)
	{
		_b3 = Matrix_create (xmin, xmax, 1, dx, x1, _f1min, _f1max, 1, dy, y1);
		if (_b3 == NULL) goto end;
	}
	if (_f4 == NULL)
	{
		_f4 = Matrix_create (xmin, xmax, 1, dx, x1, _f1min, _f1max, 1, dy, y1);
		if (_f4 == NULL) goto end;
	}
	if (_b4 == NULL) _b4 = Matrix_create (xmin, xmax, 1, dx, x1, _f1min, _f1max, 1, dy, y1);

end:

	if (Melder_hasError ())
	{
		forget (_f3); forget (_b3);
		forget (_f4); forget (_b4);
		return 0;
	}
	_f3 -> z[1][1] = f3; _b3 -> z[1][1] = b3;
	_f4 -> z[1][1] = f4; _b4 -> z[1][1] = b4;
	return 1;
}

static double Matrix_getValue (Matrix me, double x, double y)
{
	(void) x;
	(void) y;
	return my z[1][1];
}

void VowelEditor::getF3F4 (double f1, double f2, double *f3, double *b3, double *f4, double *b4)
{
	*f3 = Matrix_getValue (_f3, f2, f1);
	*b3 = Matrix_getValue (_b3, f2, f1);
	*f4 = Matrix_getValue (_f4, f2, f1);
	*b4 = Matrix_getValue (_b4, f2, f1);
}

void VowelEditor::drawBackground (Graphics g)
{
	Table thee = _marks;
	double x1, y1, x2, y2, f1, f2;

	Graphics_setInner (g);
	Graphics_setWindow (g, 0, 1, 0, 1);
	Graphics_setGrey (g, 0);
	Graphics_setLineType (g, Graphics_DRAWN);
	Graphics_setLineWidth (g, 2);
	Graphics_rectangle (g, 0, 1, 0, 1);
	Graphics_setLineWidth (g, 1);
	Graphics_setGrey (g, 0.5);
	int fontSize = Graphics_inqFontSize (g);
	// draw the markers
	if (thee != NULL)
	{
		for (int i = 1; i <= thee->rows -> size; i++)
		{
			const wchar_t *label = Table_getStringValue_Assert (thee, i, 1);
			f1 = Table_getNumericValue_Assert (thee, i, 2);
			f2 = Table_getNumericValue_Assert (thee, i, 3);
			if (f1 >= _f1min && f1 <= _f1max && f2 >= _f2min && f2 <= _f2max)
			{
				getXYFromF1F2 (f1, f2, &x1, &y1);
				int size = Table_getNumericValue_Assert (thee, i, thee->numberOfColumns);
				Graphics_setFontSize (g, size);
				Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
				Graphics_text (g, x1, y1, label);
			}
		}
	}
	Graphics_setFontSize (g, fontSize);
	// Draw the line F1=F2
	//
	getXYFromF1F2 (_f2min, _f2min, &x1, &y1);
	if (y1 >= 0 && y1 <=1)
	{
		getXYFromF1F2 (_f1max, _f1max, &x2, &y2);
		if (x2 >= 0 && x2 <= 1)
		{
			Polygon p = Polygon_create (4);
			p -> x[1] = x1; p -> x[2] = x2;
			p -> y[1] = y1; p -> y[2] = y2;
			p -> x[3] =  1; p -> x[4] = x1;
			p -> y[3] =  0; p -> y[4] = y1;
			Graphics_fillArea (g, p -> numberOfPoints, & p -> x[1], & p -> y[1]);
			// Polygon_paint does not work because of use of Graphics_setInner.
			forget (p);
			Graphics_line (g, x1, y1, x2, y2);
		}
	}
	// Draw the grid
	if (_grid.df1 < (_f1max - _f1min)) // Horizontal lines
	{
		long iline = (_f1min + _grid.df1) / _grid.df1;
		Graphics_setGrey (g, 0.5);
		Graphics_setLineType (g, Graphics_DOTTED);
		while ((f1 = iline * _grid.df1) < _f1max)
		{
			if (f1 > _f1min)
			{
				getXYFromF1F2 (f1, _f2min, &x1, &y1);
				getXYFromF1F2 (f1, _f2max, &x2, &y2);
				Graphics_line (g, x1, y1, x2, y2);
			}
			iline++;
		}
		Graphics_setLineType (g, Graphics_DRAWN);
		Graphics_setGrey (g, 0); // black
	}
	if (_grid.df2 < (_f2max - _f2min))
	{
		long iline = (_f2min + _grid.df2) / _grid.df2;
		Graphics_setGrey (g, 0.5);
		Graphics_setLineType (g, Graphics_DOTTED);
		while ((f2 = iline * _grid.df2) < _f2max) // vert line
		{
			if (f2 > _f2min)
			{
				getXYFromF1F2 (_f1min, f2, &x1, &y1);
				getXYFromF1F2 (_f1max, f2, &x2, &y2);
				Graphics_line (g, x1, y1, x2, y2);
			}
			iline++;
		}
		Graphics_setLineType (g, Graphics_DRAWN);
		Graphics_setGrey (g, 0); // black
	}
	Graphics_unsetInner (g);
	Graphics_setGrey (g, 0); // black
	Graphics_markLeft (g, 0, 0, 1, 0, Melder_double (_f1max));
	Graphics_markLeft (g, 1, 0, 1, 0, Melder_double (_f1min));
	Graphics_markTop (g, 0, 0, 1, 0, Melder_double (_f2max));
	Graphics_markTop (g, 1, 0, 1, 0, Melder_double (_f2min));

}

typedef struct
{
	long some_check_value;
	long istart;
	float *z;
} *paVowelData;
/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/

static int paCallback (const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, I)
{
	iam (paVowelData);
	float *out = (float*) outputBuffer;
	unsigned int i;
	(void) inputBuffer; /* Prevent unused variable warning. */
	(void) timeInfo;
	(void) statusFlags;

	for (i = 0; i < framesPerBuffer; i++)
	{
		*out++ = my z[my istart+i];  /* left */
		*out++ = my z[my istart+i];  /* right */
	}
	my istart += framesPerBuffer;
	return 0;
}

/********** MENU METHODS **********/

int VowelEditor::menu_cb_help (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	Melder_help (L"VowelEditor");
	return 1;
}

int VowelEditor::menu_cb_prefs (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	EDITOR_FORM (L"Preferences", 0);
		POSITIVE (L"left F1 range (Hz)", L"200.0")
		POSITIVE (L"right F1 range (Hz)", L"1000.0")
		POSITIVE (L"left F2 range (Hz)", L"500.0")
		POSITIVE (L"right F2 range (Hz)", L"2500.0")
		BOOLEAN (L"Sound-follows-mouse", 1)
	EDITOR_OK
		SET_REAL (L"left F1 range", preferences.f1min)
		SET_REAL (L"right F1 range", preferences.f1max)
		SET_REAL (L"left F2 range", preferences.f2min)
		SET_REAL (L"right F2 range", preferences.f2max)
		SET_INTEGER (L"Sound-follows-mouse", preferences.soundFollowsMouse)
	EDITOR_DO
		editor->_frequencyScale = preferences.frequencyScale;
		editor->_axisOrientation = preferences.axisOrientation;
		editor->_f1min = preferences.f1min = GET_REAL (L"left F1 range");
		editor->_f1max = preferences.f1max = GET_REAL (L"right F1 range");
		editor->_f2min = preferences.f2min = GET_REAL (L"left F2 range");
		editor->_f2max = preferences.f2max = GET_REAL (L"right F2 range");
		editor->_soundFollowsMouse = preferences.soundFollowsMouse = GET_INTEGER (L"Sound-follows-mouse");
		Graphics_updateWs (editor->_g);
	EDITOR_END
}

int VowelEditor::menu_cb_publishSound (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	Sound publish = editor->createTarget ();
	if (publish == NULL) return 0;
	if (editor->_publishCallback)	editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

int VowelEditor::menu_cb_extract_FormantGrid (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	editor->updateVowel ();
	FormantGrid publish = FormantTier_to_FormantGrid (editor->_vowel -> ft);
	if (publish == NULL) return 0;
	if (editor->_publishCallback)	editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

int VowelEditor::menu_cb_extract_KlattGrid (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	editor->updateVowel ();
	FormantGrid fg = FormantTier_to_FormantGrid (editor->_vowel -> ft);
	if (fg == NULL) return 0;
	KlattGrid publish = KlattGrid_create (fg -> xmin, fg -> xmax, fg -> formants -> size, 1, 1, 1, 1, 6, 1);
	if (publish == NULL || ! KlattGrid_addVoicingAmplitudePoint (publish, fg -> xmin, 90) ||
		! KlattGrid_replacePitchTier (publish, editor->_vowel -> pt) ||
		! KlattGrid_replaceFormantGrid (publish, KlattGrid_ORAL_FORMANTS, fg))
	{
		forget (publish); forget (fg); return 0;
	}
	forget (fg);
	if (editor->_publishCallback) editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

int VowelEditor::menu_cb_extract_PitchTier (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	editor->updateVowel ();
	PitchTier publish = (structPitchTier *)Data_copy (editor->_vowel -> pt);
	if (publish == NULL) return 0;
	if (editor->_publishCallback)	editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

int VowelEditor::menu_cb_drawTrajectory (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	EDITOR_FORM (L"Draw trajectory", 0)
		editor->form_pictureWindow (cmd);
		BOOLEAN (L"Garnish", 1)
	EDITOR_OK
		editor->ok_pictureWindow (cmd);
	EDITOR_DO
		int garnish = GET_INTEGER (L"Garnish");
		editor->do_pictureWindow (cmd);
		editor->openPraatPicture ();
		if (garnish) editor->drawBackground (editor->_pictureGraphics);
		FormantTier_drawF1F2Trajectory (editor->_vowel -> ft, editor->_pictureGraphics, editor->_f1min, editor->_f1max, editor->_f2min, editor->_f2max, editor->_markTraceEvery, editor->_width);
		editor->closePraatPicture ();
	EDITOR_END
}

int VowelEditor::menu_cb_showOneVowelMark (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	EDITOR_FORM (L"Show one vowel mark", 0);
		POSITIVE (L"F1 (Hz)", L"300.0")
		POSITIVE (L"F2 (Hz)", L"600.0")
		WORD (L"Mark", L"u")
	EDITOR_OK
	EDITOR_DO
		double f1 = GET_REAL (L"F1");
		double f2 = GET_REAL (L"F2");
		wchar_t *label = GET_STRING (L"Mark");
		if (f1 >= editor->_f1min && f1 <= editor->_f1max && f2 >= editor->_f2min && f2 <= editor->_f2max)
		{
			long irow = 1;
			if (editor->_marks == NULL)
			{
				editor->_marks = Table_createWithColumnNames (1, L"IPA F1 F2 Colour");
				if (editor->_marks == NULL) return 0;
			}
			else
			{
				if (! Table_appendRow (editor->_marks)) return 0;
			}
			irow = editor->_marks -> rows -> size;
			Table_setStringValue (editor->_marks, irow, 1, label);
			Table_setNumericValue (editor->_marks, irow, 2, f1);
			Table_setNumericValue (editor->_marks, irow, 3, f2);
			Graphics_updateWs (editor->_g);
		}
	EDITOR_END
}

int VowelEditor::menu_cb_showVowelMarks (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	EDITOR_FORM (L"Show vowel marks", 0);
		OPTIONMENU (L"Data set:", 1)
		OPTION (L"American English")
		OPTION (L"Dutch")
		OPTION (L"None")
		OPTIONMENU (L"Speaker:", 1)
		OPTION (L"Man")
		OPTION (L"Woman")
		OPTION (L"Child")
		NATURAL (L"Font size (points)", L"14")
	EDITOR_OK
	EDITOR_DO
		if (! editor->setMarks (GET_INTEGER (L"Data set"), GET_INTEGER (L"Speaker"), GET_INTEGER (L"Font size"))) return 0;
		Graphics_updateWs (editor->_g);
	EDITOR_END
}

int VowelEditor::menu_cb_setF0 (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	EDITOR_FORM (L"Set F0", 0);
		POSITIVE (L"Start F0 (Hz)", L"150.0")
		REAL (L"Slope (oct/s)", L"0.0")
	EDITOR_OK
	EDITOR_DO
		double f0 = GET_REAL (L"Start F0");
		checkF0 (&editor->_f0, &f0);
		editor->_f0.start = f0;
		editor->_f0.slopeOctPerSec = GET_REAL (L"Slope");
		if (! editor->setSource ()) return 0;
		GuiText_setString (editor->_f0TextField, Melder_double (editor->_f0.start));
		GuiText_setString (editor->_f0SlopeTextField, Melder_double (editor->_f0.slopeOctPerSec));
	EDITOR_END
}

int VowelEditor::menu_cb_setF3F4 (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	EDITOR_FORM (L"Set F3 & F4", 0);
		POSITIVE (L"F3 (Hz)", L"2500.0")
		POSITIVE (L"B3 (Hz)", L"250.0")
		POSITIVE (L"F4 (Hz)", L"3500.0")
		POSITIVE (L"B4 (Hz)", L"350.0")
	EDITOR_OK
	EDITOR_DO
		double f3 = GET_REAL (L"F3"), b3 = GET_REAL (L"B3");
		double f4 = GET_REAL (L"F4"), b4 = GET_REAL (L"B4");
		if (f3 >= f4 ) return Melder_error1 (L"F4 must be larger than F3.");
		if (! editor->setF3F4 (f3, b3, f4, b4)) return 0;
	EDITOR_END
}
int VowelEditor::menu_cb_reverseTrajectory (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	editor->Vowel_reverseFormantTier ();

	Graphics_updateWs (editor->_g);
	return 1;
}

int VowelEditor::Vowel_addData (Vowel thee, double time, double f1, double f2, double f0)
{
	FormantPoint fp = FormantPoint_create (time);
	double f3, b3, f4, b4;

	if (fp == NULL) return 0;
	fp -> formant[0] = f1;
	fp -> bandwidth[0] = f1 / 10;
	fp -> formant[1] = f2;
	fp -> bandwidth[1] = f2 / 10;
	getF3F4 (f1, f2, &f3, &b3, &f4, &b4);
	fp -> formant[2] = f3;
	fp -> bandwidth[2] = b3;
	fp -> formant[3] = f4;
	fp -> bandwidth[3] = b4;
	fp -> numberOfFormants = 4;

	if (! Collection_addItem (thee->ft -> points, fp) ||
		! RealTier_addPoint (thee->pt, time, f0)) return 0;
	return 1;
}

void VowelEditor::checkF1F2 (double *f1, double *f2)
{
	if (*f1 < _f1min) *f1 = _f1min;
	if (*f1 > _f1max) *f1 = _f1max;
	if (*f2 < _f2min) *f2 = _f2min;
	if (*f2 > _f2max) *f1 = _f2max;
}

static void checkF0 (struct structF0 *f0p, double *f0)
{
	if (*f0 == NUMundefined)  *f0 = f0p -> start;
	if (*f0 > f0p -> maximum) *f0 = f0p -> maximum;
	if (*f0 < f0p -> minimum) *f0 = f0p -> minimum;
}

static void checkXY (double *x, double *y)
{
	if (*x < 0) *x = 0;
	else if (*x > 1) *x = 1;
	if (*y < 0) *y = 0;
	else if (*y > 1) *y = 1;
}

int VowelEditor::menu_cb_newTrajectory (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	EDITOR_FORM (L"New Trajectory", 0);
		POSITIVE (L"Start F1 (Hz)", L"700.0")
		POSITIVE (L"Start F2 (Hz)", L"1200.0")
		POSITIVE (L"End F1 (Hz)", L"350.0")
		POSITIVE (L"End F2 (Hz)", L"800.0")
		POSITIVE (L"Duration (s)", L"0.25")
	EDITOR_OK
	EDITOR_DO
		double f0, f1, f2, time, duration = GET_REAL (L"Duration");
		Vowel vowel = Vowel_create (duration);
		if (vowel == NULL) goto end;

		time = 0;
		f0 =  getF0 (&editor->_f0, time);
		f1 = GET_REAL (L"Start F1");
		f2 = GET_REAL (L"Start F2");
		editor->checkF1F2 (&f1, &f2);
		if (! editor->Vowel_addData (vowel, time, f1, f2, f0)) goto end;
		time = duration;
		f0 =  getF0 (&editor->_f0, time);
		f1 = GET_REAL (L"End F1");
		f2 = GET_REAL (L"End F2");
		editor->checkF1F2 (&f1, &f2);
		if (! editor->Vowel_addData (vowel, time, f1, f2, f0)) goto end;

		GuiText_setString (editor->_durationTextField, Melder_double (MICROSECPRECISION(duration)));
end:
		if (Melder_hasError ())
		{
			forget (vowel);
			return 0;
		}

		forget (editor->_vowel);
		editor->_vowel = vowel;

		Graphics_updateWs (editor->_g);
	EDITOR_END
}

int VowelEditor::menu_cb_extendTrajectory (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	EDITOR_FORM (L"Extend Trajectory", 0);
		POSITIVE (L"To F1 (Hz)", L"500.0")
		POSITIVE (L"To F2 (Hz)", L"1500.0")
		POSITIVE (L"Extra duration (s)", L"0.1")
	EDITOR_OK
	EDITOR_DO
		Vowel thee = editor->_vowel;
		double newDuration = thee->xmax + GET_REAL (L"Extra duration");
		double f0 =  getF0 (&editor->_f0, newDuration);
		double f1 = GET_REAL (L"To F1");
		double f2 = GET_REAL (L"To F2");
		thee->xmax = thee->pt -> xmax = thee->ft -> xmax = newDuration;
		editor->checkF1F2 (&f1, &f2);
		if (! editor->Vowel_addData (thee, newDuration, f1, f2, f0)) return 0;

		GuiText_setString (editor->_durationTextField, Melder_double (MICROSECPRECISION(newDuration)));
		Graphics_updateWs (editor->_g);
	EDITOR_END
}

int VowelEditor::menu_cb_modifyTrajectoryDuration (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	EDITOR_FORM (L"Modify duration", 0);
		POSITIVE (L"New duration (s)", L"0.5")
	EDITOR_OK
	EDITOR_DO
		GuiText_setString (editor->_durationTextField, Melder_double (MICROSECPRECISION(GET_REAL (L"New duration"))));
	EDITOR_END
}

int VowelEditor::menu_cb_shiftTrajectory (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	EDITOR_FORM (L"Shift trajectory", 0);
		REAL (L"F1 (semitones)", L"0.5")
		REAL (L"F2 (semitones)", L"0.5")
	EDITOR_OK
	EDITOR_DO
		editor->shiftF1F2 (GET_REAL (L"F1"), GET_REAL (L"F2"));
		Graphics_updateWs (editor->_g);
	EDITOR_END
}

int VowelEditor::menu_cb_showTrajectoryTimeMarkersEvery (EDITOR_ARGS)
{
	VowelEditor *editor = (VowelEditor *)editor_me;
	EDITOR_FORM (L"Show trajectory time markers every", 0);
		REAL (L"Distance (s)", L"0.05")
	EDITOR_OK
	EDITOR_DO
		editor->_markTraceEvery = GET_REAL (L"Distance");
		if (editor->_markTraceEvery < 0) editor->_markTraceEvery = 0;
		Graphics_updateWs (editor->_g);
	EDITOR_END
}

/********** BUTTON METHODS **********/

void VowelEditor::gui_button_cb_play (I, GuiButtonEvent event)
{
	(void) event;
	VowelEditor *editor = (VowelEditor *)void_me;
	Sound thee = editor->createTarget ();
	Sound_play (thee, NULL, NULL);
	Graphics_updateWs (editor->_g);
	forget (thee);
}

void VowelEditor::gui_button_cb_publish (I, GuiButtonEvent event)
{
	(void) event;
	VowelEditor *editor = (VowelEditor *)void_me;
	Sound publish = editor->createTarget ();
	if (publish == NULL) return;
	if (editor->_publishCallback) editor->_publishCallback (editor, editor->_publishClosure, publish);
}

void VowelEditor::gui_button_cb_reverse (I, GuiButtonEvent event)
{
	(void) event;
	VowelEditor *editor = (VowelEditor *)void_me;

	editor->Vowel_reverseFormantTier ();
	struct structGuiButtonEvent play_event = { 0 };
	play_event.button = editor->_playButton;
	gui_button_cb_play (editor, &play_event);
}

/* Main drawing routine: it's been called after every call to Graphics_updateWs (g) */
void VowelEditor::gui_drawingarea_cb_expose (I, GuiDrawingAreaExposeEvent event)
{
	VowelEditor *editor = (VowelEditor *)void_me;
	(void) event;
	double ts = editor->_vowel -> xmin, te = editor->_vowel -> xmax;
	FormantTier ft = editor->_vowel -> ft;
	static MelderString statusInfo = { 0 };
	if (editor->_g == NULL) return;   // Could be the case in the very beginning.
	Graphics_clearWs (editor->_g);

	appendF1F2F0 (&statusInfo, STATUSINFO_STARTINTR0, FormantTier_getValueAtTime (ft, 1, ts),
		FormantTier_getValueAtTime (ft, 2, ts), getF0 (&editor->_f0, ts), STATUSINFO_ENDING);
	GuiLabel_setString (editor->_startInfo, statusInfo.string);
	MelderString_empty (&statusInfo);

	appendF1F2F0 (&statusInfo, STATUSINFO_ENDINTR0, FormantTier_getValueAtTime (ft, 1, te),
		FormantTier_getValueAtTime (ft, 2, te), getF0 (&editor->_f0, te), STATUSINFO_ENDING);
	GuiLabel_setString (editor->_endInfo, statusInfo.string);
	MelderString_empty (&statusInfo);

	Graphics_setGrey (editor->_g, 0.9);
	Graphics_fillRectangle (editor->_g, 0, 1, 0, 1);
	Graphics_setInner (editor->_g);
	Graphics_setWindow (editor->_g, 0, 1, 0, 1);
	Graphics_setGrey (editor->_g, 1);
	Graphics_fillRectangle (editor->_g, 0, 1, 0, 1);
	Graphics_unsetInner (editor->_g);
	Graphics_setGrey (editor->_g, 0);

	editor->drawBackground (editor->_g);
	FormantTier_drawF1F2Trajectory (editor->_vowel -> ft, editor->_g, editor->_f1min, editor->_f1max, editor->_f2min, editor->_f2max, editor->_markTraceEvery, editor->_width);
}

void VowelEditor::gui_drawingarea_cb_resize (I, GuiDrawingAreaResizeEvent event)
{
	VowelEditor *editor = (VowelEditor *)void_me;
	(void) event;
	if (editor == NULL || editor->_g == NULL) return;
	editor->_height = GuiObject_getHeight (editor->_drawingArea);
	editor->_width = GuiObject_getWidth (editor->_drawingArea);
	Graphics_setWsViewport (editor->_g, 0, editor->_width , 0, editor->_height);
	Graphics_setWsWindow (editor->_g, 0, editor->_width, 0, editor->_height);
	Graphics_setViewport (editor->_g, 0, editor->_width, 0, editor->_height);
	Graphics_updateWs (editor->_g);
}

int VowelEditor::Vowel_updateTiers (Vowel thee, double time, double x, double y)
{
	double f3, b3, f4, b4;
	if (time > thee->xmax)
	{
		thee->xmax = time;
		thee->ft -> xmax = time;
		thee->pt -> xmax = time;
	}
	double f0 = getF0 (& _f0, time), f1, f2;
	FormantPoint point = FormantPoint_create (time);

	if (point == NULL) return 0;

	getF1F2FromXY (x, y, &f1, &f2);
	getF3F4 (f1, f2, &f3, &b3, &f4, &b4);

	point -> formant[0] = f1;
	point -> bandwidth[0] = f1 / 10;
	point -> formant[1] = f2;
	point -> bandwidth[1] = f2 / 10;
	point -> formant[2] = f3;
	point -> bandwidth[2] = b3;
	point -> formant[3] = f4;
	point -> bandwidth[3] = b4;
	point -> numberOfFormants = 4;
	return Collection_addItem (thee->ft -> points, point) && RealTier_addPoint (thee->pt, time, f0);
}

// shift key always extends what already is.
// Special case : !soundFollowsMouse. The first click just defines the vowel's first f1f2-position,
void VowelEditor::gui_drawingarea_cb_click (I, GuiDrawingAreaClickEvent event)
{
	VowelEditor *editor = (VowelEditor *)void_me;
	(void) event;
	Vowel thee;
	double x, y, xb, yb, tb, t, dt = 0;
	double t0 = Melder_clock ();
	long iskipped = 0;
	struct structGuiButtonEvent gb_event = { 0 };
	Graphics_setInner (editor->_g);

	Graphics_getMouseLocation (editor->_g, & x, & y);
	checkXY (&x, &y);

	if (event->shiftKeyPressed)
	{
		editor->updateExtendDuration ();
		(editor->_shiftKeyPressed)++;
		thee = editor->_vowel;
		dt = thee->xmax + editor->_extendDuration;
		t = 0 + dt;
		editor->Vowel_updateTiers (thee, t, x, y);
		GuiText_setString (editor->_durationTextField, Melder_double (t));
		if (! editor->_soundFollowsMouse) goto end;
	}
	else
	{
		t = 0;
		editor->_shiftKeyPressed = 0;
		thee = Vowel_create (MINIMUM_SOUND_DURATION);
		editor->Vowel_updateTiers (thee, t, x, y);
		GuiText_setString (editor->_durationTextField, Melder_double (t));
		if (! editor->_soundFollowsMouse)
		{
			editor->Vowel_updateTiers (thee, MINIMUM_SOUND_DURATION, x, y);
			goto end;
		}
	}

	Graphics_xorOn (editor->_g, Graphics_BLUE);
	while (Graphics_mouseStillDown (editor->_g))
	{
		xb = x, yb = y, tb = t;
		t = Melder_clock () - t0 + dt; // Get relative time in seconds from the clock
		Graphics_getMouseLocation (editor->_g, & x, & y);
		checkXY (&x, &y);
		// If the new point equals the previous one: no tier update
		if (xb == x && yb == y)
		{
			iskipped++;
			continue;
		}
		// Add previous point only if at least one previous event was skipped...
		if (iskipped > 0) editor->Vowel_updateTiers (thee, tb, xb, yb);
		iskipped = 0;
		Graphics_line (editor->_g, xb, yb, x, y);

		editor->Vowel_updateTiers (thee, t, x, y);
		GuiText_setString (editor->_durationTextField, Melder_double (MICROSECPRECISION(t)));
	}
	t = Melder_clock () - t0;
	// To prevent ultra short clicks we set a minimum of 0.01 s duration
	if (t < MINIMUM_SOUND_DURATION) t = MINIMUM_SOUND_DURATION;
	t += dt;
	GuiText_setString (editor->_durationTextField, Melder_double (MICROSECPRECISION(t)));
	editor->Vowel_updateTiers (thee, t, x, y);

	Graphics_xorOff (editor->_g);

end:
	Graphics_unsetInner (editor->_g);

	if (editor->_shiftKeyPressed == 0)
	{
		forget (editor->_vowel);
		editor->_vowel = thee;
	}
	gb_event.button = editor->_drawingArea;
	gui_button_cb_play (editor, & gb_event);
}

void VowelEditor::gui_drawingarea_cb_key (I, GuiDrawingAreaKeyEvent event) {}

void VowelEditor::updateWidgets () {}

void VowelEditor::createMenus ()
{
	EditorMenu *menu = getMenu (L"File");
	menu->addCommand (L"Preferences...", 0, menu_cb_prefs);
	menu->addCommand (L"-- publish data --", 0, NULL);
	menu->addCommand (L"Publish Sound", 0, menu_cb_publishSound);
	menu->addCommand (L"Extract KlattGrid", 0, menu_cb_extract_KlattGrid);
	menu->addCommand (L"Extract FormantGrid", 0, menu_cb_extract_FormantGrid);
	menu->addCommand (L"Extract PitchTsier", 0, menu_cb_extract_PitchTier);
	menu->addCommand (L"-- script stuff --", 0, NULL);
	menu->addCommand (L"Draw trajectory...", 0, menu_cb_drawTrajectory);

	menu = getMenu (L"Edit");
	menu->addCommand (L"Show one vowel mark...", 0, menu_cb_showOneVowelMark);
	menu->addCommand (L"Show vowel marks...", 0, menu_cb_showVowelMarks);
	menu->addCommand (L"-- f0 --", 0, NULL);
	menu->addCommand (L"Set F0...", 0, menu_cb_setF0);
	menu->addCommand (L"Set F3 & F4...", 0, menu_cb_setF3F4);
	menu->addCommand (L"-- trajectory commands --", 0, NULL);
	menu->addCommand (L"Reverse trajectory", 0, menu_cb_reverseTrajectory);
	menu->addCommand (L"Modify trajectory duration...", 0, menu_cb_modifyTrajectoryDuration);
	menu->addCommand (L"New trajectory...", 0, menu_cb_newTrajectory);
	menu->addCommand (L"Extend trajectory...", 0, menu_cb_extendTrajectory);
	menu->addCommand (L"Shift trajectory...", 0, menu_cb_shiftTrajectory);
	menu->addCommand (L"Show trajectory time markers every...", 0, menu_cb_showTrajectoryTimeMarkersEvery);

	menu = getMenu (L"Help");
	menu->addCommand (L"VowelEditor help", '?', menu_cb_help);
}

void VowelEditor::createChildren ()
{
	GuiObject form;

	// Origin is top left!

	guint nrows = 25, ncols = 7, ileft, iright = 0, itop, ibottom;
	form = _dialog;
	GuiObject table = gtk_table_new (nrows, ncols, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 4);
	gtk_container_set_border_width(GTK_CONTAINER(table), 4);
	gtk_container_add (GTK_CONTAINER(form), table);

	itop = nrows - 3; ibottom = itop + 2;

	ileft = iright; iright = ileft + 1;
	_playButton = GuiButton_create (NULL, 0, 0, 0, 0, L"Play", gui_button_cb_play, this, 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _playButton, ileft, iright, itop, ibottom);

	ileft = iright; iright = ileft + 1;
	_reverseButton = GuiButton_create (NULL, 0, 0, 0, 0, L"Reverse", gui_button_cb_reverse, this, 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _reverseButton, ileft, iright, itop, ibottom);

	ileft = iright; iright = ileft + 1;
	_publishButton = GuiButton_create (NULL, 0, 0, 0, 0, L"Publish", gui_button_cb_publish, this, 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _publishButton, ileft, iright, itop, ibottom);

	ileft = iright; iright = ileft + 1;
	_durationLabel = GuiLabel_create (NULL, 0, 0, 0, 0, L"Duration (s):", 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _durationLabel, ileft, iright, itop, itop + 1);
	_durationTextField = GuiText_create (NULL, 0, 0, 0, 0, 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _durationTextField, ileft, iright, itop + 1, ibottom);

	ileft = iright; iright = ileft + 1;
	_extendLabel = GuiLabel_create (NULL, 0, 0, 0, 0, L"Extend (s):", 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _extendLabel, ileft, iright, itop, itop + 1);
	_extendTextField = GuiText_create (NULL, 0, 0, 0, 0, 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _extendTextField, ileft, iright, itop + 1, ibottom);

	ileft = iright; iright = ileft + 1;
	_f0Label = GuiLabel_create (NULL, 0, 0, 0, 0, L"Start F0 (Hz):", 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _f0Label, ileft, iright, itop, itop + 1);
	_f0TextField = GuiText_create (NULL, 0, 0, 0, 0, 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _f0TextField, ileft, iright, itop + 1, ibottom);

	ileft = iright; iright = ileft + 1;
	_f0SlopeLabel = GuiLabel_create (NULL, 0, 0, 0, 0, L"F0 slope (oct/s):", 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _f0SlopeLabel, ileft, iright, itop, itop + 1);
	_f0SlopeTextField = GuiText_create (NULL, 0, 0, 0, 0, 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _f0SlopeTextField, ileft, iright, itop + 1, ibottom);

	itop = nrows - 1; ibottom = itop + 1;
	ileft = 0; iright = ileft + 3;
	_startInfo = GuiLabel_create (NULL, 0, 0, 0, 0, L"", 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _startInfo, ileft, iright, itop, ibottom);

	ileft = iright + 1; iright = ileft + 3;
	_endInfo = GuiLabel_create (NULL, 0, 0, 0, 0, L"", 0);
	gtk_table_attach_defaults (GTK_TABLE (table), _endInfo, ileft, iright, itop, ibottom);

	_drawingArea = GuiDrawingArea_create (NULL, 0, 0, 0, 0, gui_drawingarea_cb_expose, gui_drawingarea_cb_click, gui_drawingarea_cb_key, gui_drawingarea_cb_resize, this, 0);

	gtk_widget_set_double_buffered (_drawingArea, FALSE);
	gtk_table_attach_defaults (GTK_TABLE (table), _drawingArea, 0, ncols, 0, nrows - 3);

	gtk_widget_show_all (form);
}

void VowelEditor::dataChanged () {}

int VowelEditor::setSource ()
{
	PitchTier pt = NULL; Sound thee = NULL;

	pt = to_PitchTier (_maximumDuration);
	if (pt == NULL) return 0;
	thee = PitchTier_to_Sound_pulseTrain (pt, _f0.samplingFrequency, _f0.adaptFactor, _f0.adaptTime, _f0.interpolationDepth, 0);
	if (thee == NULL) goto end;

	if (_source != NULL) forget (_source);
	_source = thee;

end:
	forget (pt);
	return ! Melder_hasError ();
}
//
Sound VowelEditor::createTarget ()
{
	Sound thee = NULL;
	updateVowel (); // update pitch and duration
	thee = Vowel_to_Sound_pulses (_vowel, 44100, 0.7, 0.05, 30);
	if (thee == NULL) return NULL;
	Vector_scale (thee, 0.99);
	Sound_fadeIn (thee, 0.005, 1);
	Sound_fadeOut(thee, 0.005);
	return thee;
}

/* End of file VowelEditor.c */
