//
// Created by lxd on 2017/2/6.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "intset.h"
#include "number.h"

#define SKIPLIST_MAXLEVEL 32
#define SKIPLIST_P 0.25
#define MAX(x, y) ((x)>(y)?(x):(y))

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


int count_bit(Word x) {
    int count = 0;
    while (x > 0) {
        count += popCountTable[x % (1 << 8)];
        x >>= 8;
    }
    return count;
}


int random_level() {
    int level = 1;
    for (; (level < (SKIPLIST_MAXLEVEL - 1)) && ((random() & 0xffff) < (SKIPLIST_P * 0xffff)); level++);
    return level;
}

void word_offset_and_mask(int index, int *word_offset, Word *mask) {
    *word_offset = index / BITS_PER_WORD;
    *mask = (Word) 1 << (index % BITS_PER_WORD);
}


int block_add(Block *block, int index) {
    int word_offset;
    Word mask;
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
    Word mask;
    word_offset_and_mask(index, &word_offset, &mask);
    if ((block->bits[word_offset] & mask) != 0) {
        block->bits[word_offset] &= ~mask;
        return 1;
    }
    return 0;
}

Block *block_copy(Block *b) {
    Block *rb = malloc(sizeof(Block));
    rb->offset = number_copy(b->offset);
    memcpy(rb->bits, b->bits, sizeof(b->bits));
    return rb;
}


int block_has(Block *block, int index) {
    int word_offset;
    Word mask;
    word_offset_and_mask(index, &word_offset, &mask);
    if ((block->bits[word_offset] & mask) != 0)
        return 1;
    return 0;
}

int block_next(Block *block, int index) {
    int word_offset;
    Word mask;
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

Number *block_max(Block *block, int *error) {
    *error = 0;
    for (int i = WORDS_PER_BLOCK - 1; i >= 0; i--) {
        Word word = block->bits[i];
        if (word == 0)continue;
        Word mask = (Word) 1 << (BITS_PER_WORD - 1);
        for (int j = 0; j < BITS_PER_WORD; j++) {
            if (word & mask) {
                return number_add(block->offset, number_get_small(i * BITS_PER_WORD + BITS_PER_WORD - j - 1));
            }
            mask >>= 1;
        }
    }
    *error = 1;
    return NULL;
}

Number *block_min(Block *block, int *error) {
    *error = 0;
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        Word word = block->bits[i];
        if (word == 0)continue;
        Word mask = 1;
        for (int j = 0; j < BITS_PER_WORD; j++) {
            if (word & mask) {
                return number_add(block->offset, number_get_small(i * BITS_PER_WORD + j));
            }
            mask <<= 1;
        }
    }
    *error = 1;
    return NULL;
}


int block_is_empty(Block *block) {
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        if (block->bits[i] != 0)
            return 0;
    }
    return 1;
}

void block_free(Block *block) {
    number_clear(block->offset);
    free(block->nexts);
    free(block);
}

void offset_and_index(Number *x, Number **offset, int *index) {
    int idx = number_slice(x, BLOCK_LEN);
    if (x->size < 0 & idx > 0) {
        idx = BITS_PER_BLOCK - idx;
    }
    *offset = number_sub(x, number_get_small(idx));
    *index = idx;
}


Block *intset_root(IntSet *set);


IntSet *intset_copy(IntSet *self) {
    IntSet *copied = calloc(1, sizeof(IntSet));
    Block *sb = intset_root(self);
    sb = sb->nexts[0];
    Block *copied_root = intset_root(copied);
    Block *update[SKIPLIST_MAXLEVEL];
    for (int i = 0; i < SKIPLIST_MAXLEVEL; i++) {
        update[i] = copied_root;
    }
    int set_level = 0;
    while (sb != self->root) {
        int level = random_level();
        set_level = MAX(set_level, level);
        Block *block = block_copy(sb);
        block->nexts = malloc(level * sizeof(Block *));
        block->level = level;
        block->prev = update[0];
        for (int i = 0; i < level; i++) {
            update[i]->nexts[i] = block;
            update[i] = block;
        }
        sb = sb->nexts[0];
    }
    copied->root->prev = update[0];
    for (int i = 0; i < set_level; i++) {
        update[i]->nexts[i] = copied->root;
    }
    copied->level = set_level;
    return copied;
}


int intset_add(IntSet *set, Number *x) {
    Block *block = intset_root(set);
    Number *offset;
    int index;
    Block *update[SKIPLIST_MAXLEVEL];
    offset_and_index(x, &offset, &index);
    for (int i = set->level - 1; i >= 0; i--) {
        while (block->nexts[i] != set->root) {
            int r = number_cmp(offset, block->nexts[i]->offset);
            if (r == 0) {
                number_clear(offset);
                return block_add(block->nexts[i], index);
            } else if (r > 0) {
                block = block->nexts[i];
            } else {
                break;
            }
        }
        update[i] = block;
    }
    int level = random_level();
    if (level > set->level) {
        for (int i = set->level; i < level; i++) {
            update[i] = set->root;
        }
        set->level = level;
    }
    Block *new_block = calloc(1, sizeof(Block));
    new_block->nexts = malloc(sizeof(Block *) * level);
    new_block->level = level;
    new_block->offset = offset;
    new_block->prev = update[0];
    update[0]->nexts[0]->prev = new_block;
    for (int i = 0; i < level; i++) {
        new_block->nexts[i] = update[i]->nexts[i];
        update[i]->nexts[i] = new_block;
    }
    return block_add(new_block, index);
}


IntSet *intset_new() {
    IntSet *set = malloc(sizeof(IntSet));
    set->root = NULL;
    return set;
}


void dropdown_level(IntSet *set) {
    Block *block = intset_root(set);
    int level = set->level - 1;
    while (level > 0) {
        if (block->nexts[level] == set->root) {
            level--;
        } else {
            set->level = level + 1;
            return;
        }
    }
}


int intset_remove(IntSet *set, Number *x) {
    Block *block = intset_root(set);
    Number *offset;
    int index;
    Block *update[SKIPLIST_MAXLEVEL];
    offset_and_index(x, &offset, &index);
    for (int i = set->level - 1; i >= 0; i--) {
        while (block->nexts[i] != set->root) {
            int r = number_cmp(offset, block->nexts[i]->offset);
            if (r == 0) {
                number_clear(offset);
                Block *found = block->nexts[i];

                if (!block_remove(found, index))
                    return 0;
                if (block_is_empty(found)) {
                    found->nexts[0]->prev = found->prev;
                    for (int j = set->level - 1; j > i; j--) {
                        update[j]->nexts[j] = found->nexts[j];
                    }
                    for (int j = i; j >= 0; j--) {
                        block->nexts[j] = found->nexts[j];
                    }
                    if (found->level == set->level) {
                        dropdown_level(set);
                    }
                    block_free(found);
                }
                return 1;
            } else if (r > 0) {
                block = block->nexts[i];
            } else {
                break;
            }
        }
        update[i] = block;
    }
    return 0;
}


int intset_has(IntSet *set, Number *x) {
    Block *block = intset_root(set);
    Number *offset;
    int index;
    offset_and_index(x, &offset, &index);
    for (int i = set->level - 1; i >= 0; i--) {
        while (block->nexts[i] != set->root) {
            int r = number_cmp(offset, block->nexts[i]->offset);
            if (r == 0) {
                number_clear(offset);
                return block_has(block->nexts[i], index);
            } else if (r > 0) {
                block = block->nexts[i];
            } else {
                break;
            }
        }
    }
    return 0;
}


Block *intset_root(IntSet *set) {
    if (set->root == NULL) {
        Block *block = calloc(1, sizeof(Block));
        block->prev = block;
        block->nexts = malloc(sizeof(Block *) * SKIPLIST_MAXLEVEL);
        block->level = SKIPLIST_MAXLEVEL;
        for (int i = 0; i < SKIPLIST_MAXLEVEL; i++) {
            block->nexts[i] = block;
        }
        set->root = block;
        set->level = 0;
    }
    return set->root;
}


int intset_len(IntSet *set) {
    int count = 0;
    for (Block *block = intset_root(set)->nexts[0]; block != set->root; block = block->nexts[0])
        count += block_size(block);
    return count;
}


int intset_is_empty(IntSet *set) {
    return intset_root(set)->nexts[0] == set->root;
}

Number *intsetiter_next(IntSetIter *iter, int *stopped) {

    IntSet *set = iter->set;
    Block *b = iter->current_block;
    int index = iter->current_index;
    while (b != set->root) {
        index = block_next(b, index);
        if (index == -1) {
            b = b->nexts[0];
            iter->current_block = b;
            index = -1;
            iter->current_index = -1;
            continue;
        }
        iter->current_index = index;
        *stopped = 0;
        return number_add(b->offset, number_get_small(index));
    }
    *stopped = 1;
    return 0;
}


Number *intset_max(IntSet *set, int *error) {
    if (intset_is_empty(set)) {
        *error = 1;
        return NULL;
    }
    return block_max(set->root->prev, error);
}

Number *intset_min(IntSet *set, int *error) {
    if (intset_is_empty(set)) {
        *error = 1;
        return NULL;
    }
    return block_min(set->root->nexts[0], error);
}


IntSet *intset_and(IntSet *self, IntSet *other) {
    Block *sb = intset_root(self)->nexts[0];
    Block *ob = intset_root(other)->nexts[0];
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *rb = intset_root(result_set);
    Block *update[SKIPLIST_MAXLEVEL];
    for (int i = 0; i < SKIPLIST_MAXLEVEL; i++) {
        update[i] = rb;
    }
    int set_level = 0;
    while (sb != self->root && ob != other->root) {
        if (number_cmp(sb->offset, ob->offset) < 0) {
            sb = sb->nexts[0];
        } else if (number_cmp(sb->offset, ob->offset) > 0) {
            ob = ob->nexts[0];
        } else {
            Number *offset = sb->offset;
            Word words[WORDS_PER_BLOCK] = {0};
            int is_empty = 1;
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                Word word = sb->bits[i] & ob->bits[i];
                if (word != 0)is_empty = 0;
                words[i] = word;
            }
            sb = sb->nexts[0];
            ob = ob->nexts[0];

            if (is_empty == 1)continue;


            int level = random_level();
            set_level = MAX(set_level, level);
            Block *block = malloc(sizeof(Block));
            block->nexts = malloc(level * sizeof(Block *));
            block->level = level;
            memcpy(block->bits, words, sizeof(words));
            block->offset = number_copy(offset);
            block->prev = update[0];
            for (int i = 0; i < level; i++) {
                update[i]->nexts[i] = block;
                update[i] = block;
            }
        }
    }
    result_set->root->prev = update[0];
    for (int i = 0; i < set_level; i++) {
        update[i]->nexts[i] = result_set->root;
    }
    result_set->level = set_level;
    return result_set;
}

void intset_merge(IntSet *self, IntSet *other) {
    Block *sb = intset_root(self);
    Block *ob = intset_root(other)->nexts[0];
    Block *update[SKIPLIST_MAXLEVEL];
    for (int i = 0; i < SKIPLIST_MAXLEVEL; i++) {
        update[i] = sb;
    }
    int set_level = self->level;
    Block *s_next;
    while (ob != other->root) {
        s_next = sb->nexts[0];
        int result = 0;
        if (s_next != self->root) {
            result = number_cmp(s_next->offset, ob->offset);
        }
        if (s_next != self->root && result == 0) {
            sb = s_next;
            for (int i = 0; i < sb->level; i++) {
                update[i] = sb;
            }
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                sb->bits[i] |= ob->bits[i];
            }
            ob = ob->nexts[0];
        } else if (s_next == self->root || result > 0) {
            Block *b = block_copy(ob);
            int level = random_level();
            set_level = MAX(set_level, level);
            b->prev = sb;
            b->nexts = malloc(level * sizeof(Block *));
            b->level = level;
            for (int i = 0; i < level; i++) {
                b->nexts[i] = update[i]->nexts[i];
                update[i]->nexts[i] = b;
                update[i] = b;
            }
            s_next->prev = b;
            ob = ob->nexts[0];
        } else {
            sb = sb->nexts[0];
            for (int i = 0; i < sb->level; i++) {
                update[i] = sb;
            }
        }
    }
    self->level = set_level;
}

IntSet *intset_or(IntSet *self, IntSet *other) {
    IntSet *result_set = intset_copy(self);
    intset_merge(result_set, other);
    return result_set;
}


IntSet *intset_sub(IntSet *self, IntSet *other) {
    Block *sb = intset_root(self)->nexts[0];
    Block *ob = intset_root(other)->nexts[0];
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *rb = intset_root(result_set);
    Block *update[SKIPLIST_MAXLEVEL];
    for (int i = 0; i < SKIPLIST_MAXLEVEL; i++) {
        update[i] = rb;
    }
    int set_level = 0;

    while (sb != self->root) {
        Block *block = calloc(1, sizeof(Block));
        if (ob == other->root || number_cmp(sb->offset, ob->offset) < 0) {
            block->offset = number_copy(sb->offset);
            memcpy(block->bits, sb->bits, sizeof(sb->bits));
            sb = sb->nexts[0];
        } else if (number_cmp(sb->offset, ob->offset) > 0) {
            ob = ob->nexts[0];
            free(block);
            continue;
        } else {
            block->offset = number_copy(sb->offset);
            Word words[WORDS_PER_BLOCK] = {0};
            int is_empty = 1;
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                Word word = sb->bits[i] & ~ob->bits[i];
                if (word != 0)is_empty = 0;
                words[i] = word;
            }

            sb = sb->nexts[0];
            ob = ob->nexts[0];

            if (is_empty == 1) {
                block_free(block);
                continue;
            }
            memcpy(block->bits, words, sizeof(words));
        }
        int level = random_level();
        set_level = MAX(set_level, level);
        block->nexts = malloc(level * sizeof(Block *));
        block->level = level;
        block->prev = update[0];
        for (int i = 0; i < level; i++) {
            update[i]->nexts[i] = block;
            update[i] = block;
        }
    }
    result_set->root->prev = update[0];
    for (int i = 0; i < set_level; i++) {
        update[i]->nexts[i] = result_set->root;
    }
    result_set->level = set_level;
    return result_set;

}


IntSet *intset_xor(IntSet *self, IntSet *other) {
    Block *sb = intset_root(self)->nexts[0];
    Block *ob = intset_root(other)->nexts[0];
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *rb = intset_root(result_set);
    Block *update[SKIPLIST_MAXLEVEL];
    for (int i = 0; i < SKIPLIST_MAXLEVEL; i++) {
        update[i] = rb;
    }
    int set_level = 0;

    while (sb != self->root || ob != other->root) {
        Block *block = calloc(1, sizeof(Block));
        if (sb == self->root) {
            block->offset = number_copy(ob->offset);
            memcpy(block->bits, ob->bits, sizeof(ob->bits));
            ob = ob->nexts[0];
        } else if (ob == other->root) {
            block->offset = number_copy(sb->offset);
            memcpy(block->bits, sb->bits, sizeof(ob->bits));
            sb = sb->nexts[0];
        } else if (number_cmp(sb->offset, ob->offset) == 0) {
            block->offset = number_copy(sb->offset);
            Word words[WORDS_PER_BLOCK] = {0};
            int is_empty = 1;
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                Word word = sb->bits[i] ^ob->bits[i];
                if (word != 0)is_empty = 0;
                words[i] = word;
            }
            sb = sb->nexts[0];
            ob = ob->nexts[0];
            if (is_empty == 1) {
                block_free(block);
                continue;
            }
            memcpy(block->bits, words, sizeof(words));
        } else if (number_cmp(sb->offset, ob->offset) > 0) {
            block->offset = number_copy(ob->offset);
            memcpy(block->bits, ob->bits, sizeof(ob->bits));
            ob = ob->nexts[0];
        } else if (number_cmp(sb->offset, ob->offset) < 0) {
            block->offset = number_copy(sb->offset);
            memcpy(block->bits, sb->bits, sizeof(ob->bits));
            sb = sb->nexts[0];
        }

        int level = random_level();
        set_level = MAX(set_level, level);
        block->nexts = malloc(sizeof(Block *) * level);
        block->level = level;
        block->prev = update[0];
        for (int i = 0; i < level; i++) {
            update[i]->nexts[i] = block;
            update[i] = block;
        }

    }
    result_set->root->prev = update[0];
    for (int i = 0; i < set_level; i++) {
        update[i]->nexts[i] = result_set->root;
    }
    result_set->level = set_level;
    return result_set;
}


int intset_equals(IntSet *self, IntSet *other) {
    //other == self
    Block *sb = intset_root(self)->nexts[0];
    Block *ob = intset_root(other)->nexts[0];

    while (1) {
        if (sb == self->root && ob == other->root) {
            return 1;
        }
        else if (sb == self->root || ob == other->root) {
            return 0;
        }
        else if (number_cmp(sb->offset, ob->offset) != 0) {
            return 0;
        }
        else {
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                if (sb->bits[i] != ob->bits[i])return 0;
            }
            sb = sb->nexts[0];
            ob = ob->nexts[0];
        }
    }
}


int intset_issuperset(IntSet *self, IntSet *other) {
    //other <= self
    Block *sb = intset_root(self)->nexts[0];
    Block *ob = intset_root(other)->nexts[0];

    while (ob != other->root) {
        if (sb == self->root) {
            return 0;
        }
        else if (number_cmp(sb->offset, ob->offset) < 0) {
            sb = sb->nexts[0];
        } else if (number_cmp(sb->offset, ob->offset) > 0) {
            return 0;
        } else {
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                if ((sb->bits[i] | ob->bits[i]) != sb->bits[i]) {
                    return 0;
                }
            }
            sb = sb->nexts[0];
            ob = ob->nexts[0];
        }
    }
    return 1;
}


int intset_issubset(IntSet *self, IntSet *other) {
    //other >= self
    Block *sb = intset_root(self)->nexts[0];
    Block *ob = intset_root(other)->nexts[0];

    while (sb != self->root) {
        if (ob == other->root) {
            return 0;
        }
        else if (number_cmp(sb->offset, ob->offset) > 0) {
            ob = ob->nexts[0];
        } else if (number_cmp(sb->offset, ob->offset) < 0) {
            return 0;
        } else {
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                if ((sb->bits[i] | ob->bits[i]) != ob->bits[i]) {
                    return 0;
                }
            }
            sb = sb->nexts[0];
            ob = ob->nexts[0];
        }
    }
    return 1;
}


Block *block_get_slice(Block *b, int start, int end) {
    Block *rb = malloc(sizeof(Block));
    rb->offset = number_copy(b->offset);
    int start_bit = start / BITS_PER_WORD;
    int end_bit = end / BITS_PER_WORD;
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        if (i < start_bit || i > end_bit) {
            rb->bits[i] = 0;
        } else if (i == start_bit && i != end_bit) {
            rb->bits[i] = b->bits[i] & ~(((Word) 1 << (start - i * BITS_PER_WORD)) - 1);
        } else if (i == end_bit && i != start_bit) {
            rb->bits[i] = b->bits[i] & (((Word) 1 << (end - i * BITS_PER_WORD)) - 1);
        } else if (i == end_bit && i == start_bit) {
            rb->bits[i] = b->bits[i] & ~(((Word) 1 << (start - i * BITS_PER_WORD)) - 1) &
                          (((Word) 1 << (end - i * BITS_PER_WORD)) - 1);
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
    Block *sb = intset_root(self)->nexts[0];
    Block *rb = intset_root(rs);
    int sum = 0;
    Block *update[SKIPLIST_MAXLEVEL];
    for (int i = 0; i < SKIPLIST_MAXLEVEL; i++) {
        update[i] = rb;
    }
    int set_level = 0;

    while (sb != self->root && sum < end) {
        int len = block_size(sb);
        int sum_next = sum + len;
        Block *new_block;
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
            new_block = block_get_slice(sb, si, ei);
        } else if (sum_next > start && start >= sum) {
            int index = -1;
            for (int i = 0; i <= start - sum; i++) {
                index = block_next(sb, index);
            }
            int si = index;
            new_block = block_get_slice(sb, si, BITS_PER_BLOCK);
        } else if (sum_next > end && end >= sum) {
            int index = -1;
            for (int i = 0; i <= end - sum; i++) {
                index = block_next(sb, index);
            }
            int ei = index;
            new_block = block_get_slice(sb, 0, ei);
        } else if (start <= sum && sum_next <= end) {
            new_block = block_copy(sb);
        } else {
            sum = sum_next;
            sb = sb->nexts[0];
            continue;
        }
        int level = random_level();
        set_level = MAX(set_level, level);
        new_block->nexts = malloc(level * sizeof(Block *));
        new_block->level = level;
        new_block->prev = rb;
        for (int i = 0; i < level; i++) {
            new_block->nexts[i] = update[i]->nexts[i];
            update[i]->nexts[i] = new_block;
        }
        rb = new_block;
        sum = sum_next;
        sb = sb->nexts[0];
    }
    rs->root->prev = rb;
    rs->level = set_level;
    return rs;
}


Number *intset_get_item(IntSet *set, int index, int *error) {
    *error = 0;
    Block *b = intset_root(set)->nexts[0];
    int sum = 0;
    while (b != set->root) {
        int len = block_size(b);
        if ((sum + len) > index) {
            break;
        }
        sum += len;
        b = b->nexts[0];
    }
    if (b == set->root) {
        *error = 1;
        return 0;
    } else {
        int result = -1;
        for (int i = 0; i <= index - sum; i++) {
            result = block_next(b, result);
        }
        return number_add(b->offset, number_get_small(result));
    }
}

void intset_clear(IntSet *set) {
    if (set->root == NULL) {
        return;
    }
    Block *b = set->root->nexts[0];
    while (b != set->root) {
        Block *next = b->nexts[0];
        block_free(b);
        b = next;
    }
    free(set->root);
    set->root = NULL;
}

IntSetIter *intset_iter(IntSet *set) {
    IntSetIter *iter = (IntSetIter *) malloc(sizeof(IntSetIter));
    iter->set = set;
    iter->current_block = intset_root(set)->nexts[0];
    iter->current_index = -1;
    return iter;
}


void print_intset(IntSet *set) {
    for (int level = 0; level < set->level; level++) {
        Block *block = intset_root(set);
        printf("level %d: root -> ", level);
        block = block->nexts[level];
        while (block != set->root) {
            printf("%li -> ", number_as_long(block->offset));
            block = block->nexts[level];
        }
        printf("root");
        printf("\n");
    }
}

