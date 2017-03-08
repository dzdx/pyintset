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


int block_add(Block *block, unsigned int index) {
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

int block_remove(Block *block, unsigned int index) {
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


int block_next(Block *block, unsigned int index) {
    int word_offset;
    unsigned long mask;
    word_offset_and_mask(index, &word_offset, &mask);
    for (int i = word_offset; i < WORDS_PER_BLOCK; i++) {
        for (unsigned int j = index % BITS_PER_WORD; j < BITS_PER_WORD; j++) {
            if ((block->bits[i] & mask) != 0) {
                return index;
            }
            index++;
            mask <<= 1;
        }
        mask = 1;
    }
    return -1;
}


int block_is_empty(Block *block) {
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


IntSet *intset_new(long xs[], int n) {
    IntSet *s = malloc(sizeof(IntSet));
    s->root = NULL;
    for (int i = 0; i < n; i++) {
        intset_add(s, xs[i]);
    }
    return s;
}

Block *intset_start(IntSet *set);


int intset_add(IntSet *set, long x) {

    Block *block = intset_start(set);

    long offset;
    unsigned int index;
    offset_and_index(x, &offset, &index);

    for (; block != set->root && block->offset <= offset;) {
        if (block->offset == offset) {
            return block_add(block, index);
        }
        block = block->next;
    }


    Block *new_block = calloc(1, sizeof(Block));


    new_block->offset = offset;
    new_block->prev = block->prev;
    new_block->next = block;

    new_block->next->prev = new_block;
    new_block->prev->next = new_block;

    return block_add(new_block, index);
}

Block *intset_get_block(IntSet *set, long offset);

void remove_block(Block *pBlock);

int intset_remove(IntSet *set, long x) {
    long offset;
    unsigned int index;
    offset_and_index(x, &offset, &index);
    Block *block = intset_get_block(set, offset);

    if (block != NULL) {
        if (!block_remove(block, index))
            return 0;
        if (block_is_empty(block))
            remove_block(block);
        return 1;
    }
    return 0;
}

int block_has(Block *block, int index) {
    int word_offset;
    unsigned long mask;
    word_offset_and_mask(index, &word_offset, &mask);
    if ((block->bits[word_offset] & mask) != 0)
        return 1;
    return 0;

}

int intset_has(IntSet *set, long x) {
    long offset;
    unsigned int index;
    offset_and_index(x, &offset, &index);
    Block *block = intset_get_block(set, offset);
    if (block != NULL)
        return block_has(block, index);
    return 0;
}

void remove_block(Block *block) {
    block->prev->next = block->next;
    block->next->prev = block->prev;
    free(block);
}


Block *intset_get_block(IntSet *set, long offset) {
    for (Block *block = intset_start(set);
         block != set->root && block->offset <= offset; block = block->next)
        if (block->offset == offset)
            return block;
    return NULL;
}


Block *intset_start(IntSet *set) {
    if (set->root == NULL) {
        Block *block = calloc(1, sizeof(Block));
        block->prev = block;
        block->next = block;
        set->root = block;

    }
    return set->root->next;
}


int intset_len(IntSet *set) {
    int count = 0;
    for (Block *block = intset_start(set); block != set->root; block = block->next)
        count += block->size;
    return count;
}


int intset_is_empty(IntSet *set) {
    return intset_start(set) == set->root;
}


void block_max(Block *block, long *result, int *error) {
    for (int i = WORDS_PER_BLOCK - 1; i >= 0; i--) {
        unsigned long word = block->bits[i];
        if (word == 0)continue;
        unsigned long mask = (unsigned long) 1 << (BITS_PER_WORD - 1);
        for (int j = 0; j < BITS_PER_WORD; j++) {
            if (word & mask){
                *result = block->offset + i * BITS_PER_WORD + BITS_PER_WORD - j - 1;
                return;
            }
            *error = 0;
            mask >>= 1;
        }
    }
    *error = 1;
}

void block_min(Block *block, long *result, int *error) {
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        unsigned long word = block->bits[i];
        if (word == 0)continue;
        unsigned long mask = (unsigned long) 1;
        for (int j = 0; j < BITS_PER_WORD; j++) {
            if (word & mask){
                *result = block->offset + i * BITS_PER_WORD + j;
                return;
            }
            *error = 0;
            mask <<= 1;

        }
    }
    *error = 1;
}

void intset_max(IntSet *set, long *result, int *error) {

    block_max(set->root->prev, result, error);
}

void intset_min(IntSet *set, long *result, int *error) {

    block_min(set->root->next, result, error);
}


IntSet *intset_and(IntSet *set_a, IntSet *set_b) {
    Block *block_a = intset_start(set_a);
    Block *block_b = intset_start(set_b);
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *result_block = intset_start(result_set);

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

            Block *block = calloc(1, sizeof(Block));
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

IntSet *intset_or(IntSet *set_a, IntSet *set_b) {

    Block *block_a = intset_start(set_a);
    Block *block_b = intset_start(set_b);
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *result_block = intset_start(result_set);

    while (block_a != set_a->root || block_b != set_b->root) {
        Block *block = malloc(sizeof(Block));
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

IntSet *intset_sub(IntSet *set_a, IntSet *set_b) {
    Block *block_a = intset_start(set_a);
    Block *block_b = intset_start(set_b);
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *result_block = intset_start(result_set);

    while (block_a != set_a->root) {
        Block *block = malloc(sizeof(Block));
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


void intset_clear(IntSet *set){
    Block * b = set->root->next;
    while(b!=set->root){
        Block * next = b->next;
        free(b);
        b = next;
    }
    free(set->root);
    set->root = NULL;
}

IntSetIter *intset_iter(IntSet *set) {
    IntSetIter *iter = (IntSetIter *) malloc(sizeof(IntSetIter));
    iter->set = set;
    iter->current_block = intset_start(set);
    iter->current_index = 0;
    return iter;
}


void intsetiter_next(IntSetIter *iter, long *val, int *stopped) {

    IntSet *set = iter->set;
    Block *b = iter->current_block;
    unsigned int index = iter->current_index;
    while (b != set->root) {
        index = block_next(b, index);
        if (index == -1) {
            b = b->next;
            iter->current_block = b;
            index = 0;
            iter->current_index = 0;
            continue;
        }
        iter->current_index = index + 1;
        *val = b->offset + index;
        *stopped = 0;
        return;
    }
    *stopped = 1;
    return;
}

