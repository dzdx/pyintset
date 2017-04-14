//
// Created by lxd on 2017/3/9.
//

#include <stdint.h>
#ifndef NUMBER_H
#define NUMBER_H

#ifdef UINT64_MAX
typedef uint64_t num;
typedef int64_t snum;
#define NUM_SHIFT 60
#else
typedef uint32_t num;
typedef int32_t snum;
#define NUM_SHIFT 30
#endif
#define NUM_MASK (((num)1<<NUM_SHIFT)-1)


typedef struct {
    int size;
    num *digits;
} Number;


#endif


Number *number_new(int size);

void number_dump(Number *x);

Number *number_from_long(long x);

Number* number_get_small(int x);

long number_as_long(Number *bn);

Number *number_add(Number *a, Number *b);

Number *number_sub(Number *n1, Number *n2);

int number_slice(Number *in, int n);

int number_cmp(Number *a, Number *b);

void number_clear(Number *a);

Number *number_copy(Number *a);

int number_bitlen(Number *v);
