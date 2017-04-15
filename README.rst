===============================
pyintset
===============================


.. image:: https://img.shields.io/pypi/v/pyintset.svg
        :target: https://pypi.python.org/pypi/pyintset

.. image:: https://img.shields.io/travis/dzdx/pyintset.svg
        :target: https://travis-ci.org/dzdx/pyintset
        :alt: Updates


pyintset is a set for storing integers.
pyintset can store any length of integer numbers, support Python int and long type.

it's intersection, union and difference the operation faster than the python standard library set, but the add and remove operations slower than the set.


* Free software: MIT license



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
pyintset supports most methods of set. In addition, because it is stored in order, it is also supported get_item and get_slice, max, min
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
  

benchmark
--------------
Test machine:
^^^^^^^^^^^^^^
Linux 3.2.0-23-generic x86_64 #36-Ubuntu SMP Tue Apr 10 20:39:51 UTC 2012

Python Versions:
^^^^^^^^^^^^^^^^
CPython 2.7.12 (default, Jul 18 2016, 14:59:49) [GCC 4.6.3]

======================================== ======================================== ========================================
benchmark                                IntSet                                   set
======================================== ======================================== ========================================
benchmark_add                            0.0082049369812                          0.00174403190613
benchmark_difference                     0.0508291721344                          1.43595719337
benchmark_discard                        0.00447106361389                         0.00148987770081
benchmark_intersection                   0.0479021072388                          2.73467516899
benchmark_issubset                       0.00648403167725                         0.0442490577698
benchmark_issuperset                     0.00768518447876                         0.0346069335938
benchmark_symmetric_difference           0.0477960109711                          2.45564603806
benchmark_union                          0.0476861000061                          2.07998609543
benchmark_update                         0.622309923172                           0.059406042099
======================================== ======================================== ========================================


memory profile
---------------

::

    Filename: memory_profile.py

    Line #    Mem usage    Increment   Line Contents
    ================================================
        13   35.633 MiB    0.000 MiB   @profile
        14                             def create_intsets():
        15   43.516 MiB    7.883 MiB       data = [IntSet(items) for _ in range(1000)]


    Filename: memory_profile.py

    Line #    Mem usage    Increment   Line Contents
    ================================================
        16   43.516 MiB    0.000 MiB   @profile
        17                             def create_long_intsets():
        18   44.805 MiB    1.289 MiB       data = [IntSet(long_items) for _ in range(1000)]


    Filename: memory_profile.py

    Line #    Mem usage    Increment   Line Contents
    ================================================
        20   33.465 MiB    0.000 MiB   @profile
        21                             def create_sets():
        22  158.652 MiB  125.188 MiB       data = [set(items) for _ in range(1000)]


    Filename: memory_profile.py

    Line #    Mem usage    Increment   Line Contents
    ================================================
        24   35.652 MiB    0.000 MiB   @profile
        25                             def create_long_sets():
        26  158.652 MiB  123.000 MiB       data = [set(long_items) for _ in range(1000)]



api
------

==============================================            =========
method example                                             doc
==============================================            =========
s.add( (<int>|<long>) )                                    Add an Integer to a intset. return None.
s.remove( (<int>|<long>) )                                 Remove an Integer from a intset. If the Integer is not a member, raise a KeyError.
s.discard( (<int>|<long>) )                                 Remove an Integer from a intset.\n If the Integer is not a member, do noting.
s.max()                                                    Get the max Integer in a intset.\n If the intset is empty, raise a ValueError.
s.min()                                                    Get the min Integer in a intset.\n If the intset is empty, raise a ValueError.
s.clear()                                                  Remove all elements from this intset.
s.copy()                                                   Return a copy of a intset.
s.issubset(<IntSet>)                                       Report whether another intset contains this intset.
s.issuperset(<IntSet>)                                     Report whether this intset contains another intset.
s.intersection(<iterable>)                                 Return the intersection of an intset and an iterable object as a new intset.
s.intersection_update(<iterable>)                          Update a intset with the intersection of itself and other iterable object.
s.union(<iterable>)                                        Return the union of an intset and an iterable object as a new intset.
s.update(<iterable>)                                       Update a intset with the union of itself and other iterable object.
s.difference(<iterable>)                                   Return the difference of an intset and an iterable object as a new intset.
s.difference_update(iterable>)                             Update a intset with the difference of itself and other iterable object.
s.symmetric_difference(<iterable>)                         Return the symmetric_difference of an intset and an iterable object as a new intset.
s.symmetric_difference_update(<iterable>)                  Update a intset with the symmetric_difference of itself and other iterable object.
s&<IntSet>                                                 Similar to intersection, but only accept an intset
s|<IntSet>                                                 Similar to union, but only accept an intset                    
s^<IntSet>                                                 Similar to symmetric_difference, but only accept an intset
s-<IntSet>                                                 Similar to difference, but only accept an intset
(<int>|<long) in s                                         Report whether an integer is a member of this intset
len(s)                                                     Return the num of members in 
s[2]                                                       Return an integer by index
s[1:10]                                                    Return a slice intset  by range
iter(s)                                                    Iter this intset, return member is it one by one
s (`<|<=|==|>|>=`) <IntSet>                                  <= is similar to issubset, >= is similar to issuperset
==============================================            =========


Credits
---------

This package was created with Cookiecutter_ and the `audreyr/cookiecutter-pypackage`_ project template.

.. _Cookiecutter: https://github.com/audreyr/cookiecutter
.. _`audreyr/cookiecutter-pypackage`: https://github.com/audreyr/cookiecutter-pypackage
