#ifndef INTSET_INTSET_H
#define INTSET_INTSET_H

#endif //INTSET_INTSET_H

#include "number.h"


#ifdef UINT64_MAX
typedef uint64_t Word;
#define BITS_PER_WORD 64
#else
typedef uint32_t Word;
#define BITS_PER_WORD 32
#endif

#define BLOCK_LEN  8
#define BITS_PER_BLOCK (1<<8)
#define WORDS_PER_BLOCK (BITS_PER_BLOCK / BITS_PER_WORD)


typedef struct B {
    Number * offset;
    Word bits[WORDS_PER_BLOCK];
    struct B *prev;
    struct B *next;

} Block;

typedef struct {
    Block *root;
} IntSet;


typedef struct {
    IntSet *set;
    Block *current_block;
    int current_index;

} IntSetIter;

IntSet *intset_copy(IntSet *set);

IntSet *intset_new(void);

void intset_add_array(IntSet *set, Number **xs, int num);

int intset_add(IntSet *set, Number* x);

int intset_remove(IntSet *set, Number* x);

int intset_has(IntSet *set, Number* x);

void intset_clear(IntSet *set);

int intset_len(IntSet *set);

Number* intset_max(IntSet *set, int *error);

Number* intset_min(IntSet *set, int *error);

IntSet *intset_and(IntSet *set_a, IntSet *set_b);

IntSet *intset_or(IntSet *set_a, IntSet *set_b);

IntSet *intset_sub(IntSet *set_a, IntSet *set_b);

IntSet *intset_xor(IntSet *set_a, IntSet *set_b);

void intset_merge(IntSet *self, IntSet *other);

IntSetIter *intset_iter(IntSet *set);

Number* intsetiter_next(IntSetIter *iter, int *stopped);

Number* intset_get_item(IntSet *set, int index, int *error);

IntSet *intset_get_slice(IntSet *set, int start, int end);

int intset_issubset(IntSet *set1, IntSet *set2);

int intset_issuperset(IntSet *set1, IntSet *set2);

int intset_equals(IntSet *self, IntSet *other);
