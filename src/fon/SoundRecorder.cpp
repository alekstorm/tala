/* SoundRecorder.c
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
/* Linux code originally by Darryl Purnell, Pretoria */

/*
 * pb 2002/07/16 GPL
 * pb 2003/02/09 better layout on MacOS X
 * pb 2003/04/07 larger default buffer
 * pb 2003/08/22 use getenv ("AUDIODEV") on Sun (thanks to Michel Scheffers)
 * pb 2003/09/12 more MelderFile
 * pb 2003/10/22 check for mono refusal on Linux
 * pb 2003/12/06 use sys/soundcard.h instead of linux/soundcard.h for FreeBSD compatibility
 * pb 2004/11/26 fake Mono on MacOS X
 * pb 2004/11/26 check available sampling frequencies on MacOS X
 * pb 2005/02/13 defended against weird meter levels
 * pb 2005/04/25 made 24 kHz available for Mac
 * pb 2005/08/22 removed reference to Control menu from message
 * pb 2005/09/28 made 12 and 64 kHz available for Mac
 * pb 2005/10/13 edition for OpenBSD
 * pb 2006/04/01 corrections for Intel Mac
 * pb 2006/08/09 acknowledge the 67 MB buffer limit on Windows XP
 * pb 2006/10/28 erased MacOS 9 stuff
 * pb 2006/12/30 stereo
 * pb 2007/01/03 CoreAudio (PortAudio)
 * pb 2007/01/03 flexible drawing area
 * pb 2007/06/10 wchar_t
 * pb 2007/08/12 wchar_t
 * pb 2007/12/02 big-endian Linux (suggested by Stefan de Konink)
 * pb 2007/12/05 prefs
 * pb 2007/12/09 192 kHz
 * pb 2007/12/23 Gui
 * pb 2008/03/20 split off Help menu
 * pb 2008/03/21 new Editor API
 * pb 2008/06/02 PortAudio for Linux
 * pb 2008/06/16 PortAudio optional
 * pb 2008/06/17 api
 * fb 2010/02/24 GTK
 * pb 2011/03/23 C++
 */

/* This source file describes interactive sound recorders for the following systems:
 *     SGI
 *     MacOS & Mach
 *     SunOS
 *     HP
 *     Linux
 *     Windows
 * Because the behaviour of these sound recorders is partly similar, partly different,
 * this would seem a good candidate for object-oriented programming
 * (one audio manager and several audio drivers).
 * However, the places where sound recorders are similar and where they are different,
 * are hard to predict. For this reason, everything is done with system #ifdefs.
 */

#include <errno.h>
#include "SoundRecorder.h"
#include "sys/Editor.h"
#include "sys/EditorM.h"
#include "sys/Preferences.h"

static struct {
	int bufferSize_MB;
} preferences;

void SoundRecorder::prefs (void) {
	Preferences_addInt (L"SoundRecorder.bufferSize_MB", & preferences.bufferSize_MB, 20);
}

int SoundRecorder::getBufferSizePref_MB (void) { return preferences.bufferSize_MB; }
void SoundRecorder::setBufferSizePref_MB (int size) { preferences.bufferSize_MB = size < 1 ? 1 : size > 1000 ? 1000: size; }

#define step 1000

/* For those systems that do not have a pollable audio control panel, */
/* the settings are saved only here, so that they are remembered across */
/* subsequent creations of a SoundRecorder. Also, this is then the way */
/* in which two simultaneously open SoundRecorders would communicate. */

static struct {
	int inputSource;   /* 1 = microphone, 2 = line, 3 = digital. */
	int leftGain, rightGain;   /* 0..255. */
	double sampleRate;
} theControlPanel =
#if defined (sgi) || defined (HPUX) || defined (linux)
	{ 1, 200, 200, 44100 };
#elif defined (macintosh)
	{ 1, 26, 26, 44100 };
#else
	{ 1, 26, 26, 44100 };
#endif

static void gui_drawingarea_cb_resize (I, GuiDrawingAreaResizeEvent event) {
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	if (soundRecorder->_graphics == NULL) return;   // Could be the case in the very beginning.
	Graphics_setWsViewport (soundRecorder->_graphics, 0, event -> width, 0, event -> height);
	Graphics_setWsWindow (soundRecorder->_graphics, 0, event -> width, 0, event -> height);
	Graphics_setViewport (soundRecorder->_graphics, 0, event -> width, 0, event -> height);
	Graphics_updateWs (soundRecorder->_graphics);
}

static Boolean workProc (XtPointer void_me) {
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	short buffertje [step*2];
	int stepje = 0;

	#if defined (linux)
		#define min(a,b) a > b ? b : a
	#endif

	/* Determine global audio parameters (may have been changed by an external control panel):
	 *   1. input source;
	 *   2. left and right gain;
	 *   3. sampling frequency.
	 */
	if (soundRecorder->_inputUsesPortAudio) {
	} else {
		#if defined (sgi)
			soundRecorder->_info [0] = AL_INPUT_RATE;
			soundRecorder->_info [2] = AL_INPUT_SOURCE;
			soundRecorder->_info [4] = AL_LEFT_INPUT_ATTEN;
			soundRecorder->_info [6] = AL_RIGHT_INPUT_ATTEN;
			soundRecorder->_info [8] = AL_DIGITAL_INPUT_RATE;
			ALgetparams (AL_DEFAULT_DEVICE, soundRecorder->_info, 10);
			theControlPanel. inputSource = soundRecorder->_info [3] == AL_INPUT_MIC ? 1 : soundRecorder->_info [3] == AL_INPUT_LINE ? 2 : 3;
			theControlPanel. leftGain = 255 - soundRecorder->_info [5];
			theControlPanel. rightGain = 255 - soundRecorder->_info [7];
			theControlPanel. sampleRate = soundRecorder->_info [3] == AL_INPUT_DIGITAL ? soundRecorder->_info [9] : soundRecorder->_info [1];
		#elif defined (macintosh)
			OSErr err;
			short macSource, isource;
			Str255 pdeviceName;
			err = SPBGetDeviceInfo (soundRecorder->_refNum, siDeviceName, & pdeviceName);
			if (err != noErr) { onceError ("SPBGetDeviceInfo (deviceName)", err); return False; }
			err = SPBGetDeviceInfo (soundRecorder->_refNum, siInputSource, & macSource);
			if (err != noErr) { onceError ("SPBGetDeviceInfo (inputSource)", err); return False; }
			for (isource = 1; isource <= _numberOfInputDevices; isource ++) {
				if (strequ ((const char *) & pdeviceName, (const char *) soundRecorder->_hybridDeviceNames [isource]) &&
						macSource == soundRecorder->_macSource [isource]) {
					theControlPanel. inputSource = isource;
					break;
				}
			}
		#elif defined (sun)
			ioctl (_fd, AUDIO_GETINFO, & soundRecorder->_info);
			theControlPanel. inputSource =
				_info. record. port == AUDIO_MICROPHONE ? 1 : 2;
			theControlPanel. leftGain = soundRecorder->_info. record. balance <= 32 ? soundRecorder->_info. record. gain:
				_info. record. gain * (64 - soundRecorder->_info. record. balance) / 32;
			theControlPanel. rightGain = soundRecorder->_info. record. balance >= 32 ? soundRecorder->_info. record. gain:
				_info. record. gain * soundRecorder->_info. record. balance / 32;
			theControlPanel. sampleRate = soundRecorder->_info. record. sample_rate;
			if (soundRecorder->_info. record. channels != soundRecorder->_numberOfChannels)
				Melder_casual ("(SoundRecorder:) Not %s.", soundRecorder->_numberOfChannels == 1 ? "mono" : "stereo");
			if (soundRecorder->_info. record. precision != 16)
				Melder_casual ("(SoundRecorder:) Not 16-bit.");
			if (soundRecorder->_info. record. encoding != AUDIO_ENCODING_LINEAR)
				Melder_casual ("(SoundRecorder:) Not linear.");
		#elif defined (HPUX)
			int leftGain = soundRecorder->_hpGains. cgain [0]. receive_gain * 11;
			int rightGain = soundRecorder->_hpGains. cgain [1]. receive_gain * 11;
			int sampleRate;
			ioctl (soundRecorder->_fd, AUDIO_GET_INPUT, & _hpInputSource);
			theControlPanel. inputSource = (soundRecorder->_hpInputSource & AUDIO_IN_MIKE) ? 1 : 2;
			ioctl (soundRecorder->_fd, AUDIO_GET_GAINS, & soundRecorder->_hpGains);
			if (leftGain < theControlPanel. leftGain - 10 || leftGain > theControlPanel. leftGain)
				theControlPanel. leftGain = leftGain;
			if (rightGain < theControlPanel. rightGain - 10 || rightGain > theControlPanel. rightGain)
				theControlPanel. rightGain = rightGain;
			ioctl (soundRecorder->_fd, AUDIO_GET_SAMPLE_RATE, & sampleRate);
			theControlPanel. sampleRate = sampleRate;
		#endif
	}

	/* Set the buttons according to the audio parameters. */

	if (soundRecorder->_recordButton) GuiObject_setSensitive (soundRecorder->_recordButton, ! soundRecorder->_recording);
	if (soundRecorder->_stopButton) GuiObject_setSensitive (soundRecorder->_stopButton, soundRecorder->_recording);
	if (soundRecorder->_playButton) GuiObject_setSensitive (soundRecorder->_playButton, ! soundRecorder->_recording && soundRecorder->_nsamp > 0);
	if (soundRecorder->_applyButton) GuiObject_setSensitive (soundRecorder->_applyButton, ! soundRecorder->_recording && soundRecorder->_nsamp > 0);
	if (soundRecorder->_okButton) GuiObject_setSensitive (soundRecorder->_okButton, ! soundRecorder->_recording && soundRecorder->_nsamp > 0);
	if (soundRecorder->_monoButton) GuiRadioButton_setValue (soundRecorder->_monoButton, soundRecorder->_numberOfChannels == 1);
	if (soundRecorder->_stereoButton) GuiRadioButton_setValue (soundRecorder->_stereoButton, soundRecorder->_numberOfChannels == 2);
	for (long i = 1; i <= SoundRecorder_IFSAMP_MAX; i ++)
		if (soundRecorder->_fsamp [i]. button)
			GuiRadioButton_setValue (soundRecorder->_fsamp [i]. button, theControlPanel. sampleRate == soundRecorder->_fsamp [i]. fsamp);
	for (long i = 1; i <= SoundRecorder_IDEVICE_MAX; i ++)
		if (soundRecorder->_device [i]. button)
			GuiRadioButton_setValue (soundRecorder->_device [i]. button, theControlPanel. inputSource == i);
	if (soundRecorder->_monoButton) GuiObject_setSensitive (soundRecorder->_monoButton, ! soundRecorder->_recording);
	if (soundRecorder->_stereoButton) GuiObject_setSensitive (soundRecorder->_stereoButton, ! soundRecorder->_recording);
	for (long i = 1; i <= SoundRecorder_IFSAMP_MAX; i ++)
		if (soundRecorder->_fsamp [i]. button)
			GuiObject_setSensitive (soundRecorder->_fsamp [i]. button, ! soundRecorder->_recording);
	for (long i = 1; i <= SoundRecorder_IDEVICE_MAX; i ++)
		if (soundRecorder->_device [i]. button)
			GuiObject_setSensitive (soundRecorder->_device [i]. button, ! soundRecorder->_recording);

	/*Graphics_setGrey (_graphics, 0.9);
	Graphics_fillRectangle (_graphics, 0.0, 1.0, 0.0, 32768.0);
	Graphics_setGrey (_graphics, 0.9);
	Graphics_fillRectangle (_graphics, 0.0, 1.0, 0.0, 32768.0);*/

	if (soundRecorder->_synchronous) {
		/*
		 * Read some samples into 'buffertje'.
		 */
		do {
			if (soundRecorder->_inputUsesPortAudio) {
				/*
				 * Asynchronous recording: do nothing.
				 */
			} else {
				#if defined (macintosh) || defined (_WIN32)
					/*
					 * Asynchronous recording on these systems: do nothing.
					 */
				#elif defined (sgi)
					ALreadsamps (soundRecorder->_port, buffertje, step * soundRecorder->_numberOfChannels);
					stepje = step;
				#else
					// linux, sun, HPUX
					if (soundRecorder->_fd != -1)
						stepje = read (soundRecorder->_fd, (void *) buffertje, step * (sizeof (short) * soundRecorder->_numberOfChannels)) / (sizeof (short) * soundRecorder->_numberOfChannels);
				#endif
			}

			if (soundRecorder->_recording) {
				memcpy (soundRecorder->_buffer + soundRecorder->_nsamp * soundRecorder->_numberOfChannels, buffertje, stepje * (sizeof (short) * soundRecorder->_numberOfChannels));
			}
			soundRecorder->showMeter (buffertje, stepje);
			if (soundRecorder->_recording) {
				soundRecorder->_nsamp += stepje;
				if (soundRecorder->_nsamp > soundRecorder->_nmax - step) soundRecorder->_recording = false;
				#if gtk
				gtk_range_set_value(GTK_RANGE(soundRecorder->_progressScale), (1000.0 * ((double) soundRecorder->_nsamp / (double) soundRecorder->_nmax)));
				#elif motif
				XmScaleSetValue (soundRecorder->_progressScale,
					(int) (1000.0f * ((float) soundRecorder->_nsamp / (float) soundRecorder->_nmax)));
				#endif
			}
		} while (soundRecorder->_recording && soundRecorder->tooManySamplesInBufferToReturnToGui ());
	} else {
		if (soundRecorder->_recording) {
			/*
			 * We have to know how far the buffer has been filled.
			 * However, the buffer may be filled at interrupt time,
			 * so that the buffer may be being filled during this workproc.
			 * So we ask for the buffer filling just once, namely here at the beginning.
			 */
			long lastSample = 0;
			if (soundRecorder->_inputUsesPortAudio) {
				 /*
				  * The buffer filling is contained in _nsamp,
				  * which has been set during interrupt time and may again be updated behind our backs during this workproc.
				  * So we do it in such a way that the compiler cannot ask for _nsamp twice.
				  */
				lastSample = soundRecorder->getMyNsamp ();
				Pa_Sleep (10);
			} else {
				#if defined (_WIN32)
					MMTIME mmtime;
					mmtime. wType = TIME_BYTES;
					if (waveInGetPosition (_hWaveIn, & mmtime, sizeof (MMTIME)) == MMSYSERR_NOERROR)
						lastSample = mmtime. u.cb / (sizeof (short) * soundRecorder->_numberOfChannels);
				#elif defined (macintosh)
					OSErr err;
					short recordingStatus, meterLevel;
					unsigned long totalSamplesToRecord, numberOfSamplesRecorded, totalMsecsToRecord, numberOfMsecsRecorded;
					err = SPBGetRecordingStatus (_refNum, & recordingStatus, & meterLevel,
							& totalSamplesToRecord, & numberOfSamplesRecorded,
							& totalMsecsToRecord, & numberOfMsecsRecorded);
					if (err != noErr) { onceError ("SPBGetRecordingStatus", err); return FALSE; }
					if (totalSamplesToRecord == 0)
						soundRecorder->_nsamp = soundRecorder->_nmax;
					else
						soundRecorder->_nsamp = numberOfSamplesRecorded / (sizeof (short) * soundRecorder->_numberOfChannels);
					lastSample = soundRecorder->_nsamp;
				#endif
			}
			long firstSample = lastSample - 1000;
			if (firstSample < 0) firstSample = 0;
			soundRecorder->showMeter (soundRecorder->_buffer + firstSample * soundRecorder->_numberOfChannels, lastSample - firstSample);
			#if gtk
			gtk_range_set_value(GTK_RANGE(soundRecorder->_progressScale), (1000.0 * ((double) lastSample / (double) soundRecorder->_nmax)));
			#elif motif
			XmScaleSetValue (soundRecorder->_progressScale, (int) (1000.0f * ((float) lastSample / (float) soundRecorder->_nmax)));
			#endif
		} else {
			soundRecorder->showMeter (NULL, 0);
		}
	}
	iferror Melder_flushError (NULL);
	
	#if gtk
	return TRUE;
	#else
	return False;
	#endif
}

/* create a SoundRecorder, which is an interactive window
   for recording in 16-bit mono or stereo (SGI, MacOS, SunOS, HPUX, Linux, Windows). */
SoundRecorder::SoundRecorder (GuiObject parent, int numberOfChannels, void *applicationContext)
	: Editor (parent, 100, 100, 600, 500, L"SoundRecorder", NULL),
	  _numberOfChannels(numberOfChannels),
	  _nsamp(0),
	  _nmax(0),
	  _fakeMono(false),
	  _synchronous(false),
	  _recording(false),
	  _lastLeftMaximum(0),
	  _lastRightMaximum(0),
	  _numberOfInputDevices(0),
	  _buffer(NULL),
	  _inputUsesPortAudio(MelderAudio_getInputUsesPortAudio ()),
	  _portaudioStream(NULL) {
	createMenus ();
	createChildren ();
	//try { // FIXME exception
		if (!_inputUsesPortAudio) {
			#if defined (_WIN32)
				UINT numberOfDevices = waveInGetNumDevs (), i;
				WAVEINCAPS caps;
				MMRESULT err;
				if (numberOfDevices == 0)
					Melder_throw ("No sound input devices available.");
				err = waveInGetDevCaps (WAVE_MAPPER, & caps, sizeof (WAVEINCAPS));
				if (numberOfChannels == 2 && caps. wChannels < 2)
					Melder_throw ("Your computer does not support stereo sound input.");
				/* BUG: should we ask whether 16 bit is supported? */
				for (i = 0; i < numberOfDevices; i ++) {
					waveInGetDevCaps (i, & caps, sizeof (WAVEINCAPS));
					/*Melder_casual ("Name of device %d: %s", i, caps. szPname);*/
				}
			#elif defined (macintosh)
				long soundFeatures;
				if (Gestalt (gestaltSoundAttr, & soundFeatures) ||
						! (soundFeatures & (1 << gestaltSoundIOMgrPresent)) ||
						! (soundFeatures & (1 << gestaltBuiltInSoundInput)) ||
						! (soundFeatures & (1 << gestaltHasSoundInputDevice)))
					Melder_throw ("Your computer does not support sound input.");
				if (! (soundFeatures & (1 << gestalt16BitSoundIO)) ||   /* Hardware. */
					! (soundFeatures & (1 << gestaltStereoInput)) ||   /* Hardware. */
					! (soundFeatures & (1 << gestalt16BitAudioSupport)))   /* Software. */
					Melder_throw ("Your computer does not support stereo sound input.");
			#endif
		}
		if (sizeof (short) != 2)
			Melder_throw ("Long shorts!!!!!");
		if (!_inputUsesPortAudio) {
			#if defined (macintosh) || defined (_WIN32)
				_synchronous = false;
			#else
				_synchronous = true;
			#endif
		}
		/*
		 * Allocate the maximum buffer.
		 */
		if (preferences.bufferSize_MB < 1) preferences.bufferSize_MB = 1;   /* Validate preferences. */
		if (preferences.bufferSize_MB > 1000) preferences.bufferSize_MB = 1000;
		if (_buffer == NULL) {
			long nmax_bytes_pref = preferences.bufferSize_MB * 1000000;
			long nmax_bytes = _inputUsesPortAudio ? nmax_bytes_pref :
				#if defined (_WIN32)
					66150000;   /* The maximum physical buffer on Windows XP; shorter than in Windows 98, alas. */
				#else
					nmax_bytes_pref;
				#endif
			_nmax = nmax_bytes / (sizeof (short) * numberOfChannels);
			for (;;) {
				_buffer = NUMsvector (0, _nmax * numberOfChannels - 1);
				if (_buffer) break;   // success
				if (_nmax < 100000) throw 1;   // failure, with error message
				Melder_clearError ();
				_nmax /= 2;   // retry with less application memory
			}
		}
		Melder_assert (_buffer != NULL);

		/*
		 * Count the number of input devices and sources.
		 */
		if (_inputUsesPortAudio) {
			static bool paInitialized = false;
			if (! paInitialized) {
				PaError err = Pa_Initialize ();
				if (Melder_debug == 20) Melder_casual ("init %s", Pa_GetErrorText (err));
				paInitialized = true;
				if (Melder_debug == 20) {
					PaHostApiIndex hostApiCount = Pa_GetHostApiCount ();
					Melder_casual ("host API count %ls", Melder_integer (hostApiCount));
					for (PaHostApiIndex iHostApi = 0; iHostApi < hostApiCount; iHostApi ++) {
						const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo (iHostApi);
						PaHostApiTypeId type = hostApiInfo -> type;
						Melder_casual ("host API %ls: %ls, \"%s\" %ls", Melder_integer (iHostApi), Melder_integer (type), hostApiInfo -> name, Melder_integer (hostApiInfo -> deviceCount));
					}
					PaHostApiIndex defaultHostApi = Pa_GetDefaultHostApi ();
					Melder_casual ("default host API %ls", Melder_integer (defaultHostApi));
					PaDeviceIndex deviceCount = Pa_GetDeviceCount ();
					Melder_casual ("device count %ls", Melder_integer (deviceCount));
				}
			}
			PaDeviceIndex deviceCount = Pa_GetDeviceCount ();
			for (PaDeviceIndex idevice = 0; idevice < deviceCount; idevice ++) {
				const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo (idevice);
				if (Melder_debug == 20) Melder_casual ("Device \"%s\", input %d, output %d, sample rate %lf", deviceInfo -> name,
					deviceInfo -> maxInputChannels, deviceInfo -> maxOutputChannels, deviceInfo -> defaultSampleRate);
				if (deviceInfo -> maxInputChannels > 0 && _numberOfInputDevices < SoundRecorder_IDEVICE_MAX) {
					_device [++ _numberOfInputDevices]. canDo = true;
					wcsncpy (_device [_numberOfInputDevices]. name, Melder_peekUtf8ToWcs (deviceInfo -> name), 40);
					_device [_numberOfInputDevices]. name [40] = '\0';
					_deviceInfos [_numberOfInputDevices] = deviceInfo;
					_deviceIndices [_numberOfInputDevices] = idevice;
				}
			}
			if (_numberOfInputDevices == 0)
				Melder_throw ("No input devices available.");
		} else {
			#if defined (macintosh)
				for (long idevice = 1; idevice <= 8; idevice ++) {
					Str255 hybridDeviceName;
					OSErr err = SPBGetIndexedDevice (idevice, & hybridDeviceName [0], NULL);
					if (err == siBadSoundInDevice) break;
					(void) PtoCstr (hybridDeviceName);
					if (SPBOpenDevice (hybridDeviceName, siWritePermission, & _refNum) == noErr) {
						Handle handle;
						if (SPBGetDeviceInfo (_refNum, siInputSourceNames, & handle) == noErr) {
							char *data = *handle, *plength;
							int numberOfDeviceSources = * (short *) data, deviceSource;
							/*HLock (handle);*/
							plength = & data [2];
							for (deviceSource = 1; deviceSource <= numberOfDeviceSources; deviceSource ++) {
								if (_numberOfInputDevices == SoundRecorder_IDEVICE_MAX) break;
								_device [++ _numberOfInputDevices]. canDo = true;
								strcpy ((char *) _hybridDeviceNames [_numberOfInputDevices], (const char *) hybridDeviceName);
								_macSource [_numberOfInputDevices] = deviceSource;
								plength [40] = '\0';
								wcsncpy (_device [_numberOfInputDevices]. name, Melder_peekUtf8ToWcs (plength + 1), *plength < 40 ? *plength : 40);
								_device [_numberOfInputDevices]. name [*plength < 40 ? *plength : 40] = '\0';
								plength += *plength + 1;
							}
							DisposeHandle (handle);
						}
						SPBCloseDevice (_refNum);
					}
				}
			#elif defined (_WIN32)
				// No device info: use Windows mixer.
			#else
				_device [1]. canDo = true;
				wcscpy (_device [1]. name, L"Microphone");
				_device [2]. canDo = true;
				wcscpy (_device [2]. name, L"Line");
				#if defined (sgi)
					_device [3]. canDo = true;
					wcscpy (_device [3]. name, L"Digital");
				#endif
			#endif
		}

		/*
		 * Sampling frequency constants.
		 */
		_fsamp [SoundRecorder_IFSAMP_8000]. fsamp = 8000.0;
		_fsamp [SoundRecorder_IFSAMP_9800]. fsamp = 9800.0;
		_fsamp [SoundRecorder_IFSAMP_11025]. fsamp = 11025.0;
		_fsamp [SoundRecorder_IFSAMP_12000]. fsamp = 12000.0;
		_fsamp [SoundRecorder_IFSAMP_16000]. fsamp = 16000.0;
		_fsamp [SoundRecorder_IFSAMP_22050]. fsamp = 22050.0;
		_fsamp [SoundRecorder_IFSAMP_22254]. fsamp = 22254.54545;
		_fsamp [SoundRecorder_IFSAMP_24000]. fsamp = 24000.0;
		_fsamp [SoundRecorder_IFSAMP_32000]. fsamp = 32000.0;
		_fsamp [SoundRecorder_IFSAMP_44100]. fsamp = 44100.0;
		_fsamp [SoundRecorder_IFSAMP_48000]. fsamp = 48000.0;
		_fsamp [SoundRecorder_IFSAMP_64000]. fsamp = 64000.0;
		_fsamp [SoundRecorder_IFSAMP_96000]. fsamp = 96000.0;
		_fsamp [SoundRecorder_IFSAMP_192000]. fsamp = 192000.0;

		/*
		 * The default set of possible sampling frequencies, to be modified in the initialize () procedure.
		 */
		for (long i = 1; i <= SoundRecorder_IFSAMP_MAX; i ++) _fsamp [i]. canDo = true;   // optimistic: can do all, except two:
		_fsamp [SoundRecorder_IFSAMP_9800]. canDo = false;   // sgi only
		_fsamp [SoundRecorder_IFSAMP_22254]. canDo = false;   // old Mac only

		/*
		 * Initialize system-dependent structures.
		 * On all systems: stereo 16-bit linear encoding.
		 * Some systems take initial values from the system control panel
		 * (automatically in the workProc), other systems from theControlPanel.
		 */
		initialize (); therror

		#if motif
		Melder_assert (XtWindow (_meter));
		#endif
		_graphics = Graphics_create_xmdrawingarea (_meter);
		Melder_assert (_graphics);
		Graphics_setWindow (_graphics, 0.0, 1.0, 0.0, 1.0);
		Graphics_setColour (_graphics, Graphics_WHITE);
		Graphics_fillRectangle (_graphics, 0.0, 1.0, 0.0, 1.0);

		struct structGuiDrawingAreaResizeEvent event = { _meter, 0 };
		event. width = GuiObject_getWidth (_meter);
		event. height = GuiObject_getHeight (_meter);
		gui_drawingarea_cb_resize (this, & event);

		#if gtk
			g_idle_add (workProc, this);
		#elif motif
			_workProcId = XtAppAddWorkProc (applicationContext, workProc, this);
		#endif
	/*} catch (...) {
		rethrowmzero ("SoundRecorder not created.");
	}*/
}

SoundRecorder::~SoundRecorder () {
	stopRecording ();   /* Must occur before freeing _buffer. */
	MelderAudio_stopPlaying (MelderAudio_IMPLICIT);   /* Must also occur before freeing _buffer. */
	#if gtk
		g_idle_remove_by_data(this);
	#elif motif
		if (_workProcId) XtRemoveWorkProc (_workProcId);
	#endif
	NUMsvector_free (_buffer, 0);

	if (_inputUsesPortAudio) {
		if (_portaudioStream) Pa_StopStream (_portaudioStream);
		if (_portaudioStream) Pa_CloseStream (_portaudioStream);
	} else {
		#if defined (_WIN32)
			if (_hWaveIn != 0) {
				waveInReset (_hWaveIn);
				waveInUnprepareHeader (_hWaveIn, & _waveHeader [0], sizeof (WAVEHDR));
				waveInClose (_hWaveIn);
			}
		#elif defined (macintosh)
			if (_refNum) SPBCloseDevice (_refNum);
		#elif defined (sgi)
			if (_port) ALcloseport (_port);
			if (_audio) ALfreeconfig (_audio);
		#elif defined (UNIX) || defined (HPUX)
			if (_fd != -1) close (_fd);
		#endif
	}
	forget (_graphics);
}

/********** ERROR HANDLING **********/

#if defined (_WIN32)
void SoundRecorder::win_fillFormat () {
	_waveFormat. nSamplesPerSec = (int) theControlPanel. sampleRate;
	_waveFormat. nChannels = _numberOfChannels;
	_waveFormat. wFormatTag = WAVE_FORMAT_PCM;
	_waveFormat. wBitsPerSample = 16;
	_waveFormat. nBlockAlign = _waveFormat. nChannels * _waveFormat. wBitsPerSample / 8;
	_waveFormat. nAvgBytesPerSec = _waveFormat. nBlockAlign * _waveFormat. nSamplesPerSec;
	_waveFormat. cbSize = 0;
}
void SoundRecorder::win_fillHeader (int which) {
	_waveHeader [which]. dwFlags = 0;
	_waveHeader [which]. lpData = which == 0 ? (char *) _buffer : which == 1 ? (char *) _buffertje1: (char *) _buffertje2;
	_waveHeader [which]. dwBufferLength = which == 0 ? _nmax * _waveFormat. nChannels * 2 : 1000 * _waveFormat. nChannels * 2;
	_waveHeader [which]. dwLoops = 0;
	_waveHeader [which]. lpNext = NULL;
	_waveHeader [which]. reserved = 0;
}
int SoundRecorder::win_waveInCheck () {
	wchar_t messageText [MAXERRORLENGTH];
	MMRESULT err;
	if (_err == MMSYSERR_NOERROR) return 1;
	err = waveInGetErrorText (_err, messageText, MAXERRORLENGTH);
	if (err == MMSYSERR_NOERROR) Melder_error1 (messageText);
	else if (err == MMSYSERR_BADERRNUM) Melder_error3 (L"Error number ", Melder_integer (_err), L" out of range.");
	else if (err == MMSYSERR_NODRIVER) Melder_error1 (L"No sound driver present.");
	else if (err == MMSYSERR_NOMEM) Melder_error1 (L"Out of memory.");
	else Melder_error1 (L"Unknown sound error.");
	return 0;
}
int SoundRecorder::win_waveInOpen () {
	_err = waveInOpen (& _hWaveIn, WAVE_MAPPER, & _waveFormat, 0, 0, CALLBACK_NULL);
	if (! win_waveInCheck (me)) return Melder_error1 (L"Audio input not opened.");
	if (Melder_debug != 8) waveInReset (_hWaveIn);
	return 1;
}
int SoundRecorder::win_waveInPrepareHeader (int which) {
	_err = waveInPrepareHeader (_hWaveIn, & _waveHeader [which], sizeof (WAVEHDR));
	if (! win_waveInCheck (me)) return Melder_error1 (L"Audio input: cannot prepare header.\n"
		"Quit some other programs or go to \"Sound input prefs\" in the Preferences menu.");
	return 1;
}
int SoundRecorder::win_waveInAddBuffer (int which) {
	_err = waveInAddBuffer (_hWaveIn, & _waveHeader [which], sizeof (WAVEHDR));
	if (! win_waveInCheck (me)) return Melder_error1 (L"Audio input: cannot add buffer.");
	return 1;
}
int SoundRecorder::win_waveInStart () {
	_err = waveInStart (_hWaveIn);   /* Asynchronous. */
	if (! win_waveInCheck (me)) return Melder_error1 (L"Audio input not started.");
	return 1;
}
int SoundRecorder::win_waveInStop () {
	_err = waveInStop (_hWaveIn);
	if (! win_waveInCheck (me)) return Melder_error1 (L"Audio input not stopped.");
	return 1;
}
int SoundRecorder::win_waveInReset () {
	_err = waveInReset (_hWaveIn);
	if (! win_waveInCheck (me)) return Melder_error1 (L"Audio input not reset.");
	return 1;
}
int SoundRecorder::win_waveInUnprepareHeader (int which) {
	_err = waveInUnprepareHeader (_hWaveIn, & _waveHeader [which], sizeof (WAVEHDR));
	if (! win_waveInCheck (me)) return Melder_error1 (L"Audio input: cannot unprepare header.");
	return 1;
}
int SoundRecorder::win_waveInClose () {
	_err = waveInClose (_hWaveIn);
	_hWaveIn = 0;
	if (! win_waveInCheck (me)) return Melder_error1 (L"Audio input not closed.");
	return 1;
}
#endif

#if defined (macintosh)
const char * SoundRecorder::errString (long err) {
	switch (err) {
		/* -54 */ case permErr: return "Attempt to open locked file for writing.";
		/* -128 */ case userCanceledErr: return "User cancelled the operation.";
		/* -200 */ case noHardwareErr: return "No such sound hardware.";
		/* -220 */ case siNoSoundInHardware: return "No sound input hardware available.";
		/* -221 */ case siBadSoundInDevice: return "Invalid sound input device.";
		/* -222 */ case siNoBufferSpecified: return "No buffer specified for synchronous recording.";
		/* -223 */ case siInvalidCompression: return "Invalid compression type.";
		/* -224 */ case siHardDriveTooSlow: return "Hard drive too slow to record.";
		/* -227 */ case siDeviceBusyErr: return "Sound input device is busy.";
		/* -228 */ case siBadDeviceName: return "Invalid device name.";
		/* -229 */ case siBadRefNum: return "Invalid reference number.";
		/* -231 */ case siUnknownInfoType: return "Unknown type of information.";
		/* -232 */ case siUnknownQuality: return "Unknown quality.";
		default: return NULL; break;
	}
	return NULL;
}
void SoundRecorder::onceError (const char *routine, long err) {
	static long notified = FALSE;
	const char *string;
	if (notified) return;
	string = errString (err);
	if (string) Melder_flushError ("(%s:) %s", routine, string);
	else Melder_flushError ("(%s:) Error %ld", routine, err);
	notified = TRUE;
}
#endif

void SoundRecorder::stopRecording () {
	if (! _recording) return;
	_recording = false;
	if (! _synchronous) {
		if (_inputUsesPortAudio) {
			Pa_StopStream (_portaudioStream);
			Pa_CloseStream (_portaudioStream);
			_portaudioStream = NULL;
		} else {
			#if defined (_WIN32)
				/*
				 * On newer systems, waveInStop waits until the buffer is full.
				 * Wrong behaviour!
				 * Therefore, we call waveInReset instead.
				 * But on these same newer systems, waveInReset causes the dwBytesRecorded
				 * attribute to go to zero, so we cannot do
				 * _nsamp = _waveHeader [0]. dwBytesRecorded / (sizeof (short) * _numberOfChannels);
				 */
				MMTIME mmtime;
				mmtime. wType = TIME_BYTES;
				_nsamp = 0;
				if (waveInGetPosition (_hWaveIn, & mmtime, sizeof (MMTIME)) == MMSYSERR_NOERROR)
					_nsamp = mmtime. u.cb / (sizeof (short) * _numberOfChannels);
				win_waveInReset (me); cherror
				if (_nsamp == 0)
					_nsamp = _waveHeader [0]. dwBytesRecorded / (sizeof (short) * _numberOfChannels);
				if (_nsamp > _nmax)
					_nsamp = _nmax;
				win_waveInUnprepareHeader (me, 0); cherror
				win_waveInClose (me); cherror
			#elif defined (macintosh)
				OSErr err;
				short recordingStatus, meterLevel;
				unsigned long totalSamplesToRecord, numberOfSamplesRecorded, totalMsecsToRecord, numberOfMsecsRecorded;
				err = SPBGetRecordingStatus (_refNum, & recordingStatus, & meterLevel,
						& totalSamplesToRecord, & numberOfSamplesRecorded,
						& totalMsecsToRecord, & numberOfMsecsRecorded);
				if (err != noErr) { onceError ("SPBGetRecordingStatus", err); return; }
				/* Melder_assert (meterLevel >= 0); Melder_assert (meterLevel <= 255); */
				if (totalSamplesToRecord == 0)
					_nsamp = _nmax;
				else
					_nsamp = numberOfSamplesRecorded / (sizeof (short) * _numberOfChannels);   /* From Mac "samples" to Mac "frames" (our "samples"). */
				err = SPBStopRecording (_refNum);
				if (err != noErr) { onceError ("SPBStopRecording", err); return; }
				_spb. bufferPtr = NULL;
				err = SPBRecord (& _spb, true);
				if (err != noErr) onceError ("SPBRecord", err);
			#endif
		}
	}
#ifdef _WIN32
end:
	iferror Melder_flushError ("Cannot stop recording.");
#endif
	Graphics_setWindow (_graphics, 0.0, 1.0, 0.0, 1.0);
	Graphics_setColour (_graphics, Graphics_WHITE);
	Graphics_fillRectangle (_graphics, 0.0, 1.0, 0.0, 1.0);
}

void SoundRecorder::showMaximum (int channel, double maximum) {
	maximum /= 32768.0;
	Graphics_setWindow (_graphics,
		_numberOfChannels == 1 || channel == 1 ? -0.1 : -2.1,
		_numberOfChannels == 1 || channel == 2 ? 1.1 : 3.1,
		-0.1, 1.1);
	Graphics_setGrey (_graphics, 0.9);
	Graphics_fillRectangle (_graphics, 0.0, 1.0, maximum, 1.0);
	Graphics_setColour (_graphics, Graphics_GREEN);
	if (maximum < 0.75) {
		Graphics_fillRectangle (_graphics, 0.0, 1.0, 0.0, maximum);
	} else {
		Graphics_fillRectangle (_graphics, 0.0, 1.0, 0.0, 0.75);
		Graphics_setColour (_graphics, Graphics_YELLOW);
		if (maximum < 0.92) {
			Graphics_fillRectangle (_graphics, 0.0, 1.0, 0.75, maximum);
		} else {
			Graphics_fillRectangle (_graphics, 0.0, 1.0, 0.75, 0.92);
			Graphics_setColour (_graphics, Graphics_RED);
			Graphics_fillRectangle (_graphics, 0.0, 1.0, 0.92, maximum);
		}
	}
}

void SoundRecorder::showMeter (short *buffer, long nsamp) {
	Melder_assert (_graphics != NULL);
	Graphics_setWindow (_graphics, 0.0, 1.0, 0.0, 1.0);
	//#ifndef _WIN32
	//	Graphics_setColour (_graphics, Graphics_WHITE);
	//	Graphics_fillRectangle (_graphics, 0.0, 1.0, 0.0, 1.0);
	//#endif
	Graphics_setColour (_graphics, Graphics_BLACK);
	if (nsamp < 1) {
		Graphics_setTextAlignment (_graphics, Graphics_CENTRE, Graphics_HALF);
		#if defined (macintosh)
			Graphics_setColour (_graphics, Graphics_WHITE);
			Graphics_fillRectangle (_graphics, 0.2, 0.8, 0.3, 0.7);
			Graphics_setColour (_graphics, Graphics_BLACK);
		#endif
		Graphics_text (_graphics, 0.5, 0.5, L"Not recording.");
		return;
	}
	short leftMaximum = 0, rightMaximum = 0;
	if (_numberOfChannels == 1) {
		for (long i = 0; i < nsamp; i ++) {
			short value = buffer [i];
			if (abs (value) > leftMaximum) leftMaximum = abs (value);
		}
	} else {
		for (long i = 0; i < nsamp; i ++) {
			long left = buffer [i+i], right = buffer [i+i+1];
			if (abs (left) > leftMaximum) leftMaximum = abs (left);
			if (abs (right) > rightMaximum) rightMaximum = abs (right);
		}
	}
	if (_lastLeftMaximum > 30000) {
		int leak = _lastLeftMaximum - 2000000 / theControlPanel. sampleRate;
		if (leftMaximum < leak) leftMaximum = leak;
	}
	showMaximum (1, leftMaximum);
	_lastLeftMaximum = leftMaximum;
	if (_numberOfChannels == 2) {
		if (_lastRightMaximum > 30000) {
			int leak = _lastRightMaximum - 2000000 / theControlPanel. sampleRate;
			if (rightMaximum < leak) rightMaximum = leak;
		}
		showMaximum (2, rightMaximum);
		_lastRightMaximum = rightMaximum;
	}
}

int SoundRecorder::tooManySamplesInBufferToReturnToGui () {
	#if defined (sgi)
		return ALgetfilled (_port) > step;
	#else
		return FALSE;
	#endif
}

long SoundRecorder::getMyNsamp () {
	volatile long nsamp = _nsamp;   // Prevent inlining.
	return nsamp;
}

static int portaudioStreamCallback (
    const void *input, void *output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *void_me)
{
	/*
	 * This procedure may be called at interrupt time.
	 * It therefore accesses only data that is constant during recording,
	 * namely me, _buffer, _numberOfChannels, and _nmax.
	 * The only thing it changes is _nsamp;
	 * the workProc will therefore have to take some care in accessing _nsamp (see there).
	 */
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	(void) output;
	(void) timeInfo;
	(void) statusFlags;
	if (Melder_debug == 20) Melder_casual ("The PortAudio stream callback receives %ld frames.", frameCount);
	Melder_assert (soundRecorder->_nsamp <= soundRecorder->_nmax);
	unsigned long samplesLeft = soundRecorder->_nmax - soundRecorder->_nsamp;
	if (samplesLeft > 0) {
		unsigned long dsamples = samplesLeft > frameCount ? frameCount : samplesLeft;
		if (Melder_debug == 20) Melder_casual ("play %ls %ls", Melder_integer (dsamples),
			Melder_double (Pa_GetStreamCpuLoad (soundRecorder->_portaudioStream)));
		memcpy (soundRecorder->_buffer + soundRecorder->_nsamp * soundRecorder->_numberOfChannels, input, 2 * dsamples * soundRecorder->_numberOfChannels);
		soundRecorder->_nsamp += dsamples;
		if (soundRecorder->_nsamp >= soundRecorder->_nmax) return paComplete;
	} else /*if (_nsamp >= _nmax)*/ {
		soundRecorder->_nsamp = soundRecorder->_nmax;
		return paComplete;
	}
	return paContinue;
}

static void gui_button_cb_record (I, GuiButtonEvent event) {
	(void) event;
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	if (soundRecorder->_recording) return;
	soundRecorder->_nsamp = 0;
	soundRecorder->_recording = true;
	soundRecorder->_lastLeftMaximum = 0;
	soundRecorder->_lastRightMaximum = 0;
	if (! soundRecorder->_synchronous) {
		if (soundRecorder->_inputUsesPortAudio) {
			PaStreamParameters streamParameters = { 0 };
			streamParameters. device = soundRecorder->_deviceIndices [theControlPanel. inputSource];
			streamParameters. channelCount = soundRecorder->_numberOfChannels;
			streamParameters. sampleFormat = paInt16;
			streamParameters. suggestedLatency = soundRecorder->_deviceInfos [theControlPanel. inputSource] -> defaultLowInputLatency;
			#if defined (macintosh)
				PaMacCoreStreamInfo macCoreStreamInfo = { 0 };
				macCoreStreamInfo. size = sizeof (PaMacCoreStreamInfo);
				macCoreStreamInfo. hostApiType = paCoreAudio;
				macCoreStreamInfo. version = 0x01;
				macCoreStreamInfo. flags = paMacCoreChangeDeviceParameters | paMacCoreFailIfConversionRequired;
				streamParameters. hostApiSpecificStreamInfo = & macCoreStreamInfo;
			#endif
			if (Melder_debug == 20) Melder_casual ("Before Pa_OpenStream");
			PaError err = Pa_OpenStream (& soundRecorder->_portaudioStream, & streamParameters, NULL,
				theControlPanel. sampleRate, 0, paNoFlag, portaudioStreamCallback, soundRecorder);
			if (Melder_debug == 20) Melder_casual ("Pa_OpenStream returns %d", err);
			if (err) { Melder_error2 (L"open ", Melder_peekUtf8ToWcs (Pa_GetErrorText (err))); goto end; }
			Pa_StartStream (soundRecorder->_portaudioStream);
			if (Melder_debug == 20) Melder_casual ("Pa_StartStream returns %d", err);
			if (err) { Melder_error2 (L"start ", Melder_peekUtf8ToWcs (Pa_GetErrorText (err))); goto end; }
		} else {
			#if defined (_WIN32)
				soundRecorder->win_fillFormat ();
				soundRecorder->win_fillHeader (0);
				soundRecorder->win_waveInOpen (); cherror
				soundRecorder->win_waveInPrepareHeader (0); cherror
				soundRecorder->win_waveInAddBuffer (0); cherror
				soundRecorder->win_waveInStart (); cherror
			#elif defined (macintosh)
				OSErr err;
				err = SPBStopRecording (soundRecorder->_refNum);
				if (err != noErr) { onceError ("SPBStopRecording", err); return; }
				soundRecorder->_spb. bufferPtr = (char *) _buffer;
				soundRecorder->_spb. bufferLength = soundRecorder->_spb. count = soundRecorder->_nmax * (sizeof (short) * soundRecorder->_numberOfChannels);
				err = SPBRecord (& soundRecorder->_spb, true);   /* Asynchronous. */
				if (err == notEnoughMemoryErr) {
					Melder_flushError ("Out of memory. Quit other programs."); return;
				} else if (err != noErr) { onceError ("SPBRecord", err); return; }
			#endif
		}
	}
end:
	Graphics_setWindow (soundRecorder->_graphics, 0.0, 1.0, 0.0, 1.0);
	Graphics_setColour (soundRecorder->_graphics, Graphics_WHITE);
	Graphics_fillRectangle (soundRecorder->_graphics, 0.0, 1.0, 0.0, 1.0);
	iferror { soundRecorder->_recording = false; Melder_flushError ("Cannot record."); }
}

static void gui_button_cb_stop (I, GuiButtonEvent event) {
	(void) event;
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	soundRecorder->stopRecording ();
}

static void gui_button_cb_play (I, GuiButtonEvent event) {
	(void) event;
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	if (soundRecorder->_recording || soundRecorder->_nsamp == 0) return;
	if (! MelderAudio_play16 (soundRecorder->_buffer, theControlPanel. sampleRate, soundRecorder->_fakeMono ? soundRecorder->_nsamp / 2 : soundRecorder->_nsamp, soundRecorder->_fakeMono ? 2 : soundRecorder->_numberOfChannels, NULL, NULL))
		Melder_flushError (NULL);
}

void SoundRecorder::publish () {
	Sound sound = NULL;
	long i, nsamp = _fakeMono ? _nsamp / 2 : _nsamp;
	double fsamp = theControlPanel. sampleRate;
	if (_nsamp == 0) return;
	if (fsamp <= 0) fsamp = 48000.0;   /* Safe. */
	sound = Sound_createSimple (_numberOfChannels, (double) nsamp / fsamp, fsamp);
	if (Melder_hasError ()) { Melder_flushError ("You can still save to file."); return; }
	if (_fakeMono) {
		for (i = 1; i <= nsamp; i ++)
			sound -> z [1] [i] = (_buffer [i + i - 2] + _buffer [i + i - 1]) * (1.0 / 65536);
	} else if (_numberOfChannels == 1) {
		for (i = 1; i <= nsamp; i ++)
			sound -> z [1] [i] = _buffer [i - 1] * (1.0 / 32768);
	} else {
		for (i = 1; i <= nsamp; i ++) {
			sound -> z [1] [i] = _buffer [i + i - 2] * (1.0 / 32768);
			sound -> z [2] [i] = _buffer [i + i - 1] * (1.0 / 32768);
		}
	}
	if (_soundName) {
		wchar_t *name = GuiText_getString (_soundName);
		Thing_setName (sound, name);
		Melder_free (name);
	}
	if (_publishCallback)
		_publishCallback (this, _publishClosure, sound);
}

static void gui_button_cb_cancel (I, GuiButtonEvent event) {
	(void) event;
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	soundRecorder->stopRecording ();
	forget (soundRecorder);
}

static void gui_button_cb_apply (I, GuiButtonEvent event) {
	(void) event;
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	soundRecorder->stopRecording ();
	soundRecorder->publish ();
}
static void gui_cb_apply (GuiObject widget, XtPointer void_me, XtPointer call) {
	(void) widget;
	(void) call;
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	soundRecorder->stopRecording ();
	soundRecorder->publish ();
}

static void gui_button_cb_ok (I, GuiButtonEvent event) {
	(void) event;
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	soundRecorder->stopRecording ();
	soundRecorder->publish ();
	forget (soundRecorder);
}

int SoundRecorder::initialize () {
	if (_inputUsesPortAudio) {
		#if defined (macintosh)
			_fsamp [SoundRecorder_IFSAMP_8000]. canDo = false;
			_fsamp [SoundRecorder_IFSAMP_11025]. canDo = false;
			_fsamp [SoundRecorder_IFSAMP_12000]. canDo = false;
			_fsamp [SoundRecorder_IFSAMP_16000]. canDo = false;
			_fsamp [SoundRecorder_IFSAMP_22050]. canDo = false;
			_fsamp [SoundRecorder_IFSAMP_24000]. canDo = false;
			_fsamp [SoundRecorder_IFSAMP_32000]. canDo = false;
			_fsamp [SoundRecorder_IFSAMP_64000]. canDo = false;
		#endif
		// else accept all standard sample rates.
	} else {
		#if defined (sgi)
			_audio = ALnewconfig ();
			if (! _audio)
				return Melder_error1 (L"Unexpected error: cannot create audio config...");
			ALsetchannels (_audio, _numberOfChannels == 1 ? AL_MONO : AL_STEREO);
			ALsetwidth (_audio, AL_SAMPLE_16);
			if (! (_port = ALopenport ("SoundRecorder", "r", _audio)))
				return Melder_error1 (L"Cannot open audio port (too many open already).");
			_can9800 = TRUE;
		#elif defined (macintosh)
			unsigned long sampleRate_uf = theControlPanel. sampleRate * 65536L;
			short numberOfChannels = _numberOfChannels, continuous = TRUE, sampleSize = 16, async;
			char levelMeterOnOff = 1;
			short inputSource = theControlPanel. inputSource, irate;
			OSType compressionType = 'NONE';
			struct { Handle dummy1; short dummy2, number; Handle handle; } sampleRateInfo;   /* Make sure that number is adjacent to handle. */
			if (SPBOpenDevice (_hybridDeviceNames [inputSource], siWritePermission, & _refNum) != noErr)
				Melder_flushError ("(Sound_record:) Cannot open audio input device.");
			/*
				From Apple:
				"Get the range of sample rates this device can produce.
				The infoData  parameter points to an integer, which is the number of sample rates the device supports, followed by a handle.
				The handle references a list of sample rates, each of type Fixed .
				If the device can record a range of sample rates, the number of sample rates is set to 0 and the handle contains two rates,
				the minimum and the maximum of the range of sample rates.
				Otherwise, a list is returned that contains the sample rates supported.
				In order to accommodate sample rates greater than 32 kHz, the most significant bit is not treated as a sign bit;
				instead, that bit is interpreted as having the value 32,768."
			 */
			SPBGetDeviceInfo (_refNum, siSampleRateAvailable, & sampleRateInfo. number);
			if (sampleRateInfo. number == 0) {
			} else {
				for (long i = 1; i <= SoundRecorder_IFSAMP_MAX; i ++) {
					_fsamp [i]. canDo = false;
				}
				for (irate = 1; irate <= sampleRateInfo. number; irate ++) {
					Fixed rate_fixed = (* (Fixed **) sampleRateInfo. handle) [irate - 1];
					unsigned short rate_ushort = * (unsigned short *) & rate_fixed;
					switch (rate_ushort) {
						case 0: _fsamp [SoundRecorder_IFSAMP_44100]. canDo = true,
								_fsamp [SoundRecorder_IFSAMP_48000]. canDo = true; break;   /* BUG */
						case 8000: _fsamp [SoundRecorder_IFSAMP_8000]. canDo = true; break;
						case 11025: _fsamp [SoundRecorder_IFSAMP_11025]. canDo = true; break;
						case 12000: _fsamp [SoundRecorder_IFSAMP_12000]. canDo = true; break;
						case 16000: _fsamp [SoundRecorder_IFSAMP_16000]. canDo = true; break;
						case 22050: _fsamp [SoundRecorder_IFSAMP_22050]. canDo = true; break;
						case 22254: _fsamp [SoundRecorder_IFSAMP_22254]. canDo = true; break;
						case 24000: _fsamp [SoundRecorder_IFSAMP_24000]. canDo = true; break;
						case 32000: _fsamp [SoundRecorder_IFSAMP_32000]. canDo = true; break;
						case 44100: _fsamp [SoundRecorder_IFSAMP_44100]. canDo = true; break;
						case 48000: _fsamp [SoundRecorder_IFSAMP_48000]. canDo = true; break;
						case 64000: _fsamp [SoundRecorder_IFSAMP_64000]. canDo = true; break;
						default: Melder_warning3 (L"Your computer seems to support a sampling frequency of ", Melder_integer (rate_ushort), L" Hz. "
							"Contact the author (paul.boersma@uva.nl) to make this frequency available to you.");
					}
				}
			}
			if (SPBSetDeviceInfo (_refNum, siInputSource, & _macSource [inputSource]) != noErr)
				Melder_flushError ("(Sound_record:) Cannot change input source.");
			/*if (SPBOpenDevice (NULL, siWritePermission, & _refNum) != noErr)
				return Melder_error1 (L"Cannot open audio input device.");*/
			if (SPBSetDeviceInfo (_refNum, siSampleRate, & sampleRate_uf) != noErr) {
				Melder_flushError ("Cannot set sampling frequency to %.5f Hz.", theControlPanel. sampleRate);
				theControlPanel. sampleRate = 44100;
			}
			if (SPBSetDeviceInfo (_refNum, siNumberChannels, & numberOfChannels) != noErr) {
				if (_numberOfChannels == 1) {
					_fakeMono = true;
				} else {
					return Melder_error1 (L"(Sound_record:) Cannot set to stereo.");
				}
			}
			if (SPBSetDeviceInfo (_refNum, siCompressionType, & compressionType) != noErr)
				return Melder_error1 (L"(Sound_record:) Cannot set to linear.");
			if (SPBSetDeviceInfo (_refNum, siSampleSize, & sampleSize) != noErr)
				return Melder_error3 (L"(Sound_record:) Cannot set to ", Melder_integer (sampleSize), L"-bit.");
			if (SPBSetDeviceInfo (_refNum, siLevelMeterOnOff, & levelMeterOnOff) != noErr)
				return Melder_error1 (L"(Sound_record:) Cannot set level meter to ON.");
			if (! _synchronous && (SPBGetDeviceInfo (_refNum, siAsync, & async) != noErr || ! async)) {
				static int warned = FALSE;
				_synchronous = true;
				if (! warned) { Melder_warning1 (L"Recording must and will be synchronous on this machine."); warned = TRUE; }
			}
			if (_synchronous && SPBSetDeviceInfo (_refNum, siContinuous, & continuous) != noErr)
				return Melder_error1 (L"(Sound_record:) Cannot set continuous recording.");
			_spb. inRefNum = _refNum;
			if (! _synchronous) {
				OSErr err;
				if (_recording) {
					_spb. bufferPtr = (char *) _buffer;
					_spb. bufferLength = _spb. count = _nmax * (sizeof (short) * _numberOfChannels);
				} else {
					_spb. bufferPtr = NULL;
				}
				err = SPBRecord (& _spb, true);
				if (err != noErr) { onceError ("SPBRecord", err); return 1; }
			}
		#elif defined (sun)
			_fd = open (getenv ("AUDIODEV") ? getenv ("AUDIODEV") : "/dev/audio", O_RDONLY);
			if (_fd == -1) {
				if (errno == EBUSY)
					return Melder_error1 (L"(SoundRecorder:) Audio device already in use.");
				else
					return Melder_error1 (L"(SoundRecorder:) Cannot open audio device.");
			}
			AUDIO_INITINFO (& _info);
			_info. record. pause = 1;
			ioctl (_fd, AUDIO_SETINFO, & _info);   /* Pause! */
			ioctl (_fd, I_FLUSH, FLUSHR);   /* Discard buffers! */

			#ifndef sun4
				AUDIO_INITINFO (& _info);
				_info. record. buffer_size = 8176;   /* Maximum. */
				ioctl (_fd, AUDIO_SETINFO, & _info);
			#endif

			AUDIO_INITINFO (& _info);
			_info. monitor_gain = 0;   /* Not rondzingen. */
			ioctl (_fd, AUDIO_SETINFO, & _info);

			/* Take over the saved settings. */

			AUDIO_INITINFO (& _info);
			_info. record. port = theControlPanel. inputSource == 2 ? AUDIO_LINE_IN :
				AUDIO_MICROPHONE;
			ioctl (_fd, AUDIO_SETINFO, & _info);
			AUDIO_INITINFO (& _info);
			_info. record. gain = theControlPanel. leftGain > theControlPanel. rightGain ?
				theControlPanel. leftGain : theControlPanel. rightGain;
			_info. record. balance = theControlPanel. leftGain == theControlPanel. rightGain ? 32 :
				theControlPanel. leftGain > theControlPanel. rightGain ?
				32 * theControlPanel. rightGain / theControlPanel. leftGain :
				64 - 32 * theControlPanel. leftGain / theControlPanel. rightGain;
			ioctl (_fd, AUDIO_SETINFO, & _info);
			AUDIO_INITINFO (& _info);
			_info. record. sample_rate = theControlPanel. sampleRate;
			ioctl (_fd, AUDIO_SETINFO, & _info);

			AUDIO_INITINFO (& _info);
			_info. record. precision = 16;
			_info. record. encoding = AUDIO_ENCODING_LINEAR;
			_info. record. channels = _numberOfChannels;
			ioctl (_fd, AUDIO_SETINFO, & _info);   /* 16-bit linear mono/stereo! */
			ioctl (_fd, AUDIO_GETINFO, & _info);
			if (_info. record. channels != _numberOfChannels)
				return Melder_error3 (L"(SoundRecorder:) Cannot set to ", _numberOfChannels == 1 ? L"mono" : L"stereo", L".");

			AUDIO_INITINFO (& _info);
			_info. record. pause = 0;
			ioctl (_fd, AUDIO_SETINFO, & _info);   /* Start listening! */
		#elif defined (HPUX)
			struct audio_limits limits;
			int dataFormat, channels, wantedInput, currentInput, sampleRate, bufferSize;
			_fd = open ("/dev/audio", O_RDONLY);
			if (_fd == -1) {
				if (errno == EBUSY)
					return Melder_error1 (L"(SoundRecorder:) Audio device already in use.");
				else
					return Melder_error1 (L"(SoundRecorder:) Cannot open audio device.");
			}
			ioctl (_fd, AUDIO_RESET, RESET_RX_BUF | RESET_RX_OVF);
			ioctl (_fd, AUDIO_PAUSE, AUDIO_RECEIVE);

			ioctl (_fd, AUDIO_GET_DATA_FORMAT, & dataFormat);
			if (dataFormat != AUDIO_FORMAT_LINEAR16BIT && ioctl (_fd, AUDIO_SET_DATA_FORMAT, AUDIO_FORMAT_LINEAR16BIT) == -1)
				return Melder_error1 (L"(SoundRecorder:) Cannot set 16-bit linear.");

			ioctl (_fd, AUDIO_GET_CHANNELS, & channels);
			if (channels != _numberOfChannels && ioctl (_fd, AUDIO_SET_CHANNELS, _numberOfChannels) == -1)
				return Melder_error3 (L"(SoundRecorder:) Cannot set ", _numberOfChannels == 1 ? L"mono" : L"stereo", L" input.");

			wantedInput = theControlPanel. inputSource == 2 ? AUDIO_IN_LINE : AUDIO_IN_MIKE;
			ioctl (_fd, AUDIO_GET_INPUT, & currentInput);
			if (currentInput != wantedInput && ioctl (_fd, AUDIO_SET_INPUT, wantedInput) == -1)
				return Melder_error1 (L"(SoundRecorder:) Cannot set input source.");

			ioctl (_fd, AUDIO_GET_SAMPLE_RATE, & sampleRate);
			if (sampleRate != theControlPanel. sampleRate && ioctl (_fd, AUDIO_SET_SAMPLE_RATE, (int) theControlPanel. sampleRate) == -1)
				return Melder_error1 (L"(SoundRecorder:) Cannot set sampling frequency.");

			ioctl (_fd, AUDIO_GET_LIMITS, & limits);
			ioctl (_fd, AUDIO_GET_RXBUFSIZE, & bufferSize);
			if (bufferSize != limits. max_receive_buffer_size && ioctl (_fd, AUDIO_SET_RXBUFSIZE, limits. max_receive_buffer_size) == -1)
				return Melder_error1 (L"(SoundRecorder:) Cannot set buffer size.");

			ioctl (_fd, AUDIO_RESUME, AUDIO_RECEIVE);
		#elif defined (_WIN32)
			(void) me;
		#elif defined (linux)
			int sampleRate = (int) theControlPanel. sampleRate, sampleSize = 16;
			int channels = _numberOfChannels, stereo = ( _numberOfChannels == 2 ), val;
			#if __BYTE_ORDER == __BIG_ENDIAN
				int format = AFMT_S16_BE;
			#else
				int format = AFMT_S16_LE;
			#endif
			int fd_mixer;
			_fd = open ("/dev/dsp", O_RDONLY);
			if (_fd == -1) {
				if (errno == EBUSY)
					return Melder_error1 (L"(SoundRecorder:) Audio device already in use.");
				else
					return Melder_error1 (L"(SoundRecorder:) Cannot open audio device.\n"
						"Please switch on PortAudio in the Sound Recording Preferences.");
			}
			ioctl (_fd, SNDCTL_DSP_RESET, NULL);
			ioctl (_fd, SNDCTL_DSP_SPEED, & sampleRate);
			ioctl (_fd, SNDCTL_DSP_SAMPLESIZE, & sampleSize);
			ioctl (_fd, SNDCTL_DSP_CHANNELS, (val = channels, & val));
			if (channels == 1 && val == 2) {
				close (_fd);
				return Melder_error1 (L"(SoundRecorder:) This sound card does not support mono.");
			}
			ioctl (_fd, SNDCTL_DSP_STEREO, & stereo);
			ioctl (_fd, SNDCTL_DSP_SETFMT, & format);
			fd_mixer = open ("/dev/mixer", O_WRONLY);		
			if (fd_mixer == -1) {
				return Melder_error1 (L"(SoundRecorder:) Cannot open /dev/mixer.");
			} else {
				int dev_mask = theControlPanel. inputSource == 2 ? SOUND_MASK_LINE : SOUND_MASK_MIC;
				if (ioctl (fd_mixer, SOUND_MIXER_WRITE_RECSRC, & dev_mask) == -1)
					Melder_flushError ("(SoundRecorder:) Can't set recording device in mixer.");		
				close (fd_mixer);
			}
		#endif
	}
	return 1;
}

static void gui_radiobutton_cb_input (I, GuiRadioButtonEvent event) {
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	theControlPanel. inputSource = 1;   // Default.
	Melder_assert (event -> toggle != NULL);
	for (long i = 1; i <= SoundRecorder_IDEVICE_MAX; i ++) {
		if (event -> toggle == soundRecorder->_device [i]. button) {
			theControlPanel. inputSource = i;
		}
	}

	/* Set system's input source. */
	if (soundRecorder->_inputUsesPortAudio) {
		// Deferred to the start of recording.
	} else {
		#if defined (_WIN32)
			// Deferred to the start of recording.
		#elif defined (sgi)
			soundRecorder->_info [0] = AL_INPUT_SOURCE;
			soundRecorder->_info [1] =
				theControlPanel. inputSource == 1 ? AL_INPUT_MIC :
				theControlPanel. inputSource == 2 ? AL_INPUT_LINE : AL_INPUT_DIGITAL;
			ALsetparams (AL_DEFAULT_DEVICE, _info, 2);
		#elif defined (macintosh)
			SPBCloseDevice (soundRecorder->_refNum);
			if (! soundRecorder->initialize ()) Melder_flushError (NULL);
		#elif defined (sun)
			AUDIO_INITINFO (& _info);
			_info. record. port =
				theControlPanel. inputSource == 1 ? AUDIO_MICROPHONE : AUDIO_LINE_IN;
			ioctl (soundRecorder->_fd, AUDIO_SETINFO, & soundRecorder->_info);
		#elif defined (HPUX)
			ioctl (soundRecorder->_fd, AUDIO_SET_INPUT,
				theControlPanel. inputSource == 1 ? AUDIO_IN_MIKE : AUDIO_IN_LINE);
		#elif defined (linux)
			int fd_mixer = open ("/dev/mixer", O_WRONLY);		
			if (fd_mixer == -1) {
				Melder_flushError ("(Sound_record:) Cannot open /dev/mixer.");
			}
			int dev_mask = theControlPanel.inputSource == 2 ? SOUND_MASK_LINE : SOUND_MASK_MIC;
			if (ioctl (fd_mixer, SOUND_MIXER_WRITE_RECSRC, & dev_mask) == -1)
				Melder_flushError ("(Sound_record:) Can't set recording device in mixer");		
			close (fd_mixer);
		#endif
	}
}

static void gui_radiobutton_cb_fsamp (I, GuiRadioButtonEvent event) {
	SoundRecorder *soundRecorder = (SoundRecorder *)void_me;
	if (soundRecorder->_recording) return;
	double fsamp = NUMundefined;
	for (long i = 1; i <= SoundRecorder_IFSAMP_MAX; i ++)
		if (event -> toggle == soundRecorder->_fsamp [i]. button)
			fsamp = soundRecorder->_fsamp [i]. fsamp;
	Melder_assert (NUMdefined (fsamp));
	/*
	 * If we push the 48000 button while the sampling frequency is 22050,
	 * we first get a message that the 22050 button has changed,
	 * and then we get a message that the 48000 button has changed.
	 * So the following will work (it used to be different with old Motif versions on Linux):
	 */
	if (fsamp == theControlPanel. sampleRate) return;
	/*
	 * Now we know, hopefully, that the message is from the button that was clicked,
	 * not the one that was unset by the radio box, so we can take action.
	 */
	theControlPanel. sampleRate = fsamp;
	/*
	 * Set the system's sampling frequency.
	 * On some systems, we cannot do this without closing the audio device,
	 * and reopening it with a new sampling frequency.
	 */
	if (soundRecorder->_inputUsesPortAudio) {
		// Deferred to the start of recording.
	} else {
		#if defined (_WIN32)
			// Deferred to the start of recording.
		#elif defined (sgi)
			soundRecorder->_info [0] = AL_INPUT_RATE;
			soundRecorder->_info [1] = (int) theControlPanel. sampleRate;
			ALsetparams (AL_DEFAULT_DEVICE, soundRecorder->_info, 2);
		#elif defined (macintosh)
			SPBCloseDevice (soundRecorder->_refNum);
			if (! soundRecorder->initialize ()) Melder_flushError (NULL);
		#elif defined (sun)
			AUDIO_INITINFO (& soundRecorder->_info);
			soundRecorder->_info. record. sample_rate = (int) theControlPanel. sampleRate;
			ioctl (soundRecorder->_fd, AUDIO_SETINFO, & soundRecorder->_info);
		#elif defined (HPUX)
			close (soundRecorder->_fd);
			sleep (1);
			if (! soundRecorder->initialize ()) Melder_flushError (NULL);
		#elif defined (linux)		
			close (soundRecorder->_fd);
			if (! soundRecorder->initialize ()) Melder_flushError (NULL);
		#endif
	}
#ifdef _WIN32
end:
	iferror Melder_flushError ("Cannot change sampling frequency.");
#endif
}

void SoundRecorder::createChildren () {
	GuiObject form, channels, inputSources, meterBox, recstopplayBox, nameBox, fsampBox, dlgCtrlBox;
	
	#if gtk
		form = _dialog;
		GuiObject hbox1 = gtk_hbox_new(FALSE, 3);		// contains {Channels, Input source}, Meter, Sampling freq
		GuiObject hbox2 = gtk_hbox_new(TRUE, 3); 		// contains {slider, {Record, Stop}}, {Name, label}
		gtk_box_pack_start(GTK_BOX(form), hbox1, TRUE, TRUE, 3);
		gtk_box_pack_start(GTK_BOX(form), hbox2, FALSE, FALSE, 3);
	#elif motif
		/* TODO */
		form = XmCreateForm (_dialog, "form", NULL, 0);
		XtVaSetValues (form,
			XmNleftAttachment, XmATTACH_FORM, XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM, XmNtopOffset, Machine_getMenuBarHeight (),
			XmNbottomAttachment, XmATTACH_FORM,
			XmNtraversalOn, False,   /* Needed in order to redirect all keyboard input to the text widget. */
			NULL);
		
		meterBox = form;
	#endif
	
	#if gtk
		GuiObject h1vbox = gtk_vbox_new(FALSE, 3);
		gtk_box_pack_start(GTK_BOX(hbox1), h1vbox, FALSE, FALSE, 3);
		GuiObject channels_frame = gtk_frame_new("Channels");
		gtk_box_pack_start(GTK_BOX(h1vbox), channels_frame, FALSE, FALSE, 3);
		channels = gtk_vbox_new(TRUE, 3);
		gtk_container_add(GTK_CONTAINER(channels_frame), channels);
	#elif motif
		GuiLabel_createShown (form, 10, 160, 20, Gui_AUTOMATIC, L"Channels:", 0);
		channels = XmCreateRadioBox (form, "channels", NULL, 0);
		XtVaSetValues (channels,
			XmNleftAttachment, XmATTACH_FORM, XmNleftOffset, 10,
			XmNtopAttachment, XmATTACH_FORM, XmNtopOffset, 45,
			XmNwidth, 150,
			NULL);
		inputSources = form;
	#endif
	
	_monoButton = GuiRadioButton_createShown (channels, Gui_AUTOMATIC, Gui_AUTOMATIC, Gui_AUTOMATIC, Gui_AUTOMATIC,
		L"Mono", NULL, NULL, 0);
	_stereoButton = GuiRadioButton_createShown (channels, Gui_AUTOMATIC, Gui_AUTOMATIC, Gui_AUTOMATIC, Gui_AUTOMATIC,
		L"Stereo", NULL, NULL, 0);
	GuiObject_show (channels);
	
	#if gtk
		GuiRadioButton_setGroup(_stereoButton, GuiRadioButton_getGroup(_monoButton));
		
		GuiObject input_sources_frame = gtk_frame_new("Input source");
		gtk_box_pack_start(GTK_BOX(h1vbox), input_sources_frame, FALSE, FALSE, 3);
		inputSources = gtk_vbox_new(TRUE, 3);
		gtk_container_add(GTK_CONTAINER(input_sources_frame), inputSources);
	#endif
	
	long y = 110, dy = 25;
	#if defined (_WIN32)
		GuiLabel_createShown (inputSources, 10, 160, y, Gui_AUTOMATIC, L"(use Windows mixer", 0);
		GuiLabel_createShown (inputSources, 10, 160, y + dy, Gui_AUTOMATIC, L"   without meters)", 0);
	#else
		#if gtk
		GSList *input_radio_list = NULL;
		#else
		GuiLabel_createShown (inputSources, 10, 160, y, Gui_AUTOMATIC, L"Input source:", 0);
		#endif
		for (long i = 1; i <= SoundRecorder_IDEVICE_MAX; i ++) {
			if (_device [i]. canDo) {
				y += dy;
				_device [i]. button = GuiRadioButton_createShown (inputSources, 10, 160, y, Gui_AUTOMATIC,
					_device [i]. name, gui_radiobutton_cb_input, this, 0);
				#if gtk
				if (input_radio_list)
					GuiRadioButton_setGroup (_device[i].button, input_radio_list);
				input_radio_list = (GSList *) GuiRadioButton_getGroup (_device[i].button);
				#endif
			}
		}
	#endif
	
	#if gtk
		meterBox = gtk_vbox_new(FALSE, 3);
		gtk_box_pack_start(GTK_BOX(hbox1), meterBox, TRUE, TRUE, 3);
	#endif
	
	GuiLabel_createShown (meterBox, 170, -170, 20, Gui_AUTOMATIC, L"Meter", GuiLabel_CENTRE);
	_meter = GuiDrawingArea_createShown (meterBox, 170, -170, 45, -150,
		NULL, NULL, NULL, gui_drawingarea_cb_resize, this, GuiDrawingArea_BORDER);

	#if gtk
		gtk_widget_set_double_buffered(_meter, FALSE);
		
		GuiObject h1vbox2 = gtk_vbox_new(FALSE, 3);
		GuiObject fsampBox_frame = gtk_frame_new("Sampling frequency");
		fsampBox = gtk_vbox_new(TRUE, 3);
		GSList *fsamp_radio_list = NULL;
		
		gtk_box_pack_start(GTK_BOX(hbox1), h1vbox2, FALSE, FALSE, 3);
		gtk_box_pack_start(GTK_BOX(h1vbox2), fsampBox_frame, FALSE, FALSE, 3);
		gtk_container_add(GTK_CONTAINER(fsampBox_frame), fsampBox);
	#elif motif
		GuiLabel_createShown (form, -160, -10, 20, Gui_AUTOMATIC, L"Sampling frequency:", 0);
		fsampBox = XmCreateRadioBox (form, "fsamp", NULL, 0);
		XtVaSetValues (fsampBox,
			XmNrightAttachment, XmATTACH_FORM, XmNrightOffset, 10,
			XmNtopAttachment, XmATTACH_FORM, XmNtopOffset, 45,
			XmNwidth, 150,
			NULL);
	#endif
	for (long i = 1; i <= SoundRecorder_IFSAMP_MAX; i ++) {
		if (_fsamp [i]. canDo) {
			double fsamp = _fsamp [i]. fsamp;
			wchar_t title [40];
			swprintf (title, 40, L"%ls Hz", fsamp == floor (fsamp) ? Melder_integer ((long) fsamp) : Melder_fixed (fsamp, 5));
			_fsamp [i]. button = GuiRadioButton_createShown (fsampBox,
				Gui_AUTOMATIC, Gui_AUTOMATIC, Gui_AUTOMATIC, Gui_AUTOMATIC,
				title, gui_radiobutton_cb_fsamp, this, 0);
			#if gtk
			if (fsamp_radio_list)
				GuiRadioButton_setGroup (_fsamp[i].button, fsamp_radio_list);
			fsamp_radio_list = (GSList *) GuiRadioButton_getGroup (_fsamp[i].button);
			#endif
		}
	}
	GuiObject_show (fsampBox);
	
	#if gtk
		GuiObject h2vbox = gtk_vbox_new(FALSE, 3);
		gtk_box_pack_start(GTK_BOX(hbox2), h2vbox, TRUE, TRUE, 3);
		
		_progressScale = gtk_hscrollbar_new(NULL);
		gtk_range_set_range(GTK_RANGE(_progressScale), 0, 1000);
		
		GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(_progressScale));
		adj->page_size = 150;
		gtk_adjustment_changed(adj);
		gtk_box_pack_start(GTK_BOX(h2vbox), _progressScale, TRUE, TRUE, 3);
	#elif motif
		_progressScale = XmCreateScale (form, "scale", NULL, 0);
		XtVaSetValues (_progressScale, XmNorientation, XmHORIZONTAL,
			XmNminimum, 0, XmNmaximum, 1000,
			XmNleftAttachment, XmATTACH_FORM, XmNleftOffset, 10,
			XmNbottomAttachment, XmATTACH_FORM, XmNbottomOffset, 90,
			XmNwidth, 250,
			#ifdef macintosh
				XmNscaleWidth, 340,
			#endif
			NULL);
	#endif
	GuiObject_show (_progressScale);

	#if gtk
		recstopplayBox = gtk_hbutton_box_new();
		gtk_button_box_set_layout(GTK_BUTTON_BOX(recstopplayBox), GTK_BUTTONBOX_START);
		gtk_box_set_spacing(GTK_BOX(recstopplayBox), 3);
		gtk_container_add(GTK_CONTAINER(h2vbox), recstopplayBox);
	#else
		recstopplayBox = form;
	#endif
	y = 60;
	_recordButton = GuiButton_createShown (recstopplayBox, 20, 90, Gui_AUTOMATIC, -y,
		L"Record", gui_button_cb_record, this, 0);
	_stopButton = GuiButton_createShown (recstopplayBox, 100, 170, Gui_AUTOMATIC, -y,
		L"Stop", gui_button_cb_stop, this, 0);
	if (_inputUsesPortAudio) {
		_playButton = GuiButton_createShown (recstopplayBox, 180, 250, Gui_AUTOMATIC, -y,
			L"Play", gui_button_cb_play, this, 0);
	} else {
		#if defined (sgi) || defined (_WIN32) || defined (macintosh)
			_playButton = GuiButton_createShown (recstopplayBox, 180, 250, Gui_AUTOMATIC, -y,
				L"Play", gui_button_cb_play, this, 0);
		#endif
	}
	
	#if gtk
		nameBox = gtk_hbox_new(FALSE, 3);
		gtk_container_add(GTK_CONTAINER(hbox2), nameBox);
	#else
		nameBox = form;
	#endif
	
	GuiLabel_createShown (nameBox, -200, -130, Gui_AUTOMATIC, -y - 2, L"Name:", GuiLabel_RIGHT);
	_soundName = GuiText_createShown (nameBox, -120, -20, Gui_AUTOMATIC, -y, 0);
	#if motif	
	XtAddCallback (_soundName, XmNactivateCallback, gui_cb_apply, this);
	#endif
	GuiText_setString (_soundName, L"untitled");

	#if gtk
		dlgCtrlBox = gtk_hbutton_box_new();		// contains buttons
		gtk_button_box_set_layout(GTK_BUTTON_BOX(dlgCtrlBox), GTK_BUTTONBOX_END);
		gtk_box_set_spacing(GTK_BOX(dlgCtrlBox), 3);
		gtk_box_pack_end(GTK_BOX(form), dlgCtrlBox, FALSE, FALSE, 3);
	#else
		dlgCtrlBox = form;
	#endif
	
	y = 20;
	_cancelButton = GuiButton_createShown (dlgCtrlBox, -350, -280, Gui_AUTOMATIC, -y,
		L"Close", gui_button_cb_cancel, this, 0);
	_applyButton = GuiButton_createShown (dlgCtrlBox, -270, -170, Gui_AUTOMATIC, -y,
		L"Save to list", gui_button_cb_apply, this, 0);
	_okButton = GuiButton_createShown (dlgCtrlBox, -160, -20, Gui_AUTOMATIC, -y,
		L"Save to list & Close", gui_button_cb_ok, this, 0);

	#if gtk
		gtk_widget_show_all(form);
	#else
		GuiObject_show (form);
	#endif
}

void SoundRecorder::writeFakeMonoFile_e (MelderFile file, int audioFileType) {
	//file -> filePointer = NULL;
//start:
	long nsamp = _nsamp / 2;
	MelderFile_create (file, Melder_macAudioFileType (audioFileType), L"PpgB", Melder_winAudioFileExtension (audioFileType));
	if (file -> filePointer) {
		MelderFile_writeAudioFileHeader16_e (file, audioFileType, theControlPanel. sampleRate, nsamp, 1); cherror
		if (Melder_defaultAudioFileEncoding16 (audioFileType) == Melder_LINEAR_16_BIG_ENDIAN) {
			for (long i = 0; i < nsamp; i ++)
				binputi2 ((_buffer [i + i - 2] + _buffer [i + i - 1]) / 2, file -> filePointer);
		} else {
			for (long i = 0; i < nsamp; i ++)
				binputi2LE ((_buffer [i + i - 2] + _buffer [i + i - 1]) / 2, file -> filePointer);
		}
	}
end:
	MelderFile_close (file);
}

int SoundRecorder::writeAudioFile (MelderFile file, int audioFileType) {
//start:
	if (_fakeMono) {
		writeFakeMonoFile_e (file, audioFileType); cherror
	} else {
		MelderFile_writeAudioFile16 (file, audioFileType, _buffer, theControlPanel. sampleRate, _nsamp, _numberOfChannels);
	}
end:
	iferror return Melder_error1 (L"Audio file not written.");
	return 1;
}

static int menu_cb_writeWav (EDITOR_ARGS) {
	SoundRecorder *soundRecorder = (SoundRecorder *)editor_me;
	EDITOR_FORM_WRITE (L"Save as WAV file", 0)
		wchar_t *name = GuiText_getString (soundRecorder->_soundName);
		swprintf (defaultName, 300, L"%ls.wav", name);
		Melder_free (name);
	EDITOR_DO_WRITE
		if (! soundRecorder->writeAudioFile (file, Melder_WAV)) return 0;
	EDITOR_END
}

static int menu_cb_writeAifc (EDITOR_ARGS) {
	SoundRecorder *soundRecorder = (SoundRecorder *)editor_me;
	EDITOR_FORM_WRITE (L"Save as AIFC file", 0)
		wchar_t *name = GuiText_getString (soundRecorder->_soundName);
		swprintf (defaultName, 300, L"%ls.aifc", name);
		Melder_free (name);
	EDITOR_DO_WRITE
		if (! soundRecorder->writeAudioFile (file, Melder_AIFC)) return 0;
	EDITOR_END
}

static int menu_cb_writeNextSun (EDITOR_ARGS) {
	SoundRecorder *soundRecorder = (SoundRecorder *)editor_me;
	EDITOR_FORM_WRITE (L"Save as NeXT/Sun file", 0)
		wchar_t *name = GuiText_getString (soundRecorder->_soundName);
		swprintf (defaultName, 300, L"%ls.au", name);
		Melder_free (name);
	EDITOR_DO_WRITE
		if (! soundRecorder->writeAudioFile (file, Melder_NEXT_SUN)) return 0;
	EDITOR_END
}

static int menu_cb_writeNist (EDITOR_ARGS) {
	SoundRecorder *soundRecorder = (SoundRecorder *)editor_me;
	EDITOR_FORM_WRITE (L"Save as NIST file", 0)
		wchar_t *name = GuiText_getString (soundRecorder->_soundName);
		swprintf (defaultName, 300, L"%ls.nist", name);
		Melder_free (name);
	EDITOR_DO_WRITE
		if (! soundRecorder->writeAudioFile (file, Melder_NIST)) return 0;
	EDITOR_END
}

static int menu_cb_SoundRecorder_help (EDITOR_ARGS) { Melder_help (L"SoundRecorder"); return 1; }

void SoundRecorder::createMenus () {
	EditorMenu *menu = getMenu (L"File");
	menu->addCommand (L"Save as WAV file...", 0, menu_cb_writeWav);
	menu->addCommand (L"Save as AIFC file...", 0, menu_cb_writeAifc);
	menu->addCommand (L"Save as NeXT/Sun file...", 0, menu_cb_writeNextSun);
	menu->addCommand (L"Save as NIST file...", 0, menu_cb_writeNist);
	menu->addCommand (L"-- write --", 0, 0);

	menu = getMenu (L"Help");
	menu->addCommand (L"SoundRecorder help", '?', menu_cb_SoundRecorder_help);
}

/* End of file SoundRecorder.cpp */
