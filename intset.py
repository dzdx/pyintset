# coding:utf-8

import cProfile
import time

import random

BITS_PER_WORD = 64
BITS_PER_BLOCK = 256
WORDS_PER_BLOCK = BITS_PER_BLOCK / BITS_PER_WORD


def _offset_and_index(x):
    base = x % BITS_PER_BLOCK
    return x - base, base


def _word_offset_and_mask(x):
    w = x / BITS_PER_WORD
    mask = 1 << (x % BITS_PER_WORD)
    return w, mask


class Block(object):
    def __init__(self):
        self.offset = None
        self.prev = None
        self.next = None
        self.bits = [0] * WORDS_PER_BLOCK
        self.size = 0

    def add(self, x):
        word_offset, mask = _word_offset_and_mask(x)
        if self.bits[word_offset] & mask == 0:
            self.bits[word_offset] |= mask
            self.size += 1
            return True
        return False

    def remove(self, x):
        word_offset, mask = _word_offset_and_mask(x)

        if self.bits[word_offset] & mask != 0:
            self.bits[word_offset] &= ~mask
            self.size -= 1
            return True
        return False

    def has(self, x):
        word_offset, mask = _word_offset_and_mask(x)
        return self.bits[word_offset] & mask != 0

    def is_empty(self):
        for i in self.bits:
            if i != 0:
                return False
        return True

    def max(self):
        for i, word in enumerate(self.bits[::-1]):
            if word == 0:
                continue
            mask = 1 << BITS_PER_WORD
            for j in xrange(BITS_PER_WORD):
                if word & mask:
                    return self.offset + (WORDS_PER_BLOCK - i - 1) * BITS_PER_WORD + (BITS_PER_WORD - j)
                mask >>= 1
        raise Exception('empty block')

    def min(self):
        for i, word in enumerate(self.bits):
            if word == 0:
                continue
            mask = 1
            for j in xrange(BITS_PER_WORD):
                if word & mask:
                    return self.offset + i * BITS_PER_WORD + j
                mask <<= 1
        raise Exception('empty block')

    def clone(self):
        block = Block()
        block.offset = self.offset
        block.bits = self.bits[:]
        return block

    def __iter__(self):
        for i, word in enumerate(self.bits):
            if word == 0:
                continue
            offset = self.offset + i * BITS_PER_WORD
            mask = 1
            for i in xrange(BITS_PER_WORD):
                if word & mask:
                    yield offset
                offset += 1
                mask <<= 1


class IntSet(object):
    def __init__(self, array=[]):
        self.root = None
        if array:
            self.add_array(array)

    def add(self, x):
        _, success = self._add(x)
        return success

    def add_array(self, array):
        array.sort()
        block = None
        for x in array:
            block, _ = self._add(x, start_block=block)

    def _add(self, x, start_block=None):
        if start_block:
            block = start_block
        else:
            block = self.start()
        offset, index = _offset_and_index(x)
        while block != self.root and block.offset <= offset:
            if block.offset == offset:
                return block, block.add(index)
            block = block.next

        new_block = Block()
        new_block.offset = offset
        new_block.next = block
        new_block.prev = block.prev
        new_block.next.prev = new_block
        new_block.prev.next = new_block
        return new_block, new_block.add(index)

    def remove(self, x):

        offset, index = _offset_and_index(x)
        block = self.block(offset)
        if block:
            if not block.remove(index):
                return False
            if block.is_empty():
                self.remove_block(block)
            return True
        return False

    def has(self, x):
        offset, index = _offset_and_index(x)
        block = self.block(offset)
        if block:
            return block.has(index)
        return False

    def start(self):
        if not self.root:
            block = Block()
            block.prev = block
            block.next = block
            self.root = block
        return self.root.next

    def block(self, offset):

        block = self.start()
        while block != self.root and block.offset <= offset:
            if block.offset == offset:
                return block
            block = block.next

    def remove_block(self, block):
        block.prev.next = block.next
        block.next.prev = block.prev

    def clear(self):
        self.root.next = self.root
        self.root.prev = self.root

    def is_empty(self):
        return self.start() == self.root

    def max(self):
        if self.is_empty():
            raise Exception('empty set')
        return self.root.prev.max()

    def min(self):
        if self.is_empty():
            raise Exception('empty set')
        return self.root.next.min()

    def copy(self):
        pass

    def __contains__(self, x):
        return self.has(x)

    def __len__(self):
        count = 0
        block = self.start()
        while block != self.root:
            count += block.size
            block = block.next
        return count

    def __iter__(self):
        block = self.start()
        while block != self.root:

            for x in block:
                yield x
            block = block.next

    def __and__(self, other):

        self_block = self.start()
        other_block = other.start()
        result_set = IntSet()
        result_block = result_set.start()

        while self_block != self.root and other_block != other.root:

            if self_block.offset < other_block.offset:
                self_block = self_block.next
            elif self_block.offset > other_block.offset:
                other_block = other_block.next
            else:
                block = Block()
                block.offset = self_block.offset
                words = [0] * WORDS_PER_BLOCK
                for i in range(WORDS_PER_BLOCK):
                    words[i] = self_block.bits[i] & other_block.bits[i]

                self_block = self_block.next
                other_block = other_block.next

                if words == [0] * WORDS_PER_BLOCK:
                    continue

                block.bits = words
                block.prev = result_block
                block.next = result_set.root

                result_block.next = block
                result_block = block
                result_set.root.prev = block

        return result_set

    def __or__(self, other):
        self_block = self.start()
        other_block = other.start()
        result_set = IntSet()
        result_block = result_set.start()

        while other_block != other.root or self_block != self.root:
            block = None
            if self_block == self.root:
                block = other_block.clone()
                other_block = other_block.next
            elif other_block == other.root:
                block = self_block.clone()
                self_block = self_block.next
            elif self_block.offset == other_block.offset:
                block = Block()
                block.offset = self_block.offset
                words = [0] * WORDS_PER_BLOCK
                for i in range(WORDS_PER_BLOCK):
                    words[i] = self_block.bits[i] | other_block.bits[i]
                block.bits = words

                self_block = self_block.next
                other_block = other_block.next

            elif self_block.offset > other_block.offset:
                block = other_block.clone()
                other_block = other_block.next
            elif self_block.offset < other_block.offset:
                block = self_block
                self_block = self_block.next

            block.prev = result_block
            block.next = result_set.root

            result_block.next = block

            result_block = block
            result_set.root.prev = block

        return result_set

    def __sub__(self, other):
        self_block = self.start()
        other_block = other.start()
        result_set = IntSet()
        result_block = result_set.start()

        while self_block != self.root:
            block = None

            if self_block.offset < other_block.offset or other_block == other.root:
                block = self_block.clone()
                self_block = self_block.next
            elif self_block.offset > other_block.offset:
                other_block.next = other_block
                continue
            else:
                block = Block()
                block.offset = self_block.offset
                words = [0] * WORDS_PER_BLOCK
                for i in range(WORDS_PER_BLOCK):
                    words[i] = self_block.bits[i] & ~other_block.bits[i]

                self_block = self_block.next
                other_block = other_block.next

                if words == [0] * WORDS_PER_BLOCK:
                    continue

                block.bits = words

            block.prev = result_block
            block.next = result_set.root

            result_block.next = block

            result_block = block
            result_set.root.prev = block

        return result_set

    def __eq__(self, other):
        self_block = self.start()
        other_block = other.start()
        while True:
            if self_block == self.root and other_block == other.root:
                return True
            elif self_block == self.root or other_block == other.root:
                return False
            elif self_block.offset != other_block.offset or self_block.bits != other_block.bits:
                return False
            else:
                self_block = self_block.next
                other_block = other_block.next


def profile():
    l1 = random.sample(xrange(50000), 10000)
    l2 = random.sample(xrange(50000), 10000)

    intset1 = IntSet(l1)
    intset2 = IntSet(l2)

    s1 = set(l1)
    s2 = set(l2)

    # for x in range(1000):
    # intset1 & intset2


if __name__ == '__main__':
    cProfile.run("profile()", sort="cumtime")
