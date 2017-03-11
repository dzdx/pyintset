#ifndef INTSET_INTSET_H
#define INTSET_INTSET_H


#endif //INTSET_INTSET_H

static const int BITS_PER_WORD = 64;
static const int BIT_PER_BLOCK = 256;
static const int WORDS_PER_BLOCK = (BIT_PER_BLOCK / BITS_PER_WORD);


typedef struct B {
    long offset;
    uint64_t bits[WORDS_PER_BLOCK];
    struct B *prev;
    struct B *next;

} Block;

typedef struct {
    Block *root;
} IntSet;


typedef struct{
    IntSet * set;
    Block * current_block;
    int current_index;

}IntSetIter;

IntSet* intset_copy(IntSet *set);
IntSet* intset_new();
void intset_add_array(IntSet *set, long* xs, int num);

int intset_add(IntSet *set, long x);
int intset_remove(IntSet *set, long x);
int intset_has(IntSet *set, long x);
void intset_clear(IntSet *set);

int intset_len(IntSet *set);

long intset_max(IntSet *set, int *error);
long intset_min(IntSet *set, int *error);

IntSet *intset_and(IntSet *set_a, IntSet *set_b);
IntSet *intset_or(IntSet *set_a, IntSet *set_b);
IntSet *intset_sub(IntSet *set_a, IntSet *set_b);
IntSet *intset_xor(IntSet *set_a, IntSet *set_b);
void intset_merge(IntSet *self, IntSet *other);

IntSetIter * intset_iter(IntSet * set);

long intsetiter_next(IntSetIter *iter, int *stopped);

int intset_issubset(IntSet * set1, IntSet *set2);
int intset_issuperset(IntSet *set1, IntSet *set2);
int intset_equals(IntSet * self, IntSet * other);
