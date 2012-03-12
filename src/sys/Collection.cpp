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

void Collection::destroy () {
	if (_item != NULL) {
		if (! _dontOwnItems) {
			for (long i = 1; i <= _size; i ++) {
				forget (_item [i]);
			}
		}
		_item ++;   // base 1
		Melder_free (_item);
	}
	Data::destroy ();
}

void Collection::info () {
	MelderInfo_writeLine2 (Melder_integer (_size), L" items");
}

int Collection::copyTo (Collection *other) {
	other->_item = NULL;   // kill shallow copy of item
	if (! ((Data *)this)->copyTo(other)) return 0;   // optimize around cherror
	other->_itemClass = _itemClass;
	other->_capacity = _capacity;
	other->_size = _size;
	other->_item = Melder_calloc_e (Thing *, _capacity); cherror   // filled with NULL
	other->_item --;   // immediately turn from base-0 into base-1
	for (long i = 1; i <= _size; i ++) {
		Thing *item = _item [i];
		if (_dontOwnItems) {
			other->_item [i] = item;   // reference copy: if me doesn't own the items, then thee shouldn't either   // NOTE: the items don't have to be Data
		} else {
			if (! item->member(L"Data"))
				error3 (L"Collection::copy: cannot copy item of class ", item->className(), L".")
			other->_item [i] = ((Data *)item)->copy(); cherror
		}
	}
end:
	// other->_item is NULL or base-1
	iferror return 0;
	return 1;
}

bool Collection::equal (Collection *other) {
	if (! ((Data*)this)->equal(other)) return false;
	if (_size != other->_size) return false;
	for (long i = 1; i <= _size; i ++) {
		if (! _item [i]->member(L"Data"))
			return Melder_error3 (L"Collection::equal: "
				"cannot compare items of class ", _item [i]->className(), L".");
		if (! other->_item [i]->member(L"Data"))
			return Melder_error3 (L"Collection::equal: "
				"cannot compare items of class ", other->_item [i]->className(), L".");
		bool equal = ((Data *)_item [i])->equal((Data *)other->_item [i]);
		//Melder_casual ("classCollection_equal: %d, items %ld, types %ls and %ls",
		//	equal, i, Thing_className (_item [i]), Thing_className (thy item [i]));
		if (! equal) return false;
	}
	return true;
}

bool Collection::canWriteAsEncoding (int encoding) {
	for (long i = 1; i <= _size; i ++) {
		Thing *thing = _item [i];
		if (thing -> getName() != NULL && ! Melder_isEncodable (thing -> getName(), encoding)) return false;
		if (!thing->member(L"Data") || ! ((Data *)thing)->canWriteAsEncoding (encoding)) return false;
	}
	return true;
}

int Collection::writeText (MelderFile file) {
	/*texputi4 (file, _size, L"size", 0,0,0,0,0);
	texputintro (file, L"item []: ", _size ? NULL : L"(empty)", 0,0,0,0);
	for (long i = 1; i <= _size; i ++) {
		Thing *thing = _item [i];
		Thing::Table *table = thing -> methods;
		texputintro (file, L"item [", Melder_integer (i), L"]:", 0,0,0);
		if (! Thing_member (thing, classData) || ! Data_canWriteText (thing))
			return Melder_error3 (L"(Collection::writeText:) "
				"Objects of class ", table -> _className, L" cannot be written.");
		texputw2 (file, table -> version > 0 ? Melder_wcscat3 (table -> _className, L" ", Melder_integer (table -> version)) : table -> _className, L"class", 0,0,0,0,0);
		texputw2 (file, thing -> name, L"name", 0,0,0,0,0);
		if (! Data_writeText (thing, file)) return 0;
		texexdent (file);
	}
	texexdent (file);*/ // FIXME
	return 1;
}

int Collection::readText (MelderReadText text) {
	if (Thing::version < 0) {
		long size;
		wchar_t *line = MelderReadText_readLine (text);
		if (line == NULL || ! swscanf (line, L"%ld", & size) || size < 0)
			return Melder_error1 (L"Collection::readText: cannot read size.");
		if (! init (NULL, size)) return 0;
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
			if (! (_item [i] = Thing::newFromClassNameA (klas))) return 0;
			Thing::version = -1;   /* Override. */
			_size ++;
			if (! _item [i]->member(L"Data") || ! ((Data *)_item[i])->canReadText())
				return Melder_error3 (L"Collection::readText: "
					"cannot read item of class ", _item [i]->className(), L".");
			if (! ((Data *)_item [i])->readText(text)) return 0;
			if (stringsRead == 3) {
				if (line [n] == ' ') n ++;   /* Skip space character. */
				length = wcslen (line+n);
				if (length > 0 && (line+n) [length - 1] == '\n') (line+n) [length - 1] = '\0';
				_item [i]->setName(line+n);
			}
		}
		return 1;
	}
	char *className = NULL;
	wchar_t *objectName = NULL;
	long size = texgeti4 (text);
	init (NULL, size); cherror
	for (long i = 1; i <= size; i ++) {
		long saveVersion = Thing::version;   /* The version of the Collection... */
		Melder_free (className); className = texgets2 (text); cherror
		_item [i] = Thing::newFromClassNameA (className); cherror
		_size ++;
		if (! _item [i]->member(L"Data") || ! ((Data *)_item [i])->canReadText())
			error3 (L"Cannot read item of class ", _item [i]->className(), L" in collection.");
		Melder_free (objectName); objectName = texgetw2 (text); cherror
		_item [i]->setName(objectName); cherror
		((Data *)_item [i])->readText(text); cherror
		Thing::version = saveVersion;
	}
end:
	Melder_free (className);
	Melder_free (objectName);
	iferror return 0;
	return 1;
}

int Collection::writeBinary (FILE *f) {
	/*binputi4 (_size, f);
	for (long i = 1; i <= _size; i ++) {
		Thing *thing = _item [i];
		Thing_Table table = thing -> methods;
		if (! Thing_member (thing, classData) || ! Data_canWriteBinary (thing))
			return Melder_error3 (L"(Collection::writeBinary:) "
				"Objects of class ", table -> _className, L" cannot be written.");
		binputw1 (table -> version > 0 ?
			Melder_wcscat3 (table -> _className, L" ", Melder_integer (table -> version)) : table -> _className, f);
		binputw2 (thing -> name, f);
		if (! Data_writeBinary (thing, f)) return 0;
	}*/
	return 1;
}

int Collection::readBinary (FILE *f) {
	if (Thing::version < 0) {
		long size = bingeti4 (f);
		if (size < 0 || ! init (NULL, size)) return 0;
		for (long i = 1; i <= size; i ++) {
			char klas [200], name [2000];
			if (fscanf (f, "%s%s", klas, name) < 2 ||
				! (_item [i] = Thing::newFromClassNameA (klas))) return 0;
			Thing::version = -1;   /* Override. */
			_size ++;
			if (! _item [i]->member(L"Data"))
				return Melder_error3 (L"Collection::readBinary: "
					"cannot read item of class ", _item [i]->className(), L".");
			if (fgetc (f) != ' ' || ! ((Data *)_item [i])->readBinary(f)) return 0;
			if (strcmp (name, "?")) _item [i]->setName(Melder_peekUtf8ToWcs (name));
		}
	} else {
		long size = bingeti4 (f);
		if (! init (NULL, size)) return 0;
		for (long i = 1; i <= size; i ++) {
			long saveVersion = Thing::version;   /* The version of the Collection... */
			char *klas = bingets1 (f);
			if (! (_item [i] = Thing::newFromClassNameA (klas))) return 0;
			Melder_free (klas);
			_size ++;
			if (! _item [i]->member(L"Data") || ! ((Data *)_item[i])->canReadBinary ())
				return Melder_error3 (L"(Collection::readBinary:) "
					"Cannot read item of class ", _item [i]->className(), L".");
			wchar_t *name = bingetw2 (f);
			_item [i]->setName(name);
			Melder_free (name);
			if (! ((Data *)_item[i])->readBinary (f)) return 0;
			Thing::version = saveVersion;
		}
	}
	return 1;
}

long Collection::size () {
	return _size;
}

/*Data::Description Collection::description [] = {
	{ L"size", longwa, (intptr_t) & ((Collection *) 0) -> _size, sizeof (long) },
	{ L"item", objectwa, (intptr_t) & ((Collection *) 0) -> _item, sizeof (Data *), L"Data", & theStructData, 1, 0, L"_size" },
	{ 0 } };*/ // FIXME

long Collection::position (Thing *data) {
	(void) data;
	return _size + 1;
}

Collection::Collection (wchar_t *itemClass, long initialCapacity) {
	init(itemClass, initialCapacity);
}

int Collection::init (wchar_t *itemClass, long initialCapacity) {
	itemClass = itemClass;
	_capacity = initialCapacity >= 1 ? initialCapacity : 1;
	_size = 0;
	_item = Melder_calloc_e (Thing *, _capacity);
	if (_item == NULL) return 0;   // optimize around cherror
	_item --;   // base 1
	return 1;
}

void Collection::dontOwnItems () {
	Melder_assert (_size == 0);
	_dontOwnItems = true;
}

int Collection::insertItem (Thing *data, long pos) {
	if (_size >= _capacity) {
	/*
	 * Check without change.
	 */
		Thing **dum = (Thing **) Melder_realloc_e (_item + 1, 2 * _capacity * sizeof (Thing *));
		if (! dum) return Melder_error1 (L"(Collection_insert:) Item not inserted.");
	/*
	 * From here: change without error.
	 */
		_item = dum - 1;
		_capacity *= 2;
	}
	_size ++;
	for (long i = _size; i > pos; i --) _item [i] = _item [i - 1];
	_item [pos] = data;
	return 1;
}

int Collection::addItem (Thing *data) {
	Melder_assert (data != NULL);
	long index = position (data);
	if (index != 0) {
		return insertItem (data, index);
	} else {
		forget (data);   /* Could not insert; I am owner, so I must dispose of the data!!! */
		return 1;   /* Refusal; all right. */
	}
}

void Collection::removeItem (long pos) {
	Melder_assert (pos >= 1 && pos <= _size);
	if (! _dontOwnItems) forget (_item [pos]);
	for (long i = pos; i < _size; i ++) _item [i] = _item [i + 1];
	_size --;
}

void Collection::undangleItem (Thing *item) {
	for (long i = _size; i > 0; i --) if (_item [i] == item) {
		for (long j = i; j < _size; j ++) _item [j] = _item [j + 1];
		_size --;
	}
}

Thing *Collection::subtractItem (long pos) {
	Melder_assert (pos >= 1 && pos <= _size);
	Thing *result = _item [pos];
	for (long i = pos; i < _size; i ++) _item [i] = _item [i + 1];
	_size --;
	return result;
}

void Collection::removeAllItems () {
	if (! _dontOwnItems) for (long i = 1; i <= _size; i ++) forget (_item [i]);
	_size = 0;
}

void Collection::shrinkToFit () {
	_capacity = _size ? _size : 1;
	_item = (Thing **) Melder_realloc_f (_item + 1, _capacity * sizeof (Thing *)) - 1;
}

Collection * Collection::merge (Collection *first, Collection *second) {
	Collection *merged;
	if (first->className() != second->className()) return (Collection *)Melder_errorp5 (L"(Collections_merge:) "
		"Objects are of different class (", first->className(), L" and ", second->className(), L").");
	if (first->_dontOwnItems != second->_dontOwnItems) return (Collection *)Melder_errorp1 (L"(Collections_merge:) "
		"Cannot mix data and references.");
	merged = (Collection *)first->copy(); cherror
	for (long i = 1; i <= second->_size; i ++) {
		Thing *tmp;
		Thing *item = (Thing *) second->_item [i];
		if (first->_dontOwnItems) {
			tmp = item;
		} else {
			if (! item->member(L"Data"))
				error3 (L"(Collections_merge:) Cannot copy item of class ", item->className(), L".")
			tmp = ((Data *)item)->copy(); cherror
		}
		if (! tmp || ! merged->addItem(tmp)) { forget (tmp); goto end; }
	}
	return merged;
end:
	forget (merged);
	return (Collection *)Melder_errorp ("Collections not merged." );
}

/********** class Ordered **********/

Ordered::Ordered (wchar_t *itemClass, long initialMaximumLength)
	: Collection (itemClass, initialMaximumLength) {}

int Ordered::addItemPos (Thing *data, long position) {
	Melder_assert (data);
	if (position < 1 || position > _size) position = _size + 1;
	return insertItem (data, position);
}

/********** class Sorted **********/

Sorted::Sorted (wchar_t *itemClass, long initialMaximumLength)
	: Collection (itemClass, initialMaximumLength) {}

long Sorted::position (Thing *data) {
	if (_size == 0 || compare (data, _item [_size]) >= 0) return _size + 1;
	if (compare (data, _item [1]) < 0) return 1;
	/* Binary search. */
	long left = 1, right = _size;
	while (left < right - 1) {
		long mid = (left + right) / 2;
		if (compare (data, _item [mid]) >= 0) left = mid; else right = mid;
	}
	Melder_assert (right == left + 1);
	return right;
}

int Sorted::compare (const void *data1, const void *data2) {
	(void) data1;
	(void) data2;
	return 0;   /* All are equal. */
}

int Sorted::addItem_unsorted (Thing *data) {
	return insertItem (data, _size + 1);
}

void Sorted::sort () {
	NUMsort_p (_size, (void **)_item, (int (*) (const void *, const void *)) compare);
}

/********** class SortedSet **********/

long SortedSet::position (Thing *data) {
	if (_size == 0) return 1;   /* Empty set? 'data' is going to be the first item. */
	int where = compare (data, _item [_size]);   /* Compare with last item. */
	if (where > 0) return _size + 1;   /* Insert at end. */
	if (where == 0) return 0;
	if (compare (data, _item [1]) < 0) return 1;   /* Compare with first item. */
	long left = 1, right = _size;
	while (left < right - 1) {
		long mid = (left + right) / 2;
		if (compare (data, _item [mid]) >= 0)
			left = mid;
		else
			right = mid;
	}
	Melder_assert (right == left + 1);
	if (! compare (data, _item [left]) || ! compare (data, _item [right]))
		return 0;
	return right;
}

int SortedSet::hasItem (Thing *item) {
	return position (item) == 0;
}

/********** class SortedSetOfInt **********/

int classSortedSetOfInt_compare (SimpleInt *first, SimpleInt *second) {
	if (first->number < second->number) return -1;
	if (first->number > second->number) return +1;
	return 0;
}

/********** class SortedSetOfShort **********/

int classSortedSetOfShort_compare (SimpleShort *first, SimpleShort *second) {
	if (first->number < second->number) return -1;
	if (first->number > second->number) return +1;
	return 0;
}

/********** class SortedSetOfLong **********/

int classSortedSetOfLong_compare (SimpleLong *first, SimpleLong *second) {
	if (first->number < second->number) return -1;
	if (first->number > second->number) return +1;
	return 0;
}

/********** class SortedSetOfDouble **********/

int classSortedSetOfDouble_compare (SimpleDouble *first, SimpleDouble *second) {
	if (first->number < second->number) return -1;
	if (first->number > second->number) return +1;
	return 0;
}

/********** class SortedSetOfString **********/

int SortedSetOfString::compare (SimpleString *first, SimpleString *second) {
	return wcscmp (first->string, second->string);
}

long SortedSetOfString::lookUp (const wchar_t *string) {
	SimpleString **items = (SimpleString **) _item;
	long numberOfItems = _size;
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

int SortedSetOfString::add (const wchar_t *string) {
	static SimpleString *simp;
	long index;
	SimpleString *newSimp;
	if (! simp) { simp = new SimpleString (L""); Melder_free (simp -> string); }
	simp -> string = (wchar_t *) string;
	if ((index = position (simp)) == 0) return 1;   /* OK: already there: do not add. */
	newSimp = new SimpleString (string);
	if (! newSimp || ! insertItem (newSimp, index)) return 0;   /* Must be out of memory. */
	return 1;   /* OK: added new string. */
}

/********** class Cyclic **********/

int Cyclic::compare (Thing *first, Thing *second) {
	(void) first;
	(void) second;
	Melder_fatal ("Cyclic::compare: subclass responsibility.");
	return 0;
}

void Cyclic::cycleLeft () {
	if (_size == 0) return;
	Thing *help = _item [1];
	for (long i = 1; i < _size; i ++) _item [i] = _item [i + 1];
	_item [_size] = help;
}

void Cyclic::unicize () {
	if (_size <= 1) return;
	long lowest = 1;
	for (long i = 1; i <= _size; i ++)
		if (compare (_item [i], _item [lowest]) < 0) lowest = i;
	for (long i = 1; i < lowest; i ++)
		cycleLeft ();
}

/* End of file Collection.c */
