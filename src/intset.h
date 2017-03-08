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


typedef struct B {
    long offset;
    unsigned long bits[WORDS_PER_BLOCK];
    int size;
    struct B *prev;
    struct B *next;

} Block;

typedef struct {
    Block *root;
} IntSet;



typedef struct{
    IntSet * set;
    Block * current_block;
    unsigned int current_index;

}IntSetIter;

int intset_add(IntSet *set, long x);

int intset_remove(IntSet *set, long x);

int intset_has(IntSet *set, long x);

int intset_len(IntSet *set);

void intset_max(IntSet *set, long *result, int *error);

void intset_min(IntSet *set, long *result, int *error);

IntSet *intset_new(long xs[], int n);

Block *intset_start(IntSet *set);

int Block_next(Block *block, unsigned int index);

IntSet *intset_and(IntSet *set_a, IntSet *set_b);


IntSet *intset_or(IntSet *set_a, IntSet *set_b);

IntSet *intset_sub(IntSet *set_a, IntSet *set_b);


void intset_clear(IntSet *set);

IntSetIter * intset_iter(IntSet * set);

void intsetiter_next(IntSetIter *iter, long *val, int *stopped);

