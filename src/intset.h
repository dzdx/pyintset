//
// Created by lxd on 2017/2/6.
//

#ifndef INTSET_INTSET_H
#define INTSET_INTSET_H

#include <stdio.h>


#define BITS_PER_WORD  64
#define BIT_PER_BLOCK  256
#define WORDS_PER_BLOCK  (BIT_PER_BLOCK / BITS_PER_WORD)

#endif //INTSET_INTSET_H


struct Block {
    long offset;
    unsigned long bits[WORDS_PER_BLOCK];
    int size;
    struct Block *prev;
    struct Block *next;

};

struct IntSet {
    struct Block *root;
};


int intset_add(struct IntSet *set, long x);

int intset_remove(struct IntSet *set, long x);

int intset_has(struct IntSet *set, long x);

int intset_len(struct IntSet *set);

long intset_max(struct IntSet *set);

long intset_min(struct IntSet *set);

struct IntSet *intset_new(long xs[], int n);

void intset_iter(struct IntSet *set, void add(long));

struct IntSet *intset_and(struct IntSet *set_a, struct IntSet *set_b);


struct IntSet *intset_or(struct IntSet *set_a, struct IntSet *set_b);

struct IntSet *intset_sub(struct IntSet *set_a, struct IntSet *set_b);
