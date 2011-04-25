#ifndef _SoundRecorder_h_
#define _SoundRecorder_h_
/* SoundRecorder.h
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
 * pb 2011/03/23
 */

/* An editor-like object that allows the user to record sounds. */

#include "sys/Editor.h"
#include "Sound.h"
#include "sys/machine.h"
#include <portaudio.h>
#if defined (macintosh)
	#include "pa_mac_core.h"
#endif

#if defined (sgi)
	#include <audio.h>
#elif defined (_WIN32)
#elif defined (macintosh)
	#define PtoCstr(p)  (p [p [0] + 1] = '\0', (char *) p + 1)
#elif defined (sun)
	#include <fcntl.h>
	#include <stropts.h>
	#include <unistd.h>
	#if defined (sun4)
		#include <sun/audioio.h>
	#else
		#include <sys/audioio.h>
	#endif
#elif defined (HPUX)
	#include <fcntl.h>
	#include <ctype.h>
	#include <unistd.h>
	#include <sys/audio.h>
	#include <sys/ioctl.h>
	#include <sys/stat.h>
#elif defined (linux)
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/ioctl.h>
	#include <fcntl.h>
	#include <unistd.h>
	#if defined (__OpenBSD__) || defined (__NetBSD__)
		#include <soundcard.h>
	#else
		#include <sys/soundcard.h>
	#endif
#endif

struct SoundRecorder_Device {
	wchar_t name [1+40];
	bool canDo;
	GuiObject button;
};
#define SoundRecorder_IDEVICE_MAX  8

struct SoundRecorder_Fsamp {
	double fsamp;
	bool canDo;
	GuiObject button;
};
#define SoundRecorder_IFSAMP_8000  1
#define SoundRecorder_IFSAMP_9800  2
#define SoundRecorder_IFSAMP_11025  3
#define SoundRecorder_IFSAMP_12000  4
#define SoundRecorder_IFSAMP_16000  5
#define SoundRecorder_IFSAMP_22050  6
#define SoundRecorder_IFSAMP_22254  7
#define SoundRecorder_IFSAMP_24000  8
#define SoundRecorder_IFSAMP_32000  9
#define SoundRecorder_IFSAMP_44100  10
#define SoundRecorder_IFSAMP_48000  11
#define SoundRecorder_IFSAMP_64000  12
#define SoundRecorder_IFSAMP_96000  13
#define SoundRecorder_IFSAMP_192000  14
#define SoundRecorder_IFSAMP_MAX  14

class SoundRecorder : public Editor {
  public:
	static void prefs (void);
	static int getBufferSizePref_MB (void);
	static void setBufferSizePref_MB (int size);

	SoundRecorder (GuiObject parent, int numberOfChannels, void *applicationContext);
	~SoundRecorder ();

	wchar_t * type () { return L"SoundRecorder"; }
	bool isEditable () { return false; }
	bool isScriptable () { return false; }

	void createMenus ();
	void stopRecording ();
	void showMaximum (int channel, double maximum);
	void showMeter (short *buffer, long nsamp);
	int tooManySamplesInBufferToReturnToGui ();
	long getMyNsamp ();
	void publish ();
	int initialize ();
	void createChildren ();
	void writeFakeMonoFile_e (MelderFile file, int audioFileType);
	int writeAudioFile (MelderFile file, int audioFileType);

	int _numberOfChannels;
	long _nsamp, _nmax;
	bool _fakeMono, _synchronous, _recording;
	int _lastLeftMaximum, _lastRightMaximum;
	long _numberOfInputDevices;
	struct SoundRecorder_Device _device [1+SoundRecorder_IDEVICE_MAX];
	struct SoundRecorder_Fsamp _fsamp [1+SoundRecorder_IFSAMP_MAX];
	short *_buffer;
	GuiObject _monoButton, _stereoButton, _meter;
	GuiObject _progressScale, _recordButton, _stopButton, _playButton;
	GuiObject _soundName, _cancelButton, _applyButton, _okButton;
	Graphics _graphics;
	bool _inputUsesPortAudio;
	const PaDeviceInfo *_deviceInfos [1+SoundRecorder_IDEVICE_MAX];
	PaDeviceIndex _deviceIndices [1+SoundRecorder_IDEVICE_MAX];
	PaStream *_portaudioStream;
#if motif
	XtWorkProcId _workProcId;
#endif

#if defined (sgi)
	ALconfig _audio;
	ALport _port;
	long _info [10];
#elif defined (_WIN32)
	void win_fillFormat ();
	void win_fillHeader (int which);
	int win_waveInCheck ();
	int win_waveInOpen ();
	int win_waveInPrepareHeader (int which);
	int win_waveInAddBuffer (int which);
	int win_waveInStart ();
	int win_waveInStop ();
	int win_waveInReset ();
	int win_waveInUnprepareHeader (int which);
	int win_waveInClose ();

	HWAVEIN _hWaveIn;
	WAVEFORMATEX _waveFormat;
	WAVEHDR _waveHeader [3];
	MMRESULT _err;
	short _buffertje1 [1000*2], _buffertje2 [1000*2];
#elif defined (macintosh)
	static const char * errString (long err);
	static void onceError (const char *routine, long err);

	short _macSource [1+8];
	Str255 _hybridDeviceNames [1+8];
	SPB _spb;
	long _refNum;
#elif defined (sun)
	int _fd;
	struct audio_info _info;
#elif defined (HPUX)
	int _fd;
	struct audio_describe _info;
	int _hpInputSource;
	struct audio_gain _hpGains;
#elif defined (linux)
	int _fd;
#else
	int _fd;
#endif
};

/* End of file SoundRecorder.h */
#endif
