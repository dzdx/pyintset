//
// Created by lxd on 2017/2/6.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "intset.h"


static const int popCountTable[1 << 8] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
                                          1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                          1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                          1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                          3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                          1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
                                          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                          3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                          2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
                                          3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                          3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
                                          4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};


int count_bit(uint64_t x) {
    int count = 0;
    while (x > 0) {
        count += popCountTable[x % (1 << 8)];
        x >>= 8;
    }
    return count;
}

inline void word_offset_and_mask(int index, int *word_offset, uint64_t *mask) {
    *word_offset = index / BITS_PER_WORD;
    *mask = (uint64_t) 1 << (index % BITS_PER_WORD);
}


int block_add(Block *block, int index) {
    int word_offset;
    uint64_t mask;
    word_offset_and_mask(index, &word_offset, &mask);
    if ((block->bits[word_offset] & mask) == 0) {
        block->bits[word_offset] |= mask;
        return 1;
    }
    return 0;
}

int block_size(Block *block) {
    int count = 0;
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        count += count_bit(block->bits[i]);
    }
    return count;
}

int block_remove(Block *block, int index) {
    int word_offset;
    uint64_t mask;
    word_offset_and_mask(index, &word_offset, &mask);
    if ((block->bits[word_offset] & mask) != 0) {
        block->bits[word_offset] &= ~mask;
        return 1;
    }
    return 0;
}

Block *block_copy(Block *b) {
    Block *rb = malloc(sizeof(Block));
    rb->offset = b->offset;
    memcpy(rb->bits, b->bits, sizeof(b->bits));
    return rb;
}


int block_has(Block *block, int index) {
    int word_offset;
    uint64_t mask;
    word_offset_and_mask(index, &word_offset, &mask);
    if ((block->bits[word_offset] & mask) != 0)
        return 1;
    return 0;
}

int block_next(Block *block, int index) {
    int word_offset;
    uint64_t mask;
    index++;
    word_offset_and_mask(index, &word_offset, &mask);
    for (int i = word_offset; i < WORDS_PER_BLOCK; i++) {
        if (block->bits[i] == 0) {
            index = (i + 1) * BITS_PER_WORD;
            continue;
        }
        for (int j = index % BITS_PER_WORD; j < BITS_PER_WORD; j++) {
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

long block_max(Block *block, int *error) {
    *error = 0;
    for (int i = WORDS_PER_BLOCK - 1; i >= 0; i--) {
        uint64_t word = block->bits[i];
        if (word == 0)continue;
        uint64_t mask = (uint64_t) 1 << (BITS_PER_WORD - 1);
        for (int j = 0; j < BITS_PER_WORD; j++) {
            if (word & mask) {
                return block->offset + i * BITS_PER_WORD + BITS_PER_WORD - j - 1;
            }
            mask >>= 1;
        }
    }
    *error = 1;
    return 0;
}

long block_min(Block *block, int *error) {
    *error = 0;
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        uint64_t word = block->bits[i];
        if (word == 0)continue;
        uint64_t mask = 1;
        for (int j = 0; j < BITS_PER_WORD; j++) {
            if (word & mask) {
                return block->offset + i * BITS_PER_WORD + j;
            }
            mask <<= 1;
        }
    }
    *error = 1;
    return 0;
}


int block_is_empty(Block *block) {
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        if (block->bits[i] != 0)
            return 0;
    }
    return 1;
}


inline void offset_and_index(long x, long *offset, int *index) {
    int base = x % BITS_PER_BLOCK;
    if (base < 0) {
        base += BITS_PER_BLOCK;
    }
    *offset = x - base;
    *index = base;
}


Block *intset_start(IntSet *set);


IntSet *intset_copy(IntSet *self) {
    IntSet *copied = calloc(1, sizeof(IntSet));

    Block *sb = intset_start(self);
    Block *copied_block = intset_start(copied);
    while (sb != self->root) {
        Block *block = block_copy(sb);
        block->prev = copied_block;

        copied_block->next = block;
        copied_block = block;

        sb = sb->next;
    }
    copied->root->prev = copied_block;
    copied_block->next = copied->root;
    return copied;
}


int intset_insert_after(IntSet *set, long x, Block **block_ref) {
    long offset;
    int index;
    offset_and_index(x, &offset, &index);

    Block *block = *block_ref;

    for (; block != set->root && block->offset <= offset; block = block->next) {
        if (block->offset == offset) {
            *block_ref = block;
            return block_add(block, index);
        }
    }
    Block *new_block = calloc(1, sizeof(Block));


    new_block->offset = offset;
    new_block->prev = block->prev;
    new_block->next = block;

    new_block->next->prev = new_block;
    new_block->prev->next = new_block;

    *block_ref = new_block;
    return block_add(new_block, index);
}


int intset_add(IntSet *set, long x) {
    Block *block = intset_start(set);
    return intset_insert_after(set, x, &block);
}


int intset_item_cmp(const void *a, const void *b) {
    return *(long *) a - *(long *) b;
}

void intset_add_array(IntSet *set, long *xs, int num) {
    Block *block = intset_start(set);
    Block **block_ref = &block;
    qsort(xs, num, sizeof(long), intset_item_cmp);
    for (int i = 0; i < num; i++) {
        long x = xs[i];
        intset_insert_after(set, x, block_ref);
    }
}


IntSet *intset_new() {
    IntSet *set = malloc(sizeof(IntSet));
    set->root = NULL;
    return set;
}


Block *intset_get_block(IntSet *set, long offset);

void intset_removeblock(Block *pBlock);

int intset_remove(IntSet *set, long x) {
    long offset;
    int index;
    offset_and_index(x, &offset, &index);
    Block *block = intset_get_block(set, offset);

    if (block != NULL) {
        if (!block_remove(block, index))
            return 0;
        if (block_is_empty(block))
            intset_removeblock(block);
        return 1;
    }
    return 0;
}


int intset_has(IntSet *set, long x) {
    long offset;
    int index;
    offset_and_index(x, &offset, &index);
    Block *block = intset_get_block(set, offset);
    if (block != NULL)
        return block_has(block, index);
    return 0;
}

void intset_removeblock(Block *block) {
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
        count += block_size(block);
    return count;
}


int intset_is_empty(IntSet *set) {
    return intset_start(set) == set->root;
}

long intsetiter_next(IntSetIter *iter, int *stopped) {

    IntSet *set = iter->set;
    Block *b = iter->current_block;
    int index = iter->current_index;
    while (b != set->root) {
        index = block_next(b, index);
        if (index == -1) {
            b = b->next;
            iter->current_block = b;
            index = -1;
            iter->current_index = -1;
            continue;
        }
        iter->current_index = index;
        *stopped = 0;
        return b->offset + index;
    }
    *stopped = 1;
    return 0;
}


long intset_max(IntSet *set, int *error) {
    return block_max(set->root->prev, error);
}

long intset_min(IntSet *set, int *error) {
    return block_min(set->root->next, error);
}


IntSet *intset_and(IntSet *self, IntSet *other) {
    Block *sb = intset_start(self);
    Block *ob = intset_start(other);
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *rb = intset_start(result_set);

    while (sb != self->root && ob != other->root) {
        if (sb->offset < ob->offset) {
            sb = sb->next;
        } else if (sb->offset > ob->offset) {
            ob = ob->next;
        } else {
            long offset = sb->offset;
            uint64_t words[WORDS_PER_BLOCK] = {0};
            int is_empty = 1;
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                uint64_t word = sb->bits[i] & ob->bits[i];
                if (word != 0)is_empty = 0;
                words[i] = word;
            }
            sb = sb->next;
            ob = ob->next;

            if (is_empty == 1)continue;

            Block *block = malloc(sizeof(Block));
            memcpy(block->bits, words, sizeof(words));
            block->offset = offset;
            block->prev = rb;

            rb->next = block;
            rb = block;
        }
    }
    result_set->root->prev = rb;
    rb->next = result_set->root;
    return result_set;
}

void intset_merge(IntSet *self, IntSet *other) {
    Block *sb = intset_start(self);
    Block *ob = intset_start(other);
    while (ob != other->root) {
        if (sb != self->root && sb->offset == ob->offset) {
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                sb->bits[i] |= ob->bits[i];
            }
            ob = ob->next;
        } else if (sb == self->root || sb->offset > ob->offset) {

            Block *b = block_copy(ob);

            b->next = sb;
            b->prev = sb->prev;

            sb->prev->next = b;
            sb->prev = b;

            ob = ob->next;
        }
        sb = sb->next;
    }
}

IntSet *intset_or(IntSet *self, IntSet *other) {
    IntSet *result_set = intset_copy(self);
    intset_merge(result_set, other);
    return result_set;
}

IntSet *intset_sub(IntSet *self, IntSet *other) {
    Block *sb = intset_start(self);
    Block *ob = intset_start(other);
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *rb = intset_start(result_set);

    while (sb != self->root) {
        Block *block = calloc(1, sizeof(Block));
        if (sb->offset < ob->offset || ob == other->root) {
            block->offset = sb->offset;
            memcpy(block->bits, sb->bits, sizeof(sb->bits));
            sb = sb->next;
        } else if (sb->offset > ob->offset) {
            ob = ob->next;
            free(block);
            continue;
        } else {
            block->offset = sb->offset;
            uint64_t words[WORDS_PER_BLOCK] = {0};
            int is_empty = 1;
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                uint64_t word = sb->bits[i] & ~ob->bits[i];
                if (word != 0)is_empty = 0;
                words[i] = word;
            }

            sb = sb->next;
            ob = ob->next;

            if (is_empty == 1) {
                free(block);
                continue;
            }
            memcpy(block->bits, words, sizeof(words));
        }
        block->prev = rb;

        rb->next = block;
        rb = block;
    }
    rb->next = result_set->root;
    result_set->root->prev = rb;

    return result_set;

}


IntSet *intset_xor(IntSet *self, IntSet *other) {
    Block *sb = intset_start(self);
    Block *ob = intset_start(other);
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *rb = intset_start(result_set);

    while (sb != self->root || ob != other->root) {
        Block *block = calloc(1, sizeof(Block));
        if (sb == self->root) {
            block->offset = ob->offset;
            memcpy(block->bits, ob->bits, sizeof(ob->bits));
            ob = ob->next;
        } else if (ob == other->root) {
            block->offset = sb->offset;
            memcpy(block->bits, sb->bits, sizeof(ob->bits));
            sb = sb->next;
        } else if (sb->offset == ob->offset) {
            block->offset = sb->offset;
            uint64_t words[WORDS_PER_BLOCK] = {0};
            int is_empty = 1;
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                uint64_t word = sb->bits[i] ^ob->bits[i];
                if (word != 0)is_empty = 0;
                words[i] = word;
            }
            sb = sb->next;
            ob = ob->next;
            if (is_empty == 1) {
                free(block);
                continue;
            }
            memcpy(block->bits, words, sizeof(words));
        } else if (sb->offset > ob->offset) {
            block->offset = ob->offset;
            memcpy(block->bits, ob->bits, sizeof(ob->bits));
            ob = ob->next;
        } else if (sb->offset < ob->offset) {
            block->offset = sb->offset;
            memcpy(block->bits, sb->bits, sizeof(ob->bits));
            sb = sb->next;
        }

        block->prev = rb;
        rb->next = block;

        rb = block;
    }
    rb->next = result_set->root;
    result_set->root->prev = rb;
    return result_set;
}


int intset_equals(IntSet *self, IntSet *other) {
    //other == self
    Block *sb = intset_start(self);
    Block *ob = intset_start(other);

    while (1) {
        if (sb == self->root && ob == other->root) {
            return 1;
        }
        else if (sb == self->root || ob == other->root) {
            return 0;
        }
        else if (sb->offset != ob->offset) {
            return 0;
        }
        else {
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                if (sb->bits[i] != ob->bits[i])return 0;
            }
            sb = sb->next;
            ob = ob->next;
        }
    }
}


int intset_issuperset(IntSet *self, IntSet *other) {
    //other <= self
    Block *sb = intset_start(self);
    Block *ob = intset_start(other);

    while (ob != other->root) {
        if (sb == self->root) {
            return 0;
        }
        else if (sb->offset < ob->offset) {
            sb = sb->next;
        } else if (sb->offset > ob->offset) {
            return 0;
        } else {
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                if ((sb->bits[i] | ob->bits[i]) != sb->bits[i]) {
                    return 0;
                }
            }
            sb = sb->next;
            ob = ob->next;
        }
    }
    return 1;
}


int intset_issubset(IntSet *self, IntSet *other) {
    //other >= self
    Block *sb = intset_start(self);
    Block *ob = intset_start(other);

    while (sb != self->root) {
        if (ob == other->root) {
            return 0;
        }
        else if (sb->offset > ob->offset) {
            ob = ob->next;
        } else if (sb->offset < ob->offset) {
            return 0;
        } else {
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                if ((sb->bits[i] | ob->bits[i]) != ob->bits[i]) {
                    return 0;
                }
            }
            sb = sb->next;
            ob = ob->next;
        }
    }
    return 1;
}


Block *block_get_slice(Block *b, int start, int end) {
    Block *rb = malloc(sizeof(Block));
    rb->offset = b->offset;
    int start_bit = start / BITS_PER_WORD;
    int end_bit = end / BITS_PER_WORD;
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        if (i < start_bit || i > end_bit) {
            rb->bits[i] = 0;
        } else if (i == start_bit && i != end_bit) {
            rb->bits[i] = b->bits[i] & ~(((uint64_t) 1 << (start - i * BITS_PER_WORD)) - 1);
        } else if (i == end_bit && i != start_bit) {
            rb->bits[i] = b->bits[i] & (((uint64_t) 1 << (end - i * BITS_PER_WORD)) - 1);
        } else if (i == end_bit && i == start_bit) {
            rb->bits[i] = b->bits[i] & ~(((uint64_t) 1 << (start - i * BITS_PER_WORD)) - 1) &
                          (((uint64_t) 1 << (end - i * BITS_PER_WORD)) - 1);
        } else if (start_bit < i && i < end_bit) {
            rb->bits[i] = b->bits[i];
        }
    }
    return rb;
}


IntSet *intset_get_slice(IntSet *self, int start, int end) {
    IntSet *rs = intset_new();
    if (end <= start) {
        return rs;
    }
    Block *sb = intset_start(self);
    Block *rb = intset_start(rs);
    int sum = 0;
    while (sb != self->root && sum < end) {
        int len = block_size(sb);
        int sum_next = sum + len;
        if (sum_next > start && start >= sum && sum_next > end && end >= sum) {
            int index = -1;
            for (int i = 0; i <= start - sum; i++) {
                index = block_next(sb, index);
            }
            int si = index;
            for (int i = start + 1; i <= end; i++) {
                index = block_next(sb, index);
            }
            int ei = index;
            Block *new_block = block_get_slice(sb, si, ei);
            new_block->prev = rb;
            new_block->next = rb->next;

            rb->next = new_block;
            rb = new_block;
        } else if (sum_next > start && start >= sum) {
            int index = -1;
            for (int i = 0; i <= start - sum; i++) {
                index = block_next(sb, index);
            }
            int si = index;
            Block *new_block = block_get_slice(sb, si, BITS_PER_BLOCK);
            new_block->prev = rb;
            new_block->next = rb->next;

            rb->next = new_block;
            rb = new_block;
        } else if (sum_next > end && end >= sum) {
            int index = -1;
            for (int i = 0; i <= end - sum; i++) {
                index = block_next(sb, index);
            }
            int ei = index;
            Block *new_block = block_get_slice(sb, 0, ei);
            new_block->prev = rb;
            new_block->next = rb->next;

            rb->next = new_block;
            rb = new_block;
        } else if (start <= sum && sum_next <= end) {
            Block *new_block = block_copy(sb);
            new_block->prev = rb;
            new_block->next = rb->next;

            rb->next = new_block;
            rb = new_block;
        }
        sum = sum_next;
        sb = sb->next;
    }
    rs->root->prev = rb;
    return rs;
}


long intset_get_item(IntSet *set, int index, int *error) {
    *error = 0;
    Block *b = intset_start(set);
    int sum = 0;
    while (b != set->root) {
        int len = block_size(b);
        if ((sum + len) > index) {
            break;
        }
        sum += len;
        b = b->next;
    }
    if (b == set->root) {
        *error = 1;
        return 0;
    } else {
        int result = -1;
        for (int i = 0; i <= index - sum; i++) {
            result = block_next(b, result);
        }
        return b->offset + result;
    }
}

void intset_clear(IntSet *set) {
    if(set->root == NULL){
        return;
    }
    Block *b = set->root->next;
    while (b != set->root) {
        Block *next = b->next;
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
    iter->current_index = -1;
    return iter;
}

