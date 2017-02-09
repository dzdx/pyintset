//
// Created by lxd on 2017/2/6.
//

#include <stdlib.h>
#include <string.h>
#include "intset.h"


void word_offset_and_mask(int index, int *word_offset, unsigned long *mask) {
    *word_offset = index / BITS_PER_WORD;
    *mask = (unsigned long) 1 << (index % BITS_PER_WORD);
}


int block_add(struct Block *block, unsigned int index) {
    int word_offset;
    unsigned long mask;
    word_offset_and_mask(index, &word_offset, &mask);
    if ((block->bits[word_offset] & mask) == 0) {
        block->bits[word_offset] |= mask;
        block->size += 1;
        return 1;
    }
    return 0;
}

int block_remove(struct Block *block, unsigned int index) {
    int word_offset;
    unsigned long mask;
    word_offset_and_mask(index, &word_offset, &mask);
    if ((block->bits[word_offset] & mask) != 0) {
        block->bits[word_offset] &= ~mask;
        block->size -= 1;
        return 1;
    }
    return 0;
}


void block_iter(struct Block *block, void add(long)) {
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        unsigned long word = block->bits[i];
        if (word == 0)continue;

        long offset = block->offset + i * BITS_PER_WORD;
        unsigned long mask = 1;
        for (int i = 0; i < BITS_PER_WORD; i++) {
            if (word & mask) {
                add(offset);
            }
            offset++;
            mask <<= 1;
        }
    }
}


int block_is_empty(struct Block *block) {
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        if (block->bits[i] != 0)
            return 0;
    }
    return 1;
}


void offset_and_index(long x, long *offset, unsigned int *index) {
    unsigned int base = (unsigned int) (int) (x % BIT_PER_BLOCK);
    *offset = x - base;
    *index = base;
}


struct IntSet *intset_new(long xs[], int n) {
    struct IntSet *s = malloc(sizeof(struct IntSet));
    s->root = NULL;
    for (int i = 0; i < n; i++) {
        intset_add(s, xs[i]);
    }
    return s;
}

struct Block *intset_start(struct IntSet *set);


int intset_add(struct IntSet *set, long x) {
    struct Block *block = intset_start(set);
    long offset;
    unsigned int index;
    offset_and_index(x, &offset, &index);
    for (; block != set->root && block->offset <= offset;) {
        if (block->offset == offset) {
            return block_add(block, index);
        }
        block = block->next;
    }

    struct Block *new_block = calloc(1, sizeof(struct Block));

    new_block->offset = offset;
    new_block->prev = block->prev;
    new_block->next = block;

    new_block->next->prev = new_block;
    new_block->prev->next = new_block;
    return block_add(new_block, index);
}

struct Block *intset_get_block(struct IntSet *set, long offset);

void remove_block(struct Block *pBlock);

int intset_remove(struct IntSet *set, long x) {
    long offset;
    unsigned int index;
    offset_and_index(x, &offset, &index);
    struct Block *block = intset_get_block(set, offset);

    if (block != NULL) {
        if (!block_remove(block, index))
            return 0;
        if (block_is_empty(block))
            remove_block(block);
        return 1;
    }
    return 0;
}

int block_has(struct Block *block, int index) {
    int word_offset;
    unsigned long mask;
    word_offset_and_mask(index, &word_offset, &mask);
    if ((block->bits[word_offset] & mask) != 0)
        return 1;
    return 0;

}

int intset_has(struct IntSet *set, long x) {
    long offset;
    unsigned int index;
    offset_and_index(x, &offset, &index);
    struct Block *block = intset_get_block(set, offset);
    if (block != NULL)
        return block_has(block, index);
    return 0;
}

void remove_block(struct Block *block) {
    block->prev->next = block->next;
    block->next->prev = block->prev;
    free(block);
}


struct Block *intset_get_block(struct IntSet *set, long offset) {
    for (struct Block *block = intset_start(set);
         block != set->root && block->offset <= offset; block = block->next)
        if (block->offset == offset)
            return block;
    return NULL;
}

void intset_iter(struct IntSet *set, void add(long)) {
    for (struct Block *block = intset_start(set); block != set->root; block = block->next)
        block_iter(block, add);
}


struct Block *intset_start(struct IntSet *set) {
    if (set->root == NULL) {
        struct Block *block = calloc(1, sizeof(struct Block));
        block->prev = block;
        block->next = block;
        set->root = block;

    }
    return set->root->next;
}


int intset_len(struct IntSet *set) {
    int count = 0;
    for (struct Block *block = intset_start(set); block != set->root; block = block->next)
        count += block->size;
    return count;
}


int intset_is_empty(struct IntSet *set) {
    //TODO
    return 0;
}


long block_max(struct Block *block) {
    for (int i = WORDS_PER_BLOCK - 1; i >= 0; i--) {
        unsigned long word = block->bits[i];
        if (word == 0)continue;
        unsigned long mask = (unsigned long) 1 << (BITS_PER_WORD - 1);
        for (int j = 0; j < BITS_PER_WORD; j++) {
            if (word & mask)
                return block->offset + i * BITS_PER_WORD + BITS_PER_WORD - j - 1;
            mask >>= 1;
        }
    }
}

long block_min(struct Block *block) {
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        unsigned long word = block->bits[i];
        if (word == 0)continue;
        unsigned long mask = (unsigned long) 1;
        for (int j = 0; j < BITS_PER_WORD; j++) {
            if (word & mask)
                return block->offset + i * BITS_PER_WORD + j;
            mask <<= 1;
        }
    }
}

long intset_max(struct IntSet *set) {
    //TODO 异常处理
    return block_max(set->root->prev);
}

long intset_min(struct IntSet *set) {
    //TODO 异常处理
    return block_min(set->root->next);
}


struct IntSet *intset_and(struct IntSet *set_a, struct IntSet *set_b) {
    struct Block *block_a = intset_start(set_a);
    struct Block *block_b = intset_start(set_b);
    struct IntSet *result_set = calloc(1, sizeof(struct IntSet));
    struct Block *result_block = intset_start(result_set);

    while (block_a != set_a->root && block_b != set_b->root) {
        if (block_a->offset < block_b->offset) {
            block_a = block_a->next;
        } else if (block_a->offset > block_b->offset) {
            block_b = block_b->next;
        } else {
            long offset = block_a->offset;
            unsigned long words[WORDS_PER_BLOCK] = {0};
            int is_empty = 0;
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                unsigned long word = block_a->bits[i] & block_b->bits[i];
                if (word != 0)is_empty = 1;
                words[i] = word;
            }
            block_a = block_a->next;
            block_b = block_b->next;

            if (is_empty == 0)continue;

            struct Block *block = calloc(1, sizeof(struct Block));
            memcpy(block->bits, words, sizeof(words));
            block->offset = offset;
            block->prev = result_block;
            block->next = result_set->root;

            result_block->next = block;
            result_block = block;
            result_set->root->prev = block;
        }
    }
    return result_set;

}

struct IntSet *intset_or(struct IntSet *set_a, struct IntSet *set_b) {

    struct Block *block_a = intset_start(set_a);
    struct Block *block_b = intset_start(set_b);
    struct IntSet *result_set = calloc(1, sizeof(struct IntSet));
    struct Block *result_block = intset_start(result_set);

    while (block_a != set_a->root || block_b != set_b->root) {
        struct Block *block = malloc(sizeof(struct Block));
        if (block_a == set_a->root) {
            block->offset = block_b->offset;
            memcpy(block->bits, block_b->bits, sizeof(block_b->bits));
            block_b = block_b->next;
        } else if (block_b == set_b->root) {
            block->offset = block_a->offset;
            memcpy(block->bits, block_a->bits, sizeof(block_b->bits));
            block_a = block_a->next;
        } else if (block_a->offset == block_b->offset) {
            unsigned long words[WORDS_PER_BLOCK] = {0};
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                words[i] = block_a->bits[i] | block_b->bits[i];
            }
            memcpy(block->bits, words, sizeof(words));
            block->offset = block_a->offset;

            block_a = block_a->next;
            block_b = block_b->next;
        } else if (block_a->offset > block_b->offset) {
            block->offset = block_b->offset;
            memcpy(block->bits, block_b->bits, sizeof(block_b->bits));
            block_b = block_b->next;
        } else if (block_a->offset < block_b->offset) {
            block->offset = block_a->offset;
            memcpy(block->bits, block_a->bits, sizeof(block_b));
            block_a = block_a->next;
        }

        block->prev = result_block;
        block->next = result_set->root;
        result_block->next = block;

        result_block = block;
        result_set->root->prev = block;
    }
    return result_set;

}

struct IntSet *intset_sub(struct IntSet *set_a, struct IntSet *set_b) {
    struct Block *block_a = intset_start(set_a);
    struct Block *block_b = intset_start(set_b);
    struct IntSet *result_set = calloc(1, sizeof(struct IntSet));
    struct Block *result_block = intset_start(result_set);

    while (block_a != set_a->root) {
        struct Block *block = malloc(sizeof(struct Block));
        if (block_a->offset < block_b->offset || block_b == set_b->root) {
            block->offset = block_a->offset;
            memcpy(block->bits, block_a->bits, sizeof(block_a->bits));
            block_a = block_a->next;
        } else if (block_a->offset > block_b->offset) {
            block_b = block_b->next;
            free(block);
            continue;
        } else {
            block->offset = block_a->offset;
            unsigned long words[WORDS_PER_BLOCK] = {0};
            int is_empty = 0;
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                unsigned long word = block_a->bits[i] & ~block_b->bits[i];
                if (word != 0)is_empty = 1;
                words[i] = word;
            }

            block_a = block_a->next;
            block_b = block_b->next;

            if (is_empty == 0) {
                free(block);
                continue;
            }

            memcpy(block->bits, words, sizeof(words));
        }
        block->prev = result_block;
        block->next = result_set->root;

        result_block->next = block;
        result_block = block;
        result_set->root->prev = block;
    }
    return result_set;

}
