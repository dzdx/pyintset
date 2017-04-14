#!/usr/bin/env python
# coding=utf-8



from intset import IntSet
import random


items = random.sample(range(20000), 2000)
long_items = random.sample(range(1<<100, (1<<100)+20000), 2000)

@profile
def create_intsets():
    data = [IntSet(items) for _ in range(1000)]
@profile
def create_long_intsets():
    data = [IntSet(long_items) for _ in range(1000)]

@profile
def create_sets():
    data = [set(items) for _ in range(1000)]

@profile
def create_long_sets():
    data = [set(long_items) for _ in range(1000)]


if __name__ == '__main__':
    create_sets()
    create_long_sets()
    create_intsets()
    create_long_intsets()

