/* praat_KlattGrid_init.c
 *
 * Copyright (C) 2009 David Weenink
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
	djmw 20090420
*/

#include "ui/editors/KlattGridEditors.h"
#include "dwtools/KlattTable.h"
#include "ui/editors/IntensityTierEditor.h"

#include "ui/praat.h"

/******************* KlattGrid  *********************************/

static wchar_t *formant_names[] = { L"ui/editors/AmplitudeTierEditor.h", L"oral ", L"nasal ", L"frication ", L"tracheal ", L"nasal anti", L"tracheal anti", L"delta "};

static void KlattGrid_4formants_addCommonField (UiForm *dia)
{
	UiForm::UiField *radio;
	OPTIONMENU(L"Formant type", 1)
	OPTION (L"Normal formant")
	OPTION (L"Nasal formant")
	OPTION (L"Frication formant")
	OPTION (L"Tracheal formant")
}

static void KlattGrid_6formants_addCommonField (UiForm *dia)
{
	UiForm::UiField *radio;
	OPTIONMENU(L"Formant type", 1)
	OPTION (L"Normal formant")
	OPTION (L"Nasal formant")
	OPTION (L"Frication formant")
	OPTION (L"Tracheal formant")
	OPTION (L"Nasal antiformant")
	OPTION (L"Tracheal antiformant")
//	OPTION (L"Delta formant")
}

static void KlattGrid_7formants_addCommonField (UiForm *dia)
{
	UiForm::UiField *radio;
	OPTIONMENU(L"Formant type", 1)
	OPTION (L"Normal formant")
	OPTION (L"Nasal formant")
	OPTION (L"Frication formant")
	OPTION (L"Tracheal formant")
	OPTION (L"Nasal antiformant")
	OPTION (L"Tracheal antiformant")
	OPTION (L"Delta formant")
}

static void KlattGrid_PhonationGridPlayOptions_addCommonFields (UiForm *dia)
{
	UiForm::UiField *radio;
	//LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Phonation options")
	BOOLEAN (L"Voicing", 1)
	BOOLEAN (L"Flutter", 1)
	BOOLEAN (L"Double pulsing", 1)
	BOOLEAN (L"Collision phase", 1)
	BOOLEAN (L"Spectral tilt", 1)
	OPTIONMENU (L"Flow function", 1)
	OPTION (L"Powers in tiers")
	OPTION (L"t^2-t^3")
	OPTION (L"t^3-t^4")
	BOOLEAN (L"Flow derivative", 1)
	BOOLEAN (L"Aspiration", 1)
	BOOLEAN (L"Breathiness", 1)
}

static void KlattGrid_PhonationGridPlayOptions_getCommonFields (UiForm *dia, KlattGrid thee)
{
		PhonationGridPlayOptions pp = thy phonation -> options;
		pp -> voicing = GET_INTEGER (L"Voicing");
		pp -> flutter = GET_INTEGER (L"Flutter");
		pp -> doublePulsing = GET_INTEGER (L"Double pulsing");
		pp -> collisionPhase = GET_INTEGER (L"Collision phase");
		pp -> spectralTilt = GET_INTEGER (L"Spectral tilt");
		pp -> flowFunction = GET_INTEGER (L"Flow function");
		pp -> flowDerivative = GET_INTEGER (L"Flow derivative");
		pp -> aspiration = GET_INTEGER (L"Aspiration");
		pp -> breathiness = GET_INTEGER (L"Breathiness");
}

static void KlattGrid_PlayOptions_addCommonFields (UiForm *dia, int sound)
{
	UiForm::UiField *radio;
	//LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Time domain")
	REAL (L"left Time range (s)", L"0")
	REAL (L"right Time range (s)", L"0")
	if (sound) POSITIVE (L"Sampling frequency (Hz)", L"44100")
	BOOLEAN (L"Scale peak", 1)
	KlattGrid_PhonationGridPlayOptions_addCommonFields (dia);
	OPTIONMENU(L"Filter options", 1)
	OPTION (L"Cascade")
	OPTION (L"Parallel")
	REAL (L"left Oral formant range", L"1")
	REAL (L"right Oral formant range", L"5")
	REAL (L"left Nasal formant range", L"1")
	REAL (L"right Nasal formant range", L"1")
	REAL (L"left Nasal antiformant range", L"1")
	REAL (L"right Nasal antiformant range", L"1")
	//LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Coupling options")
	REAL (L"left Tracheal formant range", L"1")
	REAL (L"right Tracheal formant range", L"1")
	REAL (L"left Tracheal antiformant range", L"1")
	REAL (L"right Tracheal antiformant range", L"1")
	REAL (L"left Delta formant range", L"1")
	REAL (L"right Delta formant range", L"1")
	REAL (L"left Delta bandwidth range", L"1")
	REAL (L"right Delta bandwidth range", L"1")
	//LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Frication options")
	REAL (L"left Frication formant range", L"1")
	REAL (L"right Frication formant range", L"6")
	BOOLEAN (L"Frication bypass", 1)
}

static void KlattGrid_PlayOptions_getCommonFields (UiForm *dia, int sound, KlattGrid thee)
{
		KlattGrid_setDefaultPlayOptions (thee);
		KlattGridPlayOptions pk = thy options;
		pk -> scalePeak = GET_INTEGER (L"Scale peak");
		pk -> xmin = GET_REAL (L"left Time range");
		pk -> xmax = GET_REAL (L"right Time range");
		if (sound) pk -> samplingFrequency = GET_REAL (L"Sampling frequency");
		pk -> scalePeak = GET_INTEGER (L"Scale peak");
		KlattGrid_PhonationGridPlayOptions_getCommonFields (dia, thee);
		VocalTractGridPlayOptions pv = thy vocalTract -> options;
		pv -> filterModel = GET_INTEGER (L"Filter options") == 1 ? KlattGrid_FILTER_CASCADE : KlattGrid_FILTER_PARALLEL;
		pv -> startOralFormant = GET_REAL (L"left Oral formant range");
		pv -> endOralFormant  = GET_REAL (L"right Oral formant range");
		pv -> startNasalFormant = GET_REAL (L"left Nasal formant range");
		pv -> endNasalFormant = GET_REAL (L"right Nasal formant range");
		pv -> startNasalAntiFormant = GET_REAL (L"left Nasal antiformant range");
		pv -> endNasalAntiFormant = GET_REAL (L"right Nasal antiformant range");
		CouplingGridPlayOptions pc = thy coupling -> options;
		pc -> startTrachealFormant = GET_REAL (L"left Tracheal formant range");
		pc -> endTrachealFormant = GET_REAL (L"right Tracheal formant range");
		pc -> startTrachealAntiFormant = GET_REAL (L"left Tracheal antiformant range");
		pc -> endTrachealAntiFormant = GET_REAL (L"right Tracheal antiformant range");
		pc -> startDeltaFormant = GET_REAL (L"left Delta formant range");
		pc -> endDeltaFormant = GET_REAL (L"right Delta formant range");
		pc -> startDeltaBandwidth = GET_REAL (L"left Delta bandwidth range");
		pc -> endDeltaFormant = GET_REAL (L"right Delta bandwidth range");
		FricationGridPlayOptions pf = thy frication -> options;
		pf -> startFricationFormant = GET_REAL (L"left Frication formant range");
		pf -> endFricationFormant = GET_REAL (L"right Frication formant range");
		pf -> bypass = GET_INTEGER (L"Frication bypass");
}

DIRECT (KlattGrid_createExample)
	if (! praat_new1 (KlattGrid_createExample(), L"example")) return 0;
END

FORM (KlattGrid_create, L"Create KlattGrid", L"Create KlattGrid...")
	WORD (L"Name", L"kg")
	REAL (L"Start time (s)", L"0.0")
	REAL (L"End time (s)", L"1.0")
	INTEGER (L"Number of oral formants", L"6")
	INTEGER (L"Number of nasal formants", L"1")
	INTEGER (L"Number of nasal antiformants", L"1")
	INTEGER (L"Number of frication formants", L"6")
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Coupling between source and filter")
	INTEGER (L"Number of tracheal formants", L"1")
	INTEGER (L"Number of tracheal antiformants", L"1")
	INTEGER (L"Number of delta formants", L"1")
	OK
DO
	double tmin = GET_REAL (L"Start time");
	double tmax = GET_REAL (L"End time");
	REQUIRE (tmin < tmax, L"The start time must lie before the end time.")
	long numberOfOralFormants = GET_INTEGER (L"Number of oral formants");
	long numberOfNasalFormants = GET_INTEGER (L"Number of nasal formants");
	long numberOfNasalAntiFormants = GET_INTEGER (L"Number of nasal antiformants");
	long numberOfTrachealFormants = GET_INTEGER (L"Number of tracheal formants");
	long numberOfTrachealAntiFormants = GET_INTEGER (L"Number of tracheal antiformants");
	long numberOfFricationFormants = GET_INTEGER (L"Number of frication formants");
	long numberOfDeltaFormants = GET_INTEGER (L"Number of delta formants");
	REQUIRE (numberOfOralFormants>=0 && numberOfNasalFormants >= 0 && numberOfNasalAntiFormants >= 0
		&& numberOfTrachealFormants >= 0 && numberOfTrachealAntiFormants >= 0
		&& numberOfFricationFormants >= 0 && numberOfDeltaFormants >= 0,
		L"The number of (..) formants cannot be negative!")
	if (! praat_new1 (KlattGrid_create (tmin, tmax, numberOfOralFormants,
		numberOfNasalFormants, numberOfNasalAntiFormants,
		numberOfTrachealFormants, numberOfTrachealAntiFormants,
		numberOfFricationFormants, numberOfDeltaFormants),
		GET_STRING (L"Name"))) return 0;
END


#define KlattGrid_INSTALL_TIER_EDITOR(Name,name) \
DIRECT (KlattGrid_edit##Name##Tier) \
	if (theCurrentPraatApplication -> batch) { return Melder_error1 (L"Cannot edit a KlattGrid from batch."); } \
	WHERE (SELECTED && CLASS == classKlattGrid) \
	{\
		const wchar_t *id_and_name = Melder_wcscat2 (Melder_integer (ID), L". " #name  " tier"); \
		if (! praat_installEditor (new KlattGrid_##name##TierEditor (theCurrentPraatApplication -> topShell, id_and_name, \
			(structKlattGrid *)OBJECT), IOBJECT)) return 0; \
		Melder_free (id_and_name); \
	}\
END

KlattGrid_INSTALL_TIER_EDITOR (Pitch, pitch)
KlattGrid_INSTALL_TIER_EDITOR (VoicingAmplitude, voicingAmplitude)
KlattGrid_INSTALL_TIER_EDITOR (Flutter, flutter)
KlattGrid_INSTALL_TIER_EDITOR (Power1, power1)
KlattGrid_INSTALL_TIER_EDITOR (Power2, power2)
KlattGrid_INSTALL_TIER_EDITOR (OpenPhase, openPhase)
KlattGrid_INSTALL_TIER_EDITOR (CollisionPhase, collisionPhase)
KlattGrid_INSTALL_TIER_EDITOR (DoublePulsing, doublePulsing)
KlattGrid_INSTALL_TIER_EDITOR (AspirationAmplitude, aspirationAmplitude)
KlattGrid_INSTALL_TIER_EDITOR (BreathinessAmplitude, breathinessAmplitude)
KlattGrid_INSTALL_TIER_EDITOR (SpectralTilt, spectralTilt)

KlattGrid_INSTALL_TIER_EDITOR (FricationBypass, fricationBypass)
KlattGrid_INSTALL_TIER_EDITOR (FricationAmplitude, fricationAmplitude)

#undef KlattGrid_INSTALL_TIER_EDITOR

#define KlattGRID_EDIT_FORMANTGRID(Name,formantType) \
DIRECT (KlattGrid_edit##Name##FormantGrid) \
	if (theCurrentPraatApplication -> batch) { return Melder_error1 (L"Cannot edit a KlattGrid from batch."); } \
	WHERE (SELECTED && CLASS == classKlattGrid) \
	{ \
		const wchar_t *id_and_name = Melder_wcscat4 (Melder_integer (ID), L". ", formant_names[formantType], L"formant grid"); \
		if (! praat_installEditor (new KlattGrid_formantGridEditor (theCurrentPraatApplication -> topShell, id_and_name, (structKlattGrid *)OBJECT, formantType), IOBJECT)) return 0; \
		Melder_free (id_and_name); \
	} \
END

KlattGRID_EDIT_FORMANTGRID(Oral,KlattGrid_ORAL_FORMANTS)
KlattGRID_EDIT_FORMANTGRID(Nasal,KlattGrid_NASAL_FORMANTS)
KlattGRID_EDIT_FORMANTGRID(Tracheal,KlattGrid_TRACHEAL_FORMANTS)
KlattGRID_EDIT_FORMANTGRID(NasalAnti,KlattGrid_NASAL_ANTIFORMANTS)
KlattGRID_EDIT_FORMANTGRID(TrachealAnti,KlattGrid_TRACHEAL_ANTIFORMANTS)
KlattGRID_EDIT_FORMANTGRID(Delta,KlattGrid_DELTA_FORMANTS)
KlattGRID_EDIT_FORMANTGRID(Frication,KlattGrid_FRICATION_FORMANTS)

#undef KlattGRID_EDIT_FORMANTGRID

#define KlattGrid_EDIT_FORMANT_AMPLITUDE_TIER(Name,name,formantType) \
FORM (KlattGrid_edit##Name##FormantAmplitudeTier, L"KlattGrid: View & Edit " #name "formant amplitude tier", 0) \
	NATURAL (L"Formant number", L"1") \
	OK \
DO \
	long formantNumber = GET_INTEGER (L"Formant number"); \
	if (theCurrentPraatApplication -> batch) { return Melder_error1 (L"Cannot edit a KlattGrid from batch."); } \
	WHERE (SELECTED && CLASS == classKlattGrid) \
	{ \
		KlattGrid kg = (structKlattGrid *)OBJECT; \
		Ordered *amp = (structOrdered **)KlattGrid_getAddressOfAmplitudes (kg, formantType); \
		if (amp == NULL) return Melder_error1 (L"Unknown formant type"); \
		if (formantNumber > (*amp) -> size) return Melder_error1 (L"Formant number does not exist."); \
		const wchar_t *id_and_name = Melder_wcscat4 (Melder_integer (ID), L". ", formant_names[formantType], L"formant amplitude tier"); \
		if (! praat_installEditor (new KlattGrid_decibelTierEditor (theCurrentPraatApplication -> topShell, id_and_name, kg, (structRealTier *)(*amp)->item[formantNumber]), IOBJECT)) return 0; \
		Melder_free (id_and_name); \
	} \
END

KlattGrid_EDIT_FORMANT_AMPLITUDE_TIER (Oral,oral,KlattGrid_ORAL_FORMANTS)
KlattGrid_EDIT_FORMANT_AMPLITUDE_TIER (Nasal,nasal,KlattGrid_NASAL_FORMANTS)
KlattGrid_EDIT_FORMANT_AMPLITUDE_TIER (Tracheal,tracheal,KlattGrid_TRACHEAL_FORMANTS)
KlattGrid_EDIT_FORMANT_AMPLITUDE_TIER (Frication,frication,KlattGrid_FRICATION_FORMANTS)

#undef KlattGrid_EDIT_FORMANT_AMPLITUDE_TIER

#define KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE(Name,name,unit,default_,require, requiremessage,newname,tiertype) \
FORM(KlattGrid_get##Name##AtTime, L"KlattGrid: Get " #name " at time", 0) \
	REAL (L"Time", L"0.5") \
	OK \
DO \
	Melder_informationReal (KlattGrid_get##Name##AtTime ((structKlattGrid *)ONLY_OBJECT, GET_REAL (L"Time")), unit); \
END \
FORM (KlattGrid_add##Name##Point, L"KlattGrid: Add " #name " point", 0) \
	REAL (L"Time (s)", L"0.5") \
	REAL (L"Value" unit, default_) \
	OK \
DO \
	double value = GET_REAL (L"Value"); \
	REQUIRE (require, requiremessage) \
	WHERE (SELECTED) { \
		if (! KlattGrid_add##Name##Point ((structKlattGrid *)OBJECT, GET_REAL (L"Time"), value)) return 0; \
		praat_dataChanged (OBJECT); \
	} \
END \
FORM (KlattGrid_remove##Name##Points, L"Remove " #name " points", 0) \
	REAL (L"From time (s)", L"0.3")\
	REAL (L"To time (s)", L"0.7") \
	OK \
DO \
	WHERE (SELECTED) { \
		KlattGrid_remove##Name##Points ((structKlattGrid *)OBJECT, GET_REAL (L"From time"), GET_REAL (L"To time")); \
		praat_dataChanged (OBJECT);\
	} \
END \
DIRECT (KlattGrid_extract##Name##Tier) \
	WHERE (SELECTED) { \
		if (! praat_new1 (KlattGrid_extract##Name##Tier ((structKlattGrid *)OBJECT), newname)) return 0; \
	} \
END \
DIRECT (KlattGrid_replace##Name##Tier) \
	if (! KlattGrid_replace##Name##Tier ((structKlattGrid *)ONLY(classKlattGrid), (struct##tiertype *)ONLY(class##tiertype))) return 0; \
	praat_dataChanged (OBJECT);\
END

// 55 DO_KlattGrid... functions
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (Pitch, pitch, L" (Hz)", (L"100.0"),
	(value>=0), (L"Pitch must be greater equal zero."), L"f0", PitchTier)
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE(VoicingAmplitude,voicing amplitude, L" (dB SPL)", L"90.0",
	(1), L"ui/editors/AmplitudeTierEditor.h", L"voicing", IntensityTier)
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (Flutter, flutter, L" (0..1)", (L"0.0"),
	(value>=0&&value<=1), (L"Flutter must be in [0,1]."),L"flutter",RealTier)
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (Power1, power1, L"ui/editors/AmplitudeTierEditor.h", L"3",
	(value>0), L"Power1 needs to be positive.",L"power1",RealTier)
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (Power2, power2, L"ui/editors/AmplitudeTierEditor.h", L"4",
	(value>0), L"Power2 needs to be positive.",L"power2",RealTier)
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (OpenPhase, open phase, L"ui/editors/AmplitudeTierEditor.h", L"0.7",
	(value >=0&&value<=1), L"Open phase must be greater than zero and smaller equal one.", L"openPhase",RealTier)
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (CollisionPhase, collision phase, L"ui/editors/AmplitudeTierEditor.h", L"0.03",
	(value>=0&&value<1), L"Collision phase must be greater equal zero and smaller than one.", L"collisionPhase",RealTier)
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (DoublePulsing,double pulsing,L" (0..1)",L"0.0",
	(value>=0&&value<=1), L"Double pulsing must be greater equal zero and smaller equal one.",L"doublePulsing",RealTier)
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (SpectralTilt,spectral tilt,L" (dB)",L"0.0",
	(value>=0), L"Spectral tilt must be greater equal zero.", L"spectralTilt",IntensityTier)
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (AspirationAmplitude, aspiration amplitude, L" (dB SPL)", L"0.0",
	(1), L"ui/editors/AmplitudeTierEditor.h", L"aspiration", IntensityTier)
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (BreathinessAmplitude, breathiness amplitude, L" (dB SPL)", L"30.0",
	(1), L"ui/editors/AmplitudeTierEditor.h", L"breathiness", IntensityTier)

KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (FricationAmplitude, frication amplitude, L" (dB SPL)", L"30.0",
	(1), L"ui/editors/AmplitudeTierEditor.h", L"frication", IntensityTier)
KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE (FricationBypass, frication bypass, L" (dB)", L"30.0",
	(1), L"ui/editors/AmplitudeTierEditor.h", L"bypass", IntensityTier)

#undef KlattGrid_PHONATION_GET_ADD_REMOVE_EXTRACT_REPLACE

#define KlattGrid_FORMULA_FORMANT_FBA_VALUE(Name,namef,ForBs,forbs,textfield,formantType,label) \
FORM (KlattGrid_formula##Name##Formant##ForBs, L"KlattGrid: Formula (" #namef "ormant " #forbs ")", L"Formant: Formula (" #forbs ")...") \
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"row is formant number, col is point number:\nfor row from 1 to nrow do for col from 1 to ncol do " #ForBs " (row, col) :=") \
	LABEL (L"ui/editors/AmplitudeTierEditor.h", label) \
	TEXTFIELD (L"formula", textfield) \
	OK \
DO \
	WHERE (SELECTED) { \
		if (! KlattGrid_formula_##forbs ((structKlattGrid *)OBJECT, formantType, GET_STRING (L"formula"), interpreter)) return 0; \
		praat_dataChanged (OBJECT); \
	} \
END

#define KlattGrid_ADD_FBA_VALUE(Name,namef,Form,FBA,fba,formantType,default,unit,require,requiremessage) \
FORM (KlattGrid_add##Name##Formant##FBA##Point, L"KlattGrid: Add " #namef "ormant " #fba " point", 0) \
	NATURAL (L"Formant number", L"1") \
	REAL (L"Time (s)", L"0.5") \
	REAL (L"Value " #unit, default) \
	OK \
DO \
	double value = GET_REAL (L"Value"); \
	REQUIRE (require, requiremessage) \
	WHERE (SELECTED) { \
		if (! KlattGrid_add##Form##Point ((structKlattGrid *)OBJECT, formantType, GET_INTEGER (L"Formant number"), GET_REAL (L"Time"), value)) return 0; \
		praat_dataChanged (OBJECT); \
	} \
END

#define KlattGrid_REMOVE_FBA_VALUE(Name,namef,Form,FBA,fba,formantType) \
FORM (KlattGrid_remove##Name##Formant##FBA##Points, L"KlattGrid: Remove " #namef "ormant " #fba " points", 0) \
	NATURAL (L"Formant number", L"1") \
	REAL (L"From time (s)", L"0.3")\
	REAL (L"To time (s)", L"0.7") \
	OK \
DO \
	WHERE (SELECTED) { \
		KlattGrid_remove##Form##Points ((structKlattGrid *)OBJECT, formantType, GET_INTEGER (L"Formant number"), GET_REAL (L"From time"), GET_REAL (L"To time")); \
		praat_dataChanged (OBJECT);\
	} \
END

#define KlattGrid_ADD_FORMANT(Name,namef,formantType) \
FORM (KlattGrid_add##Name##Formant, L"KlattGrid: Add " #namef "ormant", 0) \
	INTEGER (L"Position", L"0 (=at end)") \
	OK \
DO \
	WHERE (SELECTED) { \
		if (! KlattGrid_addFormant ((structKlattGrid *)OBJECT, formantType, GET_INTEGER (L"Position"))) return 0; \
		praat_dataChanged (OBJECT); \
	} \
END

#define KlattGrid_REMOVE_FORMANT(Name,namef,formantType) \
FORM (KlattGrid_remove##Name##Formant, L"KlattGrid: Remove " #namef "ormant", 0) \
	INTEGER (L"Position", L"0 (=do nothing)") \
	OK \
DO \
	WHERE (SELECTED) { \
		KlattGrid_removeFormant ((structKlattGrid *)OBJECT, formantType, GET_INTEGER (L"Position")); \
		praat_dataChanged (OBJECT); \
	} \
END

#define KlattGrid_FORMULA_ADD_REMOVE_FBA(Name,namef,formantType) \
KlattGrid_FORMULA_FORMANT_FBA_VALUE (Name, namef, Frequencies, frequencies, L"if row = 2 then self + 200 else self fi", formantType, L" ") \
KlattGrid_FORMULA_FORMANT_FBA_VALUE (Name, namef, Bandwidths, bandwidths, L"self / 10 ; 10% of frequency", formantType, L"Warning: self is formant frequency.") \
KlattGrid_ADD_FBA_VALUE (Name, namef, Formant, Frequency, frequency, formantType, L"500.0", (Hz), (value>0), L"Frequency must be greater than zero.") \
KlattGrid_ADD_FBA_VALUE (Name, namef, Bandwidth, Bandwidth, bandwidth, formantType, L"50.0", (Hz), (value>0), L"Bandwidth must be greater than zero.") \
KlattGrid_ADD_FBA_VALUE (Name, namef, Amplitude, Amplitude, amplitude, formantType, L"0.0", (dB), (NUMdefined(value)), L"Amplitude must be defined.") \
KlattGrid_REMOVE_FBA_VALUE (Name, namef, Formant, Frequency, frequency, formantType) \
KlattGrid_REMOVE_FBA_VALUE (Name, namef, Bandwidth, Bandwidth, bandwidth, formantType) \
KlattGrid_REMOVE_FBA_VALUE (Name, namef, Amplitude, Amplitude, amplitude, formantType) \
KlattGrid_ADD_FORMANT(Name,namef,formantType) \
KlattGrid_REMOVE_FORMANT(Name,namef,formantType)

#define KlattGrid_FORMULA_ADD_REMOVE_FB(Name,namef,formantType) \
KlattGrid_FORMULA_FORMANT_FBA_VALUE (Name, namef, Frequencies, frequencies, L"if row = 2 then self + 200 else self fi",formantType, L" ") \
KlattGrid_FORMULA_FORMANT_FBA_VALUE (Name, namef, Bandwidths, bandwidths, L"self / 10 ; 10% of frequency",formantType,L"Warning: self is formant frequency.") \
KlattGrid_ADD_FBA_VALUE (Name, namef, Formant,Frequency, frequency, formantType, L"500.0", (Hz), (value>0), L"Frequency must be greater than zero.") \
KlattGrid_ADD_FBA_VALUE (Name, namef, Bandwidth, Bandwidth, bandwidth, formantType,  L"50.0", (Hz), (value>0), L"Bandwidth must be greater than zero.") \
KlattGrid_REMOVE_FBA_VALUE (Name, namef, Formant, Frequency, frequency, formantType) \
KlattGrid_REMOVE_FBA_VALUE (Name, namef, Bandwidth, Bandwidth, bandwidth, formantType) \
KlattGrid_ADD_FORMANT(Name,namef,formantType) \
KlattGrid_REMOVE_FORMANT(Name,namef,formantType)

#define KlattGrid_FORMULA_ADD_REMOVE_FB_DELTA(Name,namef,formantType) \
KlattGrid_FORMULA_FORMANT_FBA_VALUE (Name, namef, Frequencies, frequencies, L"if row = 2 then self + 200 else self fi",formantType, L" ") \
KlattGrid_FORMULA_FORMANT_FBA_VALUE (Name, namef, Bandwidths, bandwidths, L"self / 10 ; 10% of frequency",formantType,L"Warning: self is formant frequency.") \
KlattGrid_ADD_FBA_VALUE (Name, namef, Formant,Frequency, frequency, formantType, L"-100.0", (Hz), (value!=NUMundefined), L"Frequency must be defined.") \
KlattGrid_ADD_FBA_VALUE (Name, namef, Bandwidth, Bandwidth, bandwidth, formantType,  L"-50.0", (Hz), (value!=NUMundefined), L"Bandwidth must be defined.") \
KlattGrid_REMOVE_FBA_VALUE (Name, namef, Formant, Frequency, frequency, formantType) \
KlattGrid_REMOVE_FBA_VALUE (Name, namef, Bandwidth, Bandwidth, bandwidth, formantType) \
KlattGrid_ADD_FORMANT(Name,namef,formantType) \
KlattGrid_REMOVE_FORMANT(Name,namef,formantType)

KlattGrid_FORMULA_ADD_REMOVE_FBA(Oral,oral f,KlattGrid_ORAL_FORMANTS)
KlattGrid_FORMULA_ADD_REMOVE_FBA(Nasal,nasal f,KlattGrid_NASAL_FORMANTS)
KlattGrid_FORMULA_ADD_REMOVE_FB(NasalAnti,nasal antif,KlattGrid_NASAL_ANTIFORMANTS)
KlattGrid_FORMULA_ADD_REMOVE_FB_DELTA(Delta,delta f,KlattGrid_DELTA_FORMANTS)
KlattGrid_FORMULA_ADD_REMOVE_FBA(Tracheal,tracheal f,KlattGrid_TRACHEAL_FORMANTS)
KlattGrid_FORMULA_ADD_REMOVE_FB(TrachealAnti,tracheal antif,KlattGrid_TRACHEAL_ANTIFORMANTS)
KlattGrid_FORMULA_ADD_REMOVE_FBA(Frication,frication f,KlattGrid_FRICATION_FORMANTS)

#undef KlattGrid_FORMULA_ADD_REMOVE_FB
#undef KlattGrid_FORMULA_ADD_REMOVE
#undef KlattGrid_ADD_FORMANT_AND_BANDWDTH_TIER
#undef KlattGrid_REMOVE_FBA_VALUE
#undef KlattGrid_ADD_FBA_VALUE
#undef KlattGrid_FORMULA_FORMANT_FB_VALUE

DIRECT (KlattGrid_extractPointProcess_glottalClosures)
	WHERE (SELECTED)
	{
		if (! praat_new1 (KlattGrid_extractPointProcess_glottalClosures ((structKlattGrid *)OBJECT), NAME)) return 0;
	}
END

FORM (KlattGrid_formula_frequencies, L"KlattGrid: Formula (frequencies)", L"Formant: Formula (frequencies)...")
	KlattGrid_6formants_addCommonField (dia);
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"row is formant number, col is point number: for row from 1 to nrow do for col from 1 to ncol do F (row, col) :=")
	TEXTFIELD (L"formula", L"if row = 2 then self + 200 else self fi")
	OK
DO
	int formantType = GET_INTEGER (L"Formant type");
	WHERE (SELECTED) {
		if (! KlattGrid_formula_frequencies ((structKlattGrid *)OBJECT, formantType, GET_STRING (L"formula"), interpreter)) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (KlattGrid_formula_bandwidths, L"KlattGrid: Formula (bandwidths)", L"Formant: Formula (bandwidths)...")
	KlattGrid_6formants_addCommonField (dia);
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"row is formant number, col is point number: for row from 1 to nrow do for col from 1 to ncol do F (row, col) :=")
	TEXTFIELD (L"formula", L"if row = 2 then self + 200 else self fi")
	OK
DO
	int formantType = GET_INTEGER (L"Formant type");
	WHERE (SELECTED) {
		if (! KlattGrid_formula_bandwidths ((structKlattGrid *)OBJECT, formantType, GET_STRING (L"formula"), interpreter)) return 0;
		praat_dataChanged (OBJECT);
	}
END

#define KlattGrid_FORMANT_GET_FB_VALUE(Name,name,ForB,forb,FormB,formantType) \
FORM (KlattGrid_get##Name##Formant##ForB##AtTime, L"KlattGrid: Get " #name " " #forb " at time", 0) \
	NATURAL (L"Formant number", L"1") \
	REAL (L"Time (s)", L"0.5") \
	OK \
DO \
	Melder_informationReal (KlattGrid_get##FormB##AtTime ((structKlattGrid *)ONLY_OBJECT, formantType, GET_INTEGER (L"Formant number"), GET_REAL (L"Time")), L" (Hz)"); \
END

#define KlattGrid_FORMANT_GET_A_VALUE(Name,name,formantType) \
FORM (KlattGrid_get##Name##FormantAmplitudeAtTime, L"KlattGrid: Get " #name " formant amplitude at time", 0) \
	NATURAL (L"Formant number", L"1") \
	REAL (L"Time (s)", L"0.5") \
	OK \
DO \
	Melder_informationReal (KlattGrid_getAmplitudeAtTime ((structKlattGrid *)ONLY_OBJECT, formantType, GET_INTEGER (L"Formant number"), GET_REAL (L"Time")), L" (dB)"); \
END

#define KlattGrid_FORMANT_GET_FB_VALUES(Name,name,formantType) \
KlattGrid_FORMANT_GET_FB_VALUE(Name,name,Frequency,frequency,Formant,formantType) \
KlattGrid_FORMANT_GET_FB_VALUE(Name,name,Bandwidth,bandwidth,Bandwidth,formantType)

KlattGrid_FORMANT_GET_FB_VALUES(Oral,oral,KlattGrid_ORAL_FORMANTS)
KlattGrid_FORMANT_GET_A_VALUE(Oral,oral,KlattGrid_ORAL_FORMANTS)
KlattGrid_FORMANT_GET_FB_VALUES(Nasal,nasal,KlattGrid_NASAL_FORMANTS)
KlattGrid_FORMANT_GET_A_VALUE(Nasal,nasal,KlattGrid_NASAL_FORMANTS)
KlattGrid_FORMANT_GET_FB_VALUES(NasalAnti,nasal anti,KlattGrid_NASAL_ANTIFORMANTS)
KlattGrid_FORMANT_GET_FB_VALUES(Tracheal,tracheal f,KlattGrid_TRACHEAL_FORMANTS)
KlattGrid_FORMANT_GET_A_VALUE(Tracheal,tracheal f,KlattGrid_TRACHEAL_FORMANTS)
KlattGrid_FORMANT_GET_FB_VALUES(Delta,delta f,KlattGrid_DELTA_FORMANTS)
KlattGrid_FORMANT_GET_FB_VALUES(TrachealAnti,tracheal antif,KlattGrid_TRACHEAL_ANTIFORMANTS)
KlattGrid_FORMANT_GET_FB_VALUES(Frication,frication,KlattGrid_FRICATION_FORMANTS)
KlattGrid_FORMANT_GET_A_VALUE(Frication,frication,KlattGrid_FRICATION_FORMANTS)

#undef KlattGrid_FORMANT_GET_FB_VALUES
#undef KlattGrid_FORMANT_GET_A_VALUE

#define KlattGrid_EXTRACT_FORMANT_GRID(Name,gridType) \
DIRECT (KlattGrid_extract##Name##FormantGrid) \
	WHERE (SELECTED) \
	{ \
		if (! praat_new1 (KlattGrid_extractFormantGrid ((structKlattGrid *)OBJECT, gridType), formant_names[gridType])) return 0; \
	} \
END

#define KlattGrid_EXTRACT_FORMANT_AMPLITUDE(Name,name,formantType) \
FORM (KlattGrid_extract##Name##FormantAmplitudeTier, L"KlattGrid: Extract " #name " formant amplitude tier", 0) \
	NATURAL (L"Formant number", L"1") \
	OK \
DO \
	WHERE (SELECTED) \
	 { \
		if (! praat_new1 (KlattGrid_extractAmplitudeTier ((structKlattGrid *)OBJECT, formantType, GET_INTEGER (L"Formant number")), formant_names[formantType])) return 0; \
	} \
END


KlattGrid_EXTRACT_FORMANT_GRID (Oral, KlattGrid_ORAL_FORMANTS)
KlattGrid_EXTRACT_FORMANT_AMPLITUDE (Oral, oral, KlattGrid_ORAL_FORMANTS)
KlattGrid_EXTRACT_FORMANT_GRID (Nasal, KlattGrid_NASAL_FORMANTS)
KlattGrid_EXTRACT_FORMANT_AMPLITUDE (Nasal, nasal, KlattGrid_NASAL_FORMANTS)
KlattGrid_EXTRACT_FORMANT_GRID (Frication, KlattGrid_FRICATION_FORMANTS)
KlattGrid_EXTRACT_FORMANT_AMPLITUDE (Frication, frication, KlattGrid_FRICATION_FORMANTS)
KlattGrid_EXTRACT_FORMANT_GRID (Tracheal, KlattGrid_TRACHEAL_FORMANTS)
KlattGrid_EXTRACT_FORMANT_AMPLITUDE (Tracheal, tracheal, KlattGrid_TRACHEAL_FORMANTS)
KlattGrid_EXTRACT_FORMANT_GRID (NasalAnti, KlattGrid_NASAL_ANTIFORMANTS)
KlattGrid_EXTRACT_FORMANT_GRID (TrachealAnti, KlattGrid_TRACHEAL_ANTIFORMANTS)
KlattGrid_EXTRACT_FORMANT_GRID (Delta, KlattGrid_DELTA_FORMANTS)

#undef KlattGrid_EXTRACT_FORMANTGRID

#define KlattGrid_REPLACE_FORMANT_GRID(Name,formantType) \
DIRECT (KlattGrid_replace##Name##FormantGrid) \
WHERE (SELECTED) \
{ \
		if (! KlattGrid_replaceFormantGrid ((structKlattGrid *)ONLY(classKlattGrid), formantType, (structFormantGrid *)ONLY(classFormantGrid))) return 0; \
		praat_dataChanged (OBJECT); \
	} \
END

#define KlattGrid_REPLACE_FORMANT_AMPLITUDE(Name,name,formantType) \
FORM (KlattGrid_replace##Name##FormantAmplitudeTier, L"KlattGrid: Replace " #name " formant amplitude tier", 0) \
	NATURAL (L"Formant number", L"1") \
	OK \
DO \
	WHERE (SELECTED) \
	{ \
		if (! KlattGrid_replaceAmplitudeTier ((structKlattGrid *)ONLY(classKlattGrid), formantType, GET_INTEGER (L"Formant number"), (structIntensityTier *)ONLY(classIntensityTier))) return 0; \
		praat_dataChanged (OBJECT); \
	} \
END

KlattGrid_REPLACE_FORMANT_GRID (Oral, KlattGrid_ORAL_FORMANTS)
KlattGrid_REPLACE_FORMANT_AMPLITUDE (Oral, oral, KlattGrid_ORAL_FORMANTS)
KlattGrid_REPLACE_FORMANT_GRID (Nasal, KlattGrid_NASAL_FORMANTS)
KlattGrid_REPLACE_FORMANT_AMPLITUDE (Nasal, nasal, KlattGrid_NASAL_FORMANTS)
KlattGrid_REPLACE_FORMANT_GRID (NasalAnti, KlattGrid_NASAL_ANTIFORMANTS)
KlattGrid_REPLACE_FORMANT_GRID (Tracheal, KlattGrid_TRACHEAL_FORMANTS)
KlattGrid_REPLACE_FORMANT_AMPLITUDE (Tracheal, tracheal, KlattGrid_TRACHEAL_FORMANTS)
KlattGrid_REPLACE_FORMANT_GRID (TrachealAnti, KlattGrid_TRACHEAL_ANTIFORMANTS)
KlattGrid_REPLACE_FORMANT_GRID (Delta, KlattGrid_DELTA_FORMANTS)
KlattGrid_REPLACE_FORMANT_GRID (Frication, KlattGrid_FRICATION_FORMANTS)
KlattGrid_REPLACE_FORMANT_AMPLITUDE (Frication, frication, KlattGrid_FRICATION_FORMANTS)

#undef KlattGrid_REPLACE_FORMANT_AMPLITUDE
#undef KlattGrid_REPLACE_FORMANTGRID

#define KlattGrid_FORMANT_GET_ADD_REMOVE(Name,name,unit,default,require,requiremessage) \
FORM (KlattGrid_get##Name##AtTime, L"KlattGrid: Get " #name " at time", 0) \
	KlattGrid_6formants_addCommonField (dia); \
	NATURAL (L"Formant number", L"1") \
	REAL (L"Time (s)", L"0.5") \
	OK \
DO \
	int formantType = GET_INTEGER (L"Formant type"); \
	Melder_informationReal (KlattGrid_get##Name##AtTime ((structKlattGrid *)ONLY_OBJECT, formantType, GET_INTEGER (L"Formant number"), GET_REAL (L"Time")), L" (Hz)"); \
END \
FORM (KlattGrid_getDelta##Name##AtTime, L"KlattGrid: Get delta " #name " at time", 0) \
	NATURAL (L"Formant number", L"1") \
	REAL (L"Time (s)", L"0.5") \
	OK \
DO \
	Melder_informationReal (KlattGrid_getDelta##Name##AtTime ((structKlattGrid *)ONLY_OBJECT, GET_INTEGER (L"Formant number"), GET_REAL (L"Time")), L" (Hz)"); \
END \
FORM (KlattGrid_add##Name##Point, L"KlattGrid: Add " #name " point", 0) \
	KlattGrid_6formants_addCommonField (dia); \
	NATURAL (L"Formant number", L"1") \
	REAL (L"Time (s)", L"0.5") \
	REAL (L"Value" unit, default) \
	OK \
DO \
	int formantType = GET_INTEGER (L"Formant type"); \
	double value = GET_REAL (L"Value"); \
	REQUIRE (require, requiremessage) \
	WHERE (SELECTED) { \
		if (! KlattGrid_add##Name##Point ((structKlattGrid *)OBJECT, formantType, GET_INTEGER (L"Formant number"), GET_REAL (L"Time"), value)) return 0; \
		praat_dataChanged (OBJECT); \
	} \
END \
FORM (KlattGrid_addDelta##Name##Point, L"KlattGrid: Add delta " #name " point", 0) \
	NATURAL (L"Formant number", L"1") \
	REAL (L"Time (s)", L"0.5") \
	REAL (L"Value" unit, default) \
	OK \
DO \
	double value = GET_REAL (L"Value"); \
	REQUIRE (require, requiremessage) \
	WHERE (SELECTED) { \
		if (! KlattGrid_addDelta##Name##Point ((structKlattGrid *)OBJECT, GET_INTEGER (L"Formant number"), GET_REAL (L"Time"), value)) return 0; \
		praat_dataChanged (OBJECT); \
	} \
END \
FORM (KlattGrid_remove##Name##Points, L"Remove " #name " points", 0) \
	KlattGrid_6formants_addCommonField (dia); \
	NATURAL (L"Formant number", L"1") \
	REAL (L"From time (s)", L"0.3")\
	REAL (L"To time (s)", L"0.7") \
	OK \
DO \
	int formantType = GET_INTEGER (L"Formant type"); \
	WHERE (SELECTED) { \
		KlattGrid_remove##Name##Points ((structKlattGrid *)OBJECT, formantType, GET_INTEGER (L"Formant number"), GET_REAL (L"From time"), GET_REAL (L"To time")); \
		praat_dataChanged (OBJECT);\
	} \
END \
FORM (KlattGrid_removeDelta##Name##Points, L"Remove delta " #name " points", 0) \
	NATURAL (L"Formant number", L"1") \
	REAL (L"From time (s)", L"0.3")\
	REAL (L"To time (s)", L"0.7") \
	OK \
DO \
	WHERE (SELECTED) { \
		KlattGrid_removeDelta##Name##Points ((structKlattGrid *)OBJECT, GET_INTEGER (L"Formant number"), GET_REAL (L"From time"), GET_REAL (L"To time")); \
		praat_dataChanged (OBJECT);\
	} \
END

KlattGrid_FORMANT_GET_ADD_REMOVE (Formant, formant, L" (Hz)", L"500.0", (value>0), L"Frequency must be greater than zero.")
KlattGrid_FORMANT_GET_ADD_REMOVE (Bandwidth, bandwidth, L" (Hz)", L"50.0", (value>0), L"Bandwidth must be greater than zero.")

#undef KlattGrid_FORMANT_GET_ADD_REMOVE

FORM (KlattGrid_addFormantAndBandwidthTier, L"ui/editors/AmplitudeTierEditor.h", 0)
	KlattGrid_7formants_addCommonField (dia);
	INTEGER (L"Position", L"0 (=at end)")
	OK
DO
	long gridType = GET_INTEGER (L"Formant type");
	WHERE (SELECTED) {
		if (! KlattGrid_addFormantAndBandwidthTier ((structKlattGrid *)OBJECT, gridType, GET_INTEGER (L"Position"))) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (KlattGrid_extractFormantGrid, L"KlattGrid: Extract formant grid", 0)
	KlattGrid_6formants_addCommonField (dia);
	OK
DO
	long gridType = GET_INTEGER (L"Formant type");
	WHERE (SELECTED) {
		if (! praat_new1 (KlattGrid_extractFormantGrid ((structKlattGrid *)OBJECT, gridType), formant_names[gridType])) return 0;
	}
END

FORM (KlattGrid_replaceFormantGrid, L"KlattGrid: Replace formant grid", 0)
	KlattGrid_6formants_addCommonField (dia);
	OK
DO
	WHERE (SELECTED) {
		if (! KlattGrid_replaceFormantGrid ((structKlattGrid *)ONLY(classKlattGrid), GET_INTEGER (L"Formant type"), (structFormantGrid *)ONLY(classFormantGrid))) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (KlattGrid_getAmplitudeAtTime, L"KlattGrid: Get amplitude at time", 0) \
	KlattGrid_4formants_addCommonField (dia);
	NATURAL (L"Formant number", L"1")
	REAL (L"Time (s)", L"0.5")
	OK
DO
	int formantType = GET_INTEGER (L"Formant type");
	Melder_informationReal (KlattGrid_getAmplitudeAtTime ((structKlattGrid *)ONLY_OBJECT, formantType, GET_INTEGER (L"Formant number"), GET_REAL (L"Time")), L" (dB)");
END

FORM (KlattGrid_addAmplitudePoint, L"KlattGrid: Add amplitude point", 0)
	KlattGrid_4formants_addCommonField (dia);
	NATURAL (L"Formant number", L"1")
	REAL (L"Time (s)", L"0.5")
	REAL (L"Value (Hz)", L"80.0")
	OK
DO
	int formantType = GET_INTEGER (L"Formant type");
	double value = GET_REAL (L"Value");
	WHERE (SELECTED) {
		if (! KlattGrid_addAmplitudePoint ((structKlattGrid *)OBJECT, formantType, GET_INTEGER (L"Formant number"), GET_REAL (L"Time"), value)) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (KlattGrid_removeAmplitudePoints, L"Remove amplitude points", 0) \
	KlattGrid_4formants_addCommonField (dia);
	NATURAL (L"Formant number", L"1")
	REAL (L"From time (s)", L"0.3")
	REAL (L"To time (s)", L"0.7")
	OK
DO
	int formantType = GET_INTEGER (L"Formant type");
	WHERE (SELECTED) {
		KlattGrid_removeAmplitudePoints ((structKlattGrid *)OBJECT, formantType, GET_INTEGER (L"Formant number"), GET_REAL (L"From time"), GET_REAL (L"To time"));
		praat_dataChanged (OBJECT);
	}
END

FORM (KlattGrid_extractAmplitudeTier, L"ui/editors/AmplitudeTierEditor.h", 0)
	KlattGrid_4formants_addCommonField (dia);
	NATURAL (L"Formant number", L"1")
	OK
DO
	int formantType = GET_INTEGER (L"Formant type");
	WHERE (SELECTED) {
		if (! praat_new1 (KlattGrid_extractAmplitudeTier ((structKlattGrid *)OBJECT, formantType, GET_INTEGER (L"Formant number")), formant_names[formantType])) return 0;
	}
END

FORM (KlattGrid_replaceAmplitudeTier, L"KlattGrid: Replace amplitude tier", 0)
	KlattGrid_4formants_addCommonField (dia);
	NATURAL (L"Formant number", L"1")
	OK
DO
	int formantType = GET_INTEGER (L"Formant type");
	WHERE (SELECTED) {
		if (! KlattGrid_replaceAmplitudeTier ((structKlattGrid *)ONLY(classKlattGrid), formantType, GET_INTEGER (L"Formant number"), (structIntensityTier *)ONLY(classIntensityTier))) return 0;
		praat_dataChanged (OBJECT);
	}
END

FORM (KlattGrid_to_Sound_special, L"KlattGrid: To Sound (special)", L"KlattGrid: To Sound (special)...")
	KlattGrid_PlayOptions_addCommonFields (dia, 1);
	OK
DO
	WHERE(SELECTED)
	{
		KlattGrid thee = (structKlattGrid *)OBJECT;
		KlattGrid_setDefaultPlayOptions (thee);
		KlattGrid_PlayOptions_getCommonFields (dia, 1, thee);
		if (! praat_new1 (KlattGrid_to_Sound (thee), NAME)) return 0;
	}
END

DIRECT (KlattGrid_to_Sound)
	WHERE(SELECTED)
	{
		KlattGrid thee = (structKlattGrid *)OBJECT;
		KlattGrid_setDefaultPlayOptions (thee);
		if (! praat_new1 (KlattGrid_to_Sound (thee), NAME)) return 0;
	}
END

FORM (KlattGrid_playSpecial, L"KlattGrid: Play special", L"KlattGrid: Play special...")
	KlattGrid_PlayOptions_addCommonFields (dia, 0);
	OK
DO
	WHERE(SELECTED)
	{
		KlattGrid thee = (structKlattGrid *)OBJECT;
		KlattGrid_setDefaultPlayOptions (thee);
		KlattGrid_PlayOptions_getCommonFields (dia, 0, thee);
		if (! KlattGrid_playSpecial (thee)) return 0;
	}
END

FORM (KlattGrid_to_Sound_phonation, L"KlattGrid: To Sound (phonation)", L"KlattGrid: To Sound (phonation)...")
	POSITIVE (L"Sampling frequency (Hz)", L"44100")
	KlattGrid_PhonationGridPlayOptions_addCommonFields (dia);
	OK
DO
	WHERE(SELECTED)
	{
		KlattGrid thee = (structKlattGrid *)OBJECT;
		KlattGrid_PhonationGridPlayOptions_getCommonFields (dia, thee);
		thy options -> samplingFrequency = GET_REAL (L"Sampling frequency");
		if (! praat_new2 (KlattGrid_to_Sound_phonation (thee), NAME, L"_phonation")) return 0;
	}
END

DIRECT (KlattGrid_help) Melder_help (L"KlattGrid"); END

DIRECT (KlattGrid_play)
	EVERY_CHECK (KlattGrid_play ((structKlattGrid *)OBJECT))
END

// y is the heigth in units of the height of one section,
// y1 is the heigth from the top to the split between the uppper, non-diffed, and lower diffed part
static void _KlattGrid_queryParallelSplit (KlattGrid me, double dy, double *y, double *y1)
{
	long ny = my vocalTract -> nasal_formants -> formants -> size + my vocalTract -> oral_formants -> formants -> size + my coupling -> tracheal_formants -> formants -> size;
	long n1 = my vocalTract -> nasal_formants -> formants -> size + (my vocalTract -> oral_formants -> formants -> size > 0 ? 1 : 0);

	long n2 = ny - n1;
	if (ny == 0) { *y = 0; *y1 = 0; return; }

	*y = ny + (ny - 1) * dy;

	if (n1 == 0) { *y1 = 0.5; }
	else if (n2 == 0) { *y1 = *y - 0.5; }
	else { *y1 = n1 + (n1 - 1) * dy + 0.5 * dy; }
	return;
}

static void getYpositions (double h1, double h2, double h3, double h4, double h5, double fractionOverlap, double *dy, double *ymin1, double *ymax1, double *ymin2, double *ymax2, double *ymin3, double *ymax3)
{
	// Given: five 'blocks' with relative heights h1..h5 in arbitrary units.
	// Problem: scale all h1..h5 such that:
	// 1. blocks h1 and h2 form one unit, with h1 on top of h2, the quotient h1/h2 is fixed
	// 2. blocks h3 and h4 form one unit, with h3 on top of h4, the quotient h3/h4 is fixed
	// 3. blocks h1 and h3 have the same baseline.
	// 4. h5 is always underneath (h1,h2) but may partially overlap (0.45) with h4.
	// 5. After scaling the new h1+h2 >= 0.3
	// 6. Optimally use the vertical space from 0.. 1, i.e the top of h1 or h3 is at 1,
	// the bottom of h5 is at 0. Preferably scale all blocks with the same factor, if not possible than
	// scale h3,h4 and h5 the same
	//
	// h1  h3
	// h2  h4
	//  h5
	/* Cases:
                  x             x       ^
         x      x x    x      x x       |
      h1 x x    x x    x x    x x h3    | h13
         -----------------------------------------------------------
      h2 x x    x x    x x    x x h4
         x      x      x x    x x
                         x      x
         x      x      x x    x x
      h5 x      x      x      x
         x      x      x      x
	*/
	double h; // h12_min = 0.3; not yet
	double h13 = h1 > h3 ? h1 : h3; // baselines are now equal
	if (h2 >= h4)
	{
		h = h13 + h2 + h5;
	}
	else // h2 < h4
	{
		double maximumOverlap3 = fractionOverlap * h5;
		if (maximumOverlap3 < (h1 + h2)) maximumOverlap3 = 0;
		else if (maximumOverlap3 > (h4 - h2)) maximumOverlap3 = h4 - h2;
		h = h13 + h4 + h5 - maximumOverlap3;
	}
	*dy = 1 / (1.1 * h);
	*ymin1 = 1 - (h13 + h2) * *dy; *ymax1 = *ymin1 + (h1 + h2) * *dy;
	*ymin2 = 1 - (h13 + h4) * *dy; *ymax2 = *ymin2 + (h3 + h4) * *dy;
	*ymin3 = 0;  *ymax3 = h5 * *dy;
}

static void rel_to_abs (double *w, double *ws, long n, double d)
{
	long i; double sum;

	for (sum = 0, i = 1; i <= n; i++) // relative
	{
		sum += w[i];
	}
	if (sum != 0)
	{
		double dw = d / sum;
		for (sum = 0, i = 1; i <= n; i++) // to absolute
		{
			w[i] *= dw;
			sum += w[i];
			ws[i] = sum;
		}
	}
}

// Calculates the intersection point (xi,yi) of a line with a circle.
// The line starts at the origin and P (xp, yp) is on that line.
static void NUMcircle_radial_intersection_sq (double x, double y, double r, double xp, double yp, double *xi, double *yi)
{
	double dx = xp - x, dy = yp - y;
	double d = sqrt (dx * dx + dy * dy);
	if (d > 0)
	{
		*xi = x + dx * r / d;
		*yi = y + dy * r / d;
	}
	else { *xi = *yi = NUMundefined; }
}

typedef struct structconnections { long numberOfConnections; double *x, *y;} *connections;

static void connections_free (connections me)
{
	if (me == NULL) return;
	NUMdvector_free (my x, 1);
	NUMdvector_free (my y, 1);
	Melder_free (me);
}

static connections connections_create (long numberOfConnections)
{
	connections me = (connections) _Melder_malloc_e (sizeof (struct structconnections));
	if (me == NULL) return NULL;
	my numberOfConnections = numberOfConnections;
	my x = NUMdvector (1, numberOfConnections);
	if (my x == NULL) goto end;
	my y = NUMdvector (1, numberOfConnections);
end:
	if (Melder_hasError ()) connections_free (me);
	return me;
}

static void summer_draw (Graphics g, double x, double y, double r, int alternating)
{
	Graphics_setLineWidth (g, 2);
	Graphics_circle (g, x, y, r);
	double dy = 3 * r / 4;
	// + symbol
	if (alternating) y += r / 4;
	Graphics_line (g, x, y + r / 2, x, y - r / 2);
	Graphics_line (g, x - r / 2, y, x + r / 2, y);
	if (alternating) Graphics_line (g, x - r / 2, y - dy , x + r / 2, y - dy);
}

static void _summer_drawConnections (Graphics g, double x, double y, double r, connections thee, int arrow, int alternating, double horizontalFraction)
{
	summer_draw (g, x, y, r, alternating);

	for (long i = 1; i <= thy numberOfConnections; i++)
	{
		double xto, yto, xp = thy x[i], yp = thy y[i];
		if (horizontalFraction > 0)
		{
			double dx = x - xp;
			if (dx > 0)
			{
				xp += horizontalFraction * dx;
				Graphics_line (g, thy x[i], yp, xp, yp);
			}
		}
		NUMcircle_radial_intersection_sq (x, y, r, xp, yp, &xto, &yto);
		if (xto == NUMundefined || yto == NUMundefined) continue;
		if (arrow) Graphics_arrow (g, xp, yp, xto, yto);
		else Graphics_line (g, xp, yp, xto, yto);
	}
}

static void summer_drawConnections (Graphics g, double x, double y, double r, connections thee, int arrow, double horizontalFraction)
{
	_summer_drawConnections (g, x, y, r, thee, arrow, 0, horizontalFraction);
}

static void alternatingSummer_drawConnections (Graphics g, double x, double y, double r, connections thee, int arrow, double horizontalFraction)
{
	_summer_drawConnections (g, x, y, r, thee, arrow, 1, horizontalFraction);
}

static void draw_oneSection (Graphics g, double xmin, double xmax, double ymin, double ymax, wchar_t *line1, wchar_t *line2, wchar_t *line3)
{
	long numberOfTextLines = 0, iline = 0;
	Graphics_rectangle (g, xmin, xmax, ymin, ymax);
	if (line1 != NULL) numberOfTextLines++;
	if (line2 != NULL) numberOfTextLines++;
	if (line3 != NULL) numberOfTextLines++;
	double y = ymax, dy = (ymax - ymin) / (numberOfTextLines + 1), ddy = dy / 10;
	double x = (xmax + xmin) / 2;
	if (line1 != NULL)
	{
		iline++;
		y -= dy - (numberOfTextLines == 2 ? ddy : 0); // extra spacing for two lines
		Graphics_text1 (g, x, y, line1);
	}
	if (line2 != NULL)
	{
		iline++;
		y -= dy - (numberOfTextLines == 2 ? (iline == 1 ? ddy : -iline * ddy) : 0);
		Graphics_text1 (g, x, y, line2);
	}
	if (line3 != NULL)
	{
		iline++;
		y -= dy - (numberOfTextLines == 2 ? -iline * ddy : 0);
		Graphics_text1 (g, x, y, line3);
	}
}

static void PhonationGrid_draw_inside (PhonationGrid me, Graphics g, double xmin, double xmax, double ymin, double ymax, double dy, double *yout)
{
	// dum voicing conn tilt conn summer
	(void) me;
	double xw[6] = { 0, 1, 0.5, 1, 0.5, 0.5 }, xws[6];
	double x1, y1, x2, y2, xs, ys, ymid, r;
	int arrow = 1;

	connections thee = connections_create (2);
	if (thee == NULL) return;

	rel_to_abs (xw, xws, 5, xmax - xmin);

	dy = (ymax - ymin) / (1 + (dy < 0 ? 0 : dy) + 1);

	x1 = xmin; x2 = x1 + xw[1];
	y2 = ymax; y1 = y2 - dy;
	draw_oneSection (g, x1, x2, y1, y2, NULL, L"Voicing", NULL);

	x1 = x2; x2 = x1 + xw[2]; ymid = (y1 + y2) / 2;
	Graphics_line (g, x1, ymid, x2, ymid);

	x1 = x2; x2 = x1 + xw[3];
	draw_oneSection (g, x1, x2, y1, y2, NULL, L"Tilt", NULL);

	thy x[1] = x2; thy y[1] = ymid;

	y2 = y1 - 0.5 * dy; y1 = y2 - dy; ymid = (y1 + y2) / 2;
	x2 = xmin + xws[3]; x1 = x2 - 1.5 * xw[3]; // some extra space
	draw_oneSection (g, x1, x2, y1, y2, NULL, L"Aspiration", NULL);

	thy x[2] = x2; thy y[2] = ymid;

	r = xw[5] / 2;
	xs = xmax - r; ys = (ymax + ymin) / 2;

	summer_drawConnections (g, xs, ys, r, thee, arrow, 0.4);
	connections_free (thee);

	if (yout != NULL) *yout = ys;
}

void PhonationGrid_draw (PhonationGrid me, Graphics g)
{
	double xmin = 0, xmax2 = 0.9, xmax = 1, ymin = 0, ymax = 1, dy = 0.5, yout;

	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);
	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	PhonationGrid_draw_inside (me, g, xmin, xmax2, ymin, ymax, dy, &yout);

	Graphics_arrow (g, xmax2, yout, xmax, yout);
	Graphics_unsetInner (g);
}

static void VocalTractGrid_CouplingGrid_drawCascade_inline (VocalTractGrid me, CouplingGrid thee, Graphics g, double xmin, double xmax, double ymin, double ymax, double *yin, double *yout)
{
	long numberOfOralFormants = my oral_formants -> formants -> size;
	long numberOfNasalFormants = my nasal_formants -> formants -> size;
	long numberOfNasalAntiFormants = my nasal_antiformants -> formants -> size;
	long numberOfTrachealFormants = thee != NULL ? thy tracheal_formants -> formants -> size : 0;
	long numberOfTrachealAntiFormants = thee != NULL ? thy tracheal_antiformants -> formants -> size : 0;
 	double x1, y1 = ymin, x2, y2 = ymax, dx, ddx = 0.2, ymid = (y1 + y2) / 2;
 	wchar_t *text[6] = { 0, L"TF", L"TAF", L"NF", L"NAF", L""};
 	long nf[6] = {0, numberOfTrachealFormants, numberOfTrachealAntiFormants, numberOfNasalFormants, numberOfNasalAntiFormants, numberOfOralFormants};
	long numberOfFilters, numberOfXSections = 5, nsx = 0, isection, i;
	MelderString ff = { 0 }, fb = { 0 };

	numberOfFilters = numberOfNasalFormants + numberOfNasalAntiFormants + numberOfTrachealFormants + numberOfTrachealAntiFormants + numberOfOralFormants;

	if (numberOfFilters == 0)
	{
		x2 = xmax;
		Graphics_line (g, xmin, ymid, x2, ymid);
		goto end;
	}

	for (isection = 1; isection <= numberOfXSections; isection++) if (nf[isection] > 0) nsx++;
	dx = (xmax - xmin) / (numberOfFilters  + (nsx - 1) * ddx);

	x1 = xmin;
	for (isection = 1; isection <= numberOfXSections; isection++)
	{
		long numberOfFormants = nf[isection];

		if (numberOfFormants == 0) continue;

		x2 = x1 + dx;
		for (i = 1; i <= numberOfFormants; i++)
		{
			MelderString_append2 (&ff, L"F", Melder_integer (i));
			MelderString_append2 (&fb, L"B", Melder_integer (i));
			draw_oneSection (g, x1, x2, y1, y2, text[isection], ff.string, fb.string);
			if (i < numberOfFormants) { x1 = x2; x2 = x1 + dx; }
			MelderString_empty (&ff); MelderString_empty (&fb);
		}

		if (isection < numberOfXSections)
		{
			x1 = x2; x2 = x1 + ddx * dx;
			Graphics_line (g, x1, ymid, x2, ymid);
			x1 = x2;
		}
	}
end:
	if (yin != NULL) *yin = ymid;
	if (yout != NULL) *yout = ymid;

	MelderString_free (&ff); MelderString_free (&fb);
}

static void VocalTractGrid_CouplingGrid_drawParallel_inline (VocalTractGrid me, CouplingGrid thee, Graphics g, double xmin, double xmax, double ymin, double ymax, double dy, double *yin, double *yout)
{
	// (0: filler) (1: hor. line to split) (2: split to diff) (3: diff) (4: diff to split)
	// (5: split to filter) (6: filters) (7: conn to summer) (8: summer)
	double xw[9] = { 0, 0.3, 0.2, 1.5, 0.5, 0.5, 1, 0.5, 0.5 }, xws[9];
	long i, isection, numberOfXSections = 8, ic = 0, numberOfYSections = 4;
	long numberOfNasalFormants = my nasal_formants -> formants -> size;
	long numberOfOralFormants = my oral_formants -> formants -> size;
	long numberOfTrachealFormants = thee != NULL ? thy tracheal_formants -> formants -> size : 0;
	long numberOfFormants = numberOfNasalFormants + numberOfOralFormants + numberOfTrachealFormants;
	long numberOfUpperPartFormants = numberOfNasalFormants + (numberOfOralFormants > 0 ? 1 : 0);
	long numberOfLowerPartFormants = numberOfFormants - numberOfUpperPartFormants;
	double ddy = dy < 0 ? 0 : dy, x1, y1, x2, y2, x3, r, ymid;
 	wchar_t *text[5] = { 0, L"Nasal", L"", L"", L"Tracheal"};
 	long nffrom[5] = {0, 1, 1, 2, 1};
 	long nfto[5] = {0, numberOfNasalFormants, (numberOfOralFormants > 0 ? 1 : 0), numberOfOralFormants, numberOfTrachealFormants};
	MelderString fba = { 0 };

	rel_to_abs (xw, xws, numberOfXSections, xmax - xmin);

	connections local_in, local_out;

	if (numberOfFormants == 0)
	{
		y1 = y2 = (ymin + ymax) / 2;
		Graphics_line (g, xmin, y1, xmax, y1);
		goto end;
	}

	{
		dy = (ymax - ymin) / (numberOfFormants * (1 + ddy) - ddy);

		local_in = connections_create (numberOfFormants);
		if (local_in == NULL) return;
		local_out = connections_create (numberOfFormants);
		if (local_out == NULL) goto end;

		// parallel section
		x1 = xmin + xws[5]; x2 = x1 + xw[6]; y2 = ymax;
		x3 = xmin + xws[4];
		for (isection = 1; isection <= numberOfYSections; isection++)
		{
			long ifrom = nffrom[isection], ito = nfto[isection];
			if (ito < ifrom) continue;
			for (i = ifrom; i <= ito; i++)
			{
				y1 = y2 - dy; ymid = (y1 + y2) / 2;
				const wchar_t *fi = Melder_integer (i);
				MelderString_append6 (&fba, L"A", fi, L" F", fi, L" B", fi);
				draw_oneSection (g, x1, x2, y1, y2, text[isection], fba.string, NULL);
				Graphics_line (g, x3, ymid, x1, ymid); // to the left
				ic++;
				local_in -> x[ic] = x3; local_out -> x[ic] = x2;
				local_in -> y[ic] = local_out -> y[ic] = ymid;
				y2 = y1 - 0.5 * dy;
				MelderString_empty (&fba);
			}
		}

		ic = 0;
		x1 = local_in -> y[1];
		if (numberOfUpperPartFormants > 0)
		{
			x1 = local_in -> x[numberOfUpperPartFormants]; y1 = local_in -> y[numberOfUpperPartFormants];
			if (numberOfUpperPartFormants > 1) Graphics_line (g, x1, y1, local_in -> x[1], local_in -> y[1]); // vertical
			x2 = xmin;
			if (numberOfLowerPartFormants > 0) { x2 += xw[1]; }
			Graphics_line (g, x1, y1, x2, y1); // done
		}
		if (numberOfLowerPartFormants > 0)
		{
			long ifrom = numberOfUpperPartFormants + 1;
			x1 = local_in -> x[ifrom]; y1 = local_in -> y[ifrom]; // at the split
			if (numberOfLowerPartFormants > 1) Graphics_line (g, x1, y1, local_in -> x[numberOfFormants], local_in -> y[numberOfFormants]); // vertical
			x2 = xmin + xws[3]; // right of diff
			Graphics_line (g, x1, y1, x2, y1); // from vertical to diff
			x1 = xmin + xws[2]; // left of diff
			draw_oneSection (g, x1, x2, y1 + 0.5 * dy, y1 - 0.5 * dy, L"Pre-emphasis", NULL, NULL);
			x2 = x1;
			if (numberOfUpperPartFormants > 0)
			{
				x2 = xmin + xw[1]; y2 = y1; // at split
				Graphics_line (g, x1, y1, x2, y1); // to split
				y1 += (1 + ddy) * dy;
				Graphics_line (g, x2, y2, x2, y1); // vertical
				y1 -= 0.5 * (1 + ddy) * dy;
			}
			Graphics_line (g, xmin, y1, x2, y1);
		}

		r = xw[8] / 2;
		x2 = xmax - r; y2 = (ymin + ymax) / 2;

		alternatingSummer_drawConnections (g, x2, y2, r, local_out, 1, 0.4);
	}

end:

	connections_free (local_out); connections_free (local_in);

	if (yin != NULL) *yin = y1;
	if (yout != NULL) *yout = y2;

	MelderString_free (&fba);
}

static void VocalTractGrid_CouplingGrid_draw_inside (VocalTractGrid me, CouplingGrid thee, Graphics g, int filterModel, double xmin, double xmax, double ymin, double ymax, double dy, double *yin, double *yout)
{
	filterModel == KlattGrid_FILTER_CASCADE ?
		VocalTractGrid_CouplingGrid_drawCascade_inline (me, thee, g, xmin, xmax, ymin, ymax, yin, yout) :
		VocalTractGrid_CouplingGrid_drawParallel_inline (me, thee, g, xmin, xmax, ymin, ymax, dy, yin, yout);
}

static void VocalTractGrid_CouplingGrid_draw (VocalTractGrid me, CouplingGrid thee, Graphics g, int filterModel)
{
	double xmin = 0, xmin1 = 0.05, xmax1 = 0.95, xmax = 1, ymin = 0, ymax = 1, dy = 0.5, yin, yout;

	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);
	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	Graphics_setLineWidth (g, 2);
	VocalTractGrid_CouplingGrid_draw_inside (me, thee, g, filterModel, xmin1, xmax1, ymin, ymax, dy, &yin, &yout);
	Graphics_line (g, xmin, yin, xmin1, yin);
	Graphics_arrow (g, xmax1, yout, xmax, yout);
	Graphics_unsetInner (g);
}

static void FricationGrid_draw_inside (FricationGrid me, Graphics g, double xmin, double xmax, double ymin, double ymax, double dy, double *yout)
{
	long numberOfXSections = 5;
	long numberOfFormants = my frication_formants -> formants -> size;
	long numberOfParts = numberOfFormants + (numberOfFormants > 1 ? 0 : 1) ; // 2..number + bypass
	// dum noise, connections, filter, connections, adder
	double xw[6] = { 0, 2, 0.6, 1.5, 0.6, 0.5 }, xws[6];
	double r, x1, y1, x2, y2, x3, xs, ys, ymid = (ymin + ymax) / 2;

	rel_to_abs (xw, xws, numberOfXSections, xmax - xmin);

	dy = dy < 0 ? 0 : dy;
	dy = (ymax - ymin) / (numberOfParts * (1 + dy) - dy);

	connections cp = connections_create (numberOfParts);
	if (cp == NULL) return;

	// section 1
	x1 = xmin; x2 = x1 + xw[1]; y1 = ymid - 0.5 * dy; y2 = y1 + dy;
	draw_oneSection (g, x1, x2, y1, y2, L"Frication", L"noise", NULL);

	// section 2, horizontal line halfway, vertical line
	x1 = x2; x2 = x1 + xw[2] / 2;
	Graphics_line (g, x1, ymid, x2, ymid);
	Graphics_line (g, x2, ymax - dy / 2, x2, ymin + dy / 2);
	x3 = x2;
	// final connection to section 2 , filters , connections to adder
	x1 = xmin + xws[2]; x2 = x1 + xw[3]; y2 = ymax;
	MelderString fba = { 0 };
	for (long i = 1; i <= numberOfParts; i++)
	{
		const wchar_t *fi = Melder_integer (i+1);
		y1 = y2 - dy;
		if (i < numberOfParts) { MelderString_append6 (&fba, L"A", fi, L" F", fi, L" B", fi); }
		else { MelderString_append1 (&fba,  L"Bypass"); }
		draw_oneSection (g, x1, x2, y1, y2, NULL, fba.string, NULL);
		double ymidi = (y1 + y2) / 2;
		Graphics_line (g, x3, ymidi, x1, ymidi); // from noise to filter
		cp -> x[i] = x2; cp -> y[i] = ymidi;
		y2 = y1 - 0.5 * dy;
		MelderString_empty (&fba);
	}

	r = xw[5] / 2;
	xs = xmax - r; ys = ymid;

	if (numberOfParts > 1) alternatingSummer_drawConnections (g, xs, ys, r, cp, 1, 0.4);
	else Graphics_line (g, cp -> x[1], cp -> y[1], xs + r, ys);

	connections_free (cp);

	if (yout != NULL) *yout = ys;
	MelderString_free (&fba);
}

void FricationGrid_draw (FricationGrid me, Graphics g)
{
	double xmin = 0, xmax = 1, xmax2 = 0.9, ymin = 0, ymax = 1, dy = 0.5, yout;

	Graphics_setInner (g);
	Graphics_setWindow (g, xmin, xmax, ymin, ymax);
	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	Graphics_setLineWidth (g, 2);

	FricationGrid_draw_inside (me, g, xmin, xmax2, ymin, ymax, dy, &yout);

	Graphics_arrow (g, xmax2, yout, xmax, yout);
	Graphics_unsetInner (g);
}

void KlattGrid_drawVocalTract (KlattGrid me, Graphics g, int filterModel, int withTrachea)
{
	VocalTractGrid_CouplingGrid_draw (my vocalTract, withTrachea ? my coupling : NULL, g, filterModel);
}

void KlattGrid_draw (KlattGrid me, Graphics g, int filterModel)
{
 	double xs1, xs2, ys1, ys2, xf1, xf2, yf1, yf2;
 	double xp1, xp2, yp1, yp2, xc1, xc2, yc1, yc2;
 	double dy, r, xs, ys;
 	double xmin = 0, xmax2 = 0.90, xmax3 = 0.95, xmax = 1, ymin = 0, ymax = 1;
	double xws[6];
	double height_phonation = 0.3;
	double dy_phonation = 0.5, dy_vocalTract_p = 0.5, dy_frication = 0.5;

	connections tf = connections_create (2);
	if (tf == NULL) return;

	Graphics_setInner (g);

	Graphics_setWindow (g, xmin, xmax, ymin, ymax);
	Graphics_setTextAlignment (g, Graphics_CENTRE, Graphics_HALF);
	Graphics_setLineWidth (g, 2);

	long nff = my frication -> frication_formants -> formants -> size - 1 + 1;
	double yh_frication = nff > 0 ? nff + (nff - 1) * dy_frication : 1;
	double yh_phonation = 1 + dy_phonation + 1;
	double yout_phonation, yout_frication;
	dy = height_phonation / yh_phonation; // 1 vertical unit in source section height units

	if (filterModel == KlattGrid_FILTER_CASCADE) // Cascade section
	{
		// source connection tract connection, out
		//     frication
		double xw[6] = {0, 1.75, 0.125, 3, 0.25, 0.125 };
		double yin_vocalTract_c, yout_vocalTract_c;

		rel_to_abs (xw, xws, 5, xmax2 - xmin);

		// limit height of frication unit dy !

		height_phonation = yh_phonation / (yh_phonation + yh_frication);
		if (height_phonation < 0.3) height_phonation = 0.3;
		dy = height_phonation / yh_phonation;

		xs1 = xmin; xs2 = xs1 + xw[1]; ys2 = ymax; ys1 = ys2 - height_phonation;
		PhonationGrid_draw_inside (my phonation, g, xs1, xs2, ys1, ys2, dy_phonation, &yout_phonation);

		// units in cascade have same heigth as units in source part.

		xc1 = xmin + xws[2]; xc2 = xc1 + xw[3];
		yc2 = yout_phonation + dy / 2; yc1 = yc2 - dy;
		VocalTractGrid_CouplingGrid_drawCascade_inline (my vocalTract, my coupling, g, xc1, xc2, yc1, yc2, &yin_vocalTract_c, &yout_vocalTract_c);

		tf -> x[1] = xc2; tf -> y[1] = yout_vocalTract_c;

		Graphics_line (g, xs2, yout_phonation, xc1, yin_vocalTract_c);

		xf1 = xmin + xws[2]; xf2 = xf1 + xw[3]; yf2 = ymax - height_phonation; yf1 = 0;

		FricationGrid_draw_inside (my frication, g, xf1, xf2, yf1, yf2, dy_frication, &yout_frication);
	}
	else // Parallel
	{
		// source connection tract connection, out
		//     frication
		double yf_parallel, yh_parallel, ytrans_phonation, ytrans_parallel, yh_overlap = 0.3, yin_vocalTract_p, yout_vocalTract_p;
		double xw[6] = { 0, 1.75, 0.125, 3, 0.25, 0.125 };

		rel_to_abs (xw, xws, 5, xmax2 - xmin);

		// optimize the vertical space for source, parallel and frication
		// source part is relatively fixed. let the number of vertical section units be the divisor
		// connector line from source to parallel has to be horizontal
		// determine y's of source and parallel section
		_KlattGrid_queryParallelSplit (me, dy_vocalTract_p, &yh_parallel, &yf_parallel);
		if (yh_parallel == 0) { yh_parallel = yh_phonation; yf_parallel = yh_parallel / 2; yh_overlap = -0.1; }

		height_phonation = yh_phonation / (yh_phonation + yh_frication);
		if (height_phonation < 0.3) height_phonation = 0.3;
		double yunit = (ymax - ymin) / (yh_parallel + (1 - yh_overlap) * yh_frication); // some overlap

		double ycs = ymax - 0.5 * height_phonation; // source output connector
		double ycp = ymax - yf_parallel * yunit; // parallel input connector
		ytrans_phonation = ycs > ycp ? ycp - ycs : 0;
		ytrans_parallel = ycp > ycs ? ycs - ycp : 0;

		// source, tract, frication
		xs1 = xmin; xs2 = xs1 + xw[1];

		double h1 = yh_phonation / 2, h2 = h1, h3 = yf_parallel, h4 = yh_parallel - h3, h5 = yh_frication;
		getYpositions (h1, h2, h3, h4, h5, yh_overlap, &dy, &ys1, &ys2, &yp1, &yp2, &yf1, &yf2);

		PhonationGrid_draw_inside (my phonation, g, xs1, xs2, ys1, ys2, dy_phonation, &yout_phonation);

		xp1 = xmin + xws[2]; xp2 = xp1 + xw[3];
		VocalTractGrid_CouplingGrid_drawParallel_inline (my vocalTract, my coupling, g, xp1, xp2, yp1, yp2, dy_vocalTract_p, &yin_vocalTract_p, &yout_vocalTract_p);

		tf -> x[1] = xp2; tf -> y[1] = yout_vocalTract_p;

		Graphics_line (g, xs2, yout_phonation, xp1, yin_vocalTract_p);

		xf1 = xmin /*+ 0.5 * xws[1]*/; xf2 = xf1 + 0.55 * (xw[2] + xws[3]);

		FricationGrid_draw_inside (my frication, g, xf1, xf2, yf1, yf2, dy_frication, &yout_frication);
	}

	tf -> x[2] = xf2; tf -> y[2] = yout_frication;
	r = (xmax3 - xmax2) / 2; xs = xmax2 + r / 2; ys = (ymax - ymin) / 2;

	summer_drawConnections (g, xs, ys, r, tf, 1, 0.6);

	Graphics_arrow (g, xs + r, ys, xmax, ys);

	Graphics_unsetInner (g);
	connections_free (tf);
}

FORM (KlattGrid_draw, L"KlattGrid: Draw", 0)
	RADIO (L"Synthesis model", 1)
	RADIOBUTTON (L"Cascade")
	RADIOBUTTON (L"Parallel")
	OK
DO
	EVERY_DRAW (KlattGrid_draw ((structKlattGrid *)OBJECT, GRAPHICS, GET_INTEGER (L"Synthesis model") - 1))
END

FORM (KlattGrid_drawVocalTract, L"KlattGrid: Draw vocal tract", 0)
	RADIO (L"Synthesis model", 1)
	RADIOBUTTON (L"Cascade")
	RADIOBUTTON (L"Parallel")
	BOOLEAN (L"Include tracheal formants", 1);
	OK
DO
	praat_picture_open ();
		KlattGrid_drawVocalTract ((structKlattGrid *)ONLY_OBJECT, GRAPHICS, GET_INTEGER (L"Synthesis model") - 1, GET_INTEGER (L"Include tracheal formants"));
	praat_picture_close (); return 1;
END

DIRECT(KlattGrid_drawPhonation)
	praat_picture_open ();
	KlattGrid thee = (structKlattGrid *)ONLY_OBJECT;
		PhonationGrid_draw (thy phonation, GRAPHICS);
	praat_picture_close (); return 1;
END

DIRECT(KlattGrid_drawFrication)
	praat_picture_open ();
	KlattGrid thee = (structKlattGrid *)ONLY_OBJECT;
		FricationGrid_draw (thy frication, GRAPHICS);
	praat_picture_close (); return 1;
END

FORM (KlattGrid_to_oralFormantGrid_openPhases, L"KlattGrid: Extract oral formant grid (open phases)", L"KlattGrid: Extract oral formant grid (open phases)...")
	REAL (L"Fade fraction (0..0.5)", L"0.1")
	OK
DO
	double fadeFraction = GET_REAL (L"Fade fraction");
	REQUIRE (fadeFraction < 0.5, L"Fade fraction has to be smaller than 0.5.")
	WHERE (SELECTED)
	{
		if (! praat_new1 (KlattGrid_to_oralFormantGrid_openPhases ((structKlattGrid *)OBJECT, fadeFraction), L"corrected")) return 0;
	}
END

FORM (Sound_KlattGrid_filterByVocalTract, L"Sound & KlattGrid: Filter by vocal tract", L"Sound & KlattGrid: Filter by vocal tract...")
	RADIO (L"Vocal tract filter model", 1)
	RADIOBUTTON (L"Cascade")
	RADIOBUTTON (L"Parallel")
	OK
DO
	Sound s = (structSound *)ONLY (classSound);
	KlattGrid kg = (structKlattGrid *)ONLY (classKlattGrid);
	int filterModel = GET_INTEGER (L"Vocal tract filter model") - 1;
	if (! praat_new3 (Sound_KlattGrid_filterByVocalTract (s, kg, filterModel), Thing_getName (s), L"_", Thing_getName (kg))) return 0;
END

extern "C" void praat_KlattGrid_init (void);
void praat_KlattGrid_init (void)
{

	Thing_recognizeClassesByName (classKlattGrid, NULL);

	praat_addMenuCommand (L"Objects", L"New", L"Acoustic synthesis (Klatt)", 0, 0, 0);
	praat_addMenuCommand (L"Objects", L"New", L"KlattGrid help", 0, 1, DO_KlattGrid_help);
	praat_addMenuCommand (L"Objects", L"New", L"-- the synthesizer grid --", 0, 1, 0);
	praat_addMenuCommand (L"Objects", L"New", L"Create KlattGrid...", 0, 1, DO_KlattGrid_create);
	praat_addMenuCommand (L"Objects", L"New", L"Create KlattGrid example", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_createExample);

/*
Edit oral/nasal/tracheal/frication/delta formant grid
Edit nasal/tracheal antiformant grid
Get oral/nasal/tracheal/frication/delta formant at time...
Get nasal/tracheal antiformant at time...
Get oral/nasal/tracheal/frication/delta formant bandwidth at time...
Get nasal/tracheal antiformant bandwidth at time...
Get oral/nasal/tracheal/frication formant amplitude at time...
Formula (oral/nasal/tracheal/frication/delta formant frequencies)...
Formula (nasal/tracheal antiformant frequencies)...
Formula (oral/nasal/tracheal/frication/delta formant bandwidths)...
Formula (nasal/tracheal antiformant bandwidths)...
Add oral/nasal/tracheal/frication/delta formant point...
Add nasal/tracheal antiformant point...
Add oral/nasal/tracheal/frication/delta formant bandwidth point...
Add nasal/tracheal antiformant bandwidth point...
Add oral/nasal/tracheal/frication formant amplitude point...
Remove oral/nasal/tracheal/frication/delta formant points...
Remove nasal/tracheal antiformant points...
Remove oral/nasal/tracheal/frication/delta bandwidth points...
Remove nasal/tracheal antiformant bandwidth points...
Remove oral/nasal/tracheal/frication formant amplitude points...
Extract oral/nasal/tracheal/frication/delta formant grid
Extract nasal/tracheal antiformant grid
Replace oral/nasal/tracheal/frication/delta formant grid
Replace nasal/tracheal antiformant grid
Add oral/nasal/tracheal/frication/delta formant and bandwidth tier
Add nasal/tracheal antiformant and bandwidth tier
#define KlattGrid_ORAL_FORMANTS 1
#define KlattGrid_NASAL_FORMANTS 2
#define KlattGrid_FRICATION_FORMANTS 3
#define KlattGrid_TRACHEAL_FORMANTS 4
#define KlattGrid_NASAL_ANTIFORMANTS 5
#define KlattGrid_TRACHEAL_ANTIFORMANTS 6
#define KlattGrid_DELTA_FORMANTS 7
*/
	praat_addAction1 (classKlattGrid, 0, L"KlattGrid help", 0, 0, DO_KlattGrid_help);
	praat_addAction1 (classKlattGrid, 0, L"Edit phonation -", 0, 0, 0);
	praat_addAction1 (classKlattGrid, 0, L"Edit pitch tier", 0, 1, DO_KlattGrid_editPitchTier);
	praat_addAction1 (classKlattGrid, 0, L"Edit voicing amplitude tier", 0, 1, DO_KlattGrid_editVoicingAmplitudeTier);
	praat_addAction1 (classKlattGrid, 0, L"Edit flutter tier", 0, 1, DO_KlattGrid_editFlutterTier);
	praat_addAction1 (classKlattGrid, 0, L"Edit power1 tier", 0, 1, DO_KlattGrid_editPower1Tier);
	praat_addAction1 (classKlattGrid, 0, L"Edit power2 tier", 0, 1, DO_KlattGrid_editPower2Tier);
	praat_addAction1 (classKlattGrid, 0, L"Edit open phase tier", 0, 1, DO_KlattGrid_editOpenPhaseTier);
	praat_addAction1 (classKlattGrid, 0, L"Edit collision phase tier", 0, 1, DO_KlattGrid_editCollisionPhaseTier);
	praat_addAction1 (classKlattGrid, 0, L"Edit double pulsing tier", 0, 1, DO_KlattGrid_editDoublePulsingTier);
	praat_addAction1 (classKlattGrid, 0, L"Edit spectral tilt tier", 0, 1, DO_KlattGrid_editSpectralTiltTier);
	praat_addAction1 (classKlattGrid, 0, L"Edit aspiration amplitude tier", 0, 1, DO_KlattGrid_editAspirationAmplitudeTier);
	praat_addAction1 (classKlattGrid, 0, L"Edit breathiness amplitude tier", 0, 1, DO_KlattGrid_editBreathinessAmplitudeTier);

	praat_addAction1 (classKlattGrid, 0, L"Edit filters -", 0, 0, 0);
	praat_addAction1 (classKlattGrid, 0, L"Edit oral formant grid", 0, 1, DO_KlattGrid_editOralFormantGrid);
	praat_addAction1 (classKlattGrid, 0, L"Edit nasal formant grid", 0, 1, DO_KlattGrid_editNasalFormantGrid);
	praat_addAction1 (classKlattGrid, 0, L"Edit nasal antiformant grid", 0, 1, DO_KlattGrid_editNasalAntiFormantGrid);
	praat_addAction1 (classKlattGrid, 0, L"Edit oral formant amplitude tier...", 0, 1, DO_KlattGrid_editOralFormantAmplitudeTier);
	praat_addAction1 (classKlattGrid, 0, L"Edit nasal formant amplitude tier...", 0, 1, DO_KlattGrid_editNasalFormantAmplitudeTier);
	praat_addAction1 (classKlattGrid, 0, L"-- edit delta formant grid --", 0, 1, 0);
	praat_addAction1 (classKlattGrid, 0, L"Edit delta formant grid", 0, 1, DO_KlattGrid_editDeltaFormantGrid);
	praat_addAction1 (classKlattGrid, 0, L"Edit tracheal formant grid", 0, 1, DO_KlattGrid_editTrachealFormantGrid);
	praat_addAction1 (classKlattGrid, 0, L"Edit tracheal antiformant grid", 0, 1, DO_KlattGrid_editTrachealAntiFormantGrid);
	praat_addAction1 (classKlattGrid, 0, L"Edit tracheal formant amplitude tier...", 0, 1, DO_KlattGrid_editTrachealFormantAmplitudeTier);
	praat_addAction1 (classKlattGrid, 0, L"-- edit frication tiers --", 0, 1, 0);
	praat_addAction1 (classKlattGrid, 1, L"Edit frication amplitude tier", 0, 1, DO_KlattGrid_editFricationAmplitudeTier);
	praat_addAction1 (classKlattGrid, 0, L"Edit frication formant grid", 0, 1, DO_KlattGrid_editFricationFormantGrid);
	praat_addAction1 (classKlattGrid, 0, L"Edit frication formant amplitude tier...", 0, 1, DO_KlattGrid_editFricationFormantAmplitudeTier);
	praat_addAction1 (classKlattGrid, 0, L"Edit frication bypass tier", 0, 1, DO_KlattGrid_editFricationBypassTier);
	praat_addAction1 (classKlattGrid, 1, L"Edit frication amplitude tier", 0, 1, DO_KlattGrid_editFricationAmplitudeTier);

	praat_addAction1 (classKlattGrid, 0, L"Play", 0, 0, DO_KlattGrid_play);
	praat_addAction1 (classKlattGrid, 0, L"Play special...", 0, 0, DO_KlattGrid_playSpecial);
	praat_addAction1 (classKlattGrid, 0, L"To Sound", 0, 0, DO_KlattGrid_to_Sound);
	praat_addAction1 (classKlattGrid, 0, L"To Sound (special)...", 0, 0, DO_KlattGrid_to_Sound_special);
	praat_addAction1 (classKlattGrid, 0, L"To Sound (phonation)...", 0, 0, DO_KlattGrid_to_Sound_phonation);

	praat_addAction1 (classKlattGrid, 0, L"Draw -", 0, 0, 0);
	praat_addAction1 (classKlattGrid, 0, L"Draw synthesizer...", 0, 1, DO_KlattGrid_draw);
	praat_addAction1 (classKlattGrid, 0, L"Draw vocal tract...", 0, 1, DO_KlattGrid_drawVocalTract);
	praat_addAction1 (classKlattGrid, 0, L"Draw phonation", 0, 1, DO_KlattGrid_drawPhonation);
	praat_addAction1 (classKlattGrid, 0, L"Draw frication", 0, 1, DO_KlattGrid_drawFrication);

	praat_addAction1 (classKlattGrid, 0, L"Query phonation -", 0, 0, 0);
	praat_addAction1 (classKlattGrid, 1, L"Get pitch at time...", 0, 1, DO_KlattGrid_getPitchAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get voicing amplitude at time...", 0, 1, DO_KlattGrid_getVoicingAmplitudeAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get flutter at time...", 0, 1, DO_KlattGrid_getFlutterAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get power1 at time...", 0, 1, DO_KlattGrid_getPower1AtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get power2 at time...", 0, 1, DO_KlattGrid_getPower2AtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get open phase at time...", 0, 1, DO_KlattGrid_getOpenPhaseAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get collision phase at time...", 0, 1, DO_KlattGrid_getCollisionPhaseAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get double pulsing at time...", 0, 1, DO_KlattGrid_getDoublePulsingAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get spectral tilt at time...", 0, 1, DO_KlattGrid_getSpectralTiltAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get aspiration amplitude at time...", 0, 1, DO_KlattGrid_getAspirationAmplitudeAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get breathiness amplitude at time...", 0, 1, DO_KlattGrid_getBreathinessAmplitudeAtTime);

	praat_addAction1 (classKlattGrid, 0, L"Query filters -", 0, 0, 0);
	praat_addAction1 (classKlattGrid, 1, L"Get formant at time...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_getFormantAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get bandwidth at time...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_getBandwidthAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get amplitude at time...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_getAmplitudeAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get delta formant at time...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_getDeltaFormantAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get delta bandwidth at time...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_getDeltaBandwidthAtTime);

#define KlattGRID_GET_FORMANT_FB_VALUES_ACTION(Name,namef) \
	praat_addAction1 (classKlattGrid, 1, L"Get " #namef "ormant frequency at time...", 0, 1, DO_KlattGrid_get##Name##FormantFrequencyAtTime); \
	praat_addAction1 (classKlattGrid, 1, L"Get " #namef "ormant bandwidth at time...", 0, 1, DO_KlattGrid_get##Name##FormantBandwidthAtTime);

#define KlattGRID_GET_FORMANT_A_VALUES_ACTION(Name,name) \
	praat_addAction1 (classKlattGrid, 1, L"Get " #name " formant amplitude at time...", 0, 1, DO_KlattGrid_get##Name##FormantAmplitudeAtTime); \

	KlattGRID_GET_FORMANT_FB_VALUES_ACTION(Oral,oral f)
	KlattGRID_GET_FORMANT_A_VALUES_ACTION(Oral,oral)
	KlattGRID_GET_FORMANT_FB_VALUES_ACTION(Nasal,nasal f)
	KlattGRID_GET_FORMANT_A_VALUES_ACTION(Nasal,nasal)
	KlattGRID_GET_FORMANT_FB_VALUES_ACTION(NasalAnti,nasal antif)

	praat_addAction1 (classKlattGrid, 1, L"-- query delta characteristics", 0, 1, 0);
	KlattGRID_GET_FORMANT_FB_VALUES_ACTION(Delta,delta f)
	KlattGRID_GET_FORMANT_FB_VALUES_ACTION(Tracheal,tracheal f)
	KlattGRID_GET_FORMANT_A_VALUES_ACTION(Tracheal,tracheal)
	KlattGRID_GET_FORMANT_FB_VALUES_ACTION(TrachealAnti,tracheal antif)
	praat_addAction1 (classKlattGrid, 1, L"-- query frication characteristics", 0, 1, 0);
	KlattGRID_GET_FORMANT_FB_VALUES_ACTION(Frication,frication f)
	KlattGRID_GET_FORMANT_A_VALUES_ACTION(Frication,frication)

#undef KlattGRID_GET_FORMANT_A_VALUES_ACTION
#undef KlattGRID_GET_FORMANT_A_VALUES_ACTION

	praat_addAction1 (classKlattGrid, 1, L"Get frication bypass at time...", 0, 1, DO_KlattGrid_getFricationBypassAtTime);
	praat_addAction1 (classKlattGrid, 1, L"Get frication amplitude at time...", 0, 1, DO_KlattGrid_getFricationAmplitudeAtTime);

	praat_addAction1 (classKlattGrid, 0, L"Modify phonation -", 0, 0, 0);
	praat_addAction1 (classKlattGrid, 0, L"Add pitch point...", 0, 1, DO_KlattGrid_addPitchPoint);
	praat_addAction1 (classKlattGrid, 0, L"Add voicing amplitude point...", 0, 1, DO_KlattGrid_addVoicingAmplitudePoint);
	praat_addAction1 (classKlattGrid, 0, L"Add flutter point...", 0, 1, DO_KlattGrid_addFlutterPoint);
	praat_addAction1 (classKlattGrid, 0, L"Add power1 point...", 0, 1, DO_KlattGrid_addPower1Point);
	praat_addAction1 (classKlattGrid, 0, L"Add power2 point...", 0, 1, DO_KlattGrid_addPower2Point);
	praat_addAction1 (classKlattGrid, 0, L"Add open phase point...", 0, 1, DO_KlattGrid_addOpenPhasePoint);
	praat_addAction1 (classKlattGrid, 0, L"Add collision phase point...", 0, 1, DO_KlattGrid_addCollisionPhasePoint);
	praat_addAction1 (classKlattGrid, 0, L"Add double pulsing point...", 0, 1, DO_KlattGrid_addDoublePulsingPoint);
	praat_addAction1 (classKlattGrid, 0, L"Add spectral tilt point...", 0, 1, DO_KlattGrid_addSpectralTiltPoint);
	praat_addAction1 (classKlattGrid, 0, L"Add aspiration amplitude point...", 0, 1, DO_KlattGrid_addAspirationAmplitudePoint);
	praat_addAction1 (classKlattGrid, 0, L"Add breathiness amplitude point...", 0, 1, DO_KlattGrid_addBreathinessAmplitudePoint);

#define KlattGrid_REMOVE_POINTS_ACTION(Name,name) \
	praat_addAction1 (classKlattGrid, 0, L"Remove " #name " points between...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_remove##Name##Points); \
	praat_addAction1 (classKlattGrid, 0, L"Remove " #name " points...", 0, 1, DO_KlattGrid_remove##Name##Points);

	KlattGrid_REMOVE_POINTS_ACTION (Pitch,pitch)
	KlattGrid_REMOVE_POINTS_ACTION (VoicingAmplitude, voicing amplitude)
	KlattGrid_REMOVE_POINTS_ACTION (Flutter, flutter)
	KlattGrid_REMOVE_POINTS_ACTION (Power1, power1)
	KlattGrid_REMOVE_POINTS_ACTION (Power2, power2)
	KlattGrid_REMOVE_POINTS_ACTION (OpenPhase, open phase)
	KlattGrid_REMOVE_POINTS_ACTION (CollisionPhase, collision phase)
	KlattGrid_REMOVE_POINTS_ACTION (DoublePulsing, double pulsing)
	KlattGrid_REMOVE_POINTS_ACTION (SpectralTilt, spectral tilt)
	KlattGrid_REMOVE_POINTS_ACTION (AspirationAmplitude, aspiration amplitude)
	KlattGrid_REMOVE_POINTS_ACTION (BreathinessAmplitude, breathiness amplitude)

	praat_addAction1 (classKlattGrid, 0, L"Modify vocal tract -", 0, 0, 0);

#define KlattGrid_MODIFY_ACTION_FBA(Name,namef) \
	praat_addAction1 (classKlattGrid, 0, L"Formula (" #namef "ormant frequencies)...", 0, 1, DO_KlattGrid_formula##Name##FormantFrequencies); \
	praat_addAction1 (classKlattGrid, 0, L"Formula (" #namef "ormant bandwidths)...", 0, 1, DO_KlattGrid_formula##Name##FormantBandwidths); \
	praat_addAction1 (classKlattGrid, 0, L"Add " #namef "ormant frequency point...", 0, 1, DO_KlattGrid_add##Name##FormantFrequencyPoint); \
	praat_addAction1 (classKlattGrid, 0, L"Add " #namef "ormant bandwidth point...", 0, 1, DO_KlattGrid_add##Name##FormantBandwidthPoint); \
	praat_addAction1 (classKlattGrid, 0, L"Add " #namef "ormant amplitude point...", 0, 1, DO_KlattGrid_add##Name##FormantAmplitudePoint); \
	praat_addAction1 (classKlattGrid, 0, L"Remove " #namef "ormant frequency points...", 0, 1, DO_KlattGrid_remove##Name##FormantFrequencyPoints); \
	praat_addAction1 (classKlattGrid, 0, L"Remove " #namef "ormant bandwidth points...", 0, 1, DO_KlattGrid_remove##Name##FormantBandwidthPoints); \
	praat_addAction1 (classKlattGrid, 0, L"Remove " #namef "ormant amplitude points...", 0, 1, DO_KlattGrid_remove##Name##FormantAmplitudePoints); \
	praat_addAction1 (classKlattGrid, 0, L"Add " #namef "ormant...", 0, 1, DO_KlattGrid_add##Name##Formant); \
	praat_addAction1 (classKlattGrid, 0, L"Remove " #namef "ormant...", 0, 1, DO_KlattGrid_remove##Name##Formant);

#define KlattGrid_MODIFY_ACTION_FB(Name,namef) \
	praat_addAction1 (classKlattGrid, 0, L"Formula (" #namef "ormant frequencies)...", 0, 1, DO_KlattGrid_formula##Name##FormantFrequencies); \
	praat_addAction1 (classKlattGrid, 0, L"Formula (" #namef "ormant bandwidths)...", 0, 1, DO_KlattGrid_formula##Name##FormantBandwidths); \
	praat_addAction1 (classKlattGrid, 0, L"Add " #namef "ormant frequency point...", 0, 1, DO_KlattGrid_add##Name##FormantFrequencyPoint); \
	praat_addAction1 (classKlattGrid, 0, L"Add " #namef "ormant bandwidth point...", 0, 1, DO_KlattGrid_add##Name##FormantBandwidthPoint); \
	praat_addAction1 (classKlattGrid, 0, L"Remove " #namef "ormant frequency points...", 0, 1, DO_KlattGrid_remove##Name##FormantFrequencyPoints); \
	praat_addAction1 (classKlattGrid, 0, L"Remove " #namef "ormant bandwidth points...", 0, 1, DO_KlattGrid_remove##Name##FormantBandwidthPoints); \
	praat_addAction1 (classKlattGrid, 0, L"Add " #namef "ormant...", 0, 1, DO_KlattGrid_add##Name##Formant); \
	praat_addAction1 (classKlattGrid, 0, L"Remove " #namef "ormant...", 0, 1, DO_KlattGrid_remove##Name##Formant);

	KlattGrid_MODIFY_ACTION_FBA (Oral, oral f)
	praat_addAction1 (classKlattGrid, 0, L"-- oral modify separator --", 0, 1, 0);
	KlattGrid_MODIFY_ACTION_FBA (Nasal, nasal f)
	praat_addAction1 (classKlattGrid, 0, L"-- nasal modify separator --", 0, 1, 0);
	KlattGrid_MODIFY_ACTION_FB (NasalAnti, nasal antif)

	praat_addAction1 (classKlattGrid, 0, L"Formula (frequencies)...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_formula_frequencies);
	praat_addAction1 (classKlattGrid, 0, L"Formula (bandwidths)...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_formula_bandwidths);
	praat_addAction1 (classKlattGrid, 0, L"Add formant point...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_addFormantPoint);
	praat_addAction1 (classKlattGrid, 0, L"Add bandwidth point...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_addBandwidthPoint);
	praat_addAction1 (classKlattGrid, 0, L"Add amplitude point...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_addAmplitudePoint);
	praat_addAction1 (classKlattGrid, 0, L"Remove formant points between...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_removeFormantPoints);
	praat_addAction1 (classKlattGrid, 0, L"Remove bandwidth points between...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_removeBandwidthPoints);
	praat_addAction1 (classKlattGrid, 0, L"Remove amplitude points between...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_removeAmplitudePoints);
	praat_addAction1 (classKlattGrid, 0, L"Modify coupling - ", 0, 0, 0);
	KlattGrid_MODIFY_ACTION_FB (Delta, delta f)
	praat_addAction1 (classKlattGrid, 0, L"-- delta modify separator --", 0, 1, 0);
	KlattGrid_MODIFY_ACTION_FBA (Tracheal, tracheal f)
	praat_addAction1 (classKlattGrid, 0, L"-- nasal modify separator --", 0, 1, 0);
	KlattGrid_MODIFY_ACTION_FB (TrachealAnti, tracheal antif)

	praat_addAction1 (classKlattGrid, 0, L"Add delta formant point...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_addDeltaFormantPoint);
	praat_addAction1 (classKlattGrid, 0, L"Add delta bandwidth point...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_addDeltaBandwidthPoint);
	praat_addAction1 (classKlattGrid, 0, L"Remove delta formant points between...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_removeDeltaFormantPoints);
	praat_addAction1 (classKlattGrid, 0, L"Remove delta bandwidth points between...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_removeDeltaBandwidthPoints);

	praat_addAction1 (classKlattGrid, 0, L"Modify frication -", 0, 0, 0);
	KlattGrid_MODIFY_ACTION_FBA (Frication, frication f)
	praat_addAction1 (classKlattGrid, 0, L"-- frication modify separator --", 0, 1, 0);

	praat_addAction1 (classKlattGrid, 0, L"Add frication bypass point...", 0, 1, DO_KlattGrid_addFricationBypassPoint);
	praat_addAction1 (classKlattGrid, 0, L"Add frication amplitude point...", 0, 1, DO_KlattGrid_addFricationAmplitudePoint);
	KlattGrid_REMOVE_POINTS_ACTION (FricationBypass, frication bypass)
	KlattGrid_REMOVE_POINTS_ACTION (FricationAmplitude, frication amplitude)
	praat_addAction1 (classKlattGrid, 0, L"Add formant and bandwidth tier...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_addFormantAndBandwidthTier);

#undef KlattGrid_REMOVE_POINTS_ACTION
#undef KlattGrid_MODIFY_ACTION_FB
#undef KlattGrid_MODIFY_ACTION_FBA

	praat_addAction1 (classKlattGrid, 0, L"Extract phonation -", 0, 0, 0);
	praat_addAction1 (classKlattGrid, 0, L"Extract pitch tier", 0, 1, DO_KlattGrid_extractPitchTier);
	praat_addAction1 (classKlattGrid, 0, L"Extract voicing amplitude tier", 0, 1, DO_KlattGrid_extractVoicingAmplitudeTier);
	praat_addAction1 (classKlattGrid, 0, L"Extract flutter tier", 0, 1, DO_KlattGrid_extractFlutterTier);
	praat_addAction1 (classKlattGrid, 0, L"Extract power1 tier", 0, 1, DO_KlattGrid_extractPower1Tier);
	praat_addAction1 (classKlattGrid, 0, L"Extract power2 tier", 0, 1, DO_KlattGrid_extractPower2Tier);
	praat_addAction1 (classKlattGrid, 0, L"Extract open phase tier", 0, 1, DO_KlattGrid_extractOpenPhaseTier);
	praat_addAction1 (classKlattGrid, 0, L"Extract collision phase tier", 0, 1, DO_KlattGrid_extractCollisionPhaseTier);
	praat_addAction1 (classKlattGrid, 0, L"Extract double pulsing tier", 0, 1, DO_KlattGrid_extractDoublePulsingTier);
	praat_addAction1 (classKlattGrid, 0, L"Extract spectral tilt tier", 0, 1, DO_KlattGrid_extractSpectralTiltTier);
	praat_addAction1 (classKlattGrid, 0, L"Extract aspiration amplitude tier", 0, 1, DO_KlattGrid_extractAspirationAmplitudeTier);
	praat_addAction1 (classKlattGrid, 0, L"Extract breathiness amplitude tier", 0, 1, DO_KlattGrid_extractBreathinessAmplitudeTier);
	praat_addAction1 (classKlattGrid, 0, L"-- extract glottal events--", 0, 1, 0);
	praat_addAction1 (classKlattGrid, 0, L"Extract PointProcess (glottal closures)", 0, 1, DO_KlattGrid_extractPointProcess_glottalClosures);

#define KlattGRID_EXTRACT_FORMANT_GRID_ACTION(Name,namef) \
	praat_addAction1 (classKlattGrid, 0, L"Extract " #namef "ormant grid", 0, 1, DO_KlattGrid_extract##Name##FormantGrid);
#define KlattGRID_EXTRACT_FORMANT_AMPLITUDE_ACTION(Name,name) \
	praat_addAction1 (classKlattGrid, 0, L"Extract " #name " formant amplitude tier...", 0, 1, DO_KlattGrid_extract##Name##FormantAmplitudeTier);

	praat_addAction1 (classKlattGrid, 0, L"Extract filters -", 0, 0, 0);
	praat_addAction1 (classKlattGrid, 0, L"Extract formant grid...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_extractFormantGrid); // deprecated
	KlattGRID_EXTRACT_FORMANT_GRID_ACTION(Oral,oral f)
	praat_addAction1 (classKlattGrid, 0, L"Extract amplitude tier...", 0, praat_DEPTH_1+praat_HIDDEN, DO_KlattGrid_extractAmplitudeTier); // deprecated
	praat_addAction1 (classKlattGrid, 0, L"Extract formant grid (open phases)...", 0, praat_HIDDEN + praat_DEPTH_1, DO_KlattGrid_to_oralFormantGrid_openPhases);
	praat_addAction1 (classKlattGrid, 0, L"Extract oral formant grid (open phases)...", 0, 1, DO_KlattGrid_to_oralFormantGrid_openPhases);
	KlattGRID_EXTRACT_FORMANT_AMPLITUDE_ACTION(Oral,oral)
	KlattGRID_EXTRACT_FORMANT_GRID_ACTION(Nasal,nasal f)
	KlattGRID_EXTRACT_FORMANT_AMPLITUDE_ACTION(Nasal,nasal)
	KlattGRID_EXTRACT_FORMANT_GRID_ACTION(NasalAnti,nasal antif)

	praat_addAction1 (classKlattGrid, 0, L"-- extract delta characteristics", 0, 1, 0);
	praat_addAction1 (classKlattGrid, 0, L"Extract delta formant grid", 0, 1, DO_KlattGrid_extractDeltaFormantGrid);
	KlattGRID_EXTRACT_FORMANT_GRID_ACTION(Tracheal,tracheal f)
	KlattGRID_EXTRACT_FORMANT_AMPLITUDE_ACTION(Tracheal,tracheal)
	KlattGRID_EXTRACT_FORMANT_GRID_ACTION(TrachealAnti,tracheal antif)
	praat_addAction1 (classKlattGrid, 0, L"-- extract frication characteristics", 0, 1, 0);
	KlattGRID_EXTRACT_FORMANT_GRID_ACTION(Frication,frication f)
	KlattGRID_EXTRACT_FORMANT_AMPLITUDE_ACTION(Frication,frication)
	praat_addAction1 (classKlattGrid, 0, L"Extract frication bypass tier", 0, 1, DO_KlattGrid_extractFricationBypassTier);
	praat_addAction1 (classKlattGrid, 0, L"Extract frication amplitude tier", 0, 1, DO_KlattGrid_extractFricationAmplitudeTier);

#undef KlattGRID_EXTRACT_FORMANT_AMPLITUDE_ACTION
#undef KlattGRID_EXTRACT_FORMANT_GRID_ACTION

	praat_addAction2 (classKlattGrid, 1, classPitchTier, 1, L"Replace pitch tier", 0, 1, DO_KlattGrid_replacePitchTier);
	praat_addAction2 (classKlattGrid, 1, classRealTier, 1, L"Replace flutter tier", 0, 1, DO_KlattGrid_replaceFlutterTier);
	praat_addAction2 (classKlattGrid, 1, classRealTier, 1, L"Replace power1 tier", 0, 1, DO_KlattGrid_replacePower1Tier);
	praat_addAction2 (classKlattGrid, 1, classRealTier, 1, L"Replace power2 tier", 0, 1, DO_KlattGrid_replacePower2Tier);
	praat_addAction2 (classKlattGrid, 1, classRealTier, 1, L"Replace open phase tier", 0, 1, DO_KlattGrid_replaceOpenPhaseTier);
	praat_addAction2 (classKlattGrid, 1, classRealTier, 1, L"Replace collision phase tier", 0, 1, DO_KlattGrid_replaceCollisionPhaseTier);
	praat_addAction2 (classKlattGrid, 1, classRealTier, 1, L"Replace double pulsing tier", 0, 1, DO_KlattGrid_replaceDoublePulsingTier);

	praat_addAction2 (classKlattGrid, 1, classIntensityTier, 1, L"-- replace formant amplitudes --", 0, 1, 0);

#define KlattGrid_REPLACE_FORMANTGRID_ACTION(Name,namef) \
	praat_addAction2 (classKlattGrid, 1, classFormantGrid, 1, L"Replace " #namef "ormant grid", 0, 1, DO_KlattGrid_replace##Name##FormantGrid);
#define KlattGrid_REPLACE_FORMANT_AMPLITUDE_ACTION(Name,namef) \
	praat_addAction2 (classKlattGrid, 1, classIntensityTier, 1, L"Replace " #namef "ormant amplitude tier...", 0, 1, DO_KlattGrid_replace##Name##FormantAmplitudeTier);


	KlattGrid_REPLACE_FORMANTGRID_ACTION(Oral, oral f)
	KlattGrid_REPLACE_FORMANTGRID_ACTION(Nasal, nasal f)
	KlattGrid_REPLACE_FORMANTGRID_ACTION(NasalAnti, nasal antif)
	praat_addAction2 (classKlattGrid, 1, classFormantGrid, 1, L"-- replace coupling --", 0, 1, 0);
	KlattGrid_REPLACE_FORMANTGRID_ACTION(Tracheal, tracheal f)
	KlattGrid_REPLACE_FORMANTGRID_ACTION(TrachealAnti, tracheal antif)
	KlattGrid_REPLACE_FORMANTGRID_ACTION(Delta, delta f)
	praat_addAction2 (classKlattGrid, 1, classFormantGrid, 1, L"-- replace frication --", 0, 1, 0);
	KlattGrid_REPLACE_FORMANTGRID_ACTION(Frication, frication f)
	praat_addAction2 (classKlattGrid, 1, classFormantGrid, 1, L"Replace formant grid...", 0, praat_HIDDEN + praat_DEPTH_1, DO_KlattGrid_replaceFormantGrid);
	praat_addAction2 (classKlattGrid, 1, classIntensityTier, 1, L"Replace voicing amplitude tier", 0, 1, DO_KlattGrid_replaceVoicingAmplitudeTier);
	praat_addAction2 (classKlattGrid, 1, classIntensityTier, 1, L"Replace spectral tilt tier", 0, 1, DO_KlattGrid_replaceSpectralTiltTier);
	praat_addAction2 (classKlattGrid, 1, classIntensityTier, 1, L"Replace aspiration amplitude tier", 0, 1,
		DO_KlattGrid_replaceAspirationAmplitudeTier);
	praat_addAction2 (classKlattGrid, 1, classIntensityTier, 1, L"Replace breathiness amplitude tier", 0, 1,
		DO_KlattGrid_replaceBreathinessAmplitudeTier);
	praat_addAction2 (classKlattGrid, 1, classIntensityTier, 1, L"Replace amplitude tier...", 0, praat_HIDDEN + praat_DEPTH_1, DO_KlattGrid_replaceAmplitudeTier);
	KlattGrid_REPLACE_FORMANT_AMPLITUDE_ACTION (Oral, oral f)
	KlattGrid_REPLACE_FORMANT_AMPLITUDE_ACTION (Nasal, nasal f)
	KlattGrid_REPLACE_FORMANT_AMPLITUDE_ACTION (Tracheal, tracheal f)
	KlattGrid_REPLACE_FORMANT_AMPLITUDE_ACTION (Frication, frication f)
	praat_addAction2 (classKlattGrid, 1, classIntensityTier, 1, L"Replace frication amplitude tier", 0, 1, DO_KlattGrid_replaceFricationAmplitudeTier);
	praat_addAction2 (classKlattGrid, 1, classIntensityTier, 1, L"Replace frication bypass tier", 0, 1, DO_KlattGrid_replaceFricationBypassTier);

#undef KlattGrid_REPLACE_FORMANT_AMPLITUDE_ACTION
#undef KlattGrid_REPLACE_FORMANTGRID_ACTION

	praat_addAction2 (classKlattGrid, 1, classSound, 1, L"Filter by vocal tract...", 0, 1, DO_Sound_KlattGrid_filterByVocalTract);
}


/* End of file praat_KlattGrid_init.c */
