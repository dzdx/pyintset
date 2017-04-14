# coding=utf-8

import sys
import platform
import timeit
import inspect

from intset import IntSet
import random

benchmark_results = []


clzs = (IntSet, set)


def benchmark_wrap(func):
    def _(*args, **kwargs):
        durations = []
        for caller in  func(*args, **kwargs):
            durations.append(min(timeit.repeat(caller, repeat=3, number=10000)))
        benchmark_results.append((func.__name__, durations))
    return _


class IntSetBenchmark(object):

    def run(self):
        for name, method in inspect.getmembers(self):
            if name.startswith("benchmark"):
                method()

    def output_report(self):
        uname_system, _, uname_release, uname_version, _, uname_processor = platform.uname()
        print("Test machine:")
        print("^^^^^^^^^^^^^^")
        print(" ".join([uname_system, uname_release, uname_processor, uname_version]))
        print("")
        print("Python Versions:")
        print("^^^^^^^^^^^^^^")
        print(" ".join([platform.python_implementation(), sys.version.replace("\n", "")]))
        print("")
        print("{:=<40} {:=<40} {:=<40}".format("", "", ""))
        print("{:<40} {:<40} {:<40}".format("benchmark", "IntSet", "set"))
        print("{:=<40} {:=<40} {:=<40}".format("", "", ""))
        for name, durations in benchmark_results:
            print("{:<40} {:<40} {:<40}".format(name, durations[0], durations[1]))
        print("{:=<40} {:=<40} {:=<40}".format("", "", ""))



    @benchmark_wrap
    def benchmark_add(self):
        l1 = random.sample(range(10000), 2000)
        v = 10001
        for clz in clzs:
            s = clz(l1)
            yield lambda:s.add(v)

    @benchmark_wrap
    def benchmark_discard(self):
        l1 = random.sample(range(10000), 2000)
        v = 2000
        for clz in clzs:
            s = clz(l1)
            yield lambda:s.discard(v)


    @benchmark_wrap
    def benchmark_intersection(self):
        l1 = random.sample(range(10000), 2000)
        l2 = random.sample(range(10000), 2000)
        for clz in clzs:
            s1 = clz(l1)
            s2 = clz(l2)
            yield lambda:s1|s2


    @benchmark_wrap
    def benchmark_difference(self):
        l1 = random.sample(range(10000), 2000)
        l2 = random.sample(range(10000), 2000)
        for clz in clzs:
            s1 = clz(l1)
            s2 = clz(l2)
            yield lambda:s1-s2


    @benchmark_wrap
    def benchmark_union(self):
        l1 = random.sample(range(10000), 2000)
        l2 = random.sample(range(10000), 2000)
        for clz in clzs:
            s1 = clz(l1)
            s2 = clz(l2)
            yield lambda:s1|s2

    @benchmark_wrap
    def benchmark_symmetric_difference(self):
        l1 = random.sample(range(10000), 2000)
        l2 = random.sample(range(10000), 2000)
        for clz in clzs:
            s1 = clz(l1)
            s2 = clz(l2)
            yield lambda:s1^s2

    @benchmark_wrap
    def benchmark_update(self):
        l1 = random.sample(range(10000), 2000)
        l2 = random.sample(range(10000), 200)
        for clz in clzs:
            s1 = clz(l1)
            yield lambda:s1.update(l2)

    @benchmark_wrap
    def benchmark_issubset(self):
        l1 = random.sample(range(10000), 2000)
        l2 = random.sample(l1, 300)
        for clz in (IntSet, set):
            s1 = clz(l1)
            s2 = clz(l2)
            yield lambda:s2.issubset(s1)

    @benchmark_wrap
    def benchmark_issuperset(self):
        l1 = random.sample(range(10000), 2000)
        l2 = random.sample(l1, 300)
        for clz in clzs:
            s1 = clz(l1)
            s2 = clz(l2)
            yield lambda:s1.issuperset(s2)



if __name__ == "__main__":
    b = IntSetBenchmark()
    b.run()
    b.output_report()



