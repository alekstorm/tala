#ifndef _complex_h_
#define _complex_h_
/* complex.h
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
 * pb 2011/03/02
 */

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct fcomplex { float re, im; } fcomplex;
typedef struct dcomplex { double re, im; } dcomplex;

/*
 * Stack-based complex arithmetic.
 * Some compilers will issue warnings about returning structs larger than 8 bytes,
 * but will still work as expected.
 */

fcomplex fcomplex_add (fcomplex a, fcomplex b);
dcomplex dcomplex_add (dcomplex a, dcomplex b);
	/* Addition: a + b */
fcomplex fcomplex_sub (fcomplex a, fcomplex b);
dcomplex dcomplex_sub (dcomplex a, dcomplex b);
	/* Subtraction: a - b */
fcomplex fcomplex_mul (fcomplex a, fcomplex b);
dcomplex dcomplex_mul (dcomplex a, dcomplex b);
	/* Multiplication: a * b */
fcomplex fcomplex_create (float re, float im);
dcomplex dcomplex_create (double re, double im);
	/* Create a complex number: { re, im } */
fcomplex fcomplex_conjugate (fcomplex z);
dcomplex dcomplex_conjugate (dcomplex z);
	/* Conjugation: { z.re, - z.im } */
fcomplex fcomplex_div (fcomplex a, fcomplex b);
dcomplex dcomplex_div (dcomplex a, dcomplex b);
	/* Division: a / b */
float fcomplex_abs (fcomplex z);
double dcomplex_abs (dcomplex z);
	/* Absolute value: | z | */
fcomplex fcomplex_sqrt (fcomplex z);
dcomplex dcomplex_sqrt (dcomplex z);
	/* Square root: sqrt (z) */
fcomplex fcomplex_rmul (float x, fcomplex a);
dcomplex dcomplex_rmul (double x, dcomplex a);
	/* Multiplication by a real number: x * a */
fcomplex fcomplex_exp (fcomplex z);
dcomplex dcomplex_exp (dcomplex z);
	/* Exponentiation: e^z */

#ifdef __cplusplus
	}
#endif

/* End of file complex.h */
#endif
