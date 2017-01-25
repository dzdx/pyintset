import unittest

import cProfile

from intset import IntSet
import random
import time


class IntSetTestCase(unittest.TestCase):
    def test_add(self):
        l1 = random.sample(xrange(10000), 2000)
        s = IntSet()
        for x in l1:
            s.add(x)
        assert list(s) == sorted(set(l1))

    def test_add_array(self):
        l1 = random.sample(xrange(10000), 2000)
        s = IntSet()
        s.add_array(l1)
        assert list(s) == sorted(set(l1))

    def test_remove(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 300)
        s1 = IntSet(l1)
        s2 = set(l1)

        for x in l2:
            s1.remove(x)
            s2.remove(x)
        assert list(s1) == sorted(s2)

    def test_has(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 30)
        s = IntSet(l1)
        for x in l2:
            assert s.has(x) == True
            assert x in s

    def test_clear(self):
        s = IntSet(random.sample(xrange(10000), 2000))
        s.clear()
        assert list(s) == []

    def test_len(self):
        s = IntSet(range(100))
        s.add_array(range(100, 200))
        assert len(s) == 200

    def test_min(self):
        s = IntSet(range(100, 1000))

        assert s.min() == 100

    def test_max(self):
        s = IntSet(range(100, 1000))
        assert s.max() == 999

    def test_and(self):

        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(xrange(10000), 2000)

        s1 = IntSet(l1)
        s2 = IntSet(l2)
        assert list(s1 & s2) == sorted(list(set(l1) & set(l2)))

    def test_or(self):

        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(xrange(10000), 2000)

        s1 = IntSet(l1)
        s2 = IntSet(l2)
        assert list(s1 | s2) == sorted(list(set(l1) | set(l2)))

    def test_sub(self):

        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(xrange(10000), 2000)

        s1 = IntSet(l1)
        s2 = IntSet(l2)
        assert list(s1 - s2) == sorted(list(set(l1) - set(l2)))

    def test_equal(self):
        l1 = random.sample(xrange(10000), 2000)

        s1 = IntSet(l1)
        s2 = IntSet(l1)
        assert s1 == s2


if __name__ == '__main__':
    unittest.main()
