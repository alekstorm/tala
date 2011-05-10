/* praat_Artsynth.cpp
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
 * pb 2011/03/22
 */

#include "artsynth/Art_Speaker.h"
#include "artsynth/Art_Speaker_Delta.h"
#include "artsynth/Art_Speaker_to_VocalTract.h"
#include "artsynth/Artword_to_Art.h"
#include "artsynth/Delta.h"
#include "artsynth/Speaker_to_Delta.h"
#include "ui/editors/ArtwordEditor.h"
#include "fon/Sound.h"
#include "fon/VocalTract_to_Spectrum.h"

#include "ui/praat.h"

#define Dymin 0.00001
#define criticalVelocity 10.0

#define noiseFactor 0.1

#define MONITOR_SAMPLES 11

/* While debugging, some of these can be 1; otherwise, they are all 0: */
#define EQUAL_TUBE_WIDTHS  0
#define CONSTANT_TUBE_LENGTHS  1
#define NO_MOVING_WALLS  0
#define NO_TURBULENCE  0
#define NO_RADIATION_DAMPING  0
#define NO_BERNOULLI_EFFECT  0
#define MASS_LEAPFROG  0
#define B91 0

extern "C" int praat_Fon_formula (UiForm *dia, Interpreter *interpreter);
Graphics Movie_create (const wchar_t *title, int width, int height);

/***** ART *****/

FORM (Art_create, L"Create a default Articulation", L"Articulatory synthesis")
	WORD (L"Name", L"articulation")
	OK
DO
	if (! praat_new1 (Art_create (), GET_STRING (L"Name"))) return 0;
END

FORM (Art_edit, L"View & Edit Articulation", 0)
	for (int i = 1; i <= kArt_muscle_MAX; i ++)
		REAL (kArt_muscle_getText (i), L"0.0")
	OK
{
	Art object = (Art) ONLY_OBJECT;
	for (int i = 1; i <= kArt_muscle_MAX; i ++)
		SET_REAL (kArt_muscle_getText (i), object -> art [i]);
}
DO
	Art object = (Art) ONLY_OBJECT;
	if (theCurrentPraatApplication -> batch) return Melder_error1 (L"Cannot edit an Art from batch.");
	for (int i = 1; i <= kArt_muscle_MAX; i ++)
		object -> art [i] = GET_REAL (kArt_muscle_getText (i));
END

/***** ARTWORD *****/

void Artword_draw (Artword me, Graphics g, int feature, int garnish) {
	long numberOfTargets = my data [feature]. numberOfTargets;
	if (numberOfTargets > 0) {
		double *x = NULL, *y = NULL;
		x = NUMdvector (1, numberOfTargets); cherror
		y = NUMdvector (1, numberOfTargets); cherror
		Graphics_setInner (g);
		Graphics_setWindow (g, 0, my totalTime, -1, 1);
		for (int i = 1; i <= numberOfTargets; i ++) {
			x [i] = my data [feature]. times [i];
			y [i] = my data [feature]. targets [i];
		}
		Graphics_polyline (g, numberOfTargets, & x [1], & y [1]);         
		Graphics_unsetInner (g);
	end:
		NUMdvector_free (x, 1);
		NUMdvector_free (y, 1);
	}

	if (garnish) {
		Graphics_drawInnerBox (g);
		Graphics_marksBottom (g, 2, TRUE, TRUE, FALSE);
		Graphics_marksLeft (g, 3, TRUE, TRUE, TRUE);
		Graphics_textTop (g, FALSE, kArt_muscle_getText (feature));
		Graphics_textBottom (g, TRUE, L"Time (s)");
	}
}

FORM (Artword_create, L"Create an empty Artword", L"Create Artword...")
	WORD (L"Name", L"hallo")
	POSITIVE (L"Duration (seconds)", L"1.0")
	OK
DO
	if (! praat_new1 (Artword_create (GET_REAL (L"Duration")), GET_STRING (L"Name"))) return 0;
END

FORM (Artword_draw, L"Draw one Artword tier", NULL)
	OPTIONMENU (L"Muscle", kArt_muscle_LUNGS)
	for (int ienum = 1; ienum <= kArt_muscle_MAX; ienum ++)
		OPTION (kArt_muscle_getText (ienum))
	BOOLEAN (L"Garnish", 1)
	OK
DO
	EVERY_DRAW (Artword_draw ((Artword) OBJECT, GRAPHICS, GET_INTEGER (L"Muscle"), GET_INTEGER (L"Garnish")))
END

DIRECT (Artword_edit)
	if (theCurrentPraatApplication -> batch) Melder_throw ("Cannot view or edit an Artword from batch.");
	WHERE (SELECTED) {
		iam_LOOP (Artword);
		ArtwordEditor *editor = new ArtwordEditor (theCurrentPraatApplication -> topShell, ID_AND_FULL_NAME, me);
		praat_installEditor (editor, IOBJECT); therror
	}
END

FORM (Artword_getTarget, L"Get one Artword target", 0)
	REAL (L"Time (seconds)", L"0.0")
	OPTIONMENU (L"Muscle", kArt_muscle_LUNGS)
	for (int ienum = 1; ienum <= kArt_muscle_MAX; ienum ++)
		OPTION (kArt_muscle_getText (ienum))
	OK
DO
	iam_ONLY (Artword);
	double target = Artword_getTarget (me, GET_INTEGER (L"Muscle"), GET_REAL (L"Time"));
	Melder_information1 (Melder_double (target));
END

DIRECT (Artword_help)
	Melder_help (L"Artword");
END

FORM (Artword_setTarget, L"Set one Artword target", 0)
	REAL (L"Time (seconds)", L"0.0")
	REAL (L"Target value (0-1)", L"0.0")
	OPTIONMENU (L"Muscle", kArt_muscle_LUNGS)
	for (int ienum = 1; ienum <= kArt_muscle_MAX; ienum ++)
		OPTION (kArt_muscle_getText (ienum))
	OK
DO
	double time = GET_REAL (L"Time");
	if (time < 0.0) Melder_throw ("Specified time should not be less than 0.");
	WHERE (SELECTED) {
		iam_LOOP (Artword);
		Artword_setTarget (me, GET_INTEGER (L"Muscle"), time, GET_REAL (L"Target value")); therror
		praat_dataChanged (me);
	}
END

FORM (Artword_to_Art, L"From Artword to Art", 0)
	REAL (L"Time (seconds)", L"0.0")
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (Artword);
		autoArt thee = Artword_to_Art (me, GET_REAL (L"Time"));
		praat_new (thee.transfer(), my name);
	}
END

/***** ART & SPEAKER *****/

void Art_Speaker_draw (Art art, Speaker speaker, Graphics g) {
	double f = speaker -> relativeSize * 1e-3;
	double intX [1 + 16], intY [1 + 16], extX [1 + 11], extY [1 + 11];
	double bodyX, bodyY;
	int i;
	Graphics_Viewport previous;

	Art_Speaker_toVocalTract (art, speaker, intX, intY, extX, extY, & bodyX, & bodyY);
	previous = Graphics_insetViewport (g, 0.1, 0.9, 0.1, 0.9);
	Graphics_setWindow (g, -0.05, 0.05, -0.05, 0.05);

	/* Draw inner contour. */

	for (i = 1; i <= 5; i ++)
		Graphics_line (g, intX [i], intY [i], intX [i + 1], intY [i + 1]);
	Graphics_arc (g, bodyX, bodyY, 20 * f,
		atan2 (intY [7] - bodyY, intX [7] - bodyX) * 180 / NUMpi,
		atan2 (intY [6] - bodyY, intX [6] - bodyX) * 180 / NUMpi);
	for (i = 7; i <= 15; i ++)
		Graphics_line (g, intX [i], intY [i], intX [i + 1], intY [i + 1]);

	/* Draw outer contour. */

	for (i = 1; i <= 5; i ++)
		Graphics_line (g, extX [i], extY [i], extX [i + 1], extY [i + 1]);
	Graphics_arc (g, 0, 0, speaker -> palate.radius,
		speaker -> alveoli.a * 180 / NUMpi,
		speaker -> velum.a * 180 / NUMpi);
	for (i = 7; i <= 10; i ++)
		Graphics_line (g, extX [i], extY [i], extX [i + 1], extY [i + 1]);
	Graphics_resetViewport (g, previous);
}

void Art_Speaker_fillInnerContour (Art art, Speaker speaker, Graphics g) {
	double f = speaker -> relativeSize * 1e-3;
	double intX [1 + 16], intY [1 + 16], extX [1 + 11], extY [1 + 11];
	double x [1 + 16], y [1 + 16];
	double bodyX, bodyY;
	int i;
	Graphics_Viewport previous;

	Art_Speaker_toVocalTract (art, speaker, intX, intY, extX, extY, & bodyX, & bodyY);
	previous = Graphics_insetViewport (g, 0.1, 0.9, 0.1, 0.9);
	Graphics_setWindow (g, -0.05, 0.05, -0.05, 0.05);
	for (i = 1; i <= 16; i ++) { x [i] = intX [i]; y [i] = intY [i]; }
	Graphics_setGrey (g, 0.8);
	Graphics_fillArea (g, 16, & x [1], & y [1]);
	Graphics_fillCircle (g, bodyX, bodyY, 20 * f);
	Graphics_setGrey (g, 0.0);
	Graphics_resetViewport (g, previous);
}

void Art_Speaker_drawMesh (Art art, Speaker speaker, Graphics graphics) {
	double xi [40], yi [40], xe [40], ye [40], xmm [40], ymm [40];
	int closed [40];
	int i;
	Graphics_Viewport previous;
	int oldLineType = Graphics_inqLineType (graphics);
	Art_Speaker_meshVocalTract (art, speaker, xi, yi, xe, ye, xmm, ymm, closed);
	previous = Graphics_insetViewport (graphics, 0.1, 0.9, 0.1, 0.9);   /* Must be square. */
	Graphics_setWindow (graphics, -0.05, 0.05, -0.05, 0.05);

	/* Mesh lines. */
	for (i = 1; i <= Art_Speaker_meshCount; i ++)
		Graphics_line (graphics, xi [i], yi [i], xe [i], ye [i]);

	/* Radii. */
	Graphics_setLineType (graphics, Graphics_DOTTED);
	for (i = 1; i <= Art_Speaker_meshCount; i ++)
		if (xe [i] <= 0.0 && ye [i] >= 0.0)
			Graphics_line (graphics, 0.0, 0.0, 0.9 * xi [i], 0.9 * yi [i]);
	Graphics_setLineType (graphics, oldLineType);

	/* Lengths. */
	for (i = 1; i <= Art_Speaker_meshCount; i ++)
		Graphics_line (graphics, xmm [i], ymm [i], xmm [i + 1], ymm [i + 1]);

	for (i = 1; i <= Art_Speaker_meshCount + 1; i ++)
		Graphics_fillCircle_mm (graphics, xmm [i], ymm [i], 1.0);
	Graphics_setTextAlignment (graphics, Graphics_LEFT, Graphics_HALF);
	Graphics_text (graphics, 0.0, 0.0, L"O");   /* Origin. */
	Graphics_resetViewport (graphics, previous);
}

DIRECT (Art_Speaker_draw)
	autoPraatPicture picture;
	iam_ONLY (Art);
	thouart_ONLY (Speaker);
	Art_Speaker_draw (me, thee, GRAPHICS);
END

DIRECT (Art_Speaker_fillInnerContour)
	autoPraatPicture picture;
	iam_ONLY (Art);
	thouart_ONLY (Speaker);
	Art_Speaker_fillInnerContour (me, thee, GRAPHICS);
END

DIRECT (Art_Speaker_drawMesh)
	autoPraatPicture picture;
	iam_ONLY (Art);
	thouart_ONLY (Speaker);
	Art_Speaker_drawMesh (me, thee, GRAPHICS);
END

DIRECT (Art_Speaker_to_VocalTract)
	iam_ONLY (Art);
	thouart_ONLY (Speaker);
	autoVocalTract him = Art_Speaker_to_VocalTract (me, thee);
	praat_new (him.transfer(), my name, L"_", thy name);
END

/***** ARTWORD & SPEAKER *****/

Sound Artword_Speaker_to_Sound (Artword artword, Speaker speaker,
	double fsamp, int oversampling,
	Sound *w1, int iw1, Sound *w2, int iw2, Sound *w3, int iw3,
	Sound *p1, int ip1, Sound *p2, int ip2, Sound *p3, int ip3,
	Sound *v1, int iv1, Sound *v2, int iv2, Sound *v3, int iv3)
{
	Delta delta;
	Sound result = Sound_createSimple (1, artword -> totalTime, fsamp);
	long numberOfSamples = result -> nx;
	double minTract [1+78], maxTract [1+78];   /* For drawing. */
	double Dt = 1 / fsamp / oversampling,
		rho0 = 1.14,
		c = 353,
		onebyc2 = 1.0 / (c * c),
		rho0c2 = rho0 * c * c,
		halfDt = 0.5 * Dt,
		twoDt = 2 * Dt,
		halfc2Dt = 0.5 * c * c * Dt,
		twoc2Dt = 2 * c * c * Dt,
		onebytworho0 = 1.0 / (2.0 * rho0),
		Dtbytworho0 = Dt / (2.0 * rho0);
	double tension, rrad, onebygrad, totalVolume;
	Art art = Art_create ();
	long sample;
	int n, m, M;
	Graphics graphics;
	delta = Speaker_to_Delta (speaker);
	if (! delta) { forget (art); return NULL; }
	graphics = (Graphics) Melder_monitor1 (0.0, L"Articulatory synthesis");
	Artword_intoArt (artword, art, 0.0);
	Art_Speaker_intoDelta (art, speaker, delta);
	M = delta -> numberOfTubes;
	if (iw1 > 0 && iw1 <= M) *w1 = Sound_createSimple (1, artword -> totalTime, fsamp); else iw1 = 0;
	if (iw2 > 0 && iw2 <= M) *w2 = Sound_createSimple (1, artword -> totalTime, fsamp); else iw2 = 0;
	if (iw3 > 0 && iw3 <= M) *w3 = Sound_createSimple (1, artword -> totalTime, fsamp); else iw3 = 0;
	if (ip1 > 0 && ip1 <= M) *p1 = Sound_createSimple (1, artword -> totalTime, fsamp); else ip1 = 0;
	if (ip2 > 0 && ip2 <= M) *p2 = Sound_createSimple (1, artword -> totalTime, fsamp); else ip2 = 0;
	if (ip3 > 0 && ip3 <= M) *p3 = Sound_createSimple (1, artword -> totalTime, fsamp); else ip3 = 0;
	if (iv1 > 0 && iv1 <= M) *v1 = Sound_createSimple (1, artword -> totalTime, fsamp); else iv1 = 0;
	if (iv2 > 0 && iv2 <= M) *v2 = Sound_createSimple (1, artword -> totalTime, fsamp); else iv2 = 0;
	if (iv3 > 0 && iv3 <= M) *v3 = Sound_createSimple (1, artword -> totalTime, fsamp); else iv3 = 0;
	/* Initialize drawing. */
	{ int i; for (i = 1; i <= 78; i ++) { minTract [i] = 100; maxTract [i] = -100; } }
	totalVolume = 0.0;
	for (m = 1; m <= M; m ++) {
		Delta_Tube t = delta->tube + m;
		if (! t -> left1 && ! t -> right1) continue;
		t->Dx = t->Dxeq; t->dDxdt = 0;   /* 5.113 */
		t->Dy = t->Dyeq; t->dDydt = 0;   /* 5.113 */
		t->Dz = t->Dzeq;   /* 5.113 */
		t->A = t->Dz * ( t->Dy >= t->dy ? t->Dy + Dymin :
			t->Dy <= - t->dy ? Dymin :
			(t->dy + t->Dy) * (t->dy + t->Dy) / (4 * t->dy) + Dymin );   /* 4.4, 4.5 */
		#if EQUAL_TUBE_WIDTHS
			t->A = 0.0001;
		#endif
		t->Jleft = t->Jright = 0;   /* 5.113 */
		t->Qleft = t->Qright = rho0c2;   /* 5.113 */
		t->pleft = t->pright = 0;   /* 5.114 */
		t->Kleft = t->Kright = 0;   /* 5.114 */
		t->V = t->A * t->Dx;   /* 5.114 */
		totalVolume += t->V;
	}
	//Melder_casual ("Starting volume: %.10g litres.", totalVolume * 1000);
	for (sample = 1; sample <= numberOfSamples; sample ++) {
		double time = (sample - 1) / fsamp;
		Artword_intoArt (artword, art, time);
		Art_Speaker_intoDelta (art, speaker, delta);
		if (sample % MONITOR_SAMPLES == 0) {
if (graphics) {   /* Because we can be in batch. */
	double area [1+78];
	Graphics_Viewport vp;
	for (int i = 1; i <= 78; i ++) {
		area [i] = delta -> tube [i]. A;
		if (area [i] < minTract [i]) minTract [i] = area [i];
		if (area [i] > maxTract [i]) maxTract [i] = area [i];
	}
	Graphics_clearWs (graphics);

	vp = Graphics_insetViewport (graphics, 0, 0.5, 0.5, 1);
	Graphics_setWindow (graphics, 0, 1, 0, 0.05);
	Graphics_setColour (graphics, Graphics_RED);
	Graphics_function (graphics, minTract, 1, 35, 0, 0.9);
	Graphics_function (graphics, maxTract, 1, 35, 0, 0.9);
	Graphics_setColour (graphics, Graphics_BLACK);
	Graphics_function (graphics, area, 1, 35, 0, 0.9);
	Graphics_setLineType (graphics, Graphics_DOTTED);
	Graphics_line (graphics, 0, 0, 1, 0);
	Graphics_setLineType (graphics, Graphics_DRAWN);
	Graphics_resetViewport (graphics, vp);

	vp = Graphics_insetViewport (graphics, 0, 0.5, 0, 0.5);
	Graphics_setWindow (graphics, 0, 1, -0.000003, 0.00001);
	Graphics_setColour (graphics, Graphics_RED);
	Graphics_function (graphics, minTract, 36, 37, 0.2, 0.8);
	Graphics_function (graphics, maxTract, 36, 37, 0.2, 0.8);
	Graphics_setColour (graphics, Graphics_BLACK);
	Graphics_function (graphics, area, 36, 37, 0.2, 0.8);
	Graphics_setLineType (graphics, Graphics_DOTTED);
	Graphics_line (graphics, 0, 0, 1, 0);
	Graphics_setLineType (graphics, Graphics_DRAWN);
	Graphics_resetViewport (graphics, vp);

	vp = Graphics_insetViewport (graphics, 0.5, 1, 0.5, 1);
	Graphics_setWindow (graphics, 0, 1, 0, 0.001);
	Graphics_setColour (graphics, Graphics_RED);
	Graphics_function (graphics, minTract, 38, 64, 0, 1);
	Graphics_function (graphics, maxTract, 38, 64, 0, 1);
	Graphics_setColour (graphics, Graphics_BLACK);
	Graphics_function (graphics, area, 38, 64, 0, 1);
	Graphics_setLineType (graphics, Graphics_DOTTED);
	Graphics_line (graphics, 0, 0, 1, 0);
	Graphics_setLineType (graphics, Graphics_DRAWN);
	Graphics_resetViewport (graphics, vp);

	vp = Graphics_insetViewport (graphics, 0.5, 1, 0, 0.5);
	Graphics_setWindow (graphics, 0, 1, 0.001, 0);
	Graphics_setColour (graphics, Graphics_RED);
	Graphics_function (graphics, minTract, 65, 78, 0.5, 1);
	Graphics_function (graphics, maxTract, 65, 78, 0.5, 1);
	Graphics_setColour (graphics, Graphics_BLACK);
	Graphics_function (graphics, area, 65, 78, 0.5, 1);
	Graphics_setLineType (graphics, Graphics_DRAWN);
	Graphics_resetViewport (graphics, vp);
}
			if (! Melder_monitor3 ((double) sample / numberOfSamples, L"Articulatory synthesis: ", Melder_half (time), L" seconds")) {
				forget (result);
				if (iw1) forget (*w1); if (iw2) forget (*w2); if (iw3) forget (*w3);
				if (ip1) forget (*p1); if (ip2) forget (*p2); if (ip3) forget (*p3);
				if (iv1) forget (*v1); if (iv2) forget (*v2); if (iv3) forget (*v3);
				goto end;
			}
		}
		for (n = 1; n <= oversampling; n ++) {
			for (m = 1; m <= M; m ++) {
				Delta_Tube t = delta -> tube + m;
				if (! t -> left1 && ! t -> right1) continue;

				/* New geometry. */

				#if CONSTANT_TUBE_LENGTHS
					t->Dxnew = t->Dx;
				#else
					t->dDxdtnew = (t->dDxdt + Dt * 10000 * (t->Dxeq - t->Dx)) /
						(1 + 200 * Dt);   /* Critical damping, 10 ms. */
					t->Dxnew = t->Dx + t->dDxdtnew * Dt;
				#endif
				/* 3-way: equal lengths. */
				/* This requires left tubes to be processed before right tubes. */
				if (t->left1 && t->left1->right2) t->Dxnew = t->left1->Dxnew;
				t->Dz = t->Dzeq;   /* immediate... */
				t->eleft = (t->Qleft - t->Kleft) * t->V;   /* 5.115 */
				t->eright = (t->Qright - t->Kright) * t->V;   /* 5.115 */
				t->e = 0.5 * (t->eleft + t->eright);   /* 5.116 */
				t->p = 0.5 * (t->pleft + t->pright);   /* 5.116 */
				t->DeltaP = t->e / t->V - rho0c2;   /* 5.117 */
				t->v = t->p / (rho0 + onebyc2 * t->DeltaP);   /* 5.118 */
				{
					double dDy = t->Dyeq - t->Dy;
					double cubic = t->k3 * dDy * dDy;
					Delta_Tube l1 = t->left1, l2 = t->left2, r1 = t->right1, r2 = t->right2;
					tension = dDy * (t->k1 + cubic);
					t->B = 2 * t->Brel * sqrt (t->mass * (t->k1 + 3 * cubic));
					if (t->k1left1 && l1)
						tension += t->k1left1 * t->k1 * (dDy - (l1->Dyeq - l1->Dy));
					if (t->k1left2 && l2)
						tension += t->k1left2 * t->k1 * (dDy - (l2->Dyeq - l2->Dy));
					if (t->k1right1 && r1)
						tension += t->k1right1 * t->k1 * (dDy - (r1->Dyeq - r1->Dy));
					if (t->k1right2 && r2)
						tension += t->k1right2 * t->k1 * (dDy - (r2->Dyeq - r2->Dy));
				}
				if (t->Dy < t->dy) {
					if (t->Dy >= - t->dy) {
						double dDy = t->dy - t->Dy, dDy2 = dDy * dDy;
						tension += dDy2 / (4 * t->dy) * (t->s1 + 0.5 * t->s3 * dDy2);
						t->B += 2 * dDy / (2 * t->dy) *
							sqrt (t->mass * (t->s1 + t->s3 * dDy2));
					} else {
						tension -= t->Dy * (t->s1 + t->s3 * (t->Dy * t->Dy + t->dy * t->dy));
						t->B += 2 * sqrt (t->mass * (t->s1 + t->s3 * (3 * t->Dy * t->Dy + t->dy * t->dy)));
					}
				}
				t->dDydtnew = (t->dDydt + Dt / t->mass * (tension + 2 * t->DeltaP * t->Dz * t->Dx)) /
					(1 + t->B * Dt / t->mass);   /* 5.119 */
				t->Dynew = t->Dy + t->dDydtnew * Dt;   /* 5.119 */
				#if NO_MOVING_WALLS
					t->Dynew = t->Dy;
				#endif
				t->Anew = t->Dz * ( t->Dynew >= t->dy ? t->Dynew + Dymin :
					t->Dynew <= - t->dy ? Dymin :
					(t->dy + t->Dynew) * (t->dy + t->Dynew) / (4 * t->dy) + Dymin );   /* 4.4, 4.5 */
				#if EQUAL_TUBE_WIDTHS
					t->Anew = 0.0001;
				#endif
				t->Ahalf = 0.5 * (t->A + t->Anew);   /* 5.120 */
				t->Dxhalf = 0.5 * (t->Dxnew + t->Dx);   /* 5.121 */
				t->Vnew = t->Anew * t->Dxnew;   /* 5.128 */
				{ double oneByDyav = t->Dz / t->A;
				/*t->R = 12 * 1.86e-5 * t->parallel * t->parallel * oneByDyav * oneByDyav;*/
				if (t->Dy < 0)
					t->R = 12 * 1.86e-5 / (Dymin * Dymin + t->dy * t->dy);
				else
					t->R = 12 * 1.86e-5 * t->parallel * t->parallel /
						((t->Dy + Dymin) * (t->Dy + Dymin) + t->dy * t->dy);
				t->R += 0.3 * t->parallel * oneByDyav;   /* 5.23 */ }
				t->r = (1 + t->R * Dt / rho0) * t->Dxhalf / t->Anew;   /* 5.122 */
				t->ehalf = t->e + halfc2Dt * (t->Jleft - t->Jright);   /* 5.123 */
				t->phalf = (t->p + halfDt * (t->Qleft - t->Qright) / t->Dx) / (1 + Dtbytworho0 * t->R);   /* 5.123 */
				#if MASS_LEAPFROG
					t->ehalf = t->ehalfold + 2 * halfc2Dt * (t->Jleft - t->Jright);
				#endif
				t->Jhalf = t->phalf * t->Ahalf;   /* 5.124 */
				t->Qhalf = t->ehalf / (t->Ahalf * t->Dxhalf) + onebytworho0 * t->phalf * t->phalf;   /* 5.124 */
				#if NO_BERNOULLI_EFFECT
					t->Qhalf = t->ehalf / (t->Ahalf * t->Dxhalf);
				#endif
			}
			for (m = 1; m <= M; m ++) {   /* Compute Jleftnew and Qleftnew. */
				Delta_Tube l = delta->tube + m, r1 = l -> right1, r2 = l -> right2, r = r1;
				Delta_Tube l1 = l, l2 = r ? r -> left2 : NULL;
				if (l->left1 == NULL) {   /* Closed boundary at the left side (diaphragm)? */
					if (r == NULL) continue;   /* Tube not connected at all. */
					l->Jleftnew = 0;   /* 5.132. */
					l->Qleftnew = (l->eleft - twoc2Dt * l->Jhalf) / l->Vnew;   /* 5.132. */
				}
				else   /* Left boundary open to another tube will be handled... */
					(void) 0;   /* ...together with the right boundary of the tube to the left. */
				if (r == NULL) {   /* Open boundary at the right side (lips, nostrils)? */
					rrad = 1 - c * Dt / 0.02;   /* Radiation resistance, 5.135. */
					onebygrad = 1 / (1 + c * Dt / 0.02);   /* Radiation conductance, 5.135. */
					#if NO_RADIATION_DAMPING
						rrad = 0;
						onebygrad = 0;
					#endif
					l->prightnew = ((l->Dxhalf / Dt + c * onebygrad) * l->pright +
						 2 * ((l->Qhalf - rho0c2) - (l->Qright - rho0c2) * onebygrad)) /
						(l->r * l->Anew / Dt + c * onebygrad);   /* 5.136 */
					l->Jrightnew = l->prightnew * l->Anew;   /* 5.136 */
					l->Qrightnew = (rrad * (l->Qright - rho0c2) +
						c * (l->prightnew - l->pright)) * onebygrad + rho0c2;   /* 5.136 */
				} else if (l2 == NULL && r2 == NULL) {   /* Two-way boundary. */
					if (l->v > criticalVelocity && l->A < r->A) {
						l->Pturbrightnew = -0.5 * rho0 * (l->v - criticalVelocity) *
							(1 - l->A / r->A) * (1 - l->A / r->A) * l->v;
						if (l->Pturbrightnew != 0.0)
							l->Pturbrightnew *= 1 + NUMrandomGauss (0, noiseFactor) /* * l->A */;
					}
					if (r->v < - criticalVelocity && r->A < l->A) {
						l->Pturbrightnew = 0.5 * rho0 * (r->v + criticalVelocity) *
							(1 - r->A / l->A) * (1 - r->A / l->A) * r->v;
						if (l->Pturbrightnew != 0.0)
							l->Pturbrightnew *= 1 + NUMrandomGauss (0, noiseFactor) /* * r->A */;
					}
					#if NO_TURBULENCE
						l->Pturbrightnew = 0;
					#endif
					l->Jrightnew = r->Jleftnew =
						(l->Dxhalf * l->pright + r->Dxhalf * r->pleft +
						 twoDt * (l->Qhalf - r->Qhalf + l->Pturbright)) /
						(l->r + r->r);   /* 5.127 */
					#if B91
						l->Jrightnew = r->Jleftnew =
							(l->pright + r->pleft +
							 2 * twoDt * (l->Qhalf - r->Qhalf + l->Pturbright) / (l->Dxhalf + r->Dxhalf)) /
							(l->r / l->Dxhalf + r->r / r->Dxhalf);
					#endif
					l->prightnew = l->Jrightnew / l->Anew;   /* 5.128 */
					r->pleftnew = r->Jleftnew / r->Anew;   /* 5.128 */
					l->Krightnew = onebytworho0 * l->prightnew * l->prightnew;   /* 5.128 */
					r->Kleftnew = onebytworho0 * r->pleftnew * r->pleftnew;   /* 5.128 */
					#if NO_BERNOULLI_EFFECT
						l->Krightnew = r->Kleftnew = 0;
					#endif
					l->Qrightnew =
						(l->eright + r->eleft + twoc2Dt * (l->Jhalf - r->Jhalf)
						 + l->Krightnew * l->Vnew + (r->Kleftnew - l->Pturbrightnew) * r->Vnew) /
						(l->Vnew + r->Vnew);   /* 5.131 */
					r->Qleftnew = l->Qrightnew + l->Pturbrightnew;   /* 5.131 */
				} else if (r2) {   /* Two adjacent tubes at the right side (velic). */
					r1->Jleftnew =
						(r1->Jleft * r1->Dxhalf * (1 / (l->A + r2->A) + 1 / r1->A) +
						 twoDt * ((l->Ahalf * l->Qhalf + r2->Ahalf * r2->Qhalf ) / (l->Ahalf  + r2->Ahalf) - r1->Qhalf)) /
						(1 / (1 / l->r + 1 / r2->r) + r1->r);   /* 5.138 */
					r2->Jleftnew =
						(r2->Jleft * r2->Dxhalf * (1 / (l->A + r1->A) + 1 / r2->A) +
						 twoDt * ((l->Ahalf * l->Qhalf + r1->Ahalf * r1->Qhalf ) / (l->Ahalf  + r1->Ahalf) - r2->Qhalf)) /
						(1 / (1 / l->r + 1 / r1->r) + r2->r);   /* 5.138 */
					l->Jrightnew = r1->Jleftnew + r2->Jleftnew;   /* 5.139 */
					l->prightnew = l->Jrightnew / l->Anew;   /* 5.128 */
					r1->pleftnew = r1->Jleftnew / r1->Anew;   /* 5.128 */
					r2->pleftnew = r2->Jleftnew / r2->Anew;   /* 5.128 */
					l->Krightnew = onebytworho0 * l->prightnew * l->prightnew;   /* 5.128 */
					r1->Kleftnew = onebytworho0 * r1->pleftnew * r1->pleftnew;   /* 5.128 */
					r2->Kleftnew = onebytworho0 * r2->pleftnew * r2->pleftnew;   /* 5.128 */
					#if NO_BERNOULLI_EFFECT
						l->Krightnew = r1->Kleftnew = r2->Kleftnew = 0;
					#endif
					l->Qrightnew = r1->Qleftnew = r2->Qleftnew =
						(l->eright + r1->eleft + r2->eleft + twoc2Dt * (l->Jhalf - r1->Jhalf - r2->Jhalf) +
						 l->Krightnew * l->Vnew + r1->Kleftnew * r1->Vnew + r2->Kleftnew * r2->Vnew) /
						(l->Vnew + r1->Vnew + r2->Vnew);   /* 5.137 */
				} else {
					Melder_assert (l2 != NULL);
					l1->Jrightnew =
						(l1->Jright * l1->Dxhalf * (1 / (r->A + l2->A) + 1 / l1->A) -
						 twoDt * ((r->Ahalf * r->Qhalf + l2->Ahalf * l2->Qhalf ) / (r->Ahalf  + l2->Ahalf) - l1->Qhalf)) /
						(1 / (1 / r->r + 1 / l2->r) + l1->r);   /* 5.138 */
					l2->Jrightnew =
						(l2->Jright * l2->Dxhalf * (1 / (r->A + l1->A) + 1 / l2->A) -
						 twoDt * ((r->Ahalf * r->Qhalf + l1->Ahalf  * l1->Qhalf ) / (r->Ahalf  + l1->Ahalf) - l2->Qhalf)) /
						(1 / (1 / r->r + 1 / l1->r) + l2->r);   /* 5.138 */
					r->Jleftnew = l1->Jrightnew + l2->Jrightnew;   /* 5.139 */
					r->pleftnew = r->Jleftnew / r->Anew;   /* 5.128 */
					l1->prightnew = l1->Jrightnew / l1->Anew;   /* 5.128 */
					l2->prightnew = l2->Jrightnew / l2->Anew;   /* 5.128 */
					r->Kleftnew = onebytworho0 * r->pleftnew * r->pleftnew;   /* 5.128 */
					l1->Krightnew = onebytworho0 * l1->prightnew * l1->prightnew;   /* 5.128 */
					l2->Krightnew = onebytworho0 * l2->prightnew * l2->prightnew;   /* 5.128 */
					#if NO_BERNOULLI_EFFECT
						r->Kleftnew = l1->Krightnew = l2->Krightnew = 0;
					#endif
					r->Qleftnew = l1->Qrightnew = l2->Qrightnew =
						(r->eleft + l1->eright + l2->eright + twoc2Dt * (l1->Jhalf + l2->Jhalf - r->Jhalf) +
						 r->Kleftnew * r->Vnew + l1->Krightnew * l1->Vnew + l2->Krightnew * l2->Vnew) /
						(r->Vnew + l1->Vnew + l2->Vnew);   /* 5.137 */
				}
			}

			/* Save some results. */

			if (n == (oversampling + 1) / 2) {
				double out = 0.0;
				for (m = 1; m <= M; m ++) {
					Delta_Tube t = delta->tube + m;
					out += rho0 * t->Dx * t->Dz * t->dDydt * Dt * 1000;   /* Radiation of wall movement, 5.140. */
					if (t->right1 == NULL)
						out += t->Jrightnew - t->Jright;   /* Radiation of open tube end. */
				}
				result -> z [1] [sample] = out /= 4 * NUMpi * 0.4 * Dt;   /* At 0.4 metres. */
				if (iw1) (*w1) -> z [1] [sample] = delta->tube[iw1].Dy;
				if (iw2) (*w2) -> z [1] [sample] = delta->tube[iw2].Dy;
				if (iw3) (*w3) -> z [1] [sample] = delta->tube[iw3].Dy;
				if (ip1) (*p1) -> z [1] [sample] = delta->tube[ip1].DeltaP;
				if (ip2) (*p2) -> z [1] [sample] = delta->tube[ip2].DeltaP;
				if (ip3) (*p3) -> z [1] [sample] = delta->tube[ip3].DeltaP;
				if (iv1) (*v1) -> z [1] [sample] = delta->tube[iv1].v;
				if (iv2) (*v2) -> z [1] [sample] = delta->tube[iv2].v;
				if (iv3) (*v3) -> z [1] [sample] = delta->tube[iv3].v;
			}
			for (m = 1; m <= M; m ++) {
				Delta_Tube t = delta->tube + m;
				t->Jleft = t->Jleftnew;
				t->Jright = t->Jrightnew;
				t->Qleft = t->Qleftnew;
				t->Qright = t->Qrightnew;
				t->Dy = t->Dynew;
				t->dDydt = t->dDydtnew;
				t->A = t->Anew;
				t->Dx = t->Dxnew;
				t->dDxdt = t->dDxdtnew;
				t->eleft = t->eleftnew;
				t->eright = t->erightnew;
				#if MASS_LEAPFROG
					t->ehalfold = t->ehalf;
				#endif
				t->pleft = t->pleftnew;
				t->pright = t->prightnew;
				t->Kleft = t->Kleftnew;
				t->Kright = t->Krightnew;
				t->V = t->Vnew;
				t->Pturbright = t->Pturbrightnew;
			}
		}
	}
	Melder_monitor1 (1.0, NULL);
	totalVolume = 0.0;
	for (m = 1; m <= M; m ++)
		totalVolume += delta->tube [m]. V;
	//Melder_casual ("Ending volume: %.10g litres.", totalVolume * 1000);
end:
	forget (delta);
	forget (art);
	return result;
}

void Artword_Speaker_draw (Artword artword, Speaker speaker, Graphics g, int numberOfSteps) {
	double oldLineWidth = Graphics_inqLineWidth (g);
	Art art = Art_create ();
	for (int i = 0; i <= numberOfSteps; i ++) {
		Artword_intoArt (artword, art, i * artword -> totalTime / numberOfSteps);
		Graphics_setLineWidth (g, 2 + i + i);
		Art_Speaker_draw (art, speaker, g);
	}
	forget (art);
	Graphics_setLineWidth (g, oldLineWidth);
}

FORM (Artword_Speaker_draw, L"Draw Artword & Speaker", 0)
	NATURAL (L"Number of steps", L"5")
	OK
DO
	autoPraatPicture picture;
	iam_ONLY (Artword);
	thouart_ONLY (Speaker);
	Artword_Speaker_draw (me, thee, GRAPHICS, GET_INTEGER (L"Number of steps"));
END

FORM (Artword_Speaker_to_Sound, L"Articulatory synthesizer", L"Artword & Speaker: To Sound...")
	POSITIVE (L"Sampling frequency (Hz)", L"22050")
	NATURAL (L"Oversampling factor", L"25")
	INTEGER (L"Width 1", L"0")
	INTEGER (L"Width 2", L"0")
	INTEGER (L"Width 3", L"0")
	INTEGER (L"Pressure 1", L"0")
	INTEGER (L"Pressure 2", L"0")
	INTEGER (L"Pressure 3", L"0")
	INTEGER (L"Velocity 1", L"0")
	INTEGER (L"Velocity 2", L"0")
	INTEGER (L"Velocity 3", L"0")
	OK
DO
	Sound w1, w2, w3, p1, p2, p3, v1, v2, v3;
	int iw1 = GET_INTEGER (L"Width 1");
	int iw2 = GET_INTEGER (L"Width 2");
	int iw3 = GET_INTEGER (L"Width 3");
	int ip1 = GET_INTEGER (L"Pressure 1");
	int ip2 = GET_INTEGER (L"Pressure 2");
	int ip3 = GET_INTEGER (L"Pressure 3");
	int iv1 = GET_INTEGER (L"Velocity 1");
	int iv2 = GET_INTEGER (L"Velocity 2");
	int iv3 = GET_INTEGER (L"Velocity 3");
	iam_ONLY (Artword);
	thouart_ONLY (Speaker);
	autoSound him = Artword_Speaker_to_Sound (me, thee,
			GET_REAL (L"Sampling frequency"), GET_INTEGER (L"Oversampling factor"),
			& w1, iw1, & w2, iw2, & w3, iw3,
			& p1, ip1, & p2, ip2, & p3, ip3,
			& v1, iv1, & v2, iv2, & v3, iv3);
	praat_new (him.transfer(), my name, L"_", thy name);
	if (iw1) praat_new (w1, L"width", Melder_integer (iw1));
	if (iw2) praat_new (w2, L"width", Melder_integer (iw2));
	if (iw3) praat_new (w3, L"width", Melder_integer (iw3));
	if (ip1) praat_new (p1, L"pressure", Melder_integer (ip1));
	if (ip2) praat_new (p2, L"pressure", Melder_integer (ip2));
	if (ip3) praat_new (p3, L"pressure", Melder_integer (ip3));
	if (iv1) praat_new (v1, L"velocity", Melder_integer (iv1));
	if (iv2) praat_new (v2, L"velocity", Melder_integer (iv2));
	if (iv3) praat_new (v3, L"velocity", Melder_integer (iv3));
END

/***** ARTWORD & SPEAKER [ & SOUND ] *****/

void Artword_Speaker_movie (Artword artword, Speaker speaker, Graphics g) {
	double timeStep = 0.00001;
	Art art = Art_create ();
	for (double tim = 0.0; tim < artword -> totalTime; tim += timeStep) {
		Artword_intoArt (artword, art, tim);
		Graphics_setViewport (g, 0, 1, 0, 1);
		Graphics_clearWs (g);
		Art_Speaker_draw (art, speaker, g);
		Graphics_flushWs (g);
		// TODO: we should pause here a bit
	}
}

struct playInfo {
	Artword artword;
	Speaker speaker;
	Graphics graphics;
};

static int playCallback (void *playClosure, int phase, double tmin, double tmax, double t) {
	struct playInfo *me = (struct playInfo *) playClosure;
	static Art art;
	(void) phase;
	(void) tmin;
	(void) tmax;
	if (! art) art = Art_create ();
	Artword_intoArt (my artword, art, t);
	Graphics_clearWs (my graphics);
	Art_Speaker_draw (art, my speaker, my graphics);
	return 1;
}

void Artword_Speaker_Sound_movie (Artword artword, Speaker speaker, Sound sound, Graphics graphics) {
	int own = FALSE;
	static struct playInfo info;   /* Must be static!!! */
	info. artword = artword;
	info. speaker = speaker;
	info. graphics = graphics;
	if (sound == NULL) {
		sound = Sound_createSimple (1, artword -> totalTime, 22050);
		if (! sound) {
			Melder_flushError (NULL);
			return;
		}
		own = TRUE;
	}
	Sound_play (sound, playCallback, & info);
	iferror Melder_clearError ();
	if (own) {
		forget (sound);
	}
}

DIRECT (Artword_Speaker_movie)
	Graphics g = Movie_create (L"Artword & Speaker movie", 300, 300);
	iam_ONLY (Artword);
	thouart_ONLY (Speaker);
	heis_ONLY (Sound);   // can be null
	Artword_Speaker_Sound_movie (me, thee, him, g);
END

/***** SPEAKER *****/

FORM (Speaker_create, L"Create a Speaker", L"Create Speaker...")
	WORD (L"Name", L"speaker")
	OPTIONMENU (L"Kind of speaker", 1)
		OPTION (L"Female")
		OPTION (L"Male")
		OPTION (L"Child")
	OPTIONMENU (L"Number of tubes in glottis", 2)
		OPTION (L"1")
		OPTION (L"2")
		OPTION (L"10")
	OK
DO
	if (! praat_new1 (Speaker_create (GET_STRING (L"Kind of speaker"),
		wcstol (GET_STRING (L"Number of tubes in glottis"), NULL, 10)), GET_STRING (L"Name"))) return 0;
END

DIRECT (Speaker_help) Melder_help (L"Speaker"); END

/***** VOCAL TRACT *****/

void Matrix_drawRows (I, Graphics g, double xmin, double xmax, double ymin, double ymax,
	double minimum, double maximum);

void VocalTract_draw (VocalTract me, Graphics g) {
	Matrix_drawRows (me, g, 0, 0, 0, 0, 0, 0);
}

FORM (VocalTract_createFromPhone, L"Create Vocal Tract from phone", L"Create Vocal Tract from phone...")
	OPTIONMENU (L"Phone", 1)
		OPTION (L"a")
		OPTION (L"e")
		OPTION (L"i")
		OPTION (L"o")
		OPTION (L"u")
		OPTION (L"y1")
		OPTION (L"y2")
		OPTION (L"y3")
		OPTION (L"jery")
		OPTION (L"p")
		OPTION (L"t")
		OPTION (L"k")
		OPTION (L"x")
		OPTION (L"pa")
		OPTION (L"ta")
		OPTION (L"ka")
		OPTION (L"pi")
		OPTION (L"ti")
		OPTION (L"ki")
		OPTION (L"pu")
		OPTION (L"tu")
		OPTION (L"ku")
	OK
DO
	if (! praat_new1 (VocalTract_createFromPhone (GET_STRING (L"Phone")), GET_STRING (L"Phone"))) return 0;
END

DIRECT (VocalTract_draw)
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (VocalTract);
		VocalTract_draw (me, GRAPHICS);
	}
END

FORM (VocalTract_formula, L"VocalTract Formula", L"Matrix: Formula...")
	LABEL (L"label", L"`x' is the distance form the glottis in metres, `col' is the section number, `self' is in m\u00B2")
	LABEL (L"label", L"x := x1;   for col := 1 to ncol do { self [col] := `formula' ; x := x + dx }")
	TEXTFIELD (L"formula", L"0")
	OK
DO
	if (! praat_Fon_formula (dia, interpreter)) return 0;
END

DIRECT (VocalTract_help) Melder_help (L"VocalTract"); END

DIRECT (VocalTract_to_Matrix)
	WHERE (SELECTED) {
		iam_LOOP (VocalTract);
		autoMatrix thee = VocalTract_to_Matrix (me);
		praat_new (thee.transfer(), my name);
	}
END

FORM (VocalTract_to_Spectrum, L"From Vocal Tract to Spectrum", 0)
	LABEL (L"ui/editors/AmplitudeTierEditor.h", L"Compute transfer function")
	NATURAL (L"Number of frequencies", L"4097")
	POSITIVE (L"Maximum frequency (Hz)", L"5000")
	REAL (L"Glottal damping", L"0.1")
	BOOLEAN (L"Radiation damping", 1)
	BOOLEAN (L"Internal damping", 1)
	OK
DO
	WHERE (SELECTED) {
		iam_LOOP (VocalTract);
		autoSpectrum thee = VocalTract_to_Spectrum (me, GET_INTEGER (L"Number of frequencies"),
			GET_REAL (L"Maximum frequency"), GET_REAL (L"Glottal damping"),
			GET_INTEGER (L"Radiation damping"), GET_INTEGER (L"Internal damping"));
		praat_new (thee.transfer(), my name);
	}
END

DIRECT (ArticulatorySynthesisTutorial) Melder_help (L"Articulatory synthesis"); END

extern "C" void praat_uvafon_Artsynth_init (void);
void praat_uvafon_Artsynth_init (void) {
	Thing_recognizeClassesByName (classArt, classArtword, classSpeaker, NULL);

	praat_addMenuCommand (L"Objects", L"New", L"Articulatory synthesis", 0, 0, 0);
	praat_addMenuCommand (L"Objects", L"New", L"Articulatory synthesis tutorial", 0, 1, DO_ArticulatorySynthesisTutorial);
	praat_addMenuCommand (L"Objects", L"New", L"-- new articulatory synthesis -- ", 0, 1, 0);
	praat_addMenuCommand (L"Objects", L"New", L"Create Articulation...", 0, 1, DO_Art_create);
	praat_addMenuCommand (L"Objects", L"New", L"Create Speaker...", 0, 1, DO_Speaker_create);
	praat_addMenuCommand (L"Objects", L"New", L"Create Artword...", 0, 1, DO_Artword_create);
	/*praat_addMenuCommand (L"Objects", L"New", L"-- figures --", 0, 1, 0);
	praat_addMenuCommand (L"Objects", L"New", L"Figures", 0, 1, 0);
	praat_addMenuCommand (L"Objects", L"New", L"PB Fig. 4.1: many tubes", 0, 2, DO_Fig_manyTubes);
	praat_addMenuCommand (L"Objects", L"New", L"PB Fig. 4.4: tube connections", 0, 2, DO_Fig_tubeConnections);
	praat_addMenuCommand (L"Objects", L"New", L"PB Fig. 4.7: vocal-tract construction", 0, 2, DO_Fig_vocalTractConstruction);
	praat_addMenuCommand (L"Objects", L"New", L"PB Fig. 5.2: pumping and sucking", 0, 2, DO_Fig_pumpingAndSucking);
	praat_addMenuCommand (L"Objects", L"New", L"PB Fig. 5.8: closed boundary", 0, 2, DO_Fig_closedBoundary);
	praat_addMenuCommand (L"Objects", L"New", L"PB Fig. 5.9: open boundary", 0, 2, DO_Fig_openBoundary);
	praat_addMenuCommand (L"Objects", L"New", L"PB Fig. 5.10: two-way boundary", 0, 2, DO_Fig_twoWayBoundary);
	praat_addMenuCommand (L"Objects", L"New", L"PB Fig. 5.11: three-way boundary", 0, 2, DO_Fig_threeWayBoundary);
	praat_addMenuCommand (L"Objects", L"New", L"PB Fig. 6.3: smoothing errors", 0, 2, DO_Fig_smoothingErrors);
	praat_addMenuCommand (L"Objects", L"New", L"PB Fig. 6.8: HNR window ripples", 0, 2, DO_Fig_hnrWindowRipples);*/
	praat_addMenuCommand (L"Objects", L"New", L"-- new vocal tract --", 0, 1, 0);
	praat_addMenuCommand (L"Objects", L"New", L"Create Vocal Tract from phone...", 0, 1, DO_VocalTract_createFromPhone);

	praat_addAction1 (classArt, 1, L"View & Edit", 0, praat_ATTRACTIVE, DO_Art_edit);
	praat_addAction1 (classArt, 1, L"Edit", 0, praat_HIDDEN, DO_Art_edit);

	praat_addAction1 (classArtword, 0, L"Artword help", 0, 0, DO_Artword_help);
	praat_addAction1 (classArtword, 1, L"View & Edit", 0, praat_ATTRACTIVE, DO_Artword_edit);
	praat_addAction1 (classArtword, 1, L"Edit", 0, praat_HIDDEN, DO_Artword_edit);
	praat_addAction1 (classArtword, 0, L"Info", 0, 0, 0);
	praat_addAction1 (classArtword, 1, L"Get target...", 0, 0, DO_Artword_getTarget);
	praat_addAction1 (classArtword, 0, L"Draw", 0, 0, 0);
	praat_addAction1 (classArtword, 0, L"Draw...", 0, 0, DO_Artword_draw);
	praat_addAction1 (classArtword, 0, L"Modify", 0, 0, 0);
	praat_addAction1 (classArtword, 1, L"Set target...", 0, 0, DO_Artword_setTarget);
	praat_addAction1 (classArtword, 0, L"Analyse", 0, 0, 0);
	praat_addAction1 (classArtword, 0, L"To Art (slice)...", 0, 0, DO_Artword_to_Art);

	praat_addAction2 (classArt, 1, classSpeaker, 1, L"Draw", 0, 0, 0);
	praat_addAction2 (classArt, 1, classSpeaker, 1, L"Draw", 0, 0, DO_Art_Speaker_draw);
	praat_addAction2 (classArt, 1, classSpeaker, 1, L"Fill inner contour", 0, 0, DO_Art_Speaker_fillInnerContour);
	praat_addAction2 (classArt, 1, classSpeaker, 1, L"Draw mesh", 0, 0, DO_Art_Speaker_drawMesh);
	praat_addAction2 (classArt, 1, classSpeaker, 1, L"Synthesize", 0, 0, 0);
	praat_addAction2 (classArt, 1, classSpeaker, 1, L"To VocalTract", 0, 0, DO_Art_Speaker_to_VocalTract);

	praat_addAction2 (classArtword, 1, classSpeaker, 1, L"Movie", 0, 0, DO_Artword_Speaker_movie);
	praat_addAction2 (classArtword, 1, classSpeaker, 1, L"Draw", 0, 0, 0);
	praat_addAction2 (classArtword, 1, classSpeaker, 1, L"Draw...", 0, 0, DO_Artword_Speaker_draw);
	praat_addAction2 (classArtword, 1, classSpeaker, 1, L"Synthesize", 0, 0, 0);
	praat_addAction2 (classArtword, 1, classSpeaker, 1, L"To Sound...", 0, 0, DO_Artword_Speaker_to_Sound);

	praat_addAction3 (classArtword, 1, classSpeaker, 1, classSound, 1, L"Movie", 0, 0, DO_Artword_Speaker_movie);

	praat_addAction1 (classSpeaker, 0, L"Speaker help", 0, 0, DO_Speaker_help);

	praat_addAction1 (classVocalTract, 0, L"VocalTract help", 0, 0, DO_VocalTract_help);
	praat_addAction1 (classVocalTract, 0, L"Draw", 0, 0, 0);
	praat_addAction1 (classVocalTract, 0, L"Draw", 0, 0, DO_VocalTract_draw);
	praat_addAction1 (classVocalTract, 0, L"Analyse", 0, 0, 0);
	praat_addAction1 (classVocalTract, 0, L"To Spectrum...", 0, 0, DO_VocalTract_to_Spectrum);
	praat_addAction1 (classVocalTract, 0, L"Modify", 0, 0, 0);
	praat_addAction1 (classVocalTract, 0, L"Formula...", 0, 0, DO_VocalTract_formula);
	praat_addAction1 (classVocalTract, 0, L"Hack", 0, 0, 0);
	praat_addAction1 (classVocalTract, 0, L"To Matrix", 0, 0, DO_VocalTract_to_Matrix);
}

/* End of file praat_Artsynth.cpp */
