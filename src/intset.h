#ifndef INTSET_INTSET_H
#define INTSET_INTSET_H


#endif //INTSET_INTSET_H

static const int BITS_PER_WORD = 64;
static const int BIT_PER_BLOCK = 256;
static const int WORDS_PER_BLOCK = (BIT_PER_BLOCK / BITS_PER_WORD);


typedef struct B {
    long offset;
    unsigned long bits[WORDS_PER_BLOCK];
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

IntSet* intset_copy(IntSet *set);

int intset_add(IntSet *set, long x);
int intset_remove(IntSet *set, long x);
int intset_has(IntSet *set, long x);
void intset_clear(IntSet *set);

int intset_len(IntSet *set);

void intset_max(IntSet *set, long *result, int *error);
void intset_min(IntSet *set, long *result, int *error);

IntSet *intset_and(IntSet *set_a, IntSet *set_b);
IntSet *intset_or(IntSet *set_a, IntSet *set_b);
IntSet *intset_sub(IntSet *set_a, IntSet *set_b);
IntSet *intset_xor(IntSet *set_a, IntSet *set_b);
void intset_merge(IntSet *self, IntSet *other);

IntSetIter * intset_iter(IntSet * set);

void intsetiter_next(IntSetIter *iter, long *val, int *stopped);

int intset_issubset(IntSet * set1, IntSet *set2);
int intset_issuperset(IntSet *set1, IntSet *set2);
int intset_equals(IntSet * self, IntSet * other);
