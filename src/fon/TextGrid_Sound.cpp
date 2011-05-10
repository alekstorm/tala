/* TextGrid_Sound.c
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
 * pb 2010/12/06 BDF/EDF files (for EEG)
 * pb 2010/12/08 split off from TextGrid.c and Sound.c
 */

#include "TextGrid_Sound.h"
#include "Pitch_to_PitchTier.h"

Collection TextGrid_Sound_extractAllIntervals (TextGrid me, Sound sound, long itier, int preserveTimes) {
	IntervalTier tier;
	long iseg;
	Collection collection;
	if (itier < 1 || itier > my tiers -> size)
		return (structCollection *)Melder_errorp ("Tier number %ld out of range 1..%ld.", itier, my tiers -> size);
	tier = (structIntervalTier *)my tiers -> item [itier];
	if (tier -> methods != classIntervalTier)
		return (structCollection *)Melder_errorp ("Tier %ld is not an interval tier.", itier);
	collection = Collection_create (NULL, tier -> intervals -> size); cherror
	for (iseg = 1; iseg <= tier -> intervals -> size; iseg ++) {
		TextInterval segment = (structTextInterval *)tier -> intervals -> item [iseg];
		Sound interval = Sound_extractPart (sound, segment -> xmin, segment -> xmax, kSound_windowShape_RECTANGULAR, 1.0, preserveTimes); cherror
		Thing_setName (interval, segment -> text ? segment -> text : L"untitled");
		Collection_addItem (collection, interval); cherror
	}
end:
	iferror forget (collection);
	return collection;
}

Collection TextGrid_Sound_extractNonemptyIntervals (TextGrid me, Sound sound, long itier, int preserveTimes) {
	IntervalTier tier;
	long iseg;
	Collection collection;
	if (itier < 1 || itier > my tiers -> size)
		return (structCollection *)Melder_errorp ("Tier number %ld out of range 1..%ld.", itier, my tiers -> size);
	tier = (structIntervalTier *)my tiers -> item [itier];
	if (tier -> methods != classIntervalTier)
		return (structCollection *)Melder_errorp ("Tier %ld is not an interval tier.", itier);
	collection = Collection_create (NULL, tier -> intervals -> size); cherror
	for (iseg = 1; iseg <= tier -> intervals -> size; iseg ++) {
		TextInterval segment = (structTextInterval *)tier -> intervals -> item [iseg];
		if (segment -> text != NULL && segment -> text [0] != '\0') {
			Sound interval = Sound_extractPart (sound, segment -> xmin, segment -> xmax, kSound_windowShape_RECTANGULAR, 1.0, preserveTimes); cherror
			Thing_setName (interval, segment -> text ? segment -> text : L"untitled");
			Collection_addItem (collection, interval); cherror
		}
	}
	if (collection -> size == 0) Melder_warning1 (L"No non-empty intervals were found.");
end:
	iferror forget (collection);
	return collection;
}

Collection TextGrid_Sound_extractIntervalsWhere (TextGrid me, Sound sound, long itier,
	int comparison_Melder_STRING, const wchar_t *text, int preserveTimes)
{
	IntervalTier tier;
	long count = 0;
	Collection collection;
	if (itier < 1 || itier > my tiers -> size)
		return (structCollection *)Melder_errorp ("Tier number %ld out of range 1..%ld.", itier, my tiers -> size);
	tier = (structIntervalTier *)my tiers -> item [itier];
	if (tier -> methods != classIntervalTier)
		return (structCollection *)Melder_errorp ("Tier %ld is not an interval tier.", itier);
	collection = Collection_create (NULL, 10);
	if (! collection) goto error;
	for (long iseg = 1; iseg <= tier -> intervals -> size; iseg ++) {
		TextInterval segment = (structTextInterval *)tier -> intervals -> item [iseg];
		if (Melder_stringMatchesCriterion (segment -> text, comparison_Melder_STRING, text)) {
			Sound interval = Sound_extractPart (sound, segment -> xmin, segment -> xmax,
				kSound_windowShape_RECTANGULAR, 1.0, preserveTimes);
			wchar_t name [1000];
			if (! interval) goto error;
			swprintf (name, 1000, L"%ls_%ls_%ld", sound -> name ? sound -> name : L"", text, ++ count);
			Thing_setName (interval, name);
			if (! Collection_addItem (collection, interval)) goto error;
		}
	}
	if (collection -> size == 0)
		Melder_warning5 (L"No label that ", kMelder_string_getText (comparison_Melder_STRING), L" the text \"", text, L"\" was found.");
	return collection;
error:
	forget (collection);
	return NULL;
}

int TextGrid_Sound_readFromBdfFile (MelderFile file, TextGrid *out_textGrid, Sound *out_sound) {
	*out_textGrid = NULL;
	*out_sound = NULL;
	Sound me = NULL;
	TextGrid thee = NULL;
	double *physicalMinimum = NULL, *physicalMaximum = NULL, *digitalMinimum = NULL, *digitalMaximum = NULL;
//start:
	FILE *f = Melder_fopen (file, "rb");
	char buffer [81];
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	fread (buffer, 1, 80, f); buffer [80] = '\0';
	//Melder_casual ("Local subject identification: \"%s\"", buffer);
	fread (buffer, 1, 80, f); buffer [80] = '\0';
	//Melder_casual ("Local recording identification: \"%s\"", buffer);
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	//Melder_casual ("Start date of recording: \"%s\"", buffer);
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	//Melder_casual ("Start time of recording: \"%s\"", buffer);
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	long numberOfBytesInHeaderRecord = atol (buffer);
	//Melder_casual ("Number of bytes in header record: %ld", numberOfBytesInHeaderRecord);
	fread (buffer, 1, 44, f); buffer [44] = '\0';
	//Melder_casual ("Version of data format: \"%s\"", buffer);
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	long numberOfDataRecords = strtol (buffer, NULL, 10);
	//Melder_casual ("Number of data records: %ld", numberOfDataRecords);
	fread (buffer, 1, 8, f); buffer [8] = '\0';
	double durationOfDataRecord = atof (buffer);
	//Melder_casual ("Duration of a data record: \"%f\"", durationOfDataRecord);
	fread (buffer, 1, 4, f); buffer [4] = '\0';
	long numberOfChannels = atol (buffer);
	//Melder_casual ("Number of channels in data record: %ld", numberOfChannels);
	if (numberOfBytesInHeaderRecord != (numberOfChannels + 1) * 256)
		error5 (L"(Read from BDF file:) Number of bytes in header record (", Melder_integer (numberOfBytesInHeaderRecord),
			L") doesn't match number of channels (", Melder_integer (numberOfChannels), L").")
	{
		for (long ichannel = 1; ichannel <= numberOfChannels; ichannel ++) {
			fread (buffer, 1, 16, f); buffer [16] = '\0';   // labels of the channels
		}
		double samplingFrequency = NUMundefined;
		for (long channel = 1; channel <= numberOfChannels; channel ++) {
			fread (buffer, 1, 80, f); buffer [80] = '\0';   // transducer type
		}
		for (long channel = 1; channel <= numberOfChannels; channel ++) {
			fread (buffer, 1, 8, f); buffer [8] = '\0';   // physical dimension of channels
			}
		physicalMinimum = NUMdvector (1, numberOfChannels); cherror
		for (long ichannel = 1; ichannel <= numberOfChannels; ichannel ++) {
			fread (buffer, 1, 8, f); buffer [8] = '\0';
			physicalMinimum [ichannel] = atof (buffer);
		}
		physicalMaximum = NUMdvector (1, numberOfChannels); cherror
		for (long ichannel = 1; ichannel <= numberOfChannels; ichannel ++) {
			fread (buffer, 1, 8, f); buffer [8] = '\0';
			physicalMaximum [ichannel] = atof (buffer);
		}
		digitalMinimum = NUMdvector (1, numberOfChannels); cherror
		for (long ichannel = 1; ichannel <= numberOfChannels; ichannel ++) {
			fread (buffer, 1, 8, f); buffer [8] = '\0';
			digitalMinimum [ichannel] = atof (buffer);
		}
		digitalMaximum = NUMdvector (1, numberOfChannels); cherror
		for (long ichannel = 1; ichannel <= numberOfChannels; ichannel ++) {
			fread (buffer, 1, 8, f); buffer [8] = '\0';
			digitalMaximum [ichannel] = atof (buffer);
		}
		for (long channel = 1; channel <= numberOfChannels; channel ++) {
			fread (buffer, 1, 80, f); buffer [80] = '\0';   // prefiltering
		}
		long numberOfSamplesPerDataRecord = 0;
		for (long channel = 1; channel <= numberOfChannels; channel ++) {
			fread (buffer, 1, 8, f); buffer [8] = '\0';   // number of samples in each data record
			long numberOfSamplesInThisDataRecord = atol (buffer);
			if (samplingFrequency == NUMundefined) {
				numberOfSamplesPerDataRecord = numberOfSamplesInThisDataRecord;
				samplingFrequency = numberOfSamplesInThisDataRecord / durationOfDataRecord;
			}
			if (numberOfSamplesInThisDataRecord / durationOfDataRecord != samplingFrequency)
				error7 (L"(Read from BDF file:) Number of samples per data record in channel ", Melder_integer (channel),
					L" (", Melder_integer (numberOfSamplesInThisDataRecord),
					L") doesn't match sampling frequency of channel 1 (", Melder_integer (numberOfChannels), L").")
		}
		for (long channel = 1; channel <= numberOfChannels; channel ++) {
			fread (buffer, 1, 32, f); buffer [32] = '\0';   // reserved
		}
		double duration = numberOfDataRecords * durationOfDataRecord;
		me = Sound_createSimple (numberOfChannels, duration, samplingFrequency); cherror
		for (long record = 1; record <= numberOfDataRecords; record ++) {
			for (long channel = 1; channel <= numberOfChannels; channel ++) {
				double factor = channel == numberOfChannels ? 1.0 : physicalMinimum [channel] / digitalMinimum [channel];
				for (long i = 1; i <= numberOfSamplesPerDataRecord; i ++) {
						long sample = i + (record - 1) * numberOfSamplesPerDataRecord;
					Melder_assert (sample <= my nx);
				my z [channel] [sample] = bingeti3LE (f) * factor;
				}
			}
		}
		thee = TextGrid_create (0, duration, L"S1 S2 S3 S4 S5 S6 S7 S8", L""); cherror
		for (int bit = 1; bit <= 8; bit ++) {
			unsigned long bitValue = 1 << (bit - 1);
			IntervalTier tier = (structIntervalTier *)thy tiers -> item [bit];
			for (long i = 1; i <= my nx; i ++) {
				unsigned long previousValue = i == 1 ? 0 : (long) my z [numberOfChannels] [i - 1];
				unsigned long thisValue = (long) my z [numberOfChannels] [i];
				if ((thisValue & bitValue) != (previousValue & bitValue)) {
					double time = i == 1 ? 0.0 : my x1 + (i - 1.5) * my dx;
					if (time != 0.0) TextGrid_insertBoundary (thee, bit, time);
					if ((thisValue & bitValue) != 0) {
						TextGrid_setIntervalText (thee, bit, tier -> intervals -> size, L"1");
					}
				}
			}
		}
	}
end:
	NUMdvector_free (physicalMinimum, 1);
	NUMdvector_free (physicalMaximum, 1);
	NUMdvector_free (digitalMinimum, 1);
	NUMdvector_free (digitalMaximum, 1);
	iferror return 0;
	*out_sound = me;
	*out_textGrid = thee;
	return 1;
}

/* End of file TextGrid_Sound.c */
