//
// Created by lxd on 2017/2/6.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "intset.h"


static const int popCountTable[] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
                                    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                    1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                    2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                    3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                    4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8};

#define byte_size 256


int count_bit(unsigned long x){
    int count = 0;
    while(x>0){
        count += popCountTable[x%byte_size];
        x>>=8;
    }
    return count;
}

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
        return 1;
    }
    return 0;
}

int block_size(Block * block){
    int count = 0;
    for(int i=0;i<WORDS_PER_BLOCK;i++){
        count += count_bit(block->bits[i]);
    }
    return count;
}

int block_remove(Block *block, unsigned int index) {
    int word_offset;
    unsigned long mask;
    word_offset_and_mask(index, &word_offset, &mask);
    if ((block->bits[word_offset] & mask) != 0) {
        block->bits[word_offset] &= ~mask;
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

long  block_max(Block *block, int *error) {
    for (int i = WORDS_PER_BLOCK - 1; i >= 0; i--) {
        unsigned long word = block->bits[i];
        if (word == 0)continue;
        unsigned long mask = (unsigned long) 1 << (BITS_PER_WORD - 1);
        for (int j = 0; j < BITS_PER_WORD; j++) {
            if (word & mask){
                return block->offset + i * BITS_PER_WORD + BITS_PER_WORD - j - 1;
            }
            *error = 0;
            mask >>= 1;
        }
    }
    *error = 1;
	return 0;
}

long block_min(Block *block, int *error) {
    for (int i = 0; i < WORDS_PER_BLOCK; i++) {
        unsigned long word = block->bits[i];
        if (word == 0)continue;
        unsigned long mask = (unsigned long) 1;
        for (int j = 0; j < BITS_PER_WORD; j++) {
            if (word & mask){
                return block->offset + i * BITS_PER_WORD + j;
            }
            *error = 0;
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


void offset_and_index(long x, long *offset, unsigned int *index) {
    unsigned int base = (unsigned int) (int) (x % BIT_PER_BLOCK);
    *offset = x - base;
    *index = base;
}


Block *intset_start(IntSet *set);


IntSet* intset_copy(IntSet *self){
    IntSet * copied = calloc(1, sizeof(IntSet));

    Block *block_self = intset_start(self);
    Block *copied_block = intset_start(copied);
    while(block_self!=self->root){
        Block *block = calloc(1, sizeof(Block));
        memcpy(block->bits, block_self->bits, sizeof(block_self->bits));
        block->offset = block_self->offset;
        block->prev = copied_block;

        copied_block->next = block;
        copied_block = block;

        block_self = block_self->next;
    }
    copied->root->prev = copied_block;
    copied_block->next = copied->root;
    return copied;
}


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

void intset_removeblock(Block *pBlock);

int intset_remove(IntSet *set, long x) {
    long offset;
    unsigned int index;
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
    unsigned int index;
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


long intset_max(IntSet *set, int *error) {
    return block_max(set->root->prev, error);
}

long intset_min(IntSet *set, int *error) {
    return block_min(set->root->next, error);
}


IntSet *intset_and(IntSet *self, IntSet *other) {
    Block *block_self = intset_start(self);
    Block *block_other = intset_start(other);
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *result_block = intset_start(result_set);

    while (block_self != self->root && block_other != other->root) {
        if (block_self->offset < block_other->offset) {
            block_self = block_self->next;
        } else if (block_self->offset > block_other->offset) {
            block_other = block_other->next;
        } else {
            long offset = block_self->offset;
            unsigned long words[WORDS_PER_BLOCK] = {0};
            int is_empty = 1;
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                unsigned long word = block_self->bits[i] & block_other->bits[i];
                if (word != 0)is_empty = 0;
                words[i] = word;
            }
            block_self = block_self->next;
            block_other = block_other->next;

            if (is_empty == 1)continue;

            Block *block = calloc(1, sizeof(Block));
            memcpy(block->bits, words, sizeof(words));
            block->offset = offset;
            block->prev = result_block;

            result_block->next = block;
            result_block = block;
        }
    }
    result_set->root->prev = result_block;
    result_block->next = result_set->root;
    return result_set;
}

void intset_merge(IntSet *self, IntSet *other){
 //   Block *block_self = intset_start(self);
  //  Block *block_other = intset_start(other);

//    while (block_self != self->root || block_other != other->root) {
//        Block * block;
//        if (block_self == self->root) {
//            block = malloc(sizeof(Block));
//            memcpy(block->bits, block_other->bits, sizeof(block_other->bits));
//            block->offset = block_other->offset;
//
//            block_self->next = block;
//            block->prev = block_self;
//
//            block_other = block_other->next;
//
//        } else if (block_other == other->root) {
//            block->offset = block_self->offset;
//            memcpy(block->bits, block_self->bits, sizeof(block_other->bits));
//            block_self = block_self->next;
//        } else if (block_self->offset == block_other->offset) {
//            unsigned long words[WORDS_PER_BLOCK] = {0};
//            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
//                words[i] = block_self->bits[i] | block_other->bits[i];
//            }
//            memcpy(block->bits, words, sizeof(words));
//            block->offset = block_self->offset;
//
//            block_self = block_self->next;
//            block_other = block_other->next;
//        } else if (block_self->offset > block_other->offset) {
//            block->offset = block_other->offset;
//            memcpy(block->bits, block_other->bits, sizeof(block_other->bits));
//            block_other = block_other->next;
//        } else if (block_self->offset < block_other->offset) {
//            block->offset = block_self->offset;
//            memcpy(block->bits, block_self->bits, sizeof(block_other));
//            block_self = block_self->next;
//        }
//
//        block->prev = result_block;
//        result_block->next = block;
//
//        result_block = block;
//    }
//    result_block->next = result_set->root;
//    result_set->root->prev = result_block;
//    return result_set;
}

IntSet *intset_or(IntSet *self, IntSet *other) {

    Block *block_self = intset_start(self);
    Block *block_other = intset_start(other);
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *result_block = intset_start(result_set);

    while (block_self != self->root || block_other != other->root) {
        Block *block = calloc(1, sizeof(Block));
        if (block_self == self->root) {
            block->offset = block_other->offset;
            memcpy(block->bits, block_other->bits, sizeof(block_other->bits));
            block_other = block_other->next;
        } else if (block_other == other->root) {
            block->offset = block_self->offset;
            memcpy(block->bits, block_self->bits, sizeof(block_other->bits));
            block_self = block_self->next;
        } else if (block_self->offset == block_other->offset) {
            unsigned long words[WORDS_PER_BLOCK] = {0};
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                words[i] = block_self->bits[i] | block_other->bits[i];
            }
            memcpy(block->bits, words, sizeof(words));
            block->offset = block_self->offset;

            block_self = block_self->next;
            block_other = block_other->next;
        } else if (block_self->offset > block_other->offset) {
            block->offset = block_other->offset;
            memcpy(block->bits, block_other->bits, sizeof(block_other->bits));
            block_other = block_other->next;
        } else if (block_self->offset < block_other->offset) {
            block->offset = block_self->offset;
            memcpy(block->bits, block_self->bits, sizeof(block_other));
            block_self = block_self->next;
        }

        block->prev = result_block;
        result_block->next = block;

        result_block = block;
    }
    result_block->next = result_set->root;
    result_set->root->prev = result_block;
    return result_set;

}

IntSet *intset_sub(IntSet *self, IntSet *other) {
    Block *block_self = intset_start(self);
    Block *block_other = intset_start(other);
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *result_block = intset_start(result_set);

    while (block_self != self->root) {
        Block *block = calloc(1, sizeof(Block));
        if (block_self->offset < block_other->offset || block_other == other->root) {
            block->offset = block_self->offset;
            memcpy(block->bits, block_self->bits, sizeof(block_self->bits));
            block_self = block_self->next;
        } else if (block_self->offset > block_other->offset) {
            block_other = block_other->next;
            free(block);
            continue;
        } else {
            block->offset = block_self->offset;
            unsigned long words[WORDS_PER_BLOCK] = {0};
            int is_empty = 1;
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                unsigned long word = block_self->bits[i] & ~block_other->bits[i];
                if (word != 0)is_empty = 0;
                words[i] = word;
            }

            block_self = block_self->next;
            block_other = block_other->next;

            if (is_empty == 1) {
                free(block);
                continue;
            }
            memcpy(block->bits, words, sizeof(words));
        }
        block->prev = result_block;

        result_block->next = block;
        result_block = block;
    }
    result_block->next = result_set->root;
    result_set->root->prev = result_block;

    return result_set;

}


IntSet * intset_xor(IntSet * self, IntSet * other){
    Block *block_self = intset_start(self);
    Block *block_other = intset_start(other);
    IntSet *result_set = calloc(1, sizeof(IntSet));
    Block *result_block = intset_start(result_set);

    while (block_self != self->root || block_other != other->root) {
        Block *block = calloc(1, sizeof(Block));
        if (block_self == self->root) {
            block->offset = block_other->offset;
            memcpy(block->bits, block_other->bits, sizeof(block_other->bits));
            block_other = block_other->next;
        } else if (block_other == other->root) {
            block->offset = block_self->offset;
            memcpy(block->bits, block_self->bits, sizeof(block_other->bits));
            block_self = block_self->next;
        } else if (block_self->offset == block_other->offset) {
            block->offset = block_self->offset;
            unsigned long words[WORDS_PER_BLOCK] = {0};
            int is_empty = 1;
            for (int i = 0; i < WORDS_PER_BLOCK; i++) {
                unsigned long word = block_self->bits[i] ^ block_other->bits[i];
                if(word!=0)is_empty=0;
                words[i] = word;
            }
            block_self = block_self->next;
            block_other = block_other->next;
            if(is_empty==1){
                continue;
                free(block);
            }
            memcpy(block->bits, words, sizeof(words));
       } else if (block_self->offset > block_other->offset) {
            block->offset = block_other->offset;
            memcpy(block->bits, block_other->bits, sizeof(block_other->bits));
            block_other = block_other->next;
        } else if (block_self->offset < block_other->offset) {
            block->offset = block_self->offset;
            memcpy(block->bits, block_self->bits, sizeof(block_other));
            block_self = block_self->next;
        }

        block->prev = result_block;
        result_block->next = block;

        result_block = block;
    }
    result_block->next = result_set->root;
    result_set->root->prev = result_block;
    return result_set;
}


int intset_equals(IntSet * self, IntSet * other){
    //other == self
    Block *block_self = intset_start(self);
    Block *block_other = intset_start(other);

    while(1){
        if(block_self==self->root && block_other==other->root){
            return 1;
        }
        else if(block_self==self->root || block_other == other->root){
            return 0;
        }
        else if(block_self->offset != block_other->offset){
          return 0;
        }
        else{
            for(int i=0;i<WORDS_PER_BLOCK;i++){
                if(block_self->bits[i]!=block_other->bits[i])return 0;
            }
            block_self = block_self->next;
            block_other = block_other->next;
        }
    }
}


int intset_issuperset(IntSet * self, IntSet* other ){
    //other <= self
    Block *block_self = intset_start(self);
    Block *block_other = intset_start(other);

    while(block_other != other->root){
        if(block_self == self->root){
            return 0;
        }
        else if(block_self->offset < block_other->offset){
            block_self = block_self->next;
        }else if(block_self->offset > block_other->offset){
            return 0;
        }else{
            for(int i=0;i<WORDS_PER_BLOCK;i++){
                if ((block_self->bits[i] | block_other->bits[i]) != block_self->bits[i]){
                    return 0;
                }
           }
            block_self = block_self->next;
            block_other = block_other->next;
        }
    }
    return 1;
}


int intset_issubset(IntSet *self, IntSet *other){
    //other >= self
    Block *block_self = intset_start(self);
    Block *block_other = intset_start(other);

    while(block_self != self->root){
        if(block_other == other->root){
            return 0;
        }
        else if(block_self->offset > block_other->offset){
            block_other = block_other->next;
        }else if(block_self->offset < block_other->offset){
            return 0;
        }else{
            for(int i=0;i<WORDS_PER_BLOCK;i++){
                if ((block_self->bits[i] | block_other->bits[i]) != block_other->bits[i]){
                    return 0;
                }
           }
            block_self = block_self->next;
            block_other = block_other->next;
        }
    }
    return 1;
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
