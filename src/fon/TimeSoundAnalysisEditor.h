#ifndef _TimeSoundAnalysisEditor_h_
#define _TimeSoundAnalysisEditor_h_
/* TimeSoundAnalysisEditor.h
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
 * pb 2002/11/19 added show-widgets
 * pb 2002/11/19 added pulses
 * pb 2003/05/20 longestAnalysis replaces pitch.timeSteps, pitch.speckle, formant.maximumDuration
 * pb 2003/05/21 pitch floor and ceiling replace the view and analysis ranges
 * pb 2003/05/27 spectrogram maximum and autoscaling
 * pb 2003/08/23 formant.numberOfTimeSteps
 * pb 2003/09/16 advanced pitch settings: pitch.timeStep, pitch.timeStepsPerView, pitch.viewFrom, pitch.viewTo
 * pb 2003/09/18 advanced formant settings: formant.timeStep, formant.timeStepsPerView
 * pb 2003/10/01 time step settings: timeStepStrategy, fixedTimeStep, numberOfTimeStepsPerView
 * pb 2004/02/15 highlight methods
 * pb 2004/07/14 pulses.maximumAmplitudeFactor
 * pb 2004/10/24 intensity.averagingMethod
 * pb 2004/10/27 intensity.subtractMeanPressure
 * pb 2005/01/11 getBottomOfSoundAndAnalysisArea
 * pb 2005/06/16 units
 * pb 2005/12/07 arrowScrollStep
 * pb 2007/06/10 wchar_t
 * pb 2007/09/02 direct drawing to picture window
 * pb 2007/09/08 inherit from TimeSoundEditor
 * pb 2007/11/01 direct intensity, formants, and pulses drawing
 * pb 2007/12/02 split off TimeSoundAnalysisEditor_enums.h
 * pb 2011/03/23 C++
 */

#include "Formant.h"
#include "Intensity.h"
#include "Pitch.h"
#include "PointProcess.h"
#include "Sound_and_Spectrogram.h"
#include "TimeSoundAnalysisEditor_enums.h"
#include "TimeSoundEditor.h"

struct FunctionEditor_spectrogram {
	Spectrogram data; bool show;
	/* Spectrogram settings: */
	double viewFrom, viewTo;   /* Hertz */
	double windowLength;   /* seconds */
	double dynamicRange;   /* dB */
	/* Advanced spectrogram settings: */
	long timeSteps, frequencySteps;
	enum kSound_to_Spectrogram_method method;
	enum kSound_to_Spectrogram_windowShape windowShape;
	bool autoscaling;
	double maximum;   /* dB/Hz */
	double preemphasis;   /* dB/octave */
	double dynamicCompression;   /* 0..1 */
	/* Dynamic information: */
	double cursor;
};

struct FunctionEditor_pitch {
	Pitch data; bool show;
	/* Pitch settings: */
	double floor, ceiling;
	enum kPitch_unit unit;
	enum kTimeSoundAnalysisEditor_pitch_drawingMethod drawingMethod;
	/* Advanced pitch settings: */
	double viewFrom, viewTo;
	enum kTimeSoundAnalysisEditor_pitch_analysisMethod method;
	bool veryAccurate;
	long maximumNumberOfCandidates; double silenceThreshold, voicingThreshold;
	double octaveCost, octaveJumpCost, voicedUnvoicedCost;
	struct { bool speckle; } picture;
};
struct FunctionEditor_intensity {
	Intensity data; bool show;
	/* Intensity settings: */
	double viewFrom, viewTo;
	enum kTimeSoundAnalysisEditor_intensity_averagingMethod averagingMethod;
	bool subtractMeanPressure;
};
struct FunctionEditor_formant {
	Formant data; bool show;
	/* Formant settings: */
	double maximumFormant; long numberOfPoles;
	double windowLength;
	double dynamicRange, dotSize;
	/* Advanced formant settings: */
	enum kTimeSoundAnalysisEditor_formant_analysisMethod method;
	double preemphasisFrom;
};
struct FunctionEditor_pulses {
	PointProcess data; bool show;
	/* Pulses settings: */
	double maximumPeriodFactor, maximumAmplitudeFactor;
};

class TimeSoundAnalysisEditor : public TimeSoundEditor {
  public:
	static void prefs (void);

	TimeSoundAnalysisEditor (GuiObject parent, const wchar_t *title, Any data, Any sound, bool ownSound);
	virtual ~TimeSoundAnalysisEditor ();

	double _longestAnalysis;
	enum kTimeSoundAnalysisEditor_timeStepStrategy _timeStepStrategy;
	double _fixedTimeStep;
	long _numberOfTimeStepsPerView;
	struct FunctionEditor_spectrogram _spectrogram;
	struct FunctionEditor_pitch _pitch;
	struct FunctionEditor_intensity _intensity;
	struct FunctionEditor_formant _formant;
	struct FunctionEditor_pulses _pulses;
	GuiObject _spectrogramToggle, _pitchToggle, _intensityToggle, _formantToggle, _pulsesToggle;

  protected:
	virtual const wchar_t * type () { return L"TimeSoundAnalysisEditor"; }
	virtual void info ();

	virtual void destroy_analysis ();
	virtual void draw_analysis ();
	virtual void draw_analysis_pulses ();
	virtual void computeSpectrogram ();
	virtual void computePitch ();
	virtual void computeIntensity ();
	virtual void computeFormants ();
	virtual void computePulses ();
	virtual int makeQueriable (int allowCursor, double *tmin, double *tmax);
	virtual int do_deleteLogFile (int which);
	virtual int do_log (int which);
	virtual Sound extractSound (double tmin, double tmax);
	virtual int do_getFormant (int iformant);
	virtual int do_getBandwidth (int iformant);
	virtual void computePitch_inside ();
	virtual int click (double xbegin, double ybegin, int shiftKeyPressed);

  private:
	static int menu_cb_logSettings (EDITOR_ARGS);
	static int menu_cb_deleteLogFile1 (EDITOR_ARGS);
	static int menu_cb_deleteLogFile2 (EDITOR_ARGS);
	static int menu_cb_log1 (EDITOR_ARGS);
	static int menu_cb_log2 (EDITOR_ARGS);
	static int menu_cb_logScript3 (EDITOR_ARGS);
	static int menu_cb_logScript4 (EDITOR_ARGS);
	static int menu_cb_showAnalyses (EDITOR_ARGS);
	static int menu_cb_timeStepSettings (EDITOR_ARGS);
	static int menu_cb_showSpectrogram (EDITOR_ARGS);
	static int menu_cb_spectrogramSettings (EDITOR_ARGS);
	static int menu_cb_advancedSpectrogramSettings (EDITOR_ARGS);
	static int menu_cb_getFrequency (EDITOR_ARGS);
	static int menu_cb_getSpectralPowerAtCursorCross (EDITOR_ARGS);
	static int menu_cb_moveFrequencyCursorTo (EDITOR_ARGS);
	static int menu_cb_extractVisibleSpectrogram (EDITOR_ARGS);
	static int menu_cb_viewSpectralSlice (EDITOR_ARGS);
	static int menu_cb_paintVisibleSpectrogram (EDITOR_ARGS);
	static int menu_cb_showPitch (EDITOR_ARGS);
	static int menu_cb_pitchSettings (EDITOR_ARGS);
	static int menu_cb_advancedPitchSettings (EDITOR_ARGS);
	static int menu_cb_pitchListing (EDITOR_ARGS);
	static int menu_cb_getPitch (EDITOR_ARGS);
	static int menu_cb_getMinimumPitch (EDITOR_ARGS);
	static int menu_cb_getMaximumPitch (EDITOR_ARGS);
	static int menu_cb_moveCursorToMinimumPitch (EDITOR_ARGS);
	static int menu_cb_moveCursorToMaximumPitch (EDITOR_ARGS);
	static int menu_cb_extractVisiblePitchContour (EDITOR_ARGS);
	static int menu_cb_drawVisiblePitchContour (EDITOR_ARGS);
	static int menu_cb_showIntensity (EDITOR_ARGS);
	static int menu_cb_intensitySettings (EDITOR_ARGS);
	static int menu_cb_extractVisibleIntensityContour (EDITOR_ARGS);
	static int menu_cb_drawVisibleIntensityContour (EDITOR_ARGS);
	static int menu_cb_intensityListing (EDITOR_ARGS);
	static int menu_cb_getIntensity (EDITOR_ARGS);
	static int menu_cb_getMinimumIntensity (EDITOR_ARGS);
	static int menu_cb_getMaximumIntensity (EDITOR_ARGS);
	static int menu_cb_showFormants (EDITOR_ARGS);
	static int menu_cb_formantSettings (EDITOR_ARGS);
	static int menu_cb_advancedFormantSettings (EDITOR_ARGS);
	static int menu_cb_extractVisibleFormantContour (EDITOR_ARGS);
	static int menu_cb_drawVisibleFormantContour (EDITOR_ARGS);
	static int menu_cb_formantListing (EDITOR_ARGS);
	static int menu_cb_getFirstFormant (EDITOR_ARGS);
	static int menu_cb_getFirstBandwidth (EDITOR_ARGS);
	static int menu_cb_getSecondFormant (EDITOR_ARGS);
	static int menu_cb_getSecondBandwidth (EDITOR_ARGS);
	static int menu_cb_getThirdFormant (EDITOR_ARGS);
	static int menu_cb_getThirdBandwidth (EDITOR_ARGS);
	static int menu_cb_getFourthFormant (EDITOR_ARGS);
	static int menu_cb_getFourthBandwidth (EDITOR_ARGS);
	static int menu_cb_getFormant (EDITOR_ARGS);
	static int menu_cb_getBandwidth (EDITOR_ARGS);
	static int menu_cb_showPulses (EDITOR_ARGS);
	static int menu_cb_advancedPulsesSettings (EDITOR_ARGS);
	static int menu_cb_extractVisiblePulses (EDITOR_ARGS);
	static int menu_cb_drawVisiblePulses (EDITOR_ARGS);
	static int menu_cb_voiceReport (EDITOR_ARGS);
	static int menu_cb_pulseListing (EDITOR_ARGS);

	void createMenus ();
};

/* End of file TimeSoundAnalysisEditor.h */
#endif
