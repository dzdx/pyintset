pyintset
=========
pyintset is a set for storing integers.

it's intersection, union and difference the operation faster than the python standard library set, but the add and remove operations slower than the set.

pyintset can store arbitrary length of integer numbers, support Python int and long type

usage
-----

init
^^^^^
pyintset can init by set, list, dict and any other iterable collection, include it self.
::
 
  >>>from intset import IntSet
  >>> IntSet(range(10))
  IntSet([0, 1, 2, 3, 4, 5, 6, 7, 8, 9])
  >>> IntSet(range(1<<100,(1<<100)+5))
  IntSet([1267650600228229401496703205376L, 1267650600228229401496703205377L, 1267650600228229401496703205378L, 1267650600228229401496703205379L, 1267650600228229401496703205380L])
  >>> IntSet(xrange(10))
  IntSet([0, 1, 2, 3, 4, 5, 6, 7, 8, 9])
  >>> IntSet(set(range(10,20)))
  IntSet([10, 11, 12, 13, 14, 15, 16, 17, 18, 19])
  >>> IntSet(IntSet(range(10,20)))
  IntSet([10, 11, 12, 13, 14, 15, 16, 17, 18, 19])
  
method
^^^^^^^^
pyintset supports most methods of set. In addition, because it is stored in order, it is also supported get_item and get_slice
::

  >>> IntSet(range(0,20))&IntSet(range(15,25))
  IntSet([15, 16, 17, 18, 19])
  >>> IntSet(range(0,20)).intersection(IntSet(range(15,25)))
  IntSet([15, 16, 17, 18, 19])
  >>> s = IntSet(range(0, 20))
  >>> s.intersection_update(IntSet(range(5,25)))
  >>> s
  IntSet([5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19])
  >>> s.intersection_update(range(6, 25))
  >>> s
  IntSet([6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19])
  >>> s = IntSet(range(10))
  >>> s.update(range(1<<64, (1<<64)+5))
  >>> s
  IntSet([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 18446744073709551616L, 18446744073709551617L, 18446744073709551618L, 18446744073709551619L, 18446744073709551620L])
  >>> IntSet(range(200, 300))[10:30]
  IntSet([210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229])
  >>> IntSet(range(200, 300))[25]
  225
  
api
^^^^

==============================================            =========
method example                                             doc
==============================================            =========
s.add( (<int>|<long>) )
s.remove( (<int>|<long>) )
s.discard( <int>|<long>) )
s.max()
s.min()
s.clear()
s.copy()
s.issubset(<IntSet>)
s.issuperset(<IntSet>)
s.intersection(<iterable>)
s.intersection_update(<iterable>)
s.union(<iterable>)
s.update(<iterable>)
s.difference(<iterable>)
s.difference_update(iterable>)
s.symmetric_difference(<iterable>)
s.symmetric_difference_update(<iterable>)
s&<IntSet>
s|<IntSet>
s^<IntSet>
s-<IntSet>
(<int>|<long) in s
len(s)
s[2]
s[1:10]
[x for x in s]
s (<|<=|==|>|>=) <IntSet>

==============================================            =========


