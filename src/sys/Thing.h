#ifndef _Thing_h_
#define _Thing_h_
/* Thing.h
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
 * pb 2002/03/07 GPL
 * pb 2002/03/11 replaced _I with void_me etc for compliance with new ctype.h
 * pb 2004/10/16 C++ compatible structs
 * pb 2004/10/25 C++ compatible assignments
 * pb 2006/12/10 update on "info" documentation
 * pb 2007/06/11 wchar_t
 * pb 2007/10/09 removed char
 * pb 2008/04/04 Thing_infoWithId
 * pb 2009/03/21 modern enums
 * pb 2009/08/17 readable-class IDs
 * pb 2011/03/05 C++
 * pb 2011/03/09 C++
 */

/* The root class of all objects. */

/* Anyone who uses Thing can also use: */
	/* Arrays with any bounds and 1 or two indices, math, and numerics: */
		#include "num/NUM.h"   /* Including math.h */
	/* The messaging mechanism: */
		#include "sys/melder/melder.h"   /* Including stdio.h string.h etc. */
	/* The macros for struct and class definitions: */
		#include "sys/oo/oo.h"
	/* The input/output mechanism: */
		#include "io/abcio.h"
		#include "io/lispio.h"

/* FIXME disables most memory management. Necessary because it assumes all
   types inherit polymorphically (in Praat's unique way) from Thing, and
   there's no way to test for objects that don't. */
#define forget(thing)  /*_Thing_forget ((Thing *) & (thing))*/
/*
	Function:
		free all memory associated with 'thing'.
	Postconditions:
		thing == NULL;
*/

class Thing {
public:
	const wchar_t * className ();
	/* Return your class name. */

	int member (wchar_t *className);
	/*
		return TRUE if you are a 'klas',
		i.e., if you are an object of the class 'klas' or of one of the classes derived from 'klas'.
		E.g., Thing_member (object, classThing) will always return TRUE.
	*/

	int subclass (wchar_t *ancestor);
	/*
		return TRUE if <klas> is a subclass of <ancestor>,
		i.e., if <klas> equals <ancestor>, or if the parent class of <klas> is a subclass of <ancestor>.
		E.g., Thing_subclass (classX, classThing) will always return TRUE.
	*/

	virtual void info ();
	virtual void infoWithId (unsigned long id);

	static void recognizeClassesByName (void *readableClass, ...);
	/*
		Function:
			make Thing_classFromClassName () and Thing_newFromClassName ()
			recognize a class from its name (a string).
		Arguments:
			as many classes as you want; finish with a NULL.
			It is not an error if a class occurs more than once in the list.
		Behaviour:
			calling this routine more than once, each time for different classes,
			has the same result as calling it once for all these classes together.
			Thing can remember up to 1000 string-readable classes.
		Usage:
			you should call this routine for all classes that you want to read by name,
			e.g., with Data_readFromTextFile () or Data_readFromBinaryFile (),
			or with Data_readText () or Data_readBinary () if the object is a Collection.
			Calls to this routine should preferably be put in the beginning of main ().
	*/

	static void recognizeClassByOtherName (void *readableClass, const wchar_t *otherName);
	static long listReadableClasses (void);

	static Thing * newFromClassNameA (const char *className);
	static Thing * newFromClassName (const wchar_t *className);
	/*
		Function:
			return a new object of class 'className', or NULL if the class name is not recognized.
		Postconditions:
			result -> methods == class'className';
			other members are 0;
			class'className' -> destroy != NULL;   // class'className' has been initialized.
		Side effect:
			see Thing_classFromClassName.
	*/

	static void *classFromClassName (const wchar_t *className);
	/*
		Function:
			Return the class table of class 'className', or NULL if it is not recognized.
			E.g. the value returned from Thing_classFromClassName (L"PietjePuk")
			will be equal to classPietjePuk.
		Postcondition:
			class'className' -> destroy != NULL;   // class'className' has been initialized.
		Side effect:
			Sets the global variable Thing_version.
			If 'className' equals L"PietjePuk 300", the value returned will be classPietjePuk,
			and Thing_version will be set to 300.
	*/

	virtual wchar_t * getName ();
	/* Return a pointer to your internal name (which can be NULL). */
	virtual wchar_t * messageName ();

	virtual void setName (const wchar_t *name);
	/*
		Function:
			remember that you are called 'name'.
		Postconditions:
			my name *and* my name are copies of 'name'.
	*/

	void overrideClass (void *klas);
	/*
		Function:
			change my class to 'klas'.
		Postconditions:
			my methods == klas;
			klas -> destroy != NULL;   // 'klas' has been initialized.
		Usage:
			- Safe typecast if my methods is a subclass of 'klas',
				in which case you can also safely use "my methods = klas".
			- Safe typecast if 'klas' is a dummy subclass of my methods,
				i.e., if 'klas' does not add members or methods (so this is just a name change);
				in this case, you cannot just use "my methods = klas" if you are not sure whether
				'klas' has been initialized (by a previous 'new' or so).
				An application of this is giving a collection of objects of class "Foo"
				the name "Foos" instead of "Collection".
			- Unsafe in all other situations. Normally, 'I' should contain the members and methods of 'klas',
				perhaps with different names.
	*/

	virtual void swap (Thing *other);
	/*
		Function:
			Swap my and thy contents.
		Precondition:
			my methods == thy methods;
		Postconditions:
			my xxx == thy old xxx;
			thy xxx == my old xxx;
		Usage:
			Swap two objects without changing any references to them.
	*/

	virtual void destroy ();
	virtual void nameChanged ();

	static long version;
	/* Set by Thing_classFromClassName. */

protected:
	wchar_t *name;
	long _version;
	long _sequentialUniqueIdOfReadableClass;

/*
	Methods:

	void destroy (I)
		Message sent by _Thing_forget:
			destroy all of my members who are arrays or objects,
			except those who are NULL already (always check).
		Inheritor:
			Use NUMxvector_free and NUMxmatrix_free for destroying arrays;
			you do not have to set the array members to NULL.
			Use 'forget' for destroying objects.
			You can call the inherited 'destroy' last, for destroying the inherited arrays and objects.
		Example:
			iam (Miep);
			NUMdvector_free (my array);
			forget (my object);
			inherited (Miep) destroy (me);
		Thing::destroy does nothing.
		After exit:
			the memory associated with me will be freed,
			and one pointer to it will be set to NULL (see 'forget').

	void info (I)
		Message sent by Thing_info:
			use a sequence of MelderInfo_writeXXX to give some information about you;
			these are often preceded by classData -> info (me).
		Thing::info shows my class name.

	void nameChanged (I)
		Message sent by Thing_setName after setting the new name:
			if you are capable of showing your name, show your new name.
		Thing::nameChanged does nothing.
*/

};

/* For the macros. */

void _Thing_forget (Thing *me);
	/* Macro 'forget'. */
void * _Thing_check (void *table, const char *fileName, int line);
	/* Macros 'iam', 'thouart', 'heis'. */

/* For debugging. */

long Thing_getTotalNumberOfThings (void);
/* This number is 0 initially, increments at every successful `new', and decrements at every `forget'. */

#ifdef __cplusplus
template <class T>
class _Thing_auto {
	T *ptr;
public:
	/*
	 * Things like
	 *    autoPitch pitch (Pitch_create (...));
	 * and
	 *    autoPitch pitch = Pitch_create (...);
	 * should work.
	 */
	_Thing_auto (T *ptr) throw (int) : ptr (ptr) {
		//if (Melder_debug == 37) Melder_casual ("begin initializing autopointer %ld with pointer %ld", this, ptr);
		iferror throw 1;   // if this happens, the destructor won't be called, but that is not necessary anyway
		//if (Melder_debug == 37) Melder_casual ("end initializing autopointer %ld with pointer %ld", this, ptr);
	}
	/*
	 * pitch should be destroyed when going out of scope,
	 * both at the end of the try block and when a throw occurs.
	 */
	~_Thing_auto () throw () {
		//if (Melder_debug == 37) Melder_casual ("begin forgetting autopointer %ld with pointer %ld", this, ptr);
		if (ptr) forget (ptr);
		//if (Melder_debug == 37) Melder_casual ("end forgetting autopointer %ld with pointer %ld", this, ptr);
	}
	T* peek () const throw () { return ptr; }
	/*
	 * The expression
	 *    pitch.ptr -> xmin
	 * should be abbreviatable by
	 *    pitch -> xmin
	 */
	T* operator-> () const throw () { return ptr; }   // as r-value
	T& operator* () const throw () { return *ptr; }   // as l-value
	/*
	 * There are two ways to access the pointer; with and without transfer of ownership.
	 *
	 * Without transfer:
	 *    autoPitch pitch = Sound_to_Pitch (...);
	 *    Pitch_draw (pitch.peek());
	 *
	 * With transfer:
	 *    return thee.transfer();
	 * and
	 *    *out_pitch = pitch.transfer();
	 *    *out_pulses = pulses.transfer();
	 * and
	 *    Collection_addItem (me, pitch.transfer());
	 * and
	 *    praat_new (pitch.transfer(), my name);
	 */
	T* transfer () throw () { T* temp = ptr; ptr = NULL; return temp; }   // make the pointer non-automatic again
	//operator T* () { return ptr; }   // this way only if peek() and transfer() are the same, e.g. in case of reference counting
	// template <class Y> Y* operator= (_Thing_auto<Y>& a) { }
	/*
	 * An autoThing can be cloned. This can be used for giving ownership without losing ownership.
	 */
	T* clone () const throw (int) { return static_cast<T *> (Data_copy (ptr)); }
	/*
	 * Replacing a pointer in an existing autoThing should be an exceptional phenomenon,
	 * and therefore has to be done explicitly (rather than via an assignment),
	 * so that you can easily spot ugly places in your source code.
	 * In order not to leak memory, the old object is destroyed.
	 */
	void reset (const T* const newPtr) { if (ptr) forget (ptr); ptr = newPtr; iferror return 1; }
private:
	/*
	 * The compiler should prevent initializations like
	 *    autoPitch pitch2 = pitch;
	 */
	template <class Y> _Thing_auto (_Thing_auto<Y> &);
	//_Thing_auto (const _Thing_auto &);
	/*
	 * The compiler should prevent assignments like
	 *    pitch2 = pitch;
	 */
	_Thing_auto& operator= (const _Thing_auto&);
	template <class Y> _Thing_auto& operator= (const _Thing_auto<Y>&);
};

#endif

/* End of file Thing.h */
#endif
