/* Collection.c
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
 * pb 2004/10/16 C++ compatible structs
 * pb 2006/08/08 reduced compiler warnings
 * pb 2006/12/17 better info
 * pb 2007/06/24 wchar_t
 * pb 2007/08/08 canWriteAsEncoding
 * pb 2007/10/01 make sure that names are encodable when writing
 * pb 2008/03/19 removed SortedSetOfFloat
 * pb 2008/07/20 wchar_t
 * pb 2010/07/28 tiny corrections (like a memory leak if out of memory...)
 * pb 2011/03/23 Collection_dontOwnItems
 */

#include "Collection.h"

/********** class Collection **********/

static void classCollection_destroy (I) {
	iam (Collection);
	if (my item != NULL) {
		if (! my _dontOwnItems) {
			for (long i = 1; i <= my size; i ++) {
				forget (my item [i]);
			}
		}
		my item ++;   // base 1
		Melder_free (my item);
	}
	inherited (Collection) destroy (me);
}

static void classCollection_info (I) {
	iam (Collection);
	MelderInfo_writeLine2 (Melder_integer (my size), L" items");
}

static int classCollection_copy (I, thou) {
	iam (Collection); thouart (Collection);
//start:
	thy item = NULL;   // kill shallow copy of item
	if (! inherited (Collection) copy (me, thee)) return 0;   // optimize around cherror
	thy itemClass = my itemClass;
	thy _capacity = my _capacity;
	thy size = my size;
	thy item = Melder_calloc_e (void *, my _capacity); cherror   // filled with NULL
	thy item --;   // immediately turn from base-0 into base-1
	for (long i = 1; i <= my size; i ++) {
		Thing item = my item [i];
		if (my _dontOwnItems) {
			thy item [i] = item;   // reference copy: if me doesn't own the items, then thee shouldn't either   // NOTE: the items don't have to be Data
		} else {
			if (! Thing_member (item, classData))
				error3 (L"Collection::copy: cannot copy item of class ", Thing_className (item), L".")
			thy item [i] = Data_copy (item); cherror
		}
	}
end:
	// thy item is NULL or base-1
	iferror return 0;
	return 1;
}

static bool classCollection_equal (I, thou) {
	iam (Collection); thouart (Collection);
	if (! inherited (Collection) equal (me, thee)) return false;
	if (my size != thy size) return false;
	for (long i = 1; i <= my size; i ++) {
		if (! Thing_member (my item [i], classData))
			return Melder_error3 (L"Collection::equal: "
				"cannot compare items of class ", Thing_className (my item [i]), L".");
		if (! Thing_member (thy item [i], classData))
			return Melder_error3 (L"Collection::equal: "
				"cannot compare items of class ", Thing_className (thy item [i]), L".");
		bool equal = Data_equal (my item [i], thy item [i]);
		//Melder_casual ("classCollection_equal: %d, items %ld, types %ls and %ls",
		//	equal, i, Thing_className (my item [i]), Thing_className (thy item [i]));
		if (! equal) return false;
	}
	return true;
}

static bool classCollection_canWriteAsEncoding (I, int encoding) {
	iam (Collection);
	for (long i = 1; i <= my size; i ++) {
		Thing thing = my item [i];
		if (thing -> name != NULL && ! Melder_isEncodable (thing -> name, encoding)) return false;
		if (! Data_canWriteAsEncoding (thing, encoding)) return false;
	}
	return true;
}

static int classCollection_writeText (I, MelderFile file) {
	iam (Collection);
	texputi4 (file, my size, L"size", 0,0,0,0,0);
	texputintro (file, L"item []: ", my size ? NULL : L"(empty)", 0,0,0,0);
	for (long i = 1; i <= my size; i ++) {
		Thing thing = my item [i];
		Thing_Table table = thing -> methods;
		texputintro (file, L"item [", Melder_integer (i), L"]:", 0,0,0);
		if (! Thing_member (thing, classData) || ! Data_canWriteText (thing))
			return Melder_error3 (L"(Collection::writeText:) "
				"Objects of class ", table -> _className, L" cannot be written.");
		texputw2 (file, table -> version > 0 ? Melder_wcscat3 (table -> _className, L" ", Melder_integer (table -> version)) : table -> _className, L"class", 0,0,0,0,0);
		texputw2 (file, thing -> name, L"name", 0,0,0,0,0);
		if (! Data_writeText (thing, file)) return 0;
		texexdent (file);
	}
	texexdent (file);
	return 1;
}

static int classCollection_readText (I, MelderReadText text) {
	iam (Collection);
	if (Thing_version < 0) {
		long size;
		wchar_t *line = MelderReadText_readLine (text);
		if (line == NULL || ! swscanf (line, L"%ld", & size) || size < 0)
			return Melder_error1 (L"Collection::readText: cannot read size.");
		if (! Collection_init (me, NULL, size)) return 0;
		for (long i = 1; i <= size; i ++) {
			long itemNumberRead;
			int n = 0, length, stringsRead;
			char klas [200], nameTag [2000];
			do { line = MelderReadText_readLine (text); if (line == NULL) return 0; }
			while (wcsncmp (line, L"Object ", 7));
			stringsRead = swscanf (line, L"Object %ld: class %s %s%n", & itemNumberRead, klas, nameTag, & n);
			if (stringsRead < 2)
				return Melder_error3 (L"Collection::readText: cannot read header of object ", Melder_integer (i), L".");
			if (itemNumberRead != i)
				return Melder_error5 (L"Collection::readText: read item number ", Melder_integer (itemNumberRead),
					L" while expecting ", Melder_integer (i), L".");
			if (stringsRead == 3 && ! strequ (nameTag, "name"))
				return Melder_error3 (L"Collection::readText: wrong header at object ", Melder_integer (i), L".");
			if (! (my item [i] = Thing_newFromClassNameA (klas))) return 0;
			Thing_version = -1;   /* Override. */
			my size ++;
			if (! Thing_member (my item [i], classData) || ! Data_canReadText (my item [i]))
				return Melder_error3 (L"Collection::readText: "
					"cannot read item of class ", Thing_className (my item [i]), L".");
			if (! Data_readText (my item [i], text)) return 0;
			if (stringsRead == 3) {
				if (line [n] == ' ') n ++;   /* Skip space character. */
				length = wcslen (line+n);
				if (length > 0 && (line+n) [length - 1] == '\n') (line+n) [length - 1] = '\0';
				Thing_setName (my item [i], line+n);
			}
		}
		return 1;
	}
	char *className = NULL;
	wchar_t *objectName = NULL;
//start:
	long size = texgeti4 (text);
	Collection_init (me, NULL, size); cherror
	for (long i = 1; i <= size; i ++) {
		long saveVersion = Thing_version;   /* The version of the Collection... */
		Melder_free (className); className = texgets2 (text); cherror
		my item [i] = Thing_newFromClassNameA (className); cherror
		my size ++;
		if (! Thing_member (my item [i], classData) || ! Data_canReadText (my item [i]))
			error3 (L"Cannot read item of class ", Thing_className (my item [i]), L" in collection.");
		Melder_free (objectName); objectName = texgetw2 (text); cherror
		Thing_setName (my item [i], objectName); cherror
		Data_readText (my item [i], text); cherror
		Thing_version = saveVersion;
	}
end:
	Melder_free (className);
	Melder_free (objectName);
	iferror return 0;
	return 1;
}

static int classCollection_writeBinary (I, FILE *f) {
	iam (Collection);
	binputi4 (my size, f);
	for (long i = 1; i <= my size; i ++) {
		Thing thing = my item [i];
		Thing_Table table = thing -> methods;
		if (! Thing_member (thing, classData) || ! Data_canWriteBinary (thing))
			return Melder_error3 (L"(Collection::writeBinary:) "
				"Objects of class ", table -> _className, L" cannot be written.");
		binputw1 (table -> version > 0 ?
			Melder_wcscat3 (table -> _className, L" ", Melder_integer (table -> version)) : table -> _className, f);
		binputw2 (thing -> name, f);
		if (! Data_writeBinary (thing, f)) return 0;
	}
	return 1;
}

static int classCollection_readBinary (I, FILE *f) {
	iam (Collection);
	if (Thing_version < 0) {
		long size = bingeti4 (f);
		if (size < 0 || ! Collection_init (me, NULL, size)) return 0;
		for (long i = 1; i <= size; i ++) {
			char klas [200], name [2000];
			if (fscanf (f, "%s%s", klas, name) < 2 ||
				! (my item [i] = Thing_newFromClassNameA (klas))) return 0;
			Thing_version = -1;   /* Override. */
			my size ++;
			if (! Thing_member (my item [i], classData))
				return Melder_error3 (L"Collection::readBinary: "
					"cannot read item of class ", Thing_className (my item [i]), L".");
			if (fgetc (f) != ' ' || ! Data_readBinary (my item [i], f)) return 0;
			if (strcmp (name, "?")) Thing_setName (my item [i], Melder_peekUtf8ToWcs (name));
		}
	} else {
		long size = bingeti4 (f);
		if (! Collection_init (me, NULL, size)) return 0;
		for (long i = 1; i <= size; i ++) {
			long saveVersion = Thing_version;   /* The version of the Collection... */
			char *klas = bingets1 (f);
			if (! (my item [i] = Thing_newFromClassNameA (klas))) return 0;
			Melder_free (klas);
			my size ++;
			if (! Thing_member (my item [i], classData) || ! Data_canReadBinary (my item [i]))
				return Melder_error3 (L"(Collection::readBinary:) "
					"Cannot read item of class ", Thing_className (my item [i]), L".");
			wchar_t *name = bingetw2 (f);
			Thing_setName (my item [i], name);
			Melder_free (name);
			if (! Data_readBinary (my item [i], f)) return 0;
			Thing_version = saveVersion;
		}
	}
	return 1;
}

static struct structData_Description classCollection_description [] = {
	{ L"size", longwa, (int) & ((Collection) 0) -> size, sizeof (long) },
	{ L"item", objectwa, (int) & ((Collection) 0) -> item, sizeof (Data), L"Data", & theStructData, 1, 0, L"my size" },
	{ 0 } };

static long classCollection_position (I, Any data) {
	iam (Collection);
	(void) data;
	return my size + 1;
}

class_methods (Collection, Data) {
	class_method_local (Collection, destroy)
	class_method_local (Collection, info)
	class_method_local (Collection, copy)
	class_method_local (Collection, equal)
	class_method_local (Collection, canWriteAsEncoding)
	class_method_local (Collection, writeText)
	class_method_local (Collection, writeBinary)
	class_method_local (Collection, readText)
	class_method_local (Collection, readBinary)
	class_method_local (Collection, description)
	class_method_local (Collection, position)
	class_methods_end
}

int Collection_init (I, void *itemClass, long initialCapacity) {
	iam (Collection);
	my itemClass = itemClass;
	my _capacity = initialCapacity >= 1 ? initialCapacity : 1;
	my size = 0;
	my item = Melder_calloc_e (void *, my _capacity);
	if (my item == NULL) return 0;   // optimize around cherror
	my item --;   // base 1
	return 1;
}

Collection Collection_create (void *itemClass, long initialCapacity) {
	Collection me = Thing_new (Collection);
	if (! me || ! Collection_init (me, itemClass, initialCapacity)) forget (me);
	return me;
}

void Collection_dontOwnItems (I) {
	iam (Collection);
	Melder_assert (my size == 0);
	my _dontOwnItems = true;
}

int _Collection_insertItem (I, Any data, long pos) {
	iam (Collection);
	if (my size >= my _capacity) {
	/*
	 * Check without change.
	 */
		Any *dum = (Any *) Melder_realloc_e (my item + 1, 2 * my _capacity * sizeof (Any));
		if (! dum) return Melder_error1 (L"(Collection_insert:) Item not inserted.");
	/*
	 * From here: change without error.
	 */
		my item = dum - 1;
		my _capacity *= 2;
	}
	my size ++;
	for (long i = my size; i > pos; i --) my item [i] = my item [i - 1];
	my item [pos] = data;
	return 1;
}

int Collection_addItem (I, Any data) {
	iam (Collection);
	Melder_assert (data != NULL);
	long index = our position (me, data);
	if (index != 0) {
		return _Collection_insertItem (me, data, index);
	} else {
		forget (data);   /* Could not insert; I am owner, so I must dispose of the data!!! */
		return 1;   /* Refusal; all right. */
	}
}

void Collection_removeItem (I, long pos) {
	iam (Collection);
	Melder_assert (pos >= 1 && pos <= my size);
	if (! my _dontOwnItems) forget (my item [pos]);
	for (long i = pos; i < my size; i ++) my item [i] = my item [i + 1];
	my size --;
}

void Collection_undangleItem (I, Any item) {
	iam (Collection);
	for (long i = my size; i > 0; i --) if (my item [i] == item) {
		for (long j = i; j < my size; j ++) my item [j] = my item [j + 1];
		my size --;
	}
}

Any Collection_subtractItem (I, long pos) {
	iam (Collection);
	Melder_assert (pos >= 1 && pos <= my size);
	Any result = my item [pos];
	for (long i = pos; i < my size; i ++) my item [i] = my item [i + 1];
	my size --;
	return result;
}

void Collection_removeAllItems (I) {
	iam (Collection);
	if (! my _dontOwnItems) for (long i = 1; i <= my size; i ++) forget (my item [i]);
	my size = 0;
}

void Collection_shrinkToFit (I) {
	iam (Collection);
	my _capacity = my size ? my size : 1;
	my item = (Any *) Melder_realloc_f (my item + 1, my _capacity * sizeof (Any)) - 1;
}

Any Collections_merge (I, thou) {
	iam (Collection); thouart (Collection);
	Collection him;
	if (my methods != thy methods) return Melder_errorp5 (L"(Collections_merge:) "
		"Objects are of different class (", Thing_className (me), L" and ", Thing_className (thee), L").");
	if (my _dontOwnItems != thy _dontOwnItems) return Melder_errorp1 (L"(Collections_merge:) "
		"Cannot mix data and references.");
	him = Data_copy (me); cherror
	for (long i = 1; i <= thy size; i ++) {
		Thing tmp;
		Thing item = (Thing) thy item [i];
		if (my _dontOwnItems) {
			tmp = item;
		} else {
			if (! Thing_member (item, classData))
				error3 (L"(Collections_merge:) Cannot copy item of class ", Thing_className (item), L".")
			tmp = Data_copy (item); cherror
		}
		if (! tmp || ! Collection_addItem (him, tmp)) { forget (tmp); goto end; }
	}
	return him;
end:
	forget (him);
	return Melder_errorp ("Collections not merged." );
}

/********** class Ordered **********/

class_methods (Ordered, Collection) {
	class_methods_end
}

int Ordered_init (I, void *itemClass, long initialMaximumLength) {
	iam (Ordered);
	if (! Collection_init (me, itemClass, initialMaximumLength)) return 0;
	return 1;
}

Ordered Ordered_create (void) {
	Ordered me = Thing_new (Ordered);
	if (! me || ! Ordered_init (me, NULL, 10)) forget (me);
	return me;
}

int Ordered_addItemPos (I, Any data, long position) {
	iam (Ordered);
	Melder_assert (data);
	if (position < 1 || position > my size) position = my size + 1;
	return _Collection_insertItem (me, data, position);
}

/********** class Sorted **********/

static long classSorted_position (I, Any data) {
	iam (Sorted);
	if (my size == 0 || our compare (data, my item [my size]) >= 0) return my size + 1;
	if (our compare (data, my item [1]) < 0) return 1;
	/* Binary search. */
	long left = 1, right = my size;
	while (left < right - 1) {
		long mid = (left + right) / 2;
		if (our compare (data, my item [mid]) >= 0) left = mid; else right = mid;
	}
	Melder_assert (right == left + 1);
	return right;
}

static int classSorted_compare (Any data1, Any data2) {
	(void) data1;
	(void) data2;
	return 0;   /* All are equal. */
}

class_methods (Sorted, Collection) {
	class_method_local (Sorted, position)
	class_method_local (Sorted, compare)
	class_methods_end
}

int Sorted_init (I, void *itemClass, long initialCapacity) {
	iam (Sorted);
	if (! Collection_init (me, itemClass, initialCapacity)) return 0;
	return 1;
}

int Sorted_addItem_unsorted (I, Any data) {
	iam (Sorted);
	return _Collection_insertItem (me, data, my size + 1);
}

void Sorted_sort (I) {
	iam (Sorted);
	NUMsort_p (my size, my item, (int (*) (const void *, const void *)) our compare);
}

/********** class SortedSet **********/

static long classSortedSet_position (I, Any data) {
	iam (SortedSet);
	if (my size == 0) return 1;   /* Empty set? 'data' is going to be the first item. */
	int where = our compare (data, my item [my size]);   /* Compare with last item. */
	if (where > 0) return my size + 1;   /* Insert at end. */
	if (where == 0) return 0;
	if (our compare (data, my item [1]) < 0) return 1;   /* Compare with first item. */
	long left = 1, right = my size;
	while (left < right - 1) {
		long mid = (left + right) / 2;
		if (our compare (data, my item [mid]) >= 0)
			left = mid;
		else
			right = mid;
	}
	Melder_assert (right == left + 1);
	if (! our compare (data, my item [left]) || ! our compare (data, my item [right]))
		return 0;
	return right;
}

class_methods (SortedSet, Sorted) {
	class_method_local (SortedSet, position)
	class_methods_end
}

int SortedSet_init (Any me, void *itemClass, long initialCapacity) {
	return Sorted_init (me, itemClass, initialCapacity);
}

int SortedSet_hasItem (I, Any item) {
	iam (SortedSet);
	return our position (me, item) == 0;
}

/********** class SortedSetOfInt **********/

static int classSortedSetOfInt_compare (I, thou) {
	iam (SimpleInt); thouart (SimpleInt);
	if (my number < thy number) return -1;
	if (my number > thy number) return +1;
	return 0;
}

class_methods (SortedSetOfInt, SortedSet) {
	class_method_local (SortedSetOfInt, compare)
	class_methods_end
}

int SortedSetOfInt_init (I) { iam (SortedSetOfInt); return SortedSet_init (me, classSimpleInt, 10); }

SortedSetOfInt SortedSetOfInt_create (void) {
	SortedSetOfInt me = Thing_new (SortedSetOfInt);
	if (! me || ! SortedSetOfInt_init (me)) forget (me);
	return me;
}

/********** class SortedSetOfShort **********/

static int classSortedSetOfShort_compare (I, thou) {
	iam (SimpleShort); thouart (SimpleShort);
	if (my number < thy number) return -1;
	if (my number > thy number) return +1;
	return 0;
}

class_methods (SortedSetOfShort, SortedSet) {
	class_method_local (SortedSetOfShort, compare)
	class_methods_end
}

int SortedSetOfShort_init (I) { iam (SortedSetOfShort); return SortedSet_init (me, classSimpleShort, 10); }

SortedSetOfShort SortedSetOfShort_create (void) {
	SortedSetOfShort me = Thing_new (SortedSetOfShort);
	if (! me || ! SortedSetOfShort_init (me)) forget (me);
	return me;
}

/********** class SortedSetOfLong **********/

static int classSortedSetOfLong_compare (I, thou) {
	iam (SimpleLong); thouart (SimpleLong);
	if (my number < thy number) return -1;
	if (my number > thy number) return +1;
	return 0;
}

class_methods (SortedSetOfLong, SortedSet) {
	class_method_local (SortedSetOfLong, compare)
	class_methods_end
}

int SortedSetOfLong_init (I) { iam (SortedSetOfLong); return SortedSet_init (me, classSimpleLong, 10); }

SortedSetOfLong SortedSetOfLong_create (void) {
	SortedSetOfLong me = Thing_new (SortedSetOfLong);
	if (! me || ! SortedSetOfLong_init (me)) forget (me);
	return me;
}

/********** class SortedSetOfDouble **********/

static int classSortedSetOfDouble_compare (I, thou) {
	iam (SimpleDouble); thouart (SimpleDouble);
	if (my number < thy number) return -1;
	if (my number > thy number) return +1;
	return 0;
}

class_methods (SortedSetOfDouble, SortedSet) {
	class_method_local (SortedSetOfDouble, compare)
	class_methods_end
}

int SortedSetOfDouble_init (I) { iam (SortedSetOfDouble); return SortedSet_init (me, classSimpleDouble, 10); }

SortedSetOfDouble SortedSetOfDouble_create (void) {
	SortedSetOfDouble me = Thing_new (SortedSetOfDouble);
	if (! me || ! SortedSetOfDouble_init (me)) forget (me);
	return me;
}

/********** class SortedSetOfString **********/

static int classSortedSetOfString_compare (I, thou) {
	iam (SimpleString); thouart (SimpleString);
	return wcscmp (my string, thy string);
}

class_methods (SortedSetOfString, SortedSet) {
	class_method_local (SortedSetOfString, compare)
	class_methods_end
}

int SortedSetOfString_init (I) { iam (SortedSetOfString); return SortedSet_init (me, classSimpleString, 10); }

SortedSetOfString SortedSetOfString_create (void) {
	SortedSetOfString me = Thing_new (SortedSetOfString);
	if (! me || ! SortedSetOfString_init (me)) forget (me);
	return me;
}

long SortedSetOfString_lookUp (SortedSetOfString me, const wchar_t *string) {
	SimpleString *items = (SimpleString *) my item;
	long numberOfItems = my size;
	long left = 1, right = numberOfItems;
	int atStart, atEnd;
	if (numberOfItems == 0) return 0;

	atEnd = wcscmp (string, items [numberOfItems] -> string);
	if (atEnd > 0) return 0;
	if (atEnd == 0) return numberOfItems;

	atStart = wcscmp (string, items [1] -> string);
	if (atStart < 0) return 0;
	if (atStart == 0) return 1;

	while (left < right - 1) {
		long mid = (left + right) / 2;
		int here = wcscmp (string, items [mid] -> string);
		if (here == 0) return mid;
		if (here > 0) left = mid; else right = mid;
	}
	Melder_assert (right == left + 1);
	return 0;
}

int SortedSetOfString_add (SortedSetOfString me, const wchar_t *string) {
	static SimpleString simp;
	long index;
	SimpleString newSimp;
	if (! simp) { simp = SimpleString_create (L""); Melder_free (simp -> string); }
	simp -> string = (wchar_t *) string;
	if ((index = our position (me, simp)) == 0) return 1;   /* OK: already there: do not add. */
	newSimp = SimpleString_create (string);
	if (! newSimp || ! _Collection_insertItem (me, newSimp, index)) return 0;   /* Must be out of memory. */
	return 1;   /* OK: added new string. */
}

/********** class Cyclic **********/

static int classCyclic_compare (I, thou) {
	(void) void_me;
	(void) void_thee;
	Melder_fatal ("Cyclic::compare: subclass responsibility.");
	return 0;
}

class_methods (Cyclic, Collection) {
	class_method_local (Cyclic, compare)
	class_methods_end
}

int Cyclic_init (Any me, void *itemClass, long initialCapacity) {
	return Collection_init (me, itemClass, initialCapacity);
}

static void cycleLeft (I) {
	iam (Cyclic);
	if (my size == 0) return;
	Data help = my item [1];
	for (long i = 1; i < my size; i ++) my item [i] = my item [i + 1];
	my item [my size] = help;
}

void Cyclic_unicize (I) {
	iam (Cyclic);
	if (my size <= 1) return;
	long lowest = 1;
	for (long i = 1; i <= my size; i ++)
		if (our compare (my item [i], my item [lowest]) < 0) lowest = i;
	for (long i = 1; i < lowest; i ++)
		cycleLeft (me);
}

/* End of file Collection.c */
