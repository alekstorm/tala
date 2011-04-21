#ifndef _RunnerMFC_h_
#define _RunnerMFC_h_
/* RunnerMFC.h
 *
 * Copyright (C) 2001-2005 Paul Boersma
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
 * pb 2005/12/08
 */

#ifndef _Editor_h_
	#include "sys/Editor.h"
#endif
#ifndef _ExperimentMFC_h_
	#include "ExperimentMFC.h"
#endif

class RunnerMFC : public Editor {
  public:
	RunnerMFC (GuiObject parent, const wchar_t *title, Ordered experiments);
	~RunnerMFC ();

	wchar_t * type () { return L"RunnerMFC"; }
	bool isEditable () { return false; }
	bool isScriptable () { return false; }

	void dataChanged ();
	int startExperiment ();
	void drawControlButton (double left, double right, double bottom, double top, const wchar_t *visibleText);
	void do_ok ();
	void do_oops ();
	void do_replay ();
	void createChildren ();

	GuiObject _drawingArea;
	Ordered _experiments;
	long _iexperiment;
	Graphics _graphics;
	long _numberOfReplays;
};

/* End of file RunnerMFC.h */
#endif
