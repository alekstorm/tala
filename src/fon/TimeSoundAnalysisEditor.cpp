/* TimeSoundAnalysisEditor.cpp
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
 * pb 2002/07/16 GPL
 * pb 2002/10/11 added a screen text for unavailable pitch
 * pb 2002/11/19 default log file names include "~" on Unix and Mach
 * pb 2002/11/19 added pulses and five separate analysis menus
 * pb 2003/02/19 clearer wording
 * pb 2003/02/24 spectral slices
 * pb 2003/03/03 Extract visible spectrogram: higher degree of oversampling than in editor
 * pb 2003/03/10 undid previous change because our PostScript code now does image interpolation
 * pb 2003/03/12 queriable
 * pb 2003/04/03 Get power at cursor cross
 * pb 2003/04/11 smaller log settings dialog
 * pb 2003/05/18 more shimmer measurements
 * pb 2003/05/20 longestAnalysis replaces the pitch/formant/intensity time steps,
 *               pitch.speckle, and formant.maximumDuration
 * pb 2003/05/21 pitch floor and ceiling replace the separate ranges for viewing and analysis
 * pb 2003/05/27 spectrogram maximum and autoscaling
 * pb 2003/07/20 moved voice report to VoiceAnalysis.c
 * pb 2003/08/23 reintroduced formant.numberOfTimeSteps
 * pb 2003/09/11 make sure that analyses objects can be extracted even before first drawing
 * pb 2003/09/16 advanced pitch settings: pitch.timeStep, pitch.viewFrom, pitch.viewTo, pitch.timeStepsPerView
 * pb 2003/10/01 time step settings: timeStepStrategy, fixedTimeStep, numberOfTimeStepsPerView
 * pb 2003/11/30 Sound_to_Spectrogram_windowShapeText
 * pb 2003/12/09 moved Spectrogram settings back in sync with preferences
 * pb 2004/02/15 highlight methods
 * pb 2004/04/16 introduced pulses.maximumPeriodFactor
 * pb 2004/05/12 extended voice report with mean F0 and harmonic-to-noise ratios
 * pb 2004/07/14 introduced pulses.maximumAmplitudeFactor
 * pb 2004/10/16 C++ compatible struct tags
 * pb 2004/10/24 intensity.averagingMethod
 * pb 2004/10/27 intensity.subtractMeanPressure
 * pb 2004/11/22 simplified Sound_to_Spectrum ()
 * pb 2004/11/28 improved screen text for unavailable pitch
 * pb 2004/11/28 warning in settings dialogs for non-standard time step strategies
 * pb 2005/01/11 getBottomOfSoundAndAnalysisArea
 * pb 2005/03/02 all pref string buffers are 260 bytes long
 * pb 2005/03/07 'intensity' logging sensitive to averaging method
 * pb 2005/06/16 units
 * pb 2005/08/18 editor name in log files
 * pb 2005/09/23 interface update
 * pb 2005/12/07 scrollStep
 * pb 2006/02/27 more helpful text when analyses are not shown
 * pb 2006/09/12 better messages if analysis not available
 * pb 2006/10/28 erased MacOS 9 stuff
 * pb 2006/12/10 MelderInfo
 * pb 2006/12/18 better info
 * pb 2007/01/27 changed Vector API
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/09/01 computeXXX procedures are global on behalf of the inheritors
 * pb 2007/09/02 Picture window drawing
 * pb 2007/09/08 inherit from TimeSoundEditor
 * pb 2007/09/19 info
 * pb 2007/09/21 split Query menu
 * pb 2007/11/01 direct intensity, formants, and pulses drawing
 * pb 2007/11/30 erased Graphics_printf
 * pb 2009/11/30 Move frequency cursor to...
 * pb 2010/01/15 corrected checking of "Show pitch" menu item (and so on) when the command is called from a script
 * pb 2011/03/23 C++
 */

#include <time.h>
#include "TimeSoundAnalysisEditor.h"
#include "sys/Preferences.h"
#include "sys/EditorM.h"
#include "Sound_and_Spectrogram.h"
#include "Sound_and_Spectrum.h"
#include "Sound_to_Pitch.h"
#include "Sound_to_Intensity.h"
#include "Sound_to_Formant.h"
#include "Pitch_to_PointProcess.h"
#include "VoiceAnalysis.h"
#include "sys/praat_script.h"

#include "sys/enums_getText.h"
#include "TimeSoundAnalysisEditor_enums.h"
#include "sys/enums_getValue.h"
#include "TimeSoundAnalysisEditor_enums.h"

static const wchar_t * theMessage_Cannot_compute_spectrogram = L"The spectrogram is not defined at the edge of the sound.";
static const wchar_t * theMessage_Cannot_compute_pitch = L"The pitch contour is not defined at the edge of the sound.";
static const wchar_t * theMessage_Cannot_compute_formant = L"The formants are not defined at the edge of the sound.";
static const wchar_t * theMessage_Cannot_compute_intensity = L"The intensity curve is not defined at the edge of the sound.";
static const wchar_t * theMessage_Cannot_compute_pulses = L"The pulses are not defined at the edge of the sound.";

#if defined (macintosh)
	static const wchar_t * LOG_1_FILE_NAME = L"~/Desktop/Pitch Log";
	static const wchar_t * LOG_2_FILE_NAME = L"~/Desktop/Formant Log";
	static const wchar_t * LOG_3_FILE_NAME = L"~/Desktop/Log script 3";
	static const wchar_t * LOG_4_FILE_NAME = L"~/Desktop/Log script 4";
#elif defined (WIN32)
	static const wchar_t * LOG_1_FILE_NAME = L"C:\\WINDOWS\\DESKTOP\\Pitch Log.txt";
	static const wchar_t * LOG_2_FILE_NAME = L"C:\\WINDOWS\\DESKTOP\\Formant Log.txt";
	static const wchar_t * LOG_3_FILE_NAME = L"C:\\WINDOWS\\DESKTOP\\Log script 3.praat";
	static const wchar_t * LOG_4_FILE_NAME = L"C:\\WINDOWS\\DESKTOP\\Log script 4.praat";
#else
	static const wchar_t * LOG_1_FILE_NAME = L"~/pitch_log";
	static const wchar_t * LOG_2_FILE_NAME = L"~/formant_log";
	static const wchar_t * LOG_3_FILE_NAME = L"~/log_script3";
	static const wchar_t * LOG_4_FILE_NAME = L"~/log_script4";
#endif
static const wchar_t * LOG_1_FORMAT = L"Time 'time:6' seconds, pitch 'f0:2' Hertz";
static const wchar_t * LOG_2_FORMAT = L"'t1:4''tab$''t2:4''tab$''f1:0''tab$''f2:0''tab$''f3:0'";

struct logInfo {
	bool toInfoWindow, toLogFile;
	wchar_t fileName [Preferences_STRING_BUFFER_SIZE], format [Preferences_STRING_BUFFER_SIZE];
};

static struct { struct {
	struct { bool garnish; } spectrogram;
	struct { bool garnish; } pitch;
	struct { bool garnish; } intensity;
	struct { bool garnish; } formant;
	struct { bool garnish; } pulses;
} picture; } prefs = { { true, true, true, true, true } };
/*prefs.picture.spectrogram.garnish = true;
prefs.picture.pitch.garnish = true;
prefs.picture.intensity.garnish = true;
prefs.picture.formant.garnish = true;
prefs.picture.pulses.garnish = true;*/

static struct {
	double longestAnalysis;
	enum kTimeSoundAnalysisEditor_timeStepStrategy timeStepStrategy;
	double fixedTimeStep;
	long numberOfTimeStepsPerView;
	struct FunctionEditor_spectrogram spectrogram;
	struct FunctionEditor_pitch pitch;
	struct FunctionEditor_intensity intensity;
	struct FunctionEditor_formant formant;
	struct FunctionEditor_pulses pulses;
	struct logInfo log [2];
	wchar_t logScript3 [Preferences_STRING_BUFFER_SIZE], logScript4 [Preferences_STRING_BUFFER_SIZE];
} preferences;

void TimeSoundAnalysisEditor::prefs (void) {
	Preferences_addDouble (L"FunctionEditor.longestAnalysis", & preferences.longestAnalysis, 10.0);   // seconds
	Preferences_addEnum (L"FunctionEditor.timeStepStrategy", & preferences.timeStepStrategy, kTimeSoundAnalysisEditor_timeStepStrategy, DEFAULT);
	Preferences_addDouble (L"FunctionEditor.fixedTimeStep", & preferences.fixedTimeStep, 0.01);   // seconds
	Preferences_addLong (L"FunctionEditor.numberOfTimeStepsPerView", & preferences.numberOfTimeStepsPerView, 100);
	Preferences_addBool (L"FunctionEditor.spectrogram.show", & preferences.spectrogram.show, true);
	Preferences_addDouble (L"FunctionEditor.spectrogram.viewFrom2", & preferences.spectrogram.viewFrom, 0.0);   // Hertz
	Preferences_addDouble (L"FunctionEditor.spectrogram.viewTo2", & preferences.spectrogram.viewTo, 5000.0);   // Hertz
	Preferences_addDouble (L"FunctionEditor.spectrogram.windowLength2", & preferences.spectrogram.windowLength, 0.005);   // Hertz
	Preferences_addDouble (L"FunctionEditor.spectrogram.dynamicRange2", & preferences.spectrogram.dynamicRange, 70.0);   // dB
	Preferences_addLong (L"FunctionEditor.spectrogram.timeSteps2", & preferences.spectrogram.timeSteps, 1000);
	Preferences_addLong (L"FunctionEditor.spectrogram.frequencySteps2", & preferences.spectrogram.frequencySteps, 250);
	Preferences_addEnum (L"FunctionEditor.spectrogram.method2", & preferences.spectrogram.method, kSound_to_Spectrogram_method, DEFAULT);
	Preferences_addEnum (L"FunctionEditor.spectrogram.windowShape2", & preferences.spectrogram.windowShape, kSound_to_Spectrogram_windowShape, DEFAULT);
	Preferences_addBool (L"FunctionEditor.spectrogram.autoscaling2", & preferences.spectrogram.autoscaling, true);
	Preferences_addDouble (L"FunctionEditor.spectrogram.maximum2", & preferences.spectrogram.maximum, 100.0);   // dB/Hz
	Preferences_addDouble (L"FunctionEditor.spectrogram.preemphasis2", & preferences.spectrogram.preemphasis, 6.0);   // dB/octave
	Preferences_addDouble (L"FunctionEditor.spectrogram.dynamicCompression2", & preferences.spectrogram.dynamicCompression, 0.0);
	Preferences_addBool (L"FunctionEditor.pitch.show", & preferences.pitch.show, true);
	Preferences_addDouble (L"FunctionEditor.pitch.floor", & preferences.pitch.floor, 75.0);
	Preferences_addDouble (L"FunctionEditor.pitch.ceiling", & preferences.pitch.ceiling, 500.0);
	Preferences_addEnum (L"FunctionEditor.pitch.unit", & preferences.pitch.unit, kPitch_unit, DEFAULT);
	Preferences_addEnum (L"FunctionEditor.pitch.drawingMethod", & preferences.pitch.drawingMethod, kTimeSoundAnalysisEditor_pitch_drawingMethod, DEFAULT);
	Preferences_addDouble (L"FunctionEditor.pitch.viewFrom", & preferences.pitch.viewFrom, 0.0);   // auto
	Preferences_addDouble (L"FunctionEditor.pitch.viewTo", & preferences.pitch.viewTo, 0.0);   // auto
	Preferences_addEnum (L"FunctionEditor.pitch.method", & preferences.pitch.method, kTimeSoundAnalysisEditor_pitch_analysisMethod, DEFAULT);
	Preferences_addBool (L"FunctionEditor.pitch.veryAccurate", & preferences.pitch.veryAccurate, false);
	Preferences_addLong (L"FunctionEditor.pitch.maximumNumberOfCandidates", & preferences.pitch.maximumNumberOfCandidates, 15);
	Preferences_addDouble (L"FunctionEditor.pitch.silenceThreshold", & preferences.pitch.silenceThreshold, 0.03);
	Preferences_addDouble (L"FunctionEditor.pitch.voicingThreshold", & preferences.pitch.voicingThreshold, 0.45);
	Preferences_addDouble (L"FunctionEditor.pitch.octaveCost", & preferences.pitch.octaveCost, 0.01);
	Preferences_addDouble (L"FunctionEditor.pitch.octaveJumpCost", & preferences.pitch.octaveJumpCost, 0.35);
	Preferences_addDouble (L"FunctionEditor.pitch.voicedUnvoicedCost", & preferences.pitch.voicedUnvoicedCost, 0.14);
	Preferences_addBool (L"FunctionEditor.intensity.show", & preferences.intensity.show, false);
	Preferences_addDouble (L"FunctionEditor.intensity.viewFrom", & preferences.intensity.viewFrom, 50.0);   // dB
	Preferences_addDouble (L"FunctionEditor.intensity.viewTo", & preferences.intensity.viewTo, 100.0);   // dB
	Preferences_addEnum (L"FunctionEditor.intensity.averagingMethod", & preferences.intensity.averagingMethod, kTimeSoundAnalysisEditor_intensity_averagingMethod, DEFAULT);
	Preferences_addBool (L"FunctionEditor.intensity.subtractMeanPressure", & preferences.intensity.subtractMeanPressure, true);
	Preferences_addBool (L"FunctionEditor.formant.show", & preferences.formant.show, false);
	Preferences_addDouble (L"FunctionEditor.formant.maximumFormant", & preferences.formant.maximumFormant, 5500.0);   // Hertz
	Preferences_addLong (L"FunctionEditor.formant.numberOfPoles", & preferences.formant.numberOfPoles, 10);
	Preferences_addDouble (L"FunctionEditor.formant.windowLength", & preferences.formant.windowLength, 0.025);   // seconds
	Preferences_addDouble (L"FunctionEditor.formant.dynamicRange", & preferences.formant.dynamicRange, 30.0);   // dB
	Preferences_addDouble (L"FunctionEditor.formant.dotSize", & preferences.formant.dotSize, 1.0);   // mm
	Preferences_addEnum (L"FunctionEditor.formant.method", & preferences.formant.method, kTimeSoundAnalysisEditor_formant_analysisMethod, DEFAULT);
	Preferences_addDouble (L"FunctionEditor.formant.preemphasisFrom", & preferences.formant.preemphasisFrom, 50.0);   // Hertz
	Preferences_addBool (L"FunctionEditor.pulses.show", & preferences.pulses.show, false);
	Preferences_addDouble (L"FunctionEditor.pulses.maximumPeriodFactor", & preferences.pulses.maximumPeriodFactor, 1.3);
	Preferences_addDouble (L"FunctionEditor.pulses.maximumAmplitudeFactor", & preferences.pulses.maximumAmplitudeFactor, 1.6);
	Preferences_addBool (L"FunctionEditor.log1.toInfoWindow", & preferences.log[0].toInfoWindow, true);
	Preferences_addBool (L"FunctionEditor.log1.toLogFile", & preferences.log[0].toLogFile, true);
	Preferences_addString (L"FunctionEditor.log1.fileName", & preferences.log[0].fileName [0], LOG_1_FILE_NAME);
	Preferences_addString (L"FunctionEditor.log1.format", & preferences.log[0].format [0], LOG_1_FORMAT);
	Preferences_addBool (L"FunctionEditor.log2.toInfoWindow", & preferences.log[1].toInfoWindow, true);
	Preferences_addBool (L"FunctionEditor.log2.toLogFile", & preferences.log[1].toLogFile, true);
	Preferences_addString (L"FunctionEditor.log2.fileName", & preferences.log[1].fileName [0], LOG_2_FILE_NAME);
	Preferences_addString (L"FunctionEditor.log2.format", & preferences.log[1].format [0], LOG_2_FORMAT);
	Preferences_addString (L"FunctionEditor.logScript3", & preferences.logScript3 [0], LOG_3_FILE_NAME);
	Preferences_addString (L"FunctionEditor.logScript4", & preferences.logScript4 [0], LOG_4_FILE_NAME);
}

TimeSoundAnalysisEditor::TimeSoundAnalysisEditor (GuiObject parent, const wchar_t *title, Any data, Any sound, bool ownSound)
	: TimeSoundEditor (parent, title, data, sound, ownSound) {
	
	_longestAnalysis = preferences.longestAnalysis;
	if (preferences.log[0].toLogFile == FALSE && preferences.log[0].toInfoWindow == FALSE)
		preferences.log[0].toLogFile = TRUE, preferences.log[0].toInfoWindow = TRUE;
	if (preferences.log[1].toLogFile == FALSE && preferences.log[1].toInfoWindow == FALSE)
		preferences.log[1].toLogFile = TRUE, preferences.log[1].toInfoWindow = TRUE;
	_timeStepStrategy = preferences.timeStepStrategy;
	_fixedTimeStep = preferences.fixedTimeStep;
	_numberOfTimeStepsPerView = preferences.numberOfTimeStepsPerView;
	_spectrogram = preferences.spectrogram;
	_pitch = preferences.pitch;
	_intensity = preferences.intensity;
	_formant = preferences.formant;
	_pulses = preferences.pulses;
}

TimeSoundAnalysisEditor::~TimeSoundAnalysisEditor () {
	destroy_analysis ();
}

void TimeSoundAnalysisEditor::destroy_analysis () {
	forget (_spectrogram.data);
	forget (_pitch.data);
	forget (_intensity.data);
	forget (_formant.data);
	forget (_pulses.data);
}

void TimeSoundAnalysisEditor::info () {
	TimeSoundAnalysisEditor::info ();
	/* Spectrogram flag: */
	MelderInfo_writeLine2 (L"Spectrogram show: ", Melder_boolean (_spectrogram.show));
	/* Spectrogram settings: */
	MelderInfo_writeLine3 (L"Spectrogram view from: ", Melder_double (_spectrogram.viewFrom), L" Hertz");
	MelderInfo_writeLine3 (L"Spectrogram view to: ", Melder_double (_spectrogram.viewTo), L" Hertz");
	MelderInfo_writeLine3 (L"Spectrogram window length: ", Melder_double (_spectrogram.windowLength), L" seconds");
	MelderInfo_writeLine3 (L"Spectrogram dynamic range: ", Melder_double (_spectrogram.dynamicRange), L" dB");
	/* Advanced spectrogram settings: */
	MelderInfo_writeLine2 (L"Spectrogram number of time steps: ", Melder_integer (_spectrogram.timeSteps));
	MelderInfo_writeLine2 (L"Spectrogram number of frequency steps: ", Melder_integer (_spectrogram.frequencySteps));
	MelderInfo_writeLine2 (L"Spectrogram method: ", L"Fourier");
	MelderInfo_writeLine2 (L"Spectrogram window shape: ", kSound_to_Spectrogram_windowShape_getText (_spectrogram.windowShape));
	MelderInfo_writeLine2 (L"Spectrogram autoscaling: ", Melder_boolean (_spectrogram.autoscaling));
	MelderInfo_writeLine3 (L"Spectrogram maximum: ", Melder_double (_spectrogram.maximum), L" dB/Hz");
	MelderInfo_writeLine3 (L"Spectrogram pre-emphasis: ", Melder_integer (_spectrogram.preemphasis), L" dB/octave");
	MelderInfo_writeLine2 (L"Spectrogram dynamicCompression: ", Melder_integer (_spectrogram.dynamicCompression));
	/* Dynamic information: */
	MelderInfo_writeLine3 (L"Spectrogram cursor frequency: ", Melder_double (_spectrogram.cursor), L" Hertz");
	/* Pitch flag: */
	MelderInfo_writeLine2 (L"Pitch show: ", Melder_boolean (_pitch.show));
	/* Pitch settings: */
	MelderInfo_writeLine3 (L"Pitch floor: ", Melder_double (_pitch.floor), L" Hertz");
	MelderInfo_writeLine3 (L"Pitch ceiling: ", Melder_double (_pitch.ceiling), L" Hertz");
	MelderInfo_writeLine2 (L"Pitch unit: ", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, _pitch.unit, Function_UNIT_TEXT_MENU));
	MelderInfo_writeLine2 (L"Pitch drawing method: ", kTimeSoundAnalysisEditor_pitch_drawingMethod_getText (_pitch.drawingMethod));
	/* Advanced pitch settings: */
	MelderInfo_writeLine4 (L"Pitch view from: ", Melder_double (_pitch.viewFrom), L" ", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, _pitch.unit, Function_UNIT_TEXT_MENU));
	MelderInfo_writeLine4 (L"Pitch view to: ", Melder_double (_pitch.viewTo), L" ", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, _pitch.unit, Function_UNIT_TEXT_MENU));
	MelderInfo_writeLine2 (L"Pitch method: ", kTimeSoundAnalysisEditor_pitch_analysisMethod_getText (_pitch.method));
	MelderInfo_writeLine2 (L"Pitch very accurate: ", Melder_boolean (_pitch.veryAccurate));
	MelderInfo_writeLine2 (L"Pitch max. number of candidates: ", Melder_integer (_pitch.maximumNumberOfCandidates));
	MelderInfo_writeLine3 (L"Pitch silence threshold: ", Melder_double (_pitch.silenceThreshold), L" of global peak");
	MelderInfo_writeLine3 (L"Pitch voicing threshold: ", Melder_double (_pitch.voicingThreshold), L" (periodic power / total power)");
	MelderInfo_writeLine3 (L"Pitch octave cost: ", Melder_double (_pitch.octaveCost), L" per octave");
	MelderInfo_writeLine3 (L"Pitch octave jump cost: ", Melder_double (_pitch.octaveJumpCost), L" per octave");
	MelderInfo_writeLine2 (L"Pitch voiced/unvoiced cost: ", Melder_double (_pitch.voicedUnvoicedCost));
	/* Intensity flag: */
	MelderInfo_writeLine2 (L"Intensity show: ", Melder_boolean (_intensity.show));
	/* Intensity settings: */
	MelderInfo_writeLine3 (L"Intensity view from: ", Melder_double (_intensity.viewFrom), L" dB");
	MelderInfo_writeLine3 (L"Intensity view to: ", Melder_double (_intensity.viewTo), L" dB");
	MelderInfo_writeLine2 (L"Intensity averaging method: ", kTimeSoundAnalysisEditor_intensity_averagingMethod_getText (_intensity.averagingMethod));
	MelderInfo_writeLine2 (L"Intensity subtract mean pressure: ", Melder_boolean (_intensity.subtractMeanPressure));
	/* Formant flag: */
	MelderInfo_writeLine2 (L"Formant show: ", Melder_boolean (_formant.show));
	/* Formant settings: */
	MelderInfo_writeLine3 (L"Formant maximum formant: ", Melder_double (_formant.maximumFormant), L" Hertz");
	MelderInfo_writeLine2 (L"Formant number of poles: ", Melder_integer (_formant.numberOfPoles));
	MelderInfo_writeLine3 (L"Formant window length: ", Melder_double (_formant.windowLength), L" seconds");
	MelderInfo_writeLine3 (L"Formant dynamic range: ", Melder_double (_formant.dynamicRange), L" dB");
	MelderInfo_writeLine3 (L"Formant dot size: ", Melder_double (_formant.dotSize), L" mm");
	/* Advanced formant settings: */
	MelderInfo_writeLine2 (L"Formant method: ", kTimeSoundAnalysisEditor_formant_analysisMethod_getText (_formant.method));
	MelderInfo_writeLine3 (L"Formant pre-emphasis from: ", Melder_double (_formant.preemphasisFrom), L" Hertz");
	/* Pulses flag: */
	MelderInfo_writeLine2 (L"Pulses show: ", Melder_boolean (_pulses.show));
	MelderInfo_writeLine2 (L"Pulses maximum period factor: ", Melder_double (_pulses.maximumPeriodFactor));
	MelderInfo_writeLine2 (L"Pulses maximum amplitude factor: ", Melder_double (_pulses.maximumAmplitudeFactor));
}

enum {
	FunctionEditor_PART_CURSOR = 1,
	FunctionEditor_PART_SELECTION = 2
};

static const wchar_t *FunctionEditor_partString (int part) {
	static const wchar_t *strings [] = { L"", L"CURSOR", L"SELECTION" };
	return strings [part];
}

static const wchar_t *FunctionEditor_partString_locative (int part) {
	static const wchar_t *strings [] = { L"", L"at CURSOR", L"in SELECTION" };
	return strings [part];
}

int TimeSoundAnalysisEditor::makeQueriable (int allowCursor, double *tmin, double *tmax) {
	if (_endWindow - _startWindow > _longestAnalysis) {
		return Melder_error5 (L"Window too long to show analyses. Zoom in to at most ", Melder_half (_longestAnalysis), L" seconds "
			"or set the \"longest analysis\" to at least ", Melder_half (_endWindow - _startWindow), L" seconds.");
	}
	if (_startSelection == _endSelection) {
		if (allowCursor) {
			*tmin = *tmax = _startSelection;
			return FunctionEditor_PART_CURSOR;
		} else {
			return Melder_error1 (L"Make a selection first.");
		}
	} else if (_startSelection < _startWindow || _endSelection > _endWindow) {
		return Melder_error1 (L"Command ambiguous: a part of the selection is out of view. Either zoom or re-select.");
	}
	*tmin = _startSelection;
	*tmax = _endSelection;
	return FunctionEditor_PART_SELECTION;
}

static int menu_cb_logSettings (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Log settings", L"Log files")
		OPTIONMENU (L"Write log 1 to", 3)
			OPTION (L"Log file only")
			OPTION (L"Info window only")
			OPTION (L"Log file and Info window")
		LABEL (L"", L"Log file 1:")
		TEXTFIELD (L"Log file 1", LOG_1_FILE_NAME)
		LABEL (L"", L"Log 1 format:")
		TEXTFIELD (L"Log 1 format", LOG_1_FORMAT)
		OPTIONMENU (L"Write log 2 to", 3)
			OPTION (L"Log file only")
			OPTION (L"Info window only")
			OPTION (L"Log file and Info window")
		LABEL (L"", L"Log file 2:")
		TEXTFIELD (L"Log file 2", LOG_2_FILE_NAME)
		LABEL (L"", L"Log 2 format:")
		TEXTFIELD (L"Log 2 format", LOG_2_FORMAT)
		LABEL (L"", L"Log script 3:")
		TEXTFIELD (L"Log script 3", LOG_3_FILE_NAME)
		LABEL (L"", L"Log script 4:")
		TEXTFIELD (L"Log script 4", LOG_4_FILE_NAME)
	EDITOR_OK
		SET_INTEGER (L"Write log 1 to", preferences.log[0].toLogFile + 2 * preferences.log[0].toInfoWindow)
		SET_STRING (L"Log file 1", preferences.log[0].fileName)
		SET_STRING (L"Log 1 format", preferences.log[0].format)
		SET_INTEGER (L"Write log 2 to", preferences.log[1].toLogFile + 2 * preferences.log[1].toInfoWindow)
		SET_STRING (L"Log file 2", preferences.log[1].fileName)
		SET_STRING (L"Log 2 format", preferences.log[1].format)
		SET_STRING (L"Log script 3", preferences.logScript3)
		SET_STRING (L"Log script 4", preferences.logScript4)
	EDITOR_DO
		preferences.log[0].toLogFile = (GET_INTEGER (L"Write log 1 to") & 1) != 0;
		preferences.log[0].toInfoWindow = (GET_INTEGER (L"Write log 1 to") & 2) != 0;
		wcscpy (preferences.log[0].fileName, GET_STRING (L"Log file 1"));
		wcscpy (preferences.log[0].format, GET_STRING (L"Log 1 format"));
		preferences.log[1].toLogFile = (GET_INTEGER (L"Write log 2 to") & 1) != 0;
		preferences.log[1].toInfoWindow = (GET_INTEGER (L"Write log 2 to") & 2) != 0;
		wcscpy (preferences.log[1].fileName, GET_STRING (L"Log file 2"));
		wcscpy (preferences.log[1].format, GET_STRING (L"Log 2 format"));
		wcscpy (preferences.logScript3, GET_STRING (L"Log script 3"));
		wcscpy (preferences.logScript4, GET_STRING (L"Log script 4"));
	EDITOR_END
}

int TimeSoundAnalysisEditor::do_deleteLogFile (int which) {
	structMelderFile file = { 0 };
	if (! Melder_pathToFile (preferences.log[which].fileName, & file)) return 0;
	MelderFile_delete (& file);
	return 1;
}
static int menu_cb_deleteLogFile1 (EDITOR_ARGS) { TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_deleteLogFile (0); }
static int menu_cb_deleteLogFile2 (EDITOR_ARGS) { TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_deleteLogFile (1); }

int TimeSoundAnalysisEditor::do_log (int which) {
	wchar_t format [1000], *p;
	double tmin, tmax;
	int part = makeQueriable (TRUE, & tmin, & tmax); iferror return 0;
	wcscpy (format, preferences.log[which].format);
	for (p = format; *p !='\0'; p ++) if (*p == '\'') {
		/*
		 * Found a left quote. Search for a matching right quote.
		 */
		wchar_t *q = p + 1, varName [300], *r, *s, *colon;
		int precision = -1;
		double value = NUMundefined;
		const wchar_t *stringValue = NULL;
		while (*q != '\0' && *q != '\'') q ++;
		if (*q == '\0') break;   /* No matching right quote: done with this line. */
		if (q - p == 1) continue;   /* Ignore empty variable names. */
		/*
		 * Found a right quote. Get potential variable name.
		 */
		for (r = p + 1, s = varName; q - r > 0; r ++, s ++) *s = *r;
		*s = '\0';   /* Trailing null byte. */
		colon = wcschr (varName, ':');
		if (colon) {
			precision = wcstol (colon + 1, NULL, 10);
			*colon = '\0';
		}
		if (wcsequ (varName, L"time")) {
			value = 0.5 * (tmin + tmax);
		} else if (wcsequ (varName, L"t1")) {
			value = tmin;
		} else if (wcsequ (varName, L"t2")) {
			value = tmax;
		} else if (wcsequ (varName, L"dur")) {
			value = tmax - tmin;
		} else if (wcsequ (varName, L"freq")) {
			value = _spectrogram.cursor;
		} else if (wcsequ (varName, L"tab$")) {
			stringValue = L"\t";
		} else if (wcsequ (varName, L"editor$")) {
			stringValue = _name;
		} else if (wcsequ (varName, L"f0")) {
			if (! _pitch.show)
				return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
			if (! _pitch.data) {
				return Melder_error1 (theMessage_Cannot_compute_pitch);
			}
			if (part == FunctionEditor_PART_CURSOR) {
				value = Pitch_getValueAtTime (_pitch.data, tmin, _pitch.unit, 1);
			} else {
				value = Pitch_getMean (_pitch.data, tmin, tmax, _pitch.unit);
			}
		} else if (varName [0] == 'f' && varName [1] >= '1' && varName [1] <= '5' && varName [2] == '\0') {
			if (! _formant.show)
				return Melder_error1 (L"No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
			if (! _formant.data) {
				return Melder_error1 (theMessage_Cannot_compute_formant);
			}
			if (part == FunctionEditor_PART_CURSOR) {
				value = Formant_getValueAtTime (_formant.data, varName [1] - '0', tmin, 0);
			} else {
				value = Formant_getMean (_formant.data, varName [1] - '0', tmin, tmax, 0);
			}
		} else if (varName [0] == 'b' && varName [1] >= '1' && varName [1] <= '5' && varName [2] == '\0') {
			if (! _formant.show)
				return Melder_error1 (L"No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
			if (! _formant.data) {
				return Melder_error1 (theMessage_Cannot_compute_formant);
			}
			value = Formant_getBandwidthAtTime (_formant.data, varName [1] - '0', 0.5 * (tmin + tmax), 0);
		} else if (wcsequ (varName, L"intensity")) {
			if (! _intensity.show)
				return Melder_error1 (L"No intensity contour is visible.\nFirst choose \"Show intensity\" from the Intensity menu.");
			if (! _intensity.data) {
				return Melder_error1 (theMessage_Cannot_compute_intensity);
			}
			if (part == FunctionEditor_PART_CURSOR) {
				value = Vector_getValueAtX (_intensity.data, tmin, Vector_CHANNEL_1, Vector_VALUE_INTERPOLATION_LINEAR);
			} else {
				value = Intensity_getAverage (_intensity.data, tmin, tmax, _intensity.averagingMethod);
			}
		} else if (wcsequ (varName, L"power")) {
			if (! _spectrogram.show)
				return Melder_error1 (L"No spectrogram is visible.\nFirst choose \"Show spectrogram\" from the Spectrum menu.");
			if (! _spectrogram.data) {
				return Melder_error1 (theMessage_Cannot_compute_spectrogram);
			}
			if (part != FunctionEditor_PART_CURSOR) return Melder_error1 (L"Click inside the spectrogram first.");
			value = Matrix_getValueAtXY (_spectrogram.data, tmin, _spectrogram.cursor);
		}
		if (NUMdefined (value)) {
			int varlen = (q - p) - 1, headlen = p - format;
			wchar_t formattedNumber [400];
			if (precision >= 0) {
				swprintf (formattedNumber, 400, L"%.*f", precision, value);
			} else {
				swprintf (formattedNumber, 400, L"%.17g", value);
			}
			int arglen = wcslen (formattedNumber);
			static MelderString buffer = { 0 };
			MelderString_ncopy (& buffer, format, headlen);
			MelderString_append2 (& buffer, formattedNumber, p + varlen + 2);
			wcscpy (format, buffer.string);
			p += arglen - 1;
		} else if (stringValue != NULL) {
			int varlen = (q - p) - 1, headlen = p - format, arglen = wcslen (stringValue);
			static MelderString buffer = { 0 };
			MelderString_ncopy (& buffer, format, headlen);
			MelderString_append2 (& buffer, stringValue, p + varlen + 2);
			wcscpy (format, buffer.string);
			p += arglen - 1;
		} else {
			p = q - 1;   /* Go to before next quote. */
		}
	}
	if (preferences.log[which].toInfoWindow) {
		MelderInfo_write1 (format);
		MelderInfo_close ();
	}
	if (preferences.log[which].toLogFile) {
		structMelderFile file = { 0 };
		wcscat (format, L"\n");
		if (! Melder_relativePathToFile (preferences.log[which].fileName, & file)) return 0;
		if (! MelderFile_appendText (& file, format)) return 0;
	}
	return 1;
}

static int menu_cb_log1 (EDITOR_ARGS) { TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_log (0); }
static int menu_cb_log2 (EDITOR_ARGS) { TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_log (1); }

static int menu_cb_logScript3 (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	return DO_RunTheScriptFromAnyAddedEditorCommand (editor, preferences.logScript3);
}
static int menu_cb_logScript4 (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	return DO_RunTheScriptFromAnyAddedEditorCommand (editor, preferences.logScript4);
}

static int menu_cb_showAnalyses (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Show analyses", 0)
		BOOLEAN (L"Show spectrogram", 1)
		BOOLEAN (L"Show pitch", 1)
		BOOLEAN (L"Show intensity", 0)
		BOOLEAN (L"Show formants", 0)
		BOOLEAN (L"Show pulses", 0)
		POSITIVE (L"Longest analysis (s)", L"5.0")
	EDITOR_OK
		SET_INTEGER (L"Show spectrogram", editor->_spectrogram.show)
		SET_INTEGER (L"Show pitch", editor->_pitch.show)
		SET_INTEGER (L"Show intensity", editor->_intensity.show)
		SET_INTEGER (L"Show formants", editor->_formant.show)
		SET_INTEGER (L"Show pulses", editor->_pulses.show)
		SET_REAL (L"Longest analysis", editor->_longestAnalysis)
	EDITOR_DO
		GuiMenuItem_check (editor->_spectrogramToggle, preferences.spectrogram.show = editor->_spectrogram.show = GET_INTEGER (L"Show spectrogram"));
		GuiMenuItem_check (editor->_pitchToggle, preferences.pitch.show = editor->_pitch.show = GET_INTEGER (L"Show pitch"));
		GuiMenuItem_check (editor->_intensityToggle, preferences.intensity.show = editor->_intensity.show = GET_INTEGER (L"Show intensity"));
		GuiMenuItem_check (editor->_formantToggle, preferences.formant.show = editor->_formant.show = GET_INTEGER (L"Show formants"));
		GuiMenuItem_check (editor->_pulsesToggle, preferences.pulses.show = editor->_pulses.show = GET_INTEGER (L"Show pulses"));
		preferences.longestAnalysis = editor->_longestAnalysis = GET_REAL (L"Longest analysis");
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_timeStepSettings (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Time step settings", L"Time step settings...")
		OPTIONMENU_ENUM (L"Time step strategy", kTimeSoundAnalysisEditor_timeStepStrategy, DEFAULT)
		LABEL (L"", L"")
		LABEL (L"", L"If the time step strategy is \"fixed\":")
		POSITIVE (L"Fixed time step (s)", L"0.01")
		LABEL (L"", L"")
		LABEL (L"", L"If the time step strategy is \"view-dependent\":")
		NATURAL (L"Number of time steps per view", L"100")
	EDITOR_OK
		SET_ENUM (L"Time step strategy", kTimeSoundAnalysisEditor_timeStepStrategy, editor->_timeStepStrategy)
		SET_REAL (L"Fixed time step", editor->_fixedTimeStep)
		SET_INTEGER (L"Number of time steps per view", editor->_numberOfTimeStepsPerView)
	EDITOR_DO
		preferences.timeStepStrategy = editor->_timeStepStrategy = GET_ENUM (kTimeSoundAnalysisEditor_timeStepStrategy, L"Time step strategy");
		preferences.fixedTimeStep = editor->_fixedTimeStep = GET_REAL (L"Fixed time step");
		preferences.numberOfTimeStepsPerView = editor->_numberOfTimeStepsPerView = GET_INTEGER (L"Number of time steps per view");
		forget (editor->_pitch.data);
		forget (editor->_formant.data);
		forget (editor->_intensity.data);
		forget (editor->_pulses.data);
		editor->redraw ();
	EDITOR_END
}

/***** SPECTROGRAM MENU *****/

static int menu_cb_showSpectrogram (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	preferences.spectrogram.show = editor->_spectrogram.show = ! editor->_spectrogram.show;
	GuiMenuItem_check (editor->_spectrogramToggle, editor->_spectrogram.show);   // in case we're called from a script
	editor->redraw ();
	return 1;
}

static int menu_cb_spectrogramSettings (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Spectrogram settings", L"Intro 3.2. Configuring the spectrogram")
		REAL (L"left View range (Hz)", L"0.0")
		POSITIVE (L"right View range (Hz)", L"5000.0")
		POSITIVE (L"Window length (s)", L"0.005")
		POSITIVE (L"Dynamic range (dB)", L"50.0")
		LABEL (L"note1", L"")
		LABEL (L"note2", L"")
	EDITOR_OK
		SET_REAL (L"left View range", editor->_spectrogram.viewFrom)
		SET_REAL (L"right View range", editor->_spectrogram.viewTo)
		SET_REAL (L"Window length", editor->_spectrogram.windowLength)
		SET_REAL (L"Dynamic range", editor->_spectrogram.dynamicRange)
		if (editor->_spectrogram.timeSteps != 1000 || editor->_spectrogram.frequencySteps != 250 || editor->_spectrogram.method != 1 ||
			editor->_spectrogram.windowShape != 5 || editor->_spectrogram.maximum != 100.0 || ! editor->_spectrogram.autoscaling ||
			editor->_spectrogram.preemphasis != 6.0 || editor->_spectrogram.dynamicCompression != 0.0)
		{
			SET_STRING (L"note1", L"Warning: you have non-standard \"advanced settings\".")
		} else {
			SET_STRING (L"note1", L"(all of your \"advanced settings\" have their standard values)")
		}
		if (editor->_timeStepStrategy != kTimeSoundAnalysisEditor_timeStepStrategy_DEFAULT) {
			SET_STRING (L"note2", L"Warning: you have a non-standard \"time step strategy\".")
		} else {
			SET_STRING (L"note2", L"(your \"time step strategy\" has its standard value: automatic)")
		}
	EDITOR_DO
		preferences.spectrogram.viewFrom = editor->_spectrogram.viewFrom = GET_REAL (L"left View range");
		preferences.spectrogram.viewTo = editor->_spectrogram.viewTo = GET_REAL (L"right View range");
		preferences.spectrogram.windowLength = editor->_spectrogram.windowLength = GET_REAL (L"Window length");
		preferences.spectrogram.dynamicRange = editor->_spectrogram.dynamicRange = GET_REAL (L"Dynamic range");
		forget (editor->_spectrogram.data);
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_advancedSpectrogramSettings (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Advanced spectrogram settings", L"Advanced spectrogram settings...")
		LABEL (L"", L"Time and frequency resolutions:")
		NATURAL (L"Number of time steps", L"1000")
		NATURAL (L"Number of frequency steps", L"250")
		LABEL (L"", L"Spectrogram analysis settings:")
		OPTIONMENU_ENUM (L"Method", kSound_to_Spectrogram_method, DEFAULT)
		OPTIONMENU_ENUM (L"Window shape", kSound_to_Spectrogram_windowShape, DEFAULT)
		LABEL (L"", L"Spectrogram view settings:")
		BOOLEAN (L"Autoscaling", 1)
		REAL (L"Maximum (dB/Hz)", L"100.0")
		REAL (L"Pre-emphasis (dB/oct)", L"6.0")
		REAL (L"Dynamic compression (0-1)", L"0.0")
	EDITOR_OK
		SET_INTEGER (L"Number of time steps", editor->_spectrogram.timeSteps)
		SET_INTEGER (L"Number of frequency steps", editor->_spectrogram.frequencySteps)
		SET_ENUM (L"Method", kSound_to_Spectrogram_method, editor->_spectrogram.method)
		SET_ENUM (L"Window shape", kSound_to_Spectrogram_windowShape, editor->_spectrogram.windowShape)
		SET_REAL (L"Maximum", editor->_spectrogram.maximum)
		SET_INTEGER (L"Autoscaling", editor->_spectrogram.autoscaling)
		SET_REAL (L"Pre-emphasis", editor->_spectrogram.preemphasis)
		SET_REAL (L"Dynamic compression", editor->_spectrogram.dynamicCompression)
	EDITOR_DO
		preferences.spectrogram.timeSteps = editor->_spectrogram.timeSteps = GET_INTEGER (L"Number of time steps");
		preferences.spectrogram.frequencySteps = editor->_spectrogram.frequencySteps = GET_INTEGER (L"Number of frequency steps");
		preferences.spectrogram.method = editor->_spectrogram.method = GET_ENUM (kSound_to_Spectrogram_method, L"Method");
		preferences.spectrogram.windowShape = editor->_spectrogram.windowShape = GET_ENUM (kSound_to_Spectrogram_windowShape, L"Window shape");
		preferences.spectrogram.maximum = editor->_spectrogram.maximum = GET_REAL (L"Maximum");
		preferences.spectrogram.autoscaling = editor->_spectrogram.autoscaling = GET_INTEGER (L"Autoscaling");
		preferences.spectrogram.preemphasis = editor->_spectrogram.preemphasis = GET_REAL (L"Pre-emphasis");
		preferences.spectrogram.dynamicCompression = editor->_spectrogram.dynamicCompression = GET_REAL (L"Dynamic compression");
		forget (editor->_spectrogram.data);
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_getFrequency (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	Melder_informationReal (editor->_spectrogram.cursor, L"Hertz");
	return 1;
}

static int menu_cb_getSpectralPowerAtCursorCross (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	double tmin, tmax;
	int part = editor->makeQueriable (TRUE, & tmin, & tmax); iferror return 0;
	if (! editor->_spectrogram.show)
		return Melder_error1 (L"No spectrogram is visible.\nFirst choose \"Show spectrogram\" from the Spectrum menu.");
	if (! editor->_spectrogram.data) {
		editor->computeSpectrogram ();
		if (! editor->_spectrogram.data) return Melder_error1 (theMessage_Cannot_compute_spectrogram);
	}
	if (part != FunctionEditor_PART_CURSOR) return Melder_error1 (L"Click inside the spectrogram first.");
	MelderInfo_open ();
	MelderInfo_write1 (Melder_double (Matrix_getValueAtXY (editor->_spectrogram.data, tmin, editor->_spectrogram.cursor)));
	MelderInfo_write5 (L" Pa2/Hz (at time = ", Melder_double (tmin), L" seconds and frequency = ",
		Melder_double (editor->_spectrogram.cursor), L" Hz)");
	MelderInfo_close ();
	return 1;
}

static int menu_cb_moveFrequencyCursorTo (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	if (! editor->_spectrogram.show)
		return Melder_error1 (L"No spectrogram is visible.\nFirst choose \"Show spectrogram\" from the Spectrum menu.");
	EDITOR_FORM (L"Move frequency cursor to", 0)
		REAL (L"Frequency (Hz)", L"0.0")
	EDITOR_OK
		SET_REAL (L"Frequency", editor->_spectrogram.cursor)
	EDITOR_DO
		double frequency = GET_REAL (L"Frequency");
		editor->_spectrogram.cursor = frequency;
		editor->redraw ();
	EDITOR_END
}

Sound TimeSoundAnalysisEditor::extractSound (double tmin, double tmax) {
	Sound sound = NULL;
	if (_longSound.data) {
		if (tmin < _longSound.data -> xmin) tmin = _longSound.data -> xmin;
		if (tmax > _longSound.data -> xmax) tmax = _longSound.data -> xmax;
		sound = LongSound_extractPart (_longSound.data, tmin, tmax, TRUE);
	} else if (_sound.data) {
		if (tmin < _sound.data -> xmin) tmin = _sound.data -> xmin;
		if (tmax > _sound.data -> xmax) tmax = _sound.data -> xmax;
		sound = Sound_extractPart (_sound.data, tmin, tmax, kSound_windowShape_RECTANGULAR, 1.0, TRUE);
	}
	return sound;
}

static int menu_cb_extractVisibleSpectrogram (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	Spectrogram publish;
	if (! editor->_spectrogram.show)
		return Melder_error1 (L"No spectrogram is visible.\nFirst choose \"Show spectrogram\" from the Spectrum menu.");
	if (! editor->_spectrogram.data) {
		editor->computeSpectrogram ();
		if (! editor->_spectrogram.data) return Melder_error1 (theMessage_Cannot_compute_spectrogram);
	}
	publish = (Spectrogram) Data_copy (editor->_spectrogram.data);
	if (publish == NULL) return 0;
	if (editor->_publishCallback)
		editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

static int menu_cb_viewSpectralSlice (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	double start = editor->_startSelection == editor->_endSelection ?
		editor->_spectrogram.windowShape == 5 ? editor->_startSelection - editor->_spectrogram.windowLength :
		editor->_startSelection - editor->_spectrogram.windowLength / 2 : editor->_startSelection;
	double finish = editor->_startSelection == editor->_endSelection ?
		editor->_spectrogram.windowShape == 5 ? editor->_endSelection + editor->_spectrogram.windowLength :
		editor->_endSelection + editor->_spectrogram.windowLength / 2 : editor->_endSelection;
	Sound sound = editor->extractSound (start, finish);
	Spectrum publish;
	if (sound == NULL) return 0;
	Sound_multiplyByWindow (sound,
		editor->_spectrogram.windowShape == kSound_to_Spectrogram_windowShape_SQUARE ? kSound_windowShape_RECTANGULAR :
		editor->_spectrogram.windowShape == kSound_to_Spectrogram_windowShape_HAMMING ? kSound_windowShape_HAMMING :
		editor->_spectrogram.windowShape == kSound_to_Spectrogram_windowShape_BARTLETT ? kSound_windowShape_TRIANGULAR :
		editor->_spectrogram.windowShape == kSound_to_Spectrogram_windowShape_WELCH ? kSound_windowShape_PARABOLIC :
		editor->_spectrogram.windowShape == kSound_to_Spectrogram_windowShape_HANNING ? kSound_windowShape_HANNING :
		editor->_spectrogram.windowShape == kSound_to_Spectrogram_windowShape_GAUSSIAN ? kSound_windowShape_GAUSSIAN_2 : kSound_windowShape_RECTANGULAR);
	publish = Sound_to_Spectrum (sound, TRUE);
	forget (sound);
	if (! publish) return 0;
	static MelderString sliceName = { 0 };
	MelderString_copy (& sliceName, editor->_data == NULL ? L"untitled" : ((Data) editor->_data) -> name);
	MelderString_appendCharacter (& sliceName, '_');
	MelderString_append (& sliceName, Melder_fixed (0.5 * (editor->_startSelection + editor->_endSelection), 3));
	Thing_setName (publish, sliceName.string);
	if (editor->_publishCallback)
		editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

static int menu_cb_paintVisibleSpectrogram (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Paint visible spectrogram", 0)
		editor->form_pictureWindow (cmd);
		editor->form_pictureMargins (cmd);
		editor->form_pictureSelection (cmd);
		BOOLEAN (L"Garnish", 1);
	EDITOR_OK
		editor->ok_pictureWindow (cmd);
		editor->ok_pictureMargins (cmd);
		editor->ok_pictureSelection (cmd);
		SET_INTEGER (L"Garnish", prefs.picture.spectrogram.garnish);
	EDITOR_DO
		editor->do_pictureWindow (cmd);
		editor->do_pictureMargins (cmd);
		editor->do_pictureSelection (cmd);
		prefs.picture.spectrogram.garnish = GET_INTEGER (L"Garnish");
		if (! editor->_spectrogram.show)
			return Melder_error1 (L"No spectrogram is visible.\nFirst choose \"Show spectrogram\" from the Spectrum menu.");
		if (! editor->_spectrogram.data) {
			editor->computeSpectrogram ();
			if (! editor->_spectrogram.data) return Melder_error1 (theMessage_Cannot_compute_spectrogram);
		}
		editor->openPraatPicture ();
		Spectrogram_paint (editor->_spectrogram.data, editor->_pictureGraphics, editor->_startWindow, editor->_endWindow, editor->_spectrogram.viewFrom, editor->_spectrogram.viewTo,
			editor->_spectrogram.maximum, editor->_spectrogram.autoscaling, editor->_spectrogram.dynamicRange, editor->_spectrogram.preemphasis,
			editor->_spectrogram.dynamicCompression, prefs.picture.spectrogram.garnish);
		editor->garnish ();
		editor->closePraatPicture ();
	EDITOR_END
}

/***** PITCH MENU *****/

static int menu_cb_showPitch (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	preferences.pitch.show = editor->_pitch.show = ! editor->_pitch.show;
	GuiMenuItem_check (editor->_pitchToggle, editor->_pitch.show);   // in case we're called from a script
	editor->redraw ();
	return 1;
}

static int menu_cb_pitchSettings (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Pitch settings", L"Intro 4.2. Configuring the pitch contour")
		POSITIVE (L"left Pitch range (Hz)", L"75.0")
		POSITIVE (L"right Pitch range (Hz)", L"500.0")
		OPTIONMENU_ENUM (L"Unit", kPitch_unit, DEFAULT)
		LABEL (L"opt1", L"The autocorrelation method optimizes for intonation research;")
		LABEL (L"opt2", L"and the cross-correlation method optimizes for voice research:")
		RADIO_ENUM (L"Analysis method", kTimeSoundAnalysisEditor_pitch_analysisMethod, DEFAULT)
		OPTIONMENU_ENUM (L"Drawing method", kTimeSoundAnalysisEditor_pitch_drawingMethod, DEFAULT)
		LABEL (L"note1", L"")
		LABEL (L"note2", L"")
	EDITOR_OK
		SET_REAL (L"left Pitch range", editor->_pitch.floor)
		SET_REAL (L"right Pitch range", editor->_pitch.ceiling)
		SET_ENUM (L"Unit", kPitch_unit, editor->_pitch.unit)
		SET_ENUM (L"Analysis method", kTimeSoundAnalysisEditor_pitch_analysisMethod, editor->_pitch.method)
		SET_ENUM (L"Drawing method", kTimeSoundAnalysisEditor_pitch_drawingMethod, editor->_pitch.drawingMethod)
		if (editor->_pitch.viewFrom != 0.0 || editor->_pitch.viewTo != 0.0 ||
			editor->_pitch.veryAccurate != FALSE || editor->_pitch.maximumNumberOfCandidates != 15 ||
			editor->_pitch.silenceThreshold != 0.03 || editor->_pitch.voicingThreshold != 0.45 || editor->_pitch.octaveCost != 0.01 ||
			editor->_pitch.octaveJumpCost != 0.35 || editor->_pitch.voicedUnvoicedCost != 0.14)
		{
			SET_STRING (L"note1", L"Warning: you have some non-standard \"advanced settings\".")
		} else {
			SET_STRING (L"note1", L"(all of your \"advanced settings\" have their standard values)")
		}
		if (editor->_timeStepStrategy != kTimeSoundAnalysisEditor_timeStepStrategy_DEFAULT) {
			SET_STRING (L"note2", L"Warning: you have a non-standard \"time step strategy\".")
		} else {
			SET_STRING (L"note2", L"(your \"time step strategy\" has its standard value: automatic)")
		}
	EDITOR_DO
		preferences.pitch.floor = editor->_pitch.floor = GET_REAL (L"left Pitch range");
		preferences.pitch.ceiling = editor->_pitch.ceiling = GET_REAL (L"right Pitch range");
		preferences.pitch.unit = editor->_pitch.unit = GET_ENUM (kPitch_unit, L"Unit");
		preferences.pitch.method = editor->_pitch.method = GET_ENUM (kTimeSoundAnalysisEditor_pitch_analysisMethod, L"Analysis method");
		preferences.pitch.drawingMethod = editor->_pitch.drawingMethod = GET_ENUM (kTimeSoundAnalysisEditor_pitch_drawingMethod, L"Drawing method");
		forget (editor->_pitch.data);
		forget (editor->_intensity.data);
		forget (editor->_pulses.data);
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_advancedPitchSettings (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Advanced pitch settings", L"Advanced pitch settings...")
		LABEL (L"", L"Make view range different from analysis range:")
		REAL (L"left View range (units)", L"0.0 (= auto)")
		REAL (L"right View range (units)", L"0.0 (= auto)")
		LABEL (L"", L"Analysis settings:")
		BOOLEAN (L"Very accurate", 0)
		NATURAL (L"Max. number of candidates", L"15")
		REAL (L"Silence threshold", L"0.03")
		REAL (L"Voicing threshold", L"0.45")
		REAL (L"Octave cost", L"0.01")
		REAL (L"Octave-jump cost", L"0.35")
		REAL (L"Voiced / unvoiced cost", L"0.14")
	EDITOR_OK
		SET_REAL (L"left View range", editor->_pitch.viewFrom)
		SET_REAL (L"right View range", editor->_pitch.viewTo)
		SET_INTEGER (L"Very accurate", editor->_pitch.veryAccurate)
		SET_INTEGER (L"Max. number of candidates", editor->_pitch.maximumNumberOfCandidates)
		SET_REAL (L"Silence threshold", editor->_pitch.silenceThreshold)
		SET_REAL (L"Voicing threshold", editor->_pitch.voicingThreshold)
		SET_REAL (L"Octave cost", editor->_pitch.octaveCost)
		SET_REAL (L"Octave-jump cost", editor->_pitch.octaveJumpCost)
		SET_REAL (L"Voiced / unvoiced cost", editor->_pitch.voicedUnvoicedCost)
	EDITOR_DO
		long maxnCandidates = GET_INTEGER (L"Max. number of candidates");
		if (maxnCandidates < 2) return Melder_error1 (L"Maximum number of candidates must be greater than 1.");
		preferences.pitch.viewFrom = editor->_pitch.viewFrom = GET_REAL (L"left View range");
		preferences.pitch.viewTo = editor->_pitch.viewTo = GET_REAL (L"right View range");
		preferences.pitch.veryAccurate = editor->_pitch.veryAccurate = GET_INTEGER (L"Very accurate");
		preferences.pitch.maximumNumberOfCandidates = editor->_pitch.maximumNumberOfCandidates = GET_INTEGER (L"Max. number of candidates");
		preferences.pitch.silenceThreshold = editor->_pitch.silenceThreshold = GET_REAL (L"Silence threshold");
		preferences.pitch.voicingThreshold = editor->_pitch.voicingThreshold = GET_REAL (L"Voicing threshold");
		preferences.pitch.octaveCost = editor->_pitch.octaveCost = GET_REAL (L"Octave cost");
		preferences.pitch.octaveJumpCost = editor->_pitch.octaveJumpCost = GET_REAL (L"Octave-jump cost");
		preferences.pitch.voicedUnvoicedCost = editor->_pitch.voicedUnvoicedCost = GET_REAL (L"Voiced / unvoiced cost");
		forget (editor->_pitch.data);
		forget (editor->_intensity.data);
		forget (editor->_pulses.data);
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_pitchListing (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	double tmin, tmax;
	int part = editor->makeQueriable (TRUE, & tmin, & tmax); iferror return 0;
	if (! editor->_pitch.show)
		return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
	if (! editor->_pitch.data) {
		editor->computePitch ();
		if (! editor->_pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	MelderInfo_open ();
	MelderInfo_writeLine2 (L"Time_s   F0_", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit, Function_UNIT_TEXT_SHORT));
	if (part == FunctionEditor_PART_CURSOR) {
		double f0 = Pitch_getValueAtTime (editor->_pitch.data, tmin, editor->_pitch.unit, TRUE);
		f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
		MelderInfo_writeLine3 (Melder_fixed (tmin, 6), L"   ", Melder_fixed (f0, 6));
	} else {
		long i, i1, i2;
		Sampled_getWindowSamples (editor->_pitch.data, tmin, tmax, & i1, & i2);
		for (i = i1; i <= i2; i ++) {
			double t = Sampled_indexToX (editor->_pitch.data, i);
			double f0 = Sampled_getValueAtSample (editor->_pitch.data, i, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
			f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
			MelderInfo_writeLine3 (Melder_fixed (t, 6), L"   ", Melder_fixed (f0, 6));
		}
	}
	MelderInfo_close ();
	return 1;
}

static int menu_cb_getPitch (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	double tmin, tmax;
	int part = editor->makeQueriable (TRUE, & tmin, & tmax); iferror return 0;
	if (! editor->_pitch.show)
		return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
	if (! editor->_pitch.data) {
		editor->computePitch ();
		if (! editor->_pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	if (part == FunctionEditor_PART_CURSOR) {
		double f0 = Pitch_getValueAtTime (editor->_pitch.data, tmin, editor->_pitch.unit, TRUE);
		f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
		Melder_information4 (Melder_double (f0), L" ", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit, 0),
			L" (interpolated pitch at CURSOR)");
	} else {
		double f0 = Pitch_getMean (editor->_pitch.data, tmin, tmax, editor->_pitch.unit);
		f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
		Melder_information6 (Melder_double (f0), L" ", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit, 0),
			L" (mean pitch ", FunctionEditor_partString_locative (part), L")");
	}
	return 1;
}

static int menu_cb_getMinimumPitch (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	double tmin, tmax, f0;
	int part = editor->makeQueriable (FALSE, & tmin, & tmax); iferror return 0;
	if (! editor->_pitch.show)
		return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
	if (! editor->_pitch.data) {
		editor->computePitch ();
		if (! editor->_pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	f0 = Pitch_getMinimum (editor->_pitch.data, tmin, tmax, editor->_pitch.unit, TRUE);
	f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
	Melder_information6 (Melder_double (f0), L" ", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit, 0),
		L" (minimum pitch ", FunctionEditor_partString_locative (part), L")");
	return 1;
}

static int menu_cb_getMaximumPitch (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	double tmin, tmax, f0;
	int part = editor->makeQueriable (FALSE, & tmin, & tmax); iferror return 0;
	if (! editor->_pitch.show)
		return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
	if (! editor->_pitch.data) {
		editor->computePitch ();
		if (! editor->_pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	f0 = Pitch_getMaximum (editor->_pitch.data, tmin, tmax, editor->_pitch.unit, TRUE);
	f0 = ClassFunction_convertToNonlogarithmic (classPitch, f0, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
	Melder_information6 (Melder_double (f0), L" ", ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit, 0),
		L" (maximum pitch ", FunctionEditor_partString_locative (part), L")");
	return 1;
}

static int menu_cb_moveCursorToMinimumPitch (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	if (! editor->_pitch.show)
		return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the View menu.");
	if (! editor->_pitch.data) {
		editor->computePitch ();
		if (! editor->_pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	if (editor->_startSelection == editor->_endSelection) {
		return Melder_error1 (L"Empty selection.");
	} else {
		double time;
		Pitch_getMinimumAndTime (editor->_pitch.data, editor->_startSelection, editor->_endSelection,
			editor->_pitch.unit, 1, NULL, & time);
		if (! NUMdefined (time))
			return Melder_error1 (L"Selection is voiceless.");
		editor->_startSelection = editor->_endSelection = time;
		editor->marksChanged ();
	}
	return 1;
}

static int menu_cb_moveCursorToMaximumPitch (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	if (! editor->_pitch.show)
		return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the View menu.");
	if (! editor->_pitch.data) {
		editor->computePitch ();
		if (! editor->_pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	if (editor->_startSelection == editor->_endSelection) {
		return Melder_error1 (L"Empty selection.");
	} else {
		double time;
		Pitch_getMaximumAndTime (editor->_pitch.data, editor->_startSelection, editor->_endSelection,
			editor->_pitch.unit, 1, NULL, & time);
		if (! NUMdefined (time))
			return Melder_error1 (L"Selection is voiceless.");
		editor->_startSelection = editor->_endSelection = time;
		editor->marksChanged ();
	}
	return 1;
}

static int menu_cb_extractVisiblePitchContour (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	if (! editor->_pitch.show)
		return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
	if (! editor->_pitch.data) {
		editor->computePitch ();
		if (! editor->_pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
	}
	Pitch publish = (Pitch) Data_copy (editor->_pitch.data);
	if (! publish) return 0;
	if (editor->_publishCallback)
		editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

static int menu_cb_drawVisiblePitchContour (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Draw visible pitch contour", 0)
		editor->form_pictureWindow (cmd);
		LABEL (L"", L"Pitch:")
		BOOLEAN (L"Speckle", 0);
		editor->form_pictureMargins (cmd);
		editor->form_pictureSelection (cmd);
		BOOLEAN (L"Garnish", 1);
	EDITOR_OK
		editor->ok_pictureWindow (cmd);
		SET_INTEGER (L"Speckle", editor->_pitch.picture.speckle);
		editor->ok_pictureMargins (cmd);
		editor->ok_pictureSelection (cmd);
		SET_INTEGER (L"Garnish", prefs.picture.pitch.garnish);
	EDITOR_DO
		editor->do_pictureWindow (cmd);
		preferences.pitch.picture.speckle = editor->_pitch.picture.speckle = GET_INTEGER (L"Speckle");
		editor->do_pictureMargins (cmd);
		editor->do_pictureSelection (cmd);
		prefs.picture.pitch.garnish = GET_INTEGER (L"Garnish");
		if (! editor->_pitch.show)
			return Melder_error1 (L"No pitch contour is visible.\nFirst choose \"Show pitch\" from the Pitch menu.");
		if (! editor->_pitch.data) {
			editor->computePitch ();
			if (! editor->_pitch.data) return Melder_error1 (theMessage_Cannot_compute_pitch);
		}
		editor->openPraatPicture ();
		double pitchFloor_hidden = ClassFunction_convertStandardToSpecialUnit (classPitch, editor->_pitch.floor, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
		double pitchCeiling_hidden = ClassFunction_convertStandardToSpecialUnit (classPitch, editor->_pitch.ceiling, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
		double pitchFloor_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchFloor_hidden, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
		double pitchCeiling_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchCeiling_hidden, Pitch_LEVEL_FREQUENCY, editor->_pitch.unit);
		double pitchViewFrom_overt = editor->_pitch.viewFrom < editor->_pitch.viewTo ? editor->_pitch.viewFrom : pitchFloor_overt;
		double pitchViewTo_overt = editor->_pitch.viewFrom < editor->_pitch.viewTo ? editor->_pitch.viewTo : pitchCeiling_overt;
		Pitch_draw (editor->_pitch.data, editor->_pictureGraphics, editor->_startWindow, editor->_endWindow, pitchViewFrom_overt, pitchViewTo_overt,
			prefs.picture.pitch.garnish, GET_INTEGER (L"Speckle"), editor->_pitch.unit);
		editor->garnish ();
		editor->closePraatPicture ();
	EDITOR_END
}

/***** INTENSITY MENU *****/

static int menu_cb_showIntensity (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	preferences.intensity.show = editor->_intensity.show = ! editor->_intensity.show;
	GuiMenuItem_check (editor->_intensityToggle, editor->_intensity.show);   // in case we're called from a script
	editor->redraw ();
	return 1;
}

static int menu_cb_intensitySettings (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Intensity settings", L"Intro 6.2. Configuring the intensity contour")
		REAL (L"left View range (dB)", L"50.0")
		REAL (L"right View range (dB)", L"100.0")
		RADIO_ENUM (L"Averaging method", kTimeSoundAnalysisEditor_intensity_averagingMethod, MEAN_ENERGY)
		BOOLEAN (L"Subtract mean pressure", 1)
		LABEL (L"", L"Note: the pitch floor is taken from the pitch settings.")
		LABEL (L"note2", L"")
	EDITOR_OK
		SET_REAL (L"left View range", editor->_intensity.viewFrom)
		SET_REAL (L"right View range", editor->_intensity.viewTo)
		SET_ENUM (L"Averaging method", kTimeSoundAnalysisEditor_intensity_averagingMethod, editor->_intensity.averagingMethod)
		SET_INTEGER (L"Subtract mean pressure", editor->_intensity.subtractMeanPressure)
		if (editor->_timeStepStrategy != kTimeSoundAnalysisEditor_timeStepStrategy_DEFAULT) {
			SET_STRING (L"note2", L"Warning: you have a non-standard \"time step strategy\".")
		} else {
			SET_STRING (L"note2", L"(your \"time step strategy\" has its standard value: automatic)")
		}
	EDITOR_DO
		preferences.intensity.viewFrom = editor->_intensity.viewFrom = GET_REAL (L"left View range");
		preferences.intensity.viewTo = editor->_intensity.viewTo = GET_REAL (L"right View range");
		preferences.intensity.averagingMethod = editor->_intensity.averagingMethod = GET_ENUM (kTimeSoundAnalysisEditor_intensity_averagingMethod, L"Averaging method");
		preferences.intensity.subtractMeanPressure = editor->_intensity.subtractMeanPressure = GET_INTEGER (L"Subtract mean pressure");
		forget (editor->_intensity.data);
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_extractVisibleIntensityContour (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	Intensity publish;
	if (! editor->_intensity.show)
		return Melder_error1 (L"No intensity contour is visible.\nFirst choose \"Show intensity\" from the Intensity menu.");
	if (! editor->_intensity.data) {
		editor->computeIntensity ();
		if (! editor->_intensity.data) return Melder_error1 (theMessage_Cannot_compute_intensity);
	}
	publish = (Intensity) Data_copy (editor->_intensity.data);
	if (! publish) return 0;
	if (editor->_publishCallback)
		editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

static int menu_cb_drawVisibleIntensityContour (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Draw visible intensity contour", 0)
		editor->form_pictureWindow (cmd);
		editor->form_pictureMargins (cmd);
		editor->form_pictureSelection (cmd);
		BOOLEAN (L"Garnish", 1);
	EDITOR_OK
		editor->ok_pictureWindow (cmd);
		editor->ok_pictureMargins (cmd);
		editor->ok_pictureSelection (cmd);
		SET_INTEGER (L"Garnish", prefs.picture.intensity.garnish);
	EDITOR_DO
		editor->do_pictureWindow (cmd);
		editor->do_pictureMargins (cmd);
		editor->do_pictureSelection (cmd);
		prefs.picture.intensity.garnish = GET_INTEGER (L"Garnish");
		if (! editor->_intensity.show)
			return Melder_error1 (L"No intensity contour is visible.\nFirst choose \"Show intensity\" from the Intensity menu.");
		if (! editor->_intensity.data) {
			editor->computeIntensity ();
			if (! editor->_intensity.data) return Melder_error1 (theMessage_Cannot_compute_intensity);
		}
		editor->openPraatPicture ();
		Intensity_draw (editor->_intensity.data, editor->_pictureGraphics, editor->_startWindow, editor->_endWindow, editor->_intensity.viewFrom, editor->_intensity.viewTo,
			prefs.picture.intensity.garnish);
		editor->garnish ();
		editor->closePraatPicture ();
	EDITOR_END
}

static int menu_cb_intensityListing (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	double tmin, tmax;
	int part = editor->makeQueriable (TRUE, & tmin, & tmax); iferror return 0;
	if (! editor->_intensity.show)
		return Melder_error1 (L"No intensity contour is visible.\nFirst choose \"Show intensity\" from the Intensity menu.");
	if (! editor->_intensity.data) {
		editor->computeIntensity ();
		if (! editor->_intensity.data) return Melder_error1 (theMessage_Cannot_compute_intensity);
	}
	MelderInfo_open ();
	MelderInfo_writeLine1 (L"Time_s   Intensity_dB");
	if (part == FunctionEditor_PART_CURSOR) {
		double intensity = Vector_getValueAtX (editor->_intensity.data, tmin, Vector_CHANNEL_1, Vector_VALUE_INTERPOLATION_LINEAR);
		MelderInfo_writeLine3 (Melder_fixed (tmin, 6), L"   ", Melder_fixed (intensity, 6));
	} else {
		long i, i1, i2;
		Sampled_getWindowSamples (editor->_intensity.data, tmin, tmax, & i1, & i2);
		for (i = i1; i <= i2; i ++) {
			double t = Sampled_indexToX (editor->_intensity.data, i);
			double intensity = Vector_getValueAtX (editor->_intensity.data, t, Vector_CHANNEL_1, Vector_VALUE_INTERPOLATION_NEAREST);
			MelderInfo_writeLine3 (Melder_fixed (t, 6), L"   ", Melder_fixed (intensity, 6));
		}
	}
	MelderInfo_close ();
	return 1;
}

static int menu_cb_getIntensity (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	double tmin, tmax;
	int part = editor->makeQueriable (TRUE, & tmin, & tmax); iferror return 0;
	if (! editor->_intensity.show)
		return Melder_error1 (L"No intensity contour is visible.\nFirst choose \"Show intensity\" from the Intensity menu.");
	if (! editor->_intensity.data) {
		editor->computeIntensity ();
		if (! editor->_intensity.data) return Melder_error1 (theMessage_Cannot_compute_intensity);
	}
	if (part == FunctionEditor_PART_CURSOR) {
		Melder_information2 (Melder_double (Vector_getValueAtX (editor->_intensity.data, tmin, Vector_CHANNEL_1, Vector_VALUE_INTERPOLATION_LINEAR)), L" dB (intensity at CURSOR)");
	} else {
		static const wchar_t *methodString [] = { L"median", L"mean-energy", L"mean-sones", L"mean-dB" };
		Melder_information6 (Melder_double (Intensity_getAverage (editor->_intensity.data, tmin, tmax, editor->_intensity.averagingMethod)),
			L" dB (", methodString [editor->_intensity.averagingMethod], L" intensity ", FunctionEditor_partString_locative (part), L")");
	}
	return 1;
}

static int menu_cb_getMinimumIntensity (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	double tmin, tmax;
	int part = editor->makeQueriable (FALSE, & tmin, & tmax); iferror return 0;
	if (! editor->_intensity.show)
		return Melder_error1 (L"No intensity contour is visible.\nFirst choose \"Show intensity\" from the Intensity menu.");
	if (! editor->_intensity.data) {
		editor->computeIntensity ();
		if (! editor->_intensity.data) return Melder_error1 (theMessage_Cannot_compute_intensity);
	}
	double intensity = Vector_getMinimum (editor->_intensity.data, tmin, tmax, NUM_PEAK_INTERPOLATE_PARABOLIC);
	Melder_information4 (Melder_double (intensity), L" dB (minimum intensity ", FunctionEditor_partString_locative (part), L")");
	return 1;
}

static int menu_cb_getMaximumIntensity (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	double tmin, tmax;
	int part = editor->makeQueriable (FALSE, & tmin, & tmax); iferror return 0;
	if (! editor->_intensity.show)
		return Melder_error1 (L"No intensity contour is visible.\nFirst choose \"Show intensity\" from the Intensity menu.");
	if (! editor->_intensity.data) {
		editor->computeIntensity ();
		if (! editor->_intensity.data) return Melder_error1 (theMessage_Cannot_compute_intensity);
	}
	double intensity = Vector_getMaximum (editor->_intensity.data, tmin, tmax, NUM_PEAK_INTERPOLATE_PARABOLIC);
	Melder_information4 (Melder_double (intensity), L" dB (maximum intensity ", FunctionEditor_partString_locative (part), L")");
	return 1;
}

/***** FORMANT MENU *****/

static int menu_cb_showFormants (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	preferences.formant.show = editor->_formant.show = ! editor->_formant.show;
	GuiMenuItem_check (editor->_formantToggle, editor->_formant.show);   // in case we're called from a script
	editor->redraw ();
	return 1;
}

static int menu_cb_formantSettings (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Formant settings", L"Intro 5.2. Configuring the formant contours")
		POSITIVE (L"Maximum formant (Hz)", L"5500.0")
		POSITIVE (L"Number of formants", L"5.0")
		POSITIVE (L"Window length (s)", L"0.025")
		REAL (L"Dynamic range (dB)", L"30.0")
		POSITIVE (L"Dot size (mm)", L"1.0")
		LABEL (L"note1", L"")
		LABEL (L"note2", L"")
	EDITOR_OK
		SET_REAL (L"Maximum formant", editor->_formant.maximumFormant)
		SET_REAL (L"Number of formants", 0.5 * editor->_formant.numberOfPoles)
		SET_REAL (L"Window length", editor->_formant.windowLength)
		SET_REAL (L"Dynamic range", editor->_formant.dynamicRange)
		SET_REAL (L"Dot size", editor->_formant.dotSize)
		if (editor->_formant.method != 1 || editor->_formant.preemphasisFrom != 50.0) {
			SET_STRING (L"note1", L"Warning: you have non-standard \"advanced settings\".")
		} else {
			SET_STRING (L"note1", L"(all of your \"advanced settings\" have their standard values)")
		}
		if (editor->_timeStepStrategy != kTimeSoundAnalysisEditor_timeStepStrategy_DEFAULT) {
			SET_STRING (L"note2", L"Warning: you have a non-standard \"time step strategy\".")
		} else {
			SET_STRING (L"note2", L"(your \"time step strategy\" has its standard value: automatic)")
		}
	EDITOR_DO
		preferences.formant.maximumFormant = editor->_formant.maximumFormant = GET_REAL (L"Maximum formant");
		preferences.formant.numberOfPoles = editor->_formant.numberOfPoles = 2.0 * GET_REAL (L"Number of formants");
		preferences.formant.windowLength = editor->_formant.windowLength = GET_REAL (L"Window length");
		preferences.formant.dynamicRange = editor->_formant.dynamicRange = GET_REAL (L"Dynamic range");
		preferences.formant.dotSize = editor->_formant.dotSize = GET_REAL (L"Dot size");
		forget (editor->_formant.data);
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_advancedFormantSettings (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Advanced formant settings", L"Advanced formant settings...")
		RADIO_ENUM (L"Method", kTimeSoundAnalysisEditor_formant_analysisMethod, BURG)
		POSITIVE (L"Pre-emphasis from (Hz)", L"50.0")
	EDITOR_OK
		SET_ENUM (L"Method", kTimeSoundAnalysisEditor_formant_analysisMethod, editor->_formant.method)
		SET_REAL (L"Pre-emphasis from", editor->_formant.preemphasisFrom)
	EDITOR_DO
		preferences.formant.method = editor->_formant.method = GET_ENUM (kTimeSoundAnalysisEditor_formant_analysisMethod, L"Method");
		preferences.formant.preemphasisFrom = editor->_formant.preemphasisFrom = GET_REAL (L"Pre-emphasis from");
		forget (editor->_formant.data);
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_extractVisibleFormantContour (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	Formant publish;
	if (! editor->_formant.show)
		return Melder_error1 (L"No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
	if (! editor->_formant.data) {
		editor->computeFormants ();
		if (! editor->_formant.data) return Melder_error1 (theMessage_Cannot_compute_formant);
	}
	publish = (Formant) Data_copy (editor->_formant.data);
	if (! publish) return 0;
	if (editor->_publishCallback)
		editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

static int menu_cb_drawVisibleFormantContour (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Draw visible formant contour", 0)
		editor->form_pictureWindow (cmd);
		editor->form_pictureMargins (cmd);
		editor->form_pictureSelection (cmd);
		BOOLEAN (L"Garnish", 1);
	EDITOR_OK
		editor->ok_pictureWindow (cmd);
		editor->ok_pictureMargins (cmd);
		editor->ok_pictureSelection (cmd);
		SET_INTEGER (L"Garnish", prefs.picture.formant.garnish);
	EDITOR_DO
		editor->do_pictureWindow (cmd);
		editor->do_pictureMargins (cmd);
		editor->do_pictureSelection (cmd);
		prefs.picture.formant.garnish = GET_INTEGER (L"Garnish");
		if (! editor->_formant.show)
			return Melder_error1 (L"No formant contour is visible.\nFirst choose \"Show formant\" from the Formant menu.");
		if (! editor->_formant.data) {
			editor->computeFormants ();
			if (! editor->_formant.data) return Melder_error1 (theMessage_Cannot_compute_formant);
		}
		editor->openPraatPicture ();
		Formant_drawSpeckles (editor->_formant.data, editor->_pictureGraphics, editor->_startWindow, editor->_endWindow,
			editor->_spectrogram.viewTo, editor->_formant.dynamicRange,
			prefs.picture.formant.garnish);
		editor->garnish ();
		editor->closePraatPicture ();
	EDITOR_END
}

static int menu_cb_formantListing (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	double tmin, tmax;
	int part = editor->makeQueriable (TRUE, & tmin, & tmax); iferror return 0;
	if (! editor->_formant.show)
		return Melder_error1 (L"No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
	if (! editor->_formant.data) {
		editor->computeFormants ();
		if (! editor->_formant.data) return Melder_error1 (theMessage_Cannot_compute_formant);
	}
	MelderInfo_open ();
	MelderInfo_writeLine1 (L"Time_s   F1_Hz   F2_Hz   F3_Hz   F4_Hz");
	if (part == FunctionEditor_PART_CURSOR) {
		double f1 = Formant_getValueAtTime (editor->_formant.data, 1, tmin, 0);
		double f2 = Formant_getValueAtTime (editor->_formant.data, 2, tmin, 0);
		double f3 = Formant_getValueAtTime (editor->_formant.data, 3, tmin, 0);
		double f4 = Formant_getValueAtTime (editor->_formant.data, 4, tmin, 0);
		MelderInfo_write5 (Melder_fixed (tmin, 6), L"   ", Melder_fixed (f1, 6), L"   ", Melder_fixed (f2, 6));
		MelderInfo_writeLine4 (L"   ", Melder_fixed (f3, 6), L"   ", Melder_fixed (f4, 6));
	} else {
		long i, i1, i2;
		Sampled_getWindowSamples (editor->_formant.data, tmin, tmax, & i1, & i2);
		for (i = i1; i <= i2; i ++) {
			double t = Sampled_indexToX (editor->_formant.data, i);
			double f1 = Formant_getValueAtTime (editor->_formant.data, 1, t, 0);
			double f2 = Formant_getValueAtTime (editor->_formant.data, 2, t, 0);
			double f3 = Formant_getValueAtTime (editor->_formant.data, 3, t, 0);
			double f4 = Formant_getValueAtTime (editor->_formant.data, 4, t, 0);
			MelderInfo_write5 (Melder_fixed (t, 6), L"   ", Melder_fixed (f1, 6), L"   ", Melder_fixed (f2, 6));
			MelderInfo_writeLine4 (L"   ", Melder_fixed (f3, 6), L"   ", Melder_fixed (f4, 6));
		}
	}
	MelderInfo_close ();
	return 1;
}

int TimeSoundAnalysisEditor::do_getFormant (int iformant) {
	double tmin, tmax;
	int part = makeQueriable (TRUE, & tmin, & tmax); iferror return 0;
	if (! _formant.show)
		return Melder_error1 (L"No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
	if (! _formant.data) {
		computeFormants ();
		if (! _formant.data) return Melder_error1 (theMessage_Cannot_compute_formant);
	}
	if (part == FunctionEditor_PART_CURSOR) {
		Melder_information4 (Melder_double (Formant_getValueAtTime (_formant.data, iformant, tmin, 0)),
			L" Hertz (nearest F", Melder_integer (iformant), L" to CURSOR)");
	} else {
		Melder_information6 (Melder_double (Formant_getMean (_formant.data, iformant, tmin, tmax, 0)),
			L" Hertz (mean F", Melder_integer (iformant), L" ", FunctionEditor_partString_locative (part), L")");
	}
	return 1;
}
int TimeSoundAnalysisEditor::do_getBandwidth (int iformant) {
	double tmin, tmax;
	int part = makeQueriable (TRUE, & tmin, & tmax); iferror return 0;
	if (! _formant.show)
		return Melder_error1 (L"No formant contour is visible.\nFirst choose \"Show formants\" from the Formant menu.");
	if (! _formant.data) {
		computeFormants ();
		if (! _formant.data) return Melder_error1 (theMessage_Cannot_compute_formant);
	}
	if (part == FunctionEditor_PART_CURSOR) {
		Melder_information4 (Melder_double (Formant_getBandwidthAtTime (_formant.data, iformant, tmin, 0)),
			L" Hertz (nearest B", Melder_integer (iformant), L" to CURSOR)");
	} else {
		Melder_information6 (Melder_double (Formant_getBandwidthAtTime (_formant.data, iformant, 0.5 * (tmin + tmax), 0)),
			L" Hertz (B", Melder_integer (iformant), L" in centre of ", FunctionEditor_partString (part), L")");
	}
	return 1;
}
static int menu_cb_getFirstFormant (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_getFormant (1); }
static int menu_cb_getFirstBandwidth (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_getBandwidth (1); }
static int menu_cb_getSecondFormant (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_getFormant (2); }
static int menu_cb_getSecondBandwidth (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_getBandwidth (2); }
static int menu_cb_getThirdFormant (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_getFormant (3); }
static int menu_cb_getThirdBandwidth (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_getBandwidth (3); }
static int menu_cb_getFourthFormant (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_getFormant (4); }
static int menu_cb_getFourthBandwidth (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me; return editor->do_getBandwidth (4); }

static int menu_cb_getFormant (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Get formant", 0)
		NATURAL (L"Formant number", L"5")
	EDITOR_OK
	EDITOR_DO
		if (! editor->do_getFormant (GET_INTEGER (L"Formant number"))) return 0;
	EDITOR_END
}

static int menu_cb_getBandwidth (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Get bandwidth", 0)
		NATURAL (L"Formant number", L"5")
	EDITOR_OK
	EDITOR_DO
		if (! editor->do_getBandwidth (GET_INTEGER (L"Formant number"))) return 0;
	EDITOR_END
}

/***** PULSE MENU *****/

static int menu_cb_showPulses (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	preferences.pulses.show = editor->_pulses.show = ! editor->_pulses.show;
	GuiMenuItem_check (editor->_pulsesToggle, editor->_pulses.show);   // in case we're called from a script
	editor->redraw ();
	return 1;
}

static int menu_cb_advancedPulsesSettings (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Advanced pulses settings", L"Advanced pulses settings...")
		POSITIVE (L"Maximum period factor", L"1.3")
		POSITIVE (L"Maximum amplitude factor", L"1.6")
	EDITOR_OK
		SET_REAL (L"Maximum period factor", editor->_pulses.maximumPeriodFactor)
		SET_REAL (L"Maximum amplitude factor", editor->_pulses.maximumAmplitudeFactor)
	EDITOR_DO
		preferences.pulses.maximumPeriodFactor = editor->_pulses.maximumPeriodFactor = GET_REAL (L"Maximum period factor");
		preferences.pulses.maximumAmplitudeFactor = editor->_pulses.maximumAmplitudeFactor = GET_REAL (L"Maximum amplitude factor");
		forget (editor->_pulses.data);
		editor->redraw ();
	EDITOR_END
}

static int menu_cb_extractVisiblePulses (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	Pitch publish;
	if (! editor->_pulses.show)
		return Melder_error1 (L"No pulses are visible.\nFirst choose \"Show pulses\" from the Pulses menu.");
	if (! editor->_pulses.data) {
		editor->computePulses ();
		if (! editor->_pulses.data) return Melder_error1 (theMessage_Cannot_compute_pulses);
	}
	publish = (Pitch) Data_copy (editor->_pulses.data);
	if (! publish) return 0;
	if (editor->_publishCallback)
		editor->_publishCallback (editor, editor->_publishClosure, publish);
	return 1;
}

static int menu_cb_drawVisiblePulses (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	EDITOR_FORM (L"Draw visible pulses", 0)
		editor->form_pictureWindow (cmd);
		editor->form_pictureMargins (cmd);
		editor->form_pictureSelection (cmd);
		BOOLEAN (L"Garnish", 1);
	EDITOR_OK
		editor->ok_pictureWindow (cmd);
		editor->ok_pictureMargins (cmd);
		editor->ok_pictureSelection (cmd);
		SET_INTEGER (L"Garnish", prefs.picture.pulses.garnish);
	EDITOR_DO
		editor->do_pictureWindow (cmd);
		editor->do_pictureMargins (cmd);
		editor->do_pictureSelection (cmd);
		prefs.picture.pulses.garnish = GET_INTEGER (L"Garnish");
		if (! editor->_pulses.show)
			return Melder_error1 (L"No pulses are visible.\nFirst choose \"Show pulses\" from the Pulses menu.");
		if (! editor->_pulses.data) {
			editor->computePulses ();
			if (! editor->_pulses.data) return Melder_error1 (theMessage_Cannot_compute_pulses);
		}
		editor->openPraatPicture ();
		PointProcess_draw (editor->_pulses.data, editor->_pictureGraphics, editor->_startWindow, editor->_endWindow,
			prefs.picture.pulses.garnish);
		editor->garnish ();
		editor->closePraatPicture ();
	EDITOR_END
}

static int menu_cb_voiceReport (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	time_t today = time (NULL);
	Sound sound = NULL;
	double tmin, tmax;
	int part = editor->makeQueriable (FALSE, & tmin, & tmax); iferror return 0;
	if (! editor->_pulses.show)
		return Melder_error1 (L"No pulses are visible.\nFirst choose \"Show pulses\" from the Pulses menu.");
	if (! editor->_pulses.data) {
		editor->computePulses ();
		if (! editor->_pulses.data) return Melder_error1 (theMessage_Cannot_compute_pulses);
	}
	sound = editor->extractSound (tmin, tmax);
	if (! sound) return Melder_error1 (L"Selection too small (or out of memory).");
	MelderInfo_open ();
	MelderInfo_writeLine4 (L"-- Voice report for ", editor->_name, L" --\nDate: ", Melder_peekUtf8ToWcs (ctime (& today)));
	if ((editor->_pitch.method & 2) == 0)
		MelderInfo_writeLine1 (L"WARNING: some of the following measurements may be imprecise.\n"
			"For more precision, go to \"Pitch settings\" and choose \"Optimize for voice analysis\".\n");
	MelderInfo_writeLine2 (L"Time range of ", FunctionEditor_partString (part));
	Sound_Pitch_PointProcess_voiceReport (sound, editor->_pitch.data, editor->_pulses.data, tmin, tmax,
		editor->_pitch.floor, editor->_pitch.ceiling, editor->_pulses.maximumPeriodFactor, editor->_pulses.maximumAmplitudeFactor, editor->_pitch.silenceThreshold, editor->_pitch.voicingThreshold);
	MelderInfo_close ();
	forget (sound);
	return 1;
}

static int menu_cb_pulseListing (EDITOR_ARGS) {
	TimeSoundAnalysisEditor *editor = (TimeSoundAnalysisEditor *)editor_me;
	long i, i1, i2;
	double tmin, tmax;
	editor->makeQueriable (FALSE, & tmin, & tmax); iferror return 0;
	if (! editor->_pulses.show)
		return Melder_error1 (L"No pulses are visible.\nFirst choose \"Show pulses\" from the Pulses menu.");
	if (! editor->_pulses.data) {
		editor->computePulses ();
		if (! editor->_pulses.data) return Melder_error1 (theMessage_Cannot_compute_pulses);
	}
	MelderInfo_open ();
	MelderInfo_writeLine1 (L"Time_s");
	i1 = PointProcess_getHighIndex (editor->_pulses.data, tmin);
	i2 = PointProcess_getLowIndex (editor->_pulses.data, tmax);
	for (i = i1; i <= i2; i ++) {
		double t = editor->_pulses.data -> t [i];
		MelderInfo_writeLine1 (Melder_fixed (t, 12));
	}
	MelderInfo_close ();
	return 1;
}

/*
static int cb_getJitter_xx (double (*PointProcess_getJitter_xx) (PointProcess, double, double, double, double, double)) {
	double minimumPeriod = 0.8 / _pitch.ceiling, maximumPeriod = 1.25 / _pitch.floor;
	if (! _pulses.show)
		return Melder_error1 (L"No pulses are visible.\nFirst choose \"Show pulses\" from the Pulses menu.");
	if (! _pulses.data) {
		computePulses (me);
		if (! _pulses.data) return Melder_error1 (theMessage_Cannot_compute_pulses);
	}
	if (_startSelection == _endSelection)
		return Melder_error1 (L"Make a selection first.");
	if (! queriable (me)) return 0;
	Melder_informationReal (PointProcess_getJitter_xx (_pulses.data, _startSelection, _endSelection,
		minimumPeriod, maximumPeriod, _pulses.maximumPeriodFactor), NULL);
	return 1;
}
DIRECT (TimeSoundAnalysisEditor, cb_getJitter_local) if (! cb_getJitter_xx (me, PointProcess_getJitter_local)) return 0; END
DIRECT (TimeSoundAnalysisEditor, cb_getJitter_local_absolute) if (! cb_getJitter_xx (me, PointProcess_getJitter_local_absolute)) return 0; END
DIRECT (TimeSoundAnalysisEditor, cb_getJitter_rap) if (! cb_getJitter_xx (me, PointProcess_getJitter_rap)) return 0; END
DIRECT (TimeSoundAnalysisEditor, cb_getJitter_ppq5) if (! cb_getJitter_xx (me, PointProcess_getJitter_ppq5)) return 0; END
DIRECT (TimeSoundAnalysisEditor, cb_getJitter_ddp) if (! cb_getJitter_xx (me, PointProcess_getJitter_ddp)) return 0; END

static int cb_getShimmer_xx (double (*PointProcess_Sound_getShimmer_xx) (PointProcess, Sound, double, double, double, double, double)) {
	Sound sound = NULL;
	double minimumPeriod = 0.8 / _pitch.ceiling, maximumPeriod = 1.25 / _pitch.floor;
	if (! _pulses.show)
		return Melder_error1 (L"No pulses are visible.\nFirst choose \"Show pulses\" from the Pulses menu.");
	if (! _pulses.data) {
		computePulses (me);
		if (! _pulses.data) return Melder_error1 (theMessage_Cannot_compute_pulses);
	}
	if (_startSelection == _endSelection)
		return Melder_error1 (L"Make a selection first.");
	if (! queriable (me)) return 0;
	sound = extractSound (me, _startSelection, _endSelection);
	if (! sound) return Melder_error1 (L"Selection too small (or out of memory).");
	Melder_informationReal (PointProcess_Sound_getShimmer_xx (_pulses.data, sound, _startSelection, _endSelection,
		minimumPeriod, maximumPeriod, _pulses.maximumAmplitudeFactor), NULL);
	forget (sound);
	return 1;
}
DIRECT (TimeSoundAnalysisEditor, cb_getShimmer_local) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_local)) return 0; END
DIRECT (TimeSoundAnalysisEditor, cb_getShimmer_local_dB) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_local_dB)) return 0; END
DIRECT (TimeSoundAnalysisEditor, cb_getShimmer_apq3) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_apq3)) return 0; END
DIRECT (TimeSoundAnalysisEditor, cb_getShimmer_apq5) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_apq5)) return 0; END
DIRECT (TimeSoundAnalysisEditor, cb_getShimmer_apq11) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_apq11)) return 0; END
DIRECT (TimeSoundAnalysisEditor, cb_getShimmer_dda) if (! cb_getShimmer_xx (me, PointProcess_Sound_getShimmer_dda)) return 0; END
*/

void TimeSoundAnalysisEditor::createMenuItems_view_sound (EditorMenu *menu) {
	createMenuItems_view_sound_analysis (menu);
}

void TimeSoundAnalysisEditor::createMenuItems_view_sound_analysis (EditorMenu *menu) {
	menu->addCommand (L"Analysis window:", GuiMenu_INSENSITIVE, menu_cb_showAnalyses);
	menu->addCommand (L"Show analyses...", 0, menu_cb_showAnalyses);
	menu->addCommand (L"Time step settings...", 0, menu_cb_timeStepSettings);
	menu->addCommand (L"-- sound analysis --", 0, 0);
}

void TimeSoundAnalysisEditor::createMenuItems_query (EditorMenu *menu) {
	if (_sound.data || _longSound.data) {
		createMenuItems_query_log (menu);
	}
}

void TimeSoundAnalysisEditor::createMenuItems_query_log (EditorMenu *menu) {
	menu->addCommand (L"-- query log --", 0, NULL);
	menu->addCommand (L"Log settings...", 0, menu_cb_logSettings);
	menu->addCommand (L"Delete log file 1", 0, menu_cb_deleteLogFile1);
	menu->addCommand (L"Delete log file 2", 0, menu_cb_deleteLogFile2);
	menu->addCommand (L"Log 1", GuiMenu_F12, menu_cb_log1);
	menu->addCommand (L"Log 2", GuiMenu_F12 + GuiMenu_SHIFT, menu_cb_log2);
	menu->addCommand (L"Log script 3 (...)", GuiMenu_F12 + GuiMenu_OPTION, menu_cb_logScript3);
	menu->addCommand (L"Log script 4 (...)", GuiMenu_F12 + GuiMenu_COMMAND, menu_cb_logScript4);
}

void TimeSoundAnalysisEditor::createMenus_analysis () {
	EditorMenu *menu;

	menu = addMenu (L"Spectrum", 0);
	_spectrogramToggle = menu->addCommand (L"Show spectrogram",
		GuiMenu_CHECKBUTTON | (preferences.spectrogram.show ? GuiMenu_TOGGLE_ON : 0), menu_cb_showSpectrogram);
	menu->addCommand (L"Spectrogram settings...", 0, menu_cb_spectrogramSettings);
	menu->addCommand (L"Advanced spectrogram settings...", 0, menu_cb_advancedSpectrogramSettings);
	menu->addCommand (L"-- spectrum query --", 0, NULL);
	menu->addCommand (L"Query:", GuiMenu_INSENSITIVE, menu_cb_getFrequency /* dummy */);
	menu->addCommand (L"Get frequency at frequency cursor", 0, menu_cb_getFrequency);
	menu->addCommand (L"Get spectral power at cursor cross", GuiMenu_F7, menu_cb_getSpectralPowerAtCursorCross);
	menu->addCommand (L"-- spectrum select --", 0, NULL);
	menu->addCommand (L"Select:", GuiMenu_INSENSITIVE, menu_cb_moveFrequencyCursorTo/* dummy */);
	menu->addCommand (L"Move frequency cursor to...", 0, menu_cb_moveFrequencyCursorTo);
	createMenuItems_spectrum_picture (menu);
	menu->addCommand (L"-- spectrum extract --", 0, NULL);
	menu->addCommand (L"Extract to objects window:", GuiMenu_INSENSITIVE, menu_cb_extractVisibleSpectrogram /* dummy */);
	menu->addCommand (L"Extract visible spectrogram", 0, menu_cb_extractVisibleSpectrogram);
	menu->addCommand (L"View spectral slice", 'L', menu_cb_viewSpectralSlice);

	menu = addMenu (L"Pitch", 0);
	_pitchToggle = menu->addCommand (L"Show pitch",
		GuiMenu_CHECKBUTTON | (preferences.pitch.show ? GuiMenu_TOGGLE_ON : 0), menu_cb_showPitch);
	menu->addCommand (L"Pitch settings...", 0, menu_cb_pitchSettings);
	menu->addCommand (L"Advanced pitch settings...", 0, menu_cb_advancedPitchSettings);
	menu->addCommand (L"-- pitch query --", 0, NULL);
	menu->addCommand (L"Query:", GuiMenu_INSENSITIVE, menu_cb_getFrequency /* dummy */);
	menu->addCommand (L"Pitch listing", 0, menu_cb_pitchListing);
	menu->addCommand (L"Get pitch", GuiMenu_F5, menu_cb_getPitch);
	menu->addCommand (L"Get minimum pitch", GuiMenu_F5 + GuiMenu_COMMAND, menu_cb_getMinimumPitch);
	menu->addCommand (L"Get maximum pitch", GuiMenu_F5 + GuiMenu_SHIFT, menu_cb_getMaximumPitch);
	menu->addCommand (L"-- pitch select --", 0, NULL);
	menu->addCommand (L"Select:", GuiMenu_INSENSITIVE, menu_cb_moveCursorToMinimumPitch /* dummy */);
	menu->addCommand (L"Move cursor to minimum pitch", GuiMenu_COMMAND + GuiMenu_SHIFT + 'L', menu_cb_moveCursorToMinimumPitch);
	menu->addCommand (L"Move cursor to maximum pitch", GuiMenu_COMMAND + GuiMenu_SHIFT + 'H', menu_cb_moveCursorToMaximumPitch);
	createMenuItems_pitch_picture (menu);
	menu->addCommand (L"-- pitch extract --", 0, NULL);
	menu->addCommand (L"Extract to objects window:", GuiMenu_INSENSITIVE, menu_cb_extractVisiblePitchContour /* dummy */);
	menu->addCommand (L"Extract visible pitch contour", 0, menu_cb_extractVisiblePitchContour);

	menu = addMenu (L"Intensity", 0);
	_intensityToggle = menu->addCommand (L"Show intensity",
		GuiMenu_CHECKBUTTON | (preferences.intensity.show ? GuiMenu_TOGGLE_ON : 0), menu_cb_showIntensity);
	menu->addCommand (L"Intensity settings...", 0, menu_cb_intensitySettings);
	menu->addCommand (L"-- intensity query --", 0, NULL);
	menu->addCommand (L"Query:", GuiMenu_INSENSITIVE, menu_cb_getFrequency /* dummy */);
	menu->addCommand (L"Intensity listing", 0, menu_cb_intensityListing);
	menu->addCommand (L"Get intensity", GuiMenu_F8, menu_cb_getIntensity);
	menu->addCommand (L"Get minimum intensity", GuiMenu_F8 + GuiMenu_COMMAND, menu_cb_getMinimumIntensity);
	menu->addCommand (L"Get maximum intensity", GuiMenu_F8 + GuiMenu_SHIFT, menu_cb_getMaximumIntensity);
	createMenuItems_intensity_picture (menu);
	menu->addCommand (L"-- intensity extract --", 0, NULL);
	menu->addCommand (L"Extract to objects window:", GuiMenu_INSENSITIVE, menu_cb_extractVisibleIntensityContour /* dummy */);
	menu->addCommand (L"Extract visible intensity contour", 0, menu_cb_extractVisibleIntensityContour);

	menu = addMenu (L"Formant", 0);
	_formantToggle = menu->addCommand (L"Show formants",
		GuiMenu_CHECKBUTTON | (preferences.formant.show ? GuiMenu_TOGGLE_ON : 0), menu_cb_showFormants);
	menu->addCommand (L"Formant settings...", 0, menu_cb_formantSettings);
	menu->addCommand (L"Advanced formant settings...", 0, menu_cb_advancedFormantSettings);
	menu->addCommand (L"-- formant query --", 0, NULL);
	menu->addCommand (L"Query:", GuiMenu_INSENSITIVE, menu_cb_getFrequency /* dummy */);
	menu->addCommand (L"Formant listing", 0, menu_cb_formantListing);
	menu->addCommand (L"Get first formant", GuiMenu_F1, menu_cb_getFirstFormant);
	menu->addCommand (L"Get first bandwidth", 0, menu_cb_getFirstBandwidth);
	menu->addCommand (L"Get second formant", GuiMenu_F2, menu_cb_getSecondFormant);
	menu->addCommand (L"Get second bandwidth", 0, menu_cb_getSecondBandwidth);
	menu->addCommand (L"Get third formant", GuiMenu_F3, menu_cb_getThirdFormant);
	menu->addCommand (L"Get third bandwidth", 0, menu_cb_getThirdBandwidth);
	menu->addCommand (L"Get fourth formant", GuiMenu_F4, menu_cb_getFourthFormant);
	menu->addCommand (L"Get fourth bandwidth", 0, menu_cb_getFourthBandwidth);
	menu->addCommand (L"Get formant...", 0, menu_cb_getFormant);
	menu->addCommand (L"Get bandwidth...", 0, menu_cb_getBandwidth);
	createMenuItems_formant_picture (menu);
	menu->addCommand (L"-- formant extract --", 0, NULL);
	menu->addCommand (L"Extract to objects window:", GuiMenu_INSENSITIVE, menu_cb_extractVisibleFormantContour /* dummy */);
	menu->addCommand (L"Extract visible formant contour", 0, menu_cb_extractVisibleFormantContour);

	menu = addMenu (L"Pulses", 0);
	_pulsesToggle = menu->addCommand (L"Show pulses",
		GuiMenu_CHECKBUTTON | (preferences.pulses.show ? GuiMenu_TOGGLE_ON : 0), menu_cb_showPulses);
	menu->addCommand (L"Advanced pulses settings...", 0, menu_cb_advancedPulsesSettings);
	menu->addCommand (L"-- pulses query --", 0, NULL);
	menu->addCommand (L"Query:", GuiMenu_INSENSITIVE, menu_cb_getFrequency /* dummy */);
	menu->addCommand (L"Voice report", 0, menu_cb_voiceReport);
	menu->addCommand (L"Pulse listing", 0, menu_cb_pulseListing);
	/*
	menu->addCommand (L"Get jitter (local)", 0, cb_getJitter_local);
	menu->addCommand (L"Get jitter (local, absolute)", 0, cb_getJitter_local_absolute);
	menu->addCommand (L"Get jitter (rap)", 0, cb_getJitter_rap);
	menu->addCommand (L"Get jitter (ppq5)", 0, cb_getJitter_ppq5);
	menu->addCommand (L"Get jitter (ddp)", 0, cb_getJitter_ddp);
	menu->addCommand (L"Get shimmer (local)", 0, cb_getShimmer_local);
	menu->addCommand (L"Get shimmer (local_dB)", 0, cb_getShimmer_local_dB);
	menu->addCommand (L"Get shimmer (apq3)", 0, cb_getShimmer_apq3);
	menu->addCommand (L"Get shimmer (apq5)", 0, cb_getShimmer_apq5);
	menu->addCommand (L"Get shimmer (apq11)", 0, cb_getShimmer_apq11);
	menu->addCommand (L"Get shimmer (dda)", 0, cb_getShimmer_dda);
	*/
	createMenuItems_pulses_picture (menu);
	menu->addCommand (L"-- pulses extract --", 0, NULL);
	menu->addCommand (L"Extract to objects window:", GuiMenu_INSENSITIVE, menu_cb_extractVisiblePulses /* dummy */);
	menu->addCommand (L"Extract visible pulses", 0, menu_cb_extractVisiblePulses);
}

void TimeSoundAnalysisEditor::createMenuItems_spectrum_picture (EditorMenu *menu) {
	menu->addCommand (L"-- spectrum draw --", 0, NULL);
	menu->addCommand (L"Draw to picture window:", GuiMenu_INSENSITIVE, menu_cb_paintVisibleSpectrogram /* dummy */);
	menu->addCommand (L"Paint visible spectrogram...", 0, menu_cb_paintVisibleSpectrogram);
}

void TimeSoundAnalysisEditor::createMenuItems_pitch_picture (EditorMenu *menu) {
	menu->addCommand (L"-- pitch draw --", 0, NULL);
	menu->addCommand (L"Draw to picture window:", GuiMenu_INSENSITIVE, menu_cb_drawVisiblePitchContour /* dummy */);
	menu->addCommand (L"Draw visible pitch contour...", 0, menu_cb_drawVisiblePitchContour);
}

void TimeSoundAnalysisEditor::createMenuItems_intensity_picture (EditorMenu *menu) {
	menu->addCommand (L"-- intensity draw --", 0, NULL);
	menu->addCommand (L"Draw to picture window:", GuiMenu_INSENSITIVE, menu_cb_drawVisibleIntensityContour /* dummy */);
	menu->addCommand (L"Draw visible intensity contour...", 0, menu_cb_drawVisibleIntensityContour);
}

void TimeSoundAnalysisEditor::createMenuItems_formant_picture (EditorMenu *menu) {
	menu->addCommand (L"-- formant draw --", 0, NULL);
	menu->addCommand (L"Draw to picture window:", GuiMenu_INSENSITIVE, menu_cb_drawVisibleFormantContour /* dummy */);
	menu->addCommand (L"Draw visible formant contour...", 0, menu_cb_drawVisibleFormantContour);
}

void TimeSoundAnalysisEditor::createMenuItems_pulses_picture (EditorMenu *menu) {
	menu->addCommand (L"-- pulses draw --", 0, NULL);
	menu->addCommand (L"Draw to picture window:", GuiMenu_INSENSITIVE, menu_cb_drawVisiblePulses /* dummy */);
	menu->addCommand (L"Draw visible pulses...", 0, menu_cb_drawVisiblePulses);
}

void TimeSoundAnalysisEditor::computeSpectrogram () {
	Melder_progressOff ();
	if (_spectrogram.show && _endWindow - _startWindow <= _longestAnalysis &&
		(_spectrogram.data == NULL || _spectrogram.data -> xmin != _startWindow || _spectrogram.data -> xmax != _endWindow))
	{
		Sound sound = NULL;
		double margin = _spectrogram.windowShape == kSound_to_Spectrogram_windowShape_GAUSSIAN ? _spectrogram.windowLength : 0.5 * _spectrogram.windowLength;
		forget (_spectrogram.data);
		sound = extractSound (_startWindow - margin, _endWindow + margin);
		if (sound != NULL) {
			_spectrogram.data = Sound_to_Spectrogram (sound, _spectrogram.windowLength,
				_spectrogram.viewTo, (_endWindow - _startWindow) / _spectrogram.timeSteps,
				_spectrogram.viewTo / _spectrogram.frequencySteps, _spectrogram.windowShape, 8.0, 8.0);
			if (_spectrogram.data != NULL) _spectrogram.data -> xmin = _startWindow, _spectrogram.data -> xmax = _endWindow;
			else Melder_clearError ();
			forget (sound);
		} else Melder_clearError ();
	}
	Melder_progressOn ();
}

void TimeSoundAnalysisEditor::computePitch_inside () {
	Sound sound = NULL;
	double margin = _pitch.veryAccurate ? 3.0 / _pitch.floor : 1.5 / _pitch.floor;
	forget (_pitch.data);
	sound = extractSound (_startWindow - margin, _endWindow + margin);
	if (sound != NULL) {
		double pitchTimeStep =
			_timeStepStrategy == kTimeSoundAnalysisEditor_timeStepStrategy_FIXED ? _fixedTimeStep :
			_timeStepStrategy == kTimeSoundAnalysisEditor_timeStepStrategy_VIEW_DEPENDENT ? (_endWindow - _startWindow) / _numberOfTimeStepsPerView :
			0.0;   /* The default: determined by pitch floor. */
		_pitch.data = Sound_to_Pitch_any (sound, pitchTimeStep,
			_pitch.floor,
			_pitch.method == kTimeSoundAnalysisEditor_pitch_analysisMethod_AUTOCORRELATION ? 3.0 : 1.0,
			_pitch.maximumNumberOfCandidates,
			(_pitch.method - 1) * 2 + _pitch.veryAccurate,
			_pitch.silenceThreshold, _pitch.voicingThreshold,
			_pitch.octaveCost, _pitch.octaveJumpCost, _pitch.voicedUnvoicedCost, _pitch.ceiling);
		if (_pitch.data != NULL) _pitch.data -> xmin = _startWindow, _pitch.data -> xmax = _endWindow;
		else Melder_clearError ();
		forget (sound);
	} else Melder_clearError ();
}

void TimeSoundAnalysisEditor::computePitch () {
	Melder_progressOff ();
	if (_pitch.show && _endWindow - _startWindow <= _longestAnalysis &&
		(_pitch.data == NULL || _pitch.data -> xmin != _startWindow || _pitch.data -> xmax != _endWindow))
	{
		computePitch_inside ();
	}
	Melder_progressOn ();
}

void TimeSoundAnalysisEditor::computeIntensity () {
	Melder_progressOff ();
	if (_intensity.show && _endWindow - _startWindow <= _longestAnalysis &&
		(_intensity.data == NULL || _intensity.data -> xmin != _startWindow || _intensity.data -> xmax != _endWindow))
	{
		Sound sound = NULL;
		double margin = 3.2 / _pitch.floor;
		forget (_intensity.data);
		sound = extractSound (_startWindow - margin, _endWindow + margin);
		if (sound != NULL) {
			_intensity.data = Sound_to_Intensity (sound, _pitch.floor,
				_endWindow - _startWindow > _longestAnalysis ? (_endWindow - _startWindow) / 100 : 0.0,
				_intensity.subtractMeanPressure);
			if (_intensity.data != NULL) _intensity.data -> xmin = _startWindow, _intensity.data -> xmax = _endWindow;
			else Melder_clearError ();
			forget (sound);
		} else Melder_clearError ();
	}
	Melder_progressOn ();
}

void TimeSoundAnalysisEditor::computeFormants () {
	Melder_progressOff ();
	if (_formant.show && _endWindow - _startWindow <= _longestAnalysis &&
		(_formant.data == NULL || _formant.data -> xmin != _startWindow || _formant.data -> xmax != _endWindow))
	{
		Sound sound = NULL;
		double margin = _formant.windowLength;
		forget (_formant.data);
		if (_endWindow - _startWindow > _longestAnalysis)
			sound = extractSound (
				0.5 * (_startWindow + _endWindow - _longestAnalysis) - margin,
				0.5 * (_startWindow + _endWindow + _longestAnalysis) + margin);
		else
			sound = extractSound (_startWindow - margin, _endWindow + margin);
		if (sound != NULL) {
			double formantTimeStep =
				_timeStepStrategy == kTimeSoundAnalysisEditor_timeStepStrategy_FIXED ? _fixedTimeStep :
				_timeStepStrategy == kTimeSoundAnalysisEditor_timeStepStrategy_VIEW_DEPENDENT ? (_endWindow - _startWindow) / _numberOfTimeStepsPerView :
				0.0;   /* The default: determined by analysis window length. */
			_formant.data = Sound_to_Formant_any (sound, formantTimeStep,
				_formant.numberOfPoles, _formant.maximumFormant,
				_formant.windowLength, _formant.method, _formant.preemphasisFrom, 50.0);
			if (_formant.data != NULL) _formant.data -> xmin = _startWindow, _formant.data -> xmax = _endWindow;
			else Melder_clearError ();
			forget (sound);
		} else Melder_clearError ();
	}
	Melder_progressOn ();
}

void TimeSoundAnalysisEditor::computePulses () {
	Melder_progressOff ();
	if (_pulses.show && _endWindow - _startWindow <= _longestAnalysis &&
		(_pulses.data == NULL || _pulses.data -> xmin != _startWindow || _pulses.data -> xmax != _endWindow))
	{
		forget (_pulses.data);   /* 20060912 */
		if (_pitch.data == NULL || _pitch.data -> xmin != _startWindow || _pitch.data -> xmax != _endWindow) {
			computePitch_inside ();
		}
		if (_pitch.data != NULL) {
			Sound sound = NULL;
			/* forget (_pulses.data);   /* 20060912 */
			sound = extractSound (_startWindow, _endWindow);
			if (sound != NULL) {
				_pulses.data = Sound_Pitch_to_PointProcess_cc (sound, _pitch.data);
				if (_pulses.data == NULL) Melder_clearError ();
				forget (sound);
			} else Melder_clearError ();
		} else Melder_clearError ();
	}
	Melder_progressOn ();
}

void TimeSoundAnalysisEditor::draw_analysis () {
	double pitchFloor_hidden = ClassFunction_convertStandardToSpecialUnit (classPitch, _pitch.floor, Pitch_LEVEL_FREQUENCY, _pitch.unit);
	double pitchCeiling_hidden = ClassFunction_convertStandardToSpecialUnit (classPitch, _pitch.ceiling, Pitch_LEVEL_FREQUENCY, _pitch.unit);
	double pitchFloor_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchFloor_hidden, Pitch_LEVEL_FREQUENCY, _pitch.unit);
	double pitchCeiling_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchCeiling_hidden, Pitch_LEVEL_FREQUENCY, _pitch.unit);
	double pitchViewFrom_overt = _pitch.viewFrom < _pitch.viewTo ? _pitch.viewFrom : pitchFloor_overt;
	double pitchViewTo_overt = _pitch.viewFrom < _pitch.viewTo ? _pitch.viewTo : pitchCeiling_overt;
	double pitchViewFrom_hidden = ClassFunction_isUnitLogarithmic (classPitch, Pitch_LEVEL_FREQUENCY, _pitch.unit) ? log10 (pitchViewFrom_overt) : pitchViewFrom_overt;
	double pitchViewTo_hidden = ClassFunction_isUnitLogarithmic (classPitch, Pitch_LEVEL_FREQUENCY, _pitch.unit) ? log10 (pitchViewTo_overt) : pitchViewTo_overt;

	Graphics_setWindow (_graphics, 0.0, 1.0, 0.0, 1.0);
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_fillRectangle (_graphics, 0.0, 1.0, 0.0, 1.0);
	Graphics_setColour (_graphics, Graphics_BLACK);
	Graphics_rectangle (_graphics, 0.0, 1.0, 0.0, 1.0);

	if (_endWindow - _startWindow > _longestAnalysis) {
		Graphics_setFont (_graphics, kGraphics_font_HELVETICA);
		Graphics_setFontSize (_graphics, 10);
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_text3 (_graphics, 0.5, 0.67, L"(To see the analyses, zoom in to at most ", Melder_half (_longestAnalysis), L" seconds,");
		Graphics_text (_graphics, 0.5, 0.33, L"or raise the \"longest analysis\" setting with \"Show analyses\" in the View menu.)");
		Graphics_setFontSize (_graphics, 12);
		return;
	}
	computeSpectrogram ();
	if (_spectrogram.show && _spectrogram.data != NULL) {
		Spectrogram_paintInside (_spectrogram.data, _graphics, _startWindow, _endWindow, 
			_spectrogram.viewFrom, _spectrogram.viewTo, _spectrogram.maximum, _spectrogram.autoscaling,
			_spectrogram.dynamicRange, _spectrogram.preemphasis, _spectrogram.dynamicCompression);
	}
	computePitch ();
	if (_pitch.show && _pitch.data != NULL) {
		double periodsPerAnalysisWindow = _pitch.method == kTimeSoundAnalysisEditor_pitch_analysisMethod_AUTOCORRELATION ? 3.0 : 1.0;
		double greatestNonUndersamplingTimeStep = 0.5 * periodsPerAnalysisWindow / _pitch.floor;
		double defaultTimeStep = 0.5 * greatestNonUndersamplingTimeStep;
		double timeStep =
			_timeStepStrategy == kTimeSoundAnalysisEditor_timeStepStrategy_FIXED ? _fixedTimeStep :
			_timeStepStrategy == kTimeSoundAnalysisEditor_timeStepStrategy_VIEW_DEPENDENT ? (_endWindow - _startWindow) / _numberOfTimeStepsPerView :
			defaultTimeStep;
		int undersampled = timeStep > greatestNonUndersamplingTimeStep;
		long numberOfVisiblePitchPoints = (long) ((_endWindow - _startWindow) / timeStep);
		Graphics_setColour (_graphics, Graphics_CYAN);
		Graphics_setLineWidth (_graphics, 3.0);
		if ((_pitch.drawingMethod == kTimeSoundAnalysisEditor_pitch_drawingMethod_AUTOMATIC && (undersampled || numberOfVisiblePitchPoints < 101)) ||
		    _pitch.drawingMethod == kTimeSoundAnalysisEditor_pitch_drawingMethod_SPECKLE)
		{
			Pitch_drawInside (_pitch.data, _graphics, _startWindow, _endWindow, pitchViewFrom_overt, pitchViewTo_overt, 2, _pitch.unit);
		}
		if ((_pitch.drawingMethod == kTimeSoundAnalysisEditor_pitch_drawingMethod_AUTOMATIC && ! undersampled) ||
		    _pitch.drawingMethod == kTimeSoundAnalysisEditor_pitch_drawingMethod_CURVE)
		{
			Pitch_drawInside (_pitch.data, _graphics, _startWindow, _endWindow, pitchViewFrom_overt, pitchViewTo_overt, FALSE, _pitch.unit);
		}
		Graphics_setColour (_graphics, Graphics_BLUE);
		Graphics_setLineWidth (_graphics, 1.0);
		if ((_pitch.drawingMethod == kTimeSoundAnalysisEditor_pitch_drawingMethod_AUTOMATIC && (undersampled || numberOfVisiblePitchPoints < 101)) ||
		    _pitch.drawingMethod == kTimeSoundAnalysisEditor_pitch_drawingMethod_SPECKLE)
		{
			Pitch_drawInside (_pitch.data, _graphics, _startWindow, _endWindow, pitchViewFrom_overt, pitchViewTo_overt, 1, _pitch.unit);
		}
		if ((_pitch.drawingMethod == kTimeSoundAnalysisEditor_pitch_drawingMethod_AUTOMATIC && ! undersampled) ||
		    _pitch.drawingMethod == kTimeSoundAnalysisEditor_pitch_drawingMethod_CURVE)
		{
			Pitch_drawInside (_pitch.data, _graphics, _startWindow, _endWindow, pitchViewFrom_overt, pitchViewTo_overt, FALSE, _pitch.unit);
		}
		Graphics_setColour (_graphics, Graphics_BLACK);
	}
	computeIntensity ();
	if (_intensity.show && _intensity.data != NULL) {
		Graphics_setColour (_graphics, _spectrogram.show ? Graphics_YELLOW : Graphics_LIME);
		Graphics_setLineWidth (_graphics, _spectrogram.show ? 1.0 : 3.0);
		Intensity_drawInside (_intensity.data, _graphics, _startWindow, _endWindow,
			_intensity.viewFrom, _intensity.viewTo);
		Graphics_setLineWidth (_graphics, 1.0);
		Graphics_setColour (_graphics, Graphics_BLACK);
	}
	computeFormants ();
	if (_formant.show && _formant.data != NULL) {
		Graphics_setColour (_graphics, Graphics_RED);
		Formant_drawSpeckles_inside (_formant.data, _graphics, _startWindow, _endWindow, 
			_spectrogram.viewFrom, _spectrogram.viewTo, _formant.dynamicRange, _formant.dotSize);
		Graphics_setColour (_graphics, Graphics_BLACK);
	}
	/*
	 * Draw vertical scales.
	 */
	if (_pitch.show) {
		double pitchCursor_overt = NUMundefined, pitchCursor_hidden = NUMundefined;
		Graphics_setWindow (_graphics, _startWindow, _endWindow, pitchViewFrom_hidden, pitchViewTo_hidden);
		Graphics_setColour (_graphics, Graphics_BLUE);
		if (_pitch.data) {
			if (_startSelection == _endSelection)
				pitchCursor_hidden = Pitch_getValueAtTime (_pitch.data, _startSelection, _pitch.unit, 1);
			else
				pitchCursor_hidden = Pitch_getMean (_pitch.data, _startSelection, _endSelection, _pitch.unit);
			pitchCursor_overt = ClassFunction_convertToNonlogarithmic (classPitch, pitchCursor_hidden, Pitch_LEVEL_FREQUENCY, _pitch.unit);
			if (NUMdefined (pitchCursor_hidden)) {
				Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_HALF);
				Graphics_text3 (_graphics, _endWindow, pitchCursor_hidden, Melder_float (Melder_half (pitchCursor_overt)), L" ",
					ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, _pitch.unit,
						Function_UNIT_TEXT_SHORT | Function_UNIT_TEXT_GRAPHICAL));
			}
			if (! NUMdefined (pitchCursor_hidden) || Graphics_dyWCtoMM (_graphics, pitchCursor_hidden - pitchViewFrom_hidden) > 5.0) {
				Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_BOTTOM);
				Graphics_text3 (_graphics, _endWindow, pitchViewFrom_hidden - Graphics_dyMMtoWC (_graphics, 0.5),
					Melder_float (Melder_half (pitchViewFrom_overt)), L" ",
					ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, _pitch.unit,
						Function_UNIT_TEXT_SHORT | Function_UNIT_TEXT_GRAPHICAL));
			}
			if (! NUMdefined (pitchCursor_hidden) || Graphics_dyWCtoMM (_graphics, pitchViewTo_hidden - pitchCursor_hidden) > 5.0) {
				Graphics_setTextAlignment (_graphics, Graphics_LEFT, Graphics_TOP);
				Graphics_text3 (_graphics, _endWindow, pitchViewTo_hidden, Melder_float (Melder_half (pitchViewTo_overt)), L" ",
					ClassFunction_getUnitText (classPitch, Pitch_LEVEL_FREQUENCY, _pitch.unit,
						Function_UNIT_TEXT_SHORT | Function_UNIT_TEXT_GRAPHICAL));
			}
		} else {
			Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
			Graphics_setFontSize (_graphics, 10);
			Graphics_text (_graphics, 0.5 * (_startWindow + _endWindow), 0.5 * (pitchViewFrom_hidden + pitchViewTo_hidden),
				L"(Cannot show pitch contour. Zoom out or change bottom of pitch range in pitch settings.)");
			Graphics_setFontSize (_graphics, 12);
		}
		Graphics_setColour (_graphics, Graphics_BLACK);
	}
	if (_intensity.show) {
		double intensityCursor = NUMundefined;
		int intensityCursorVisible;
		Graphics_Colour textColour;
		int alignment;
		double y;
		if (! _pitch.show) textColour = Graphics_GREEN, alignment = Graphics_LEFT, y = _endWindow;
		else if (! _spectrogram.show && ! _formant.show) textColour = Graphics_GREEN, alignment = Graphics_RIGHT, y = _startWindow;
		else textColour = _spectrogram.show ? Graphics_LIME : Graphics_GREEN, alignment = Graphics_RIGHT, y = _endWindow;
		Graphics_setWindow (_graphics, _startWindow, _endWindow, _intensity.viewFrom, _intensity.viewTo);
		if (_intensity.data) {
			if (_startSelection == _endSelection) {
				intensityCursor = Vector_getValueAtX (_intensity.data, _startSelection, Vector_CHANNEL_1, Vector_VALUE_INTERPOLATION_LINEAR);
			} else {
				intensityCursor = Intensity_getAverage (_intensity.data, _startSelection, _endSelection, _intensity.averagingMethod);
			}
		}
		Graphics_setColour (_graphics, textColour);
		intensityCursorVisible = NUMdefined (intensityCursor) && intensityCursor > _intensity.viewFrom && intensityCursor < _intensity.viewTo;
		if (intensityCursorVisible) {
			static const wchar_t *methodString [] = { L" (.5)", L" (\\muE)", L" (\\muS)", L" (\\mu)" };
			Graphics_setTextAlignment (_graphics, alignment, Graphics_HALF);
			Graphics_text3 (_graphics, y, intensityCursor, Melder_float (Melder_half (intensityCursor)), L" dB",
				_startSelection == _endSelection ? L"" : methodString [_intensity.averagingMethod]);
		}
		if (! intensityCursorVisible || Graphics_dyWCtoMM (_graphics, intensityCursor - _intensity.viewFrom) > 5.0) {
			Graphics_setTextAlignment (_graphics, alignment, Graphics_BOTTOM);
			Graphics_text2 (_graphics, y, _intensity.viewFrom - Graphics_dyMMtoWC (_graphics, 0.5),
				Melder_float (Melder_half (_intensity.viewFrom)), L" dB");
		}
		if (! intensityCursorVisible || Graphics_dyWCtoMM (_graphics, _intensity.viewTo - intensityCursor) > 5.0) {
			Graphics_setTextAlignment (_graphics, alignment, Graphics_TOP);
			Graphics_text2 (_graphics, y, _intensity.viewTo, Melder_float (Melder_half (_intensity.viewTo)), L" dB");
		}
		Graphics_setColour (_graphics, Graphics_BLACK);
	}
	if (_spectrogram.show || _formant.show) {
		static MelderString text = { 0 };
		int frequencyCursorVisible = _spectrogram.cursor > _spectrogram.viewFrom && _spectrogram.cursor < _spectrogram.viewTo;
		Graphics_setWindow (_graphics, _startWindow, _endWindow, _spectrogram.viewFrom, _spectrogram.viewTo);
		/*
		 * Range marks.
		 */
		Graphics_setLineType (_graphics, Graphics_DRAWN);
		Graphics_setColour (_graphics, Graphics_BLACK);
		if (! frequencyCursorVisible || Graphics_dyWCtoMM (_graphics, _spectrogram.cursor - _spectrogram.viewFrom) > 5.0) {
			MelderString_empty (& text);
			MelderString_append2 (& text, Melder_half (_spectrogram.viewFrom), L" Hz");
			Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_BOTTOM);
			Graphics_text (_graphics, _startWindow, _spectrogram.viewFrom - Graphics_dyMMtoWC (_graphics, 0.5), Melder_float (text.string));
		}
		if (! frequencyCursorVisible || Graphics_dyWCtoMM (_graphics, _spectrogram.viewTo - _spectrogram.cursor) > 5.0) {
			MelderString_empty (& text);
			MelderString_append2 (& text, Melder_half (_spectrogram.viewTo), L" Hz");
			Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_TOP);
			Graphics_text (_graphics, _startWindow, _spectrogram.viewTo, Melder_float (text.string));
		}
		/*
		 * Cursor lines.
		 */
		Graphics_setLineType (_graphics, Graphics_DOTTED);
		Graphics_setColour (_graphics, Graphics_RED);
		if (frequencyCursorVisible) {
			double x = _startWindow, y = _spectrogram.cursor;
			Graphics_setTextAlignment (_graphics, Graphics_RIGHT, Graphics_HALF);
			Graphics_text2 (_graphics, x, y, Melder_float (Melder_half (y)), L" Hz");
			Graphics_line (_graphics, x, y, _endWindow, y);
		}
		/*
		if (_startSelection >= _startWindow && _startSelection <= _endWindow)
			Graphics_line (_graphics, _startSelection, _spectrogram.viewFrom, _startSelection, _spectrogram.viewTo);
		if (_endSelection > _startWindow && _endSelection < _endWindow && _endSelection != _startSelection)
			Graphics_line (_graphics, _endSelection, _spectrogram.viewFrom, _endSelection, _spectrogram.viewTo);*/
		/*
		 * Cadre.
		 */
		Graphics_setLineType (_graphics, Graphics_DRAWN);
		Graphics_setColour (_graphics, Graphics_BLACK);
		Graphics_rectangle (_graphics, _startWindow, _endWindow, _spectrogram.viewFrom, _spectrogram.viewTo);
	}
}

void TimeSoundAnalysisEditor::draw_analysis_pulses () {
	computePulses ();
	if (_pulses.show && _endWindow - _startWindow <= _longestAnalysis && _pulses.data != NULL) {
		long i;
		PointProcess point = _pulses.data;
		Graphics_setWindow (_graphics, _startWindow, _endWindow, -1.0, 1.0);
		Graphics_setColour (_graphics, Graphics_BLUE);
		if (point -> nt < 2000) for (i = 1; i <= point -> nt; i ++) {
			double t = point -> t [i];
			if (t >= _startWindow && t <= _endWindow)
				Graphics_line (_graphics, t, -0.9, t, 0.9);
		}
		Graphics_setColour (_graphics, Graphics_BLACK);
	}
}

int TimeSoundAnalysisEditor::click (double xbegin, double ybegin, int shiftKeyPressed) {
	if (_pitch.show) {
		//Melder_warning3 (Melder_double (xbegin), L" ", Melder_double (ybegin));
		if (xbegin >= _endWindow && ybegin > 0.48 && ybegin <= 0.50) {
			_pitch.ceiling *= 1.26;
			forget (_pitch.data);
			forget (_intensity.data);
			forget (_pulses.data);
			return 1;
		}
		if (xbegin >= _endWindow && ybegin > 0.46 && ybegin <= 0.48) {
			_pitch.ceiling /= 1.26;
			forget (_pitch.data);
			forget (_intensity.data);
			forget (_pulses.data);
			return 1;
		}
	}
	return TimeSoundEditor::click (xbegin, ybegin, shiftKeyPressed);
}

/* End of file TimeSoundAnalysisEditor.cpp */
