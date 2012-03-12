#ifndef _Collection_h_
#define _Collection_h_
/* Collection.h
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

/* Collections contain a number of items whose class is a subclass of Data.

	class Collection = Data {
		void *itemClass;   // The class of which all items must be members (see Thing_member).
		long size;
		Any item [1..size];
	introduce:
		long position (Any data);
	override:
		void destroy ();   // Destroys all the items.
		int copy (thou);   // Copies all the items.
		int equal (thou);   // Compares 'my item [i]' with 'thy item [i]', i = 1..size.
	};
	class Ordered = Collection {
	};
	class Sorted = Collection {
	introduce:
		int compare (thou);   // Compares the keys of two items;
				// returns negative if me < thee, 0 if me == thee, and positive if me > thee.
	};
	class SortedSet = Sorted {   // Every item must be unique (by key).
		override long position (Any data);   // Returns 0 (refusal) if the key of 'data' already occurs.
	};
	class Cyclic = Collection;   // The cyclic list (a, b, c, d) equals (b, c, d, a) but not (d, c, a, b).
*/

#include "Simple.h"

class Collection : public Data {
public:
	virtual long position (Thing *item);

/*
	An object of type Collection is a collection of items of any class.
	It is the owner of its items.
	You can access the items in the collection as item [1] through item [size].
	There can be no NULL items.

	Attributes:
		_capacity >= size		// private; grows as you add items.
		size			// the current number of items.
		item [1..size]		// the items.
*/

Collection (wchar_t *itemClass, long initialCapacity);
/*
	Function:
		return a new empty Collection, or NULL if out of memory.
	Preconditions:
		initialCapacity >= 1;
	Postconditions:
		my _capacity == initialCapacity;
*/

virtual void destroy ();
virtual void info ();

virtual int copyTo (Collection *other);
virtual bool equal (Collection *other);
virtual bool canWriteAsEncoding(int encoding);
virtual int writeText(MelderFile file);
virtual int readText(MelderReadText text);
virtual int writeBinary(FILE *file);
virtual int readBinary(FILE *file);

virtual long size ();

void dontOwnItems ();

/*
	Data_copy, Data_equal, Data_writeXXX, Data_readXXX
	try to copy, compare, write, or read all the items.
	However, if any of the items is not of class Data,
	these routines fail with a message and return 0.
*/

int addItem (Thing *item);
/*
	Function:
		add the 'item' to the collection. Return 0 if out of memory, else 1.
	Preconditions:
 		item != NULL;
	Postconditions if result == 1:
		my size >= my old size + 1;
		if (my size > my old _capacity) my _capacity == 2 * my old _capacity;
	When calling this function, you transfer ownership of 'item' to the Collection.
	For a SortedSet, this may mean that the Collection immediately disposes of 'item',
	if that item already occurred in the Collection.
*/

void removeItem (long position);
/*
	Function:
		remove the item at 'position' from the collection and from memory.
	Preconditions:
		1 <= position <= my size;
	Postconditions:
		my size == my old size - 1;
		my _capacity not changed;
*/

void undangleItem (Thing *item);
/*
	Function:
		remove the item from the collection, without destroying it.
	Postconditions:
		item not found || my size == my old size - 1;
	Usage:
		this is the way in which an item can detach itself from a list;
		often used just before the item is destroyed, hence the name of this procedure.
*/

Thing * subtractItem (long position);
/*
	Function:
		remove the item at 'position' from the collection and transfer ownership to the caller.
	Return value:
		the subtracted item; the caller is responsible for eventually removing it.
	Preconditions:
		1 <= position <= my size;
	Postconditions:
		my size == my old size - 1;
		my _capacity not changed;
*/

void removeAllItems ();
/*
	Function:
		remove all items from the collection and from memory.
	Postconditions:
		my size == 0;
		my _capacity not changed;
*/

void shrinkToFit ();
/*
	Function:
		release as much memory as possible without affecting the items.
	Postconditions:
		my _capacity == max (my size, 1);
*/

static Collection * merge (Collection *first, Collection *second);
/*
	Function:
		merge two Collections into a new one.
	Postconditions:
		result -> size >= my size;
		result -> size >= thy size;
*/

protected:

static Description description [];

virtual int insertItem (Thing *item, long position);
virtual int init (wchar_t *itemClass, long initialCapacity);

/* Methods:

	static long position (Any data, long hint);
		Question asked by Collection_addItem: return a position for the data.
	Collection::position always returns my size + 1 (add item at the end).

*/

	wchar_t *_itemClass;
	long _capacity, _size;
	bool _dontOwnItems;
	Thing **_item;
};

/********** class Ordered **********/

class Ordered : public Collection {
public:
Ordered (wchar_t *itemClass, long initialCapacity);

/* Behaviour:
	Collection_addItem (Ordered) inserts an item at the end.
*/

int addItemPos (Thing *item, long position);
/*
	Function:
		insert an item at 'position'.
		If 'position' is less than 1 or greater than the current 'size',
		insert the item at the end.
*/

};

/********** class Sorted **********/
/* A Sorted is a sorted Collection. */

class Sorted : public Collection {
public:
Sorted (wchar_t *itemClass, long initialCapacity);
virtual long position (Thing *data);
static int compare (const void *first, const void *second);

/* Behaviour:
	Collection_addItem (Sorted) inserts an item at such a position that the collection stays sorted.
	Collections_merge (Sorted) yields a Sorted.
*/

/***** Two routines for optimization. ******/
/* If you want to add a large group of items,
	it is best to call Sorted_addItem_unsorted () repeatedly,
	and finish with Sorted_sort (); this uses the fast 'heapsort' algorithm.
	Calling Collection_addItem () repeatedly would be slower,
	because on the average half the collection is moved in memory
	with every insertion.
*/

virtual int addItem_unsorted (Thing *item);
/*
	Function:
		add an item to the collection, quickly at the end.
	Warning:
		this leaves the collection unsorted; follow by Sorted_sort ().
*/
virtual void sort ();
/* Call this after a number of calls to Sorted_addItem_unsorted (). */
/* The procedure used is 'heapsort'. */

};

/********** class SortedSet **********/

class SortedSet : public Sorted {
public:
SortedSet (wchar_t *itemClass, long initialCapacity);

virtual long position (Thing *data);
/* Behaviour:
	Collection_addItem (SortedSet) refuses to insert an item if this item already occurs.
		Equality is there when the compare routine returns 0.
	Collections_merge (SortedSet) yields a SortedSet that is the union of the two sources.
*/

int hasItem (Thing *item);

};

/********** class SortedSetOfInt **********/

class SortedSetOfInt : public SortedSet {
public:
SortedSetOfInt ();

};

/********** class SortedSetOfShort **********/

class SortedSetOfShort : public SortedSet {
public:
SortedSetOfShort ();

};

/********** class SortedSetOfLong **********/

class SortedSetOfLong : public SortedSet {
public:
SortedSetOfLong ();

};

/********** class SortedSetOfDouble **********/

class SortedSetOfDouble : public SortedSet {
public:
SortedSetOfDouble ();

};

/********** class SortedSetOfString **********/

class SortedSetOfString : public SortedSet {
public:
SortedSetOfString ();
long lookUp (const wchar_t *string);
int add (const wchar_t *string);
static int compare(SimpleString *first, SimpleString *second);
};

/********** class Cyclic **********/

class Cyclic : public Collection {
public:
Cyclic (wchar_t *itemClass, long initialCapacity);

void unicize ();
void cycleLeft ();

static int compare (Thing *first, Thing *second);

};

/* End of file Collection.h */
#endif
