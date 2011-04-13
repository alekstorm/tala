/* Artword_Speaker_to_Sound.cpp
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
 * pb 2006/12/30 new Sound_create API
 * pb 2008/01/19 double
 * pb 2011/03/22 C++
 */

#include "Speaker_to_Delta.h"
#include "Art_Speaker_Delta.h"
#include "Artword_Speaker_to_Sound.h"

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

/* End of file Artword_Speaker_to_Sound.cpp */
