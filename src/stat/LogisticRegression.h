#ifndef _LogisticRegression_h_
#define _LogisticRegression_h_
/* LogisticRegression.h
 *
 * Copyright (C) 2005-2011 Paul Boersma
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
 * pb 2011/03/03
 */

#include "Regression.h"

#ifdef __cplusplus
	extern "C" {
#endif

#include "LogisticRegression_def.h"
#define LogisticRegression_methods  Regression_methods
oo_CLASS_CREATE (LogisticRegression, Regression);

LogisticRegression LogisticRegression_create (const wchar_t *dependent1, const wchar_t *dependent2);

LogisticRegression Table_to_LogisticRegression (Table me, const wchar_t *columnsWithFactors_string,
	const wchar_t *columnWithDependent1_string, const wchar_t *columnWithDependent2_string);

#ifdef __cplusplus
	}
#endif

/* End of file LogisticRegression.h */
#endif
