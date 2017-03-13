# coding=utf-8

import platform
import sys
import inspect
import StringIO

from intset import IntSet

benchmark_results = []


class IntSetBenchmark(object):
    def __init__(self):
        self.result = []

    def run(self):
        for name, method in inspect.getmembers(self):
            if name.startswith("benchmark"):
                method()

    def add_report(self, name):
        pass

    def benchmark_load_iterable(self):
        pass

    def benchmark_add(self):
        pass

    def benchmark_remove(self):
        pass

    def benchmark_intersection(self):
        pass

    def benchmark_difference(self):
        pass

    def benchmark_union(self):
        pass

    def benchmark_symmetric_difference(self):
        pass

    def benchmark_update(self):
        pass

    def benchmark_subset(self):
        pass

    def benchmark_superset(self):
        pass



if __name__ == "__main__":
    b = IntSetBenchmark()
    b.run()



