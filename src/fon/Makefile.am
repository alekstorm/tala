AM_CFLAGS = $(COMMON) -I @abs_top_srcdir@/src -std=gnu99
AM_CPPFLAGS = $(COMMON) -I @abs_top_srcdir@/src

noinst_LIBRARIES = libfon.a
libfon_a_SOURCES = Transition.c Distributions_and_Transition.c \
   Function.c Sampled.c Matrix.c Vector.c Polygon.c PointProcess.c \
   Matrix_and_PointProcess.c Matrix_and_Polygon.c AnyTier.c RealTier.c \
   Sound.c LongSound.c Sound_files.c Sound_audio.c PointProcess_and_Sound.c Sound_PointProcess.c ParamCurve.c \
   Pitch.c Harmonicity.c Intensity.c Matrix_and_Pitch.c Sound_to_Pitch.c \
   Sound_to_Intensity.c Sound_to_Harmonicity.c Sound_to_Harmonicity_GNE.c Sound_to_PointProcess.c \
   Pitch_to_PointProcess.c Pitch_to_Sound.c Pitch_Intensity.c \
   PitchTier.c Pitch_to_PitchTier.c PitchTier_to_PointProcess.c PitchTier_to_Sound.c Manipulation.c \
   Pitch_AnyTier_to_PitchTier.c IntensityTier.c DurationTier.c AmplitudeTier.c \
   Spectrum.c Ltas.c Spectrogram.c SpectrumTier.c Ltas_to_SpectrumTier.c \
   Formant.c Image.c Sound_to_Formant.c Sound_and_Spectrogram.c \
   Sound_and_Spectrum.c Spectrum_and_Spectrogram.c Spectrum_to_Formant.c \
   FormantTier.c TextGrid.c TextGrid_Sound.c Label.c FormantGrid.c \
   Excitation.c Cochleagram.c Cochleagram_and_Excitation.c Excitation_to_Formant.c \
   Sound_to_Cochleagram.c Spectrum_to_Excitation.c \
   VocalTract.c VocalTract_to_Spectrum.c \
   SoundRecorder.c Sound_enhance.c VoiceAnalysis.c \
   FunctionEditor.c TimeSoundEditor.c TimeSoundAnalysisEditor.c \
   PitchEditor.c SoundEditor.c SpectrumEditor.c SpectrogramEditor.c PointEditor.c \
   RealTierEditor.c PitchTierEditor.c IntensityTierEditor.c \
   DurationTierEditor.c AmplitudeTierEditor.c \
   ManipulationEditor.c TextGridEditor.c FormantGridEditor.c \
   WordList.c SpellingChecker.c \
   FujisakiPitch.c \
   ExperimentMFC.c RunnerMFC.c praat_Exp.c \
   Praat_tests.cpp praat_Sound_init.cpp praat_TextGrid_init.cpp praat_Fon.cpp
