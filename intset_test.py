import unittest

import cProfile

from intset import IntSet
import random
import time


class IntSetTestCase(unittest.TestCase):
    def test_iter(self):

        data = random.sample(xrange(10000), 2000)
        s = IntSet(data)
        assert list(s) == sorted(data)

    def test_add(self):
        l1 = random.sample(xrange(10000), 2000)
        s = IntSet()
        for x in l1:
            s.add(x)
        assert list(s) == sorted(l1)

    def test_remove(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 300)
        s1 = IntSet(l1)
        s2 = set(l1)

        for x in l2:
            s1.remove(x)
            s2.remove(x)
        assert list(s1) == sorted(s2)

    def test_discard(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 300)
        s1 = IntSet(l1)
        s2 = set(l1)

        for x in l2:
            s1.discard(x)
            s2.discard(x)
        assert list(s1) == sorted(s2)

    def test_contains(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 30)
        s = IntSet(l1)
        for x in l2:
            assert x in s

    def test_clear(self):
         s = IntSet(random.sample(xrange(10000), 2000))
         s.clear()
         assert list(s) == []

    def test_len(self):
        s = IntSet(range(100))
        assert len(s) == 100

    def test_min(self):
        l1 = random.sample(xrange(10000), 2000)
        s = IntSet(l1)
        assert s.min() == min(l1)

    def test_max(self):
        l1 = random.sample(xrange(10000), 2000)
        s = IntSet(l1)
        assert s.max() == max(l1)

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

    def test_benchmark_and(self):
        l1 = random.sample(xrange(20000), 2000)
        l2 = random.sample(xrange(20000), 2000)

        is1 = IntSet(l1)
        is2 = IntSet(l2)

        s1 = set(l1)
        s2 = set(l2)

        t1 = time.time()
        for _ in range(10000):
            is1&is2
        t2 = time.time()
        for _ in range(10000):
            s1&s2
        t3 = time.time()
        print "and", t2-t1, t3-t2

    def test_benchmark_or(self):
        l1 = random.sample(xrange(20000), 2000)
        l2 = random.sample(xrange(20000), 2000)

        is1 = IntSet(l1)
        is2 = IntSet(l2)

        s1 = set(l1)
        s2 = set(l2)

        t1 = time.time()
        for _ in range(10000):
            is1|is2
        t2 = time.time()
        for _ in range(10000):
            s1|s2
        t3 = time.time()
        print "or", t2-t1, t3-t2


    def test_benchmark_sub(self):
        l1 = random.sample(xrange(20000), 2000)
        l2 = random.sample(xrange(20000), 2000)

        is1 = IntSet(l1)
        is2 = IntSet(l2)

        s1 = set(l1)
        s2 = set(l2)

        t1 = time.time()
        for _ in range(10000):
            is1-is2
        t2 = time.time()
        for _ in range(10000):
            s1-s2
        t3 = time.time()
        print "sub", t2-t1, t3-t2



if __name__ == '__main__':
    unittest.main()
