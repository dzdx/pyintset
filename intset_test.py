import unittest

import cProfile

from intset import IntSet
import random
import time


class IntSetTestCase(unittest.TestCase):

    def test_iter(self):
        data = random.sample(xrange(10000), 2000)
        s = IntSet(data)
        self.assertListEqual(list(s), sorted(data))

    def test_add(self):
        l1 = random.sample(xrange(10000), 2000)
        s = IntSet()
        for x in l1:
            s.add(x)
        self.assertListEqual(list(s), sorted(l1))

    def test_contains(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 30)
        s = IntSet(l1)
        for x in l2:
            self.assertIn(x, s)

    def test_clear(self):
         s = IntSet(random.sample(xrange(10000), 2000))
         s.clear()
         self.assertListEqual(list(s), [])

    def test_len(self):
        s = IntSet(random.sample(xrange(10000), 2000))
        self.assertEqual(len(s), 2000)

    def test_min(self):
        l1 = random.sample(xrange(10000), 2000)
        s = IntSet(l1)
        self.assertEqual(s.min(), min(l1))

    def test_max(self):
        l1 = random.sample(xrange(10000), 2000)
        s = IntSet(l1)
        self.assertEqual(s.max(), max(l1))

    def test_and(self):

        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(xrange(10000), 2000)

        s1 = IntSet(l1)
        s2 = IntSet(l2)
        self.assertListEqual(list(s1 & s2), sorted(list(set(l1) & set(l2))))

    def test_or(self):

        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(xrange(10000), 2000)

        s1 = IntSet(l1)
        s2 = IntSet(l2)
        self.assertListEqual(list(s1 | s2), sorted(list(set(l1) | set(l2))))


    def test_sub(self):

        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(xrange(10000), 2000)

        s1 = IntSet(l1)
        s2 = IntSet(l2)

        self.assertEqual(list(s1 - s2), sorted(list(set(l1) - set(l2))))

    def test_xor(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(xrange(10000), 2000)

        s1 = IntSet(l1)
        s2 = IntSet(l2)
        self.assertEqual(list(s1^s2), sorted(list(set(l1)^set(l2))))
    def test_cmp(self):
        l1 = random.sample(xrange(10000), 2000)

        s1 = IntSet(l1)
        s2 = IntSet(l1)

        self.assertEqual(s1, s2)
        self.assertTrue(s1 >= s2)
        self.assertTrue(s1 <= s2)

        s2.add(10001)

        self.assertNotEqual(s1, s2)
        self.assertTrue(s2 > s1)
        self.assertTrue(s1 < s2)
        self.assertTrue(s2 >= s1)
        self.assertTrue(s1 <= s2)


    def test_intersection(self):
        pass

    def test_intersection_update(self):
        pass


    def test_difference(self):
        pass

    def test_difference_update(self):
        pass

    def test_union(self):
        pass

    def test_update(self):
        pass

    def test_symmetric_difference(self):
        pass

    def test_symmetric_difference_update(self):
        pass

    def test_issubset(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 1000)
        s1 = IntSet(l2)
        s2 = IntSet(l1)
        self.assertTrue(s1.issubset(s2))

    def test_issuperset(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 1000)
        s1 = IntSet(l1)
        s2 = IntSet(l2)
        self.assertTrue(s1.issuperset(s2))


    def test_remove(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 300)
        s1 = IntSet(l1)
        s2 = set(l1)

        for x in l2:
            s1.remove(x)
            s2.remove(x)
        self.assertListEqual(list(s1), sorted(s2))
        self.assertRaises(KeyError, s1.remove, 10001)

    def test_discard(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 300)
        s1 = IntSet(l1)
        s2 = set(l1)

        for x in l2:
            s1.discard(x)
            s2.discard(x)

        self.assertListEqual(list(s1), sorted(s2))
        s1.discard(10001)

    def test_copy(self):
        l1 = random.sample(xrange(10000), 2000)
        s1 = IntSet(l1)
        s2 = s1.copy()
        self.assertEqual(s1, s2)


if __name__ == '__main__':
    unittest.main()
