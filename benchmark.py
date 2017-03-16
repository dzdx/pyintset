# coding=utf-8

import platform
import time
import sys
import inspect
import StringIO

from intset import IntSet
import random

benchmark_results = []


class IntSetBenchmark(object):
    def __init__(self):
        self.result = []

    def run(self):
        for name, method in inspect.getmembers(self):
            if name.startswith("benchmark"):
                print '-'*20+method.__name__+'-'*20
                method()

    def add_report(self, name):
        pass

    def benchmark_load_iterable(self):
        pass

    def benchmark_add(self):
        l1 = random.sample(xrange(10000), 2000)
        for clz in (IntSet, set):
            t1 = time.time()
            for _ in xrange(100):
                s1 = clz()
                for x in l1:
                    s1.add(x)
            print clz.__name__, time.time() - t1

    def benchmark_remove(self):
        l1 = random.sample(xrange(10000), 2000)
        for clz in (IntSet, set):
            t1 = time.time()
            for _ in xrange(100):
                s1 = clz(l1)
                for x in l1:
                    s1.remove(x)
            print clz.__name__, time.time() - t1

    def benchmark_intersection(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(xrange(10000), 2000)


        for clz in (IntSet, set):
            s1 = clz(l1)
            s2 = clz(l2)
            t1 = time.time()
            for _ in xrange(10000):
                s1 & s2
            print  clz.__name__, time.time() - t1


    def benchmark_difference(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(xrange(10000), 2000)


        for clz in (IntSet, set):
            s1 = clz(l1)
            s2 = clz(l2)
            t1 = time.time()
            for _ in xrange(10000):
                s1 - s2
            print clz.__name__, time.time() - t1

    def benchmark_union(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(xrange(10000), 2000)


        for clz in (IntSet, set):
            s1 = clz(l1)
            s2 = clz(l2)
            t1 = time.time()
            for _ in xrange(10000):
                s1 | s2
            print clz.__name__, time.time() - t1

    def benchmark_symmetric_difference(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(xrange(10000), 2000)


        for clz in (IntSet, set):
            s1 = clz(l1)
            s2 = clz(l2)
            t1 = time.time()
            for _ in xrange(10000):
                s1 ^ s2
            print clz.__name__, time.time() - t1


    def benchmark_update(self):
        pass

    def benchmark_subset(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 300)


        for clz in (IntSet, set):
            s1 = clz(l1)
            s2 = clz(l2)
            t1 = time.time()
            for _ in xrange(10000):
                s2.issubset(s1)
            print clz.__name__, time.time() - t1

    def benchmark_superset(self):
        l1 = random.sample(xrange(10000), 2000)
        l2 = random.sample(l1, 300)


        for clz in (IntSet, set):
            s1 = clz(l1)
            s2 = clz(l2)
            t1 = time.time()
            for _ in xrange(10000):
                s1.issuperset(s2)
            print clz.__name__, time.time() - t1


if __name__ == "__main__":
    b = IntSetBenchmark()
    b.run()



