/* Sound_and_FilterBank.c
 *
 * Copyright (C) 1993-2007 David Weenink
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
 djmw 20010718
 djmw 20020813 GPL header.
 djmw 20041124 Changed call to Sound_to_Spectrum.
 djmw 20070103 Sound interface changes
 djmw 20071107 Errors/warnings text changes
 djmw 20071202 Melder_warning<n>
*/

#include "Sound_and_FilterBank.h"
#include "Sound_extensions.h"
#include "Sound_and_Spectrum.h"
#include "Sound_to_Pitch.h"
#include "Vector.h"
#include "NUM2.h"

#define MIN(m,n) ((m) < (n) ? (m) : (n))

/*
	The gaussian(x) = (exp(-48*((i-(n+1)/2)/(n+1))^2)-exp(-12))/(1-exp(-12));
	For power we need thearea under the square of this window:
		Integrate (gaussian(i)^2,i=1..n) =

	(sqrt(Pi)*sqrt(3)*sqrt(2)*erf(2*(n-1)*sqrt(3)*sqrt(2)/(n+1))*(n+1)+
		24*exp(-24)*(n-1)+
	-4*sqrt(Pi)*sqrt(3)*exp(-12)*erf(2*(n-1)*sqrt(3)/(n+1))*(n+1))/
	(24 * (-1+exp(-12))^2)
	To compare with the rectangular window we need to divide this by the
	window width (n-1) x 1^2.
*/
static double gaussian_window_squared_correction (long n)
{
	double e12 = exp (-12), denum = (e12 - 1) * (e12 - 1) * 24 * (n - 1);
	double sqrt3 = sqrt (3), sqrt2 = sqrt (2), sqrtpi = sqrt (NUMpi);
	double arg1 = 2 * sqrt3 * (n - 1) / (n + 1), arg2 = arg1 * sqrt2;
	double p2 = sqrtpi * sqrt3 * sqrt2 * (1 - NUMerfcc (arg2)) * (n + 1);
	double p1 = 4 * sqrtpi * sqrt3 * e12 * (1 - NUMerfcc (arg1)) * (n + 1);
	
	return (p2 - p1 + 24 * (n - 1) * e12 * e12) / denum;
}

static Matrix Sound_to_spectralpower (Sound me)
{
	Matrix thee = NULL;
	Spectrum s = Sound_to_Spectrum (me, TRUE);
	
	if (s != NULL)
	{
		thee = Matrix_create (s -> xmin, s -> xmax, s -> nx, s -> dx,
			s -> x1, 1, 1, 1, 1, 1);
		if (thee != NULL)
		{
			long i;
			double scale = 2.0 * s -> dx / (my xmax - my xmin);
			/*
				factor '2' because of positive and negative frequencies
				s -> dx : width of frequncy bin
				my xmax - my xmin : duration of sound
			*/
			double *z = thy z[1], *re = s -> z[1], *im = s -> z[2];
			for (i = 1; i <= s -> nx; i++)
			{
				z[i] = scale * (re[i] * re[i] + im[i] * im [i]);
			}
			/*
				Frequency bins at 0 Hz and nyquist don't count for two.
			*/
	
			z[1] *= 0.5;
			z[s -> nx] *= 0.5;
		} 
		forget (s);
	}
	return thee;
}

static int Sound_into_BarkFilter_frame (Sound me, BarkFilter thee, long frame)
{
	double *z = NULL;
	long i, j, nf;
	Matrix pv = Sound_to_spectralpower (me);
	
	if (pv == NULL) return 0;
	
	nf = pv -> nx;
	
	if ((z = NUMdvector (1, nf)) == NULL) goto end;
		
	for (j = 1; j <= nf; j++)
	{
		z[j] = HZTOBARK (pv -> x1 + (j - 1) * pv -> dx);
	}
	
	for (i = 1; i <= thy ny; i++)
	{
		double p = 0;
		double z0 = thy y1 + (i - 1) * thy dy;
		double *pow = pv -> z[1];		
		for (j = 1; j <= nf; j++)
		{
			/*
				Sekey & Hanson filter is defined in the power domain.
				We therefore multiply the power with a (and not a^2).
				integral (F(z),z=0..25) = 1.58/9
			*/
			
			double a = NUMsekeyhansonfilter_amplitude (z0, z[j]);
			p += a * pow[j] ;
		}
		thy z[i][frame] = p;
	}
		
end:

	NUMdvector_free (z, 1);
	forget (pv);
	return Melder_hasError() ? 0 : 1;	
}

BarkFilter Sound_to_BarkFilter (Sound me, double analysisWidth, double dt,
	double f1_bark, double fmax_bark, double df_bark)
{
	BarkFilter thee = NULL;
	Sound sframe = NULL, window = NULL;
	double t1, nyquist = 0.5 / my dx, samplingFrequency = 2 * nyquist;
	double windowDuration = 2 * analysisWidth; /* gaussian window */
	double zmax = NUMhertzToBark2 (nyquist);
	double ref, fmin_bark = 0;
	long nt, nf, i, frameErrorCount = 0;

	/*
		Check defaults.
	*/
	if (f1_bark <= 0) f1_bark = 1;
	if (fmax_bark <= 0) fmax_bark = zmax;
	if (df_bark <= 0) df_bark = 1;
	
	fmax_bark = MIN (fmax_bark, zmax);
	nf = floor ((fmax_bark - f1_bark) / df_bark + 0.5);
	if (nf <= 0) return Melder_errorp1 (L"The combination of filter parameters is not valid.");
		
	if (! Sampled_shortTermAnalysis (me, windowDuration, dt, & nt, & t1) ||
		((sframe = Sound_createSimple (1, windowDuration, samplingFrequency)) == NULL) ||
		((window = Sound_createGaussian (windowDuration, samplingFrequency)) == NULL) ||
		((thee = BarkFilter_create (my xmin, my xmax, nt, dt, t1, 
				fmin_bark, fmax_bark, nf, df_bark, f1_bark)) == NULL)) goto end;
		 
	Melder_progress1 (0.0, L"BarkFilter analysis");

	for (i=1; i <= nt; i++)
	{
		double t = Sampled_indexToX (thee,i);
		
		Sound_into_Sound (me, sframe, t - windowDuration/2);
		
		Sounds_multiply (sframe, window);
						
		if (! Sound_into_BarkFilter_frame (sframe, thee, i)) frameErrorCount++;
		
		if ((i % 10) == 1 && ! Melder_progress5 ((double)i / nt,  L"BarkFilter analysis: frame ",
			Melder_integer (i), L" from ", Melder_integer (nt), L".")) goto end;
	}
	
	if (frameErrorCount > 0)
	{
		Melder_warning5 (L"Analysis results of ", Melder_integer (frameErrorCount), L" frame(s) out of ", Melder_integer (nt),
			L" will be suspect.");
	}

	ref = FilterBank_DBREF * gaussian_window_squared_correction (window -> nx);
	
	NUMdmatrix_to_dBs (thy z, 1, thy ny, 1, thy nx, ref,
		FilterBank_DBFAC, FilterBank_DBFLOOR);

end:

	Melder_progress1 (1.0, NULL);
	forget (sframe); forget (window);
	if (Melder_hasError()) forget (thee);

	return thee;	
} 


static int Sound_into_MelFilter_frame (Sound me, MelFilter thee, long frame)
{
	double z1, dz, df = thy dy;
	long i, j, nf; 
	Matrix pv = Sound_to_spectralpower (me);
	
	if (pv == NULL) return 0;
	
	z1 = pv -> x1;
	dz = pv -> dx;
	nf = pv -> nx;
		
	for (i = 1; i <= thy ny; i++)
	{
		double p = 0;
		double fc_mel = thy y1 + (i - 1) * df;
		double fc_hz = MELTOHZ (fc_mel);
		double fl_hz = MELTOHZ (fc_mel - df);
		double fh_hz =  MELTOHZ (fc_mel + df);
		double *pow = pv -> z[1];	
		for (j = 1; j <= nf; j++)
		{
			/*
				Filter with a triangular filter the power (=amplitude-squared)
			*/
			
			double f = z1 + (j - 1) * dz;
			double a = NUMtriangularfilter_amplitude (fl_hz, fc_hz, fh_hz, f);
			p += a * pow[j] ;
		}
		thy z[i][frame] = p;
	}
		
	forget (pv);
	return Melder_hasError() ? 0 : 1;	
}

MelFilter Sound_to_MelFilter (Sound me, double analysisWidth, double dt,
	double f1_mel, double fmax_mel, double df_mel)
{
	Sound sframe = NULL, window = NULL;
	MelFilter thee = NULL;
	double t1, samplingFrequency = 1 / my dx, nyquist = 0.5 * samplingFrequency;
	double windowDuration = 2 * analysisWidth; /* gaussian window */
	double ref, fmin_mel = 0;
	double fbottom = HZTOMEL (100.0), fceiling = HZTOMEL (nyquist);
	long nf, nt, i, frameErrorCount = 0; 

	/*
		Check defaults.
		
	*/
	if (fmax_mel <= 0 || fmax_mel > fceiling) fmax_mel = fceiling;
	if (fmax_mel <= f1_mel)
	{
		f1_mel = fbottom; fmax_mel = fceiling;
	}
	if (f1_mel <= 0) f1_mel = fbottom;
	if (df_mel <= 0) df_mel = 100.0;
	
	
	/*
		Determine the number of filters.
	*/
	
	nf = floor ((fmax_mel - f1_mel) / df_mel + 0.5);
	fmax_mel = f1_mel + nf * df_mel;

	if (! Sampled_shortTermAnalysis (me, windowDuration, dt, &nt, &t1) ||
		((sframe = Sound_createSimple (1, windowDuration, samplingFrequency)) == NULL) ||
		((window = Sound_createGaussian (windowDuration, samplingFrequency)) == NULL) ||
		((thee = MelFilter_create (my xmin, my xmax, nt, dt, t1, fmin_mel, 
			fmax_mel, nf, df_mel, f1_mel)) == NULL)) goto end;

	Melder_progress1 (0.0, L"MelFilters analysis");

	for (i = 1; i <= nt; i++)
	{
		double t = Sampled_indexToX (thee, i);
		Sound_into_Sound (me, sframe, t - windowDuration / 2);
		Sounds_multiply (sframe, window);				
		if (! Sound_into_MelFilter_frame (sframe, thee, i)) frameErrorCount++;
		if ((i % 10) == 1 && ! Melder_progress5 ((double)i / nt, L"Frame ",
			Melder_integer (i), L" out of ", Melder_integer (nt), L".")) goto end;
	}
	
	if (frameErrorCount) Melder_warning5 (L"Analysis results of ", Melder_integer (frameErrorCount), L" frame(s) out of ", 
		Melder_integer (nt), L" will be suspect.");

	/*
		Window correction. 
	*/

	ref = FilterBank_DBREF * gaussian_window_squared_correction (window -> nx);
		
	NUMdmatrix_to_dBs (thy z, 1, thy ny, 1, thy nx, ref,
		FilterBank_DBFAC, FilterBank_DBFLOOR);

end:

	Melder_progress1 (1.1, NULL);
	forget (sframe); 
	forget (window); 
	if (Melder_hasError()) forget (thee);
	return thee;	
}

/*
	Analog formant filter response :
	H(f) = i f B / (f1^2 - f^2 + i f B) 
*/
static int Sound_into_FormantFilter_frame (Sound me, FormantFilter thee, 
	long frame, double bw)
{
	double z1, dz, f1 = thy y1, df = thy dy;
	long i, j, nf;
	Matrix pv;
	
	Melder_assert (bw > 0);
	
	pv = Sound_to_spectralpower (me);
	if (pv == NULL) return 0;
	
	z1 = pv -> x1;
	dz = pv -> dx;
	nf = pv -> nx;
	
	for (i = 1; i <= thy ny; i++)
	{
		double p = 0;
		double fc = f1 + (i - 1) * df;
		double *pow = pv -> z[1];		
		for (j = 1; j <= nf; j++)
		{
			/*
				H(f) = ifB / (fc^2 - f^2 + ifB)
				|H(f)| = fB / sqrt ((fc^2 - f^2)^2 + f^2B^2)
				|H(f)|^2 = f^2B^2 / ((fc^2 - f^2)^2 + f^2B^2)
				         = 1 / (((fc^2 - f^2) /fB)^2 + 1)
			*/
			
			double f = z1 + (j - 1) * dz;
			double a = NUMformantfilter_amplitude (fc, bw, f);
			 p += a * pow[j];
			/* p += a * pow[j];*/
		}		
		thy z[i][frame] = p;
	}

	forget (pv);
	return Melder_hasError() ? 0 : 1;	
}

FormantFilter Sound_to_FormantFilter (Sound me, double analysisWidth,
	double dt, double f1_hz, double fmax_hz, double df_hz, double relative_bw,
	double minimumPitch, double maximumPitch)
{
	Pitch thee = NULL;
	FormantFilter ff = NULL;
	double floor = 80;
	double ceiling = 600;
	
	if (minimumPitch >= maximumPitch)
	{
		minimumPitch = floor; maximumPitch = ceiling;
	}
	if (minimumPitch <= 0) minimumPitch = floor;
	if (maximumPitch <= 0) maximumPitch = ceiling;
	
	if ((thee = Sound_to_Pitch (me, dt, minimumPitch, maximumPitch)) == NULL)
		return NULL;
		
	ff = Sound_and_Pitch_to_FormantFilter (me, thee, analysisWidth, dt, 
		f1_hz, fmax_hz, df_hz, relative_bw);
	
	forget (thee);
	
	return ff;
}


FormantFilter Sound_and_Pitch_to_FormantFilter (Sound me, Pitch thee, 
	double analysisWidth, double dt, double f1_hz, double fmax_hz,
	double df_hz, double relative_bw)
{
	FormantFilter him = NULL;
	Sound sframe = NULL, window = NULL; 
	double t1, windowDuration = 2 * analysisWidth; /* gaussian window */
	double nyquist = 0.5 / my dx, samplingFrequency = 2 * nyquist;
	double f0_median, fmin_hz = 0, ref;
	long i, nt, nf, f0_undefined = 0;
	
	if (my xmin > thy xmin || my xmax > thy xmax) return Melder_errorp
		("The domain of the Sound is not included in the domain of the Pitch.");
	
	f0_median = Pitch_getQuantile (thee, thy xmin, thy xmax, 0.5, kPitch_unit_HERTZ);
	
	if (f0_median == NUMundefined || f0_median == 0)
	{
		f0_median = 100;
		Melder_warning1 (L"Pitch values undefined. Bandwith fixed to 100 Hz. ");
	}

	if (f1_hz <= 0) f1_hz = 100;
	if (fmax_hz <= 0) fmax_hz = nyquist;
	if (df_hz <= 0) df_hz = f0_median / 2;
	if (relative_bw <= 0) relative_bw = 1.1;
	
	fmax_hz = MIN (fmax_hz, nyquist);
	nf = floor ((fmax_hz - f1_hz) / df_hz + 0.5);
			
	if(! Sampled_shortTermAnalysis (me, windowDuration, dt, &nt, &t1) ||	
		(him = FormantFilter_create (my xmin, my xmax, nt, dt, t1, 
		fmin_hz, fmax_hz, nf, df_hz, f1_hz)) == NULL) return NULL;
		
	/*
		Temporary objects
	*/
	
	if (((sframe = Sound_createSimple (1, windowDuration, samplingFrequency)) == NULL) ||
		((window = Sound_createGaussian (windowDuration, samplingFrequency)) == NULL))
			goto end;

	for (i = 1; i <= nt; i++)
	{
		double t = Sampled_indexToX (him, i);
		double b, f0 = Pitch_getValueAtTime (thee, t, kPitch_unit_HERTZ, 0);
		
		if (f0 == NUMundefined || f0 == 0)
		{
			f0_undefined++; f0 = f0_median;
		}
		b = relative_bw * f0;
		Sound_into_Sound (me, sframe, t - windowDuration / 2);
		Sounds_multiply (sframe, window);
						
	 	if (! Sound_into_FormantFilter_frame (sframe, him, i, b)) goto end;
		
		if ((i % 10) == 1 && ! Melder_progress5 ((double)i / nt, L"Frame ",
			Melder_integer (i), L" out of ", Melder_integer (nt), L".")) goto end;
	}

	ref = FilterBank_DBREF * gaussian_window_squared_correction (window -> nx);
		
	NUMdmatrix_to_dBs (his z, 1, his ny, 1, his nx, ref, 
		FilterBank_DBFAC, FilterBank_DBFLOOR);

end:
	Melder_progress1 (1.0, NULL);
	forget (sframe); forget (window);
	if (Melder_hasError()) forget (him);
	return him;
}

/* End of file Sound_and_FilterBank.c */
