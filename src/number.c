//
// Created by lxd on 2017/3/9.
//


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "number.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define CACHED_LIMIT 256





static Number small_numbers[CACHED_LIMIT];
static int small_numbers_is_fill = 0;


Number *number_new(int size) {
    Number *n = malloc(sizeof(Number));
    n->size = size;
    num *ds = calloc((size_t) ABS(size), sizeof(num));
    n->digits = ds;
    return n;
}


int number_bitlen(Number *v){
    num d = v->digits[ABS(v->size)-1];
    for(int i=0;i<NUM_SHIFT;i--){
        if((d & (1<<(NUM_SHIFT-1 - i)))!=0){
            return (ABS(v->size)-1)*NUM_SHIFT - i;
        }
    }
    return (ABS(v->size)-2)*NUM_SHIFT;
}

Number *number_normalize(Number *v) {
    int i = ABS(v->size);
    while (i > 0 && v->digits[i - 1] == 0) {
        i--;
    }
    v->size = (v->size < 0) ? -i : i;
    return v;
}


long number_as_long(Number *bn) {
    long r = 0;
    int size = ABS(bn->size);
    for (int i = 0; i < size; i++) {
        r += (((long) bn->digits[i]) << (i * NUM_SHIFT));
    }
    return (bn->size < 0) ? -r : r;
}


Number *number_from_long(long l){
    long x = ABS(l);
    long y = x;
    int size = 0;
    while (y > 0) {
        y >>= NUM_SHIFT;
        size++;
    }

    Number *z = number_new(size);
    for (int i = 0; i < size; i++) {
        z->digits[i] = (num) (x & NUM_MASK);
        x >>= NUM_SHIFT;
    }
    z->size = (l < 0) ? -size : size;
    return z;
}

Number* number_get_small(int x){
    if(x >= CACHED_LIMIT){
        return NULL;
    }
    if(!small_numbers_is_fill){
        for(int i=0;i<CACHED_LIMIT;i++){
            small_numbers[i] = *number_from_long(i);
        }
        small_numbers_is_fill = 1;
    }
    return &small_numbers[x];
}

Number *x_add(Number *a, Number *b) {
    int size_a = ABS(a->size),
            size_b = ABS(b->size);

    //保证size_a >=size_b
    if (size_a < size_b) {
        Number *tmp = a;
        a = b;
        b = tmp;
        int size_tmp = size_a;
        size_a = size_b;
        size_b = size_tmp;
    }
    Number *z = number_new(size_a + 1);

    num carry = 0;
    int i;
    for (i = 0; i < size_b; i++) {
        carry += a->digits[i] + b->digits[i];
        z->digits[i] = (num) (carry & NUM_MASK);
        carry >>= NUM_SHIFT;
    }
    for (; i < size_a; i++) {
        carry += a->digits[i];
        z->digits[i] = (num) (carry & NUM_MASK);
        carry >>= NUM_SHIFT;
    }
    z->digits[i] = carry;
    return number_normalize(z);
}


Number *x_sub(Number *a, Number *b) {
    Number *z;
    int size_a = ABS(a->size), size_b = ABS(b->size);
    int sign = 1;
    //abs(a) >= abs(b)
    if (size_a < size_b) {
        sign = -1;
        Number *tmp = a;
        a = b;
        b = tmp;
        int size_tmp = size_a;
        size_a = size_b;
        size_b = size_tmp;

    } else if (size_a == size_b) {
        int i = size_a;
        while (--i >= 0 && a->digits[i] == b->digits[i]);

        if (i < 0)
            return number_new(0);
        if (a->digits[i] < b->digits[i]) {
            sign = -1;
            Number *tmp = a;
            a = b;
            b = tmp;
        }
        size_a = size_b = i + 1;
    }

    z = number_new(size_a);
    int i;
    num borrow = 0;
    for (i = 0; i < size_b; i++) {
        borrow = a->digits[i] - b->digits[i] - borrow;
        z->digits[i] = (num) (borrow & NUM_MASK);
        borrow >>= NUM_SHIFT;
        borrow &= 1;
    }
    for (; i < size_a; i++) {
        borrow = a->digits[i] - borrow;
        z->digits[i] = (num) (borrow & NUM_MASK);
        borrow >>= NUM_SHIFT;
        borrow &= 1;
    }
    if (sign < 0)
        z->size = -z->size;
    return number_normalize(z);

}

Number *number_add(Number *a, Number *b) {
    Number *z;
    if (a->size < 0) {
        if (b->size < 0) {
            z = x_add(a, b);
            z->size = -z->size;
        } else
            z = x_sub(b, a);
    } else {
        if (b->size < 0)
            z = x_sub(a, b);
        else
            z = x_add(a, b);
    }
    return z;
}

Number *number_sub(Number *a, Number *b) {
    Number *z;

    if (a->size < 0) {
        if (b->size < 0)
            z = x_sub(a, b);
        else
            z = x_add(a, b);
        z->size = -z->size;
    } else {
        if (b->size < 0)
            z = x_add(a, b);
        else
            z = x_sub(a, b);
    }
    return z;

}


int number_cmp(Number *a, Number *b) {
    snum sign;
    if (a->size != b->size) {
        sign = (snum)(a->size - b->size);
    } else {
        int i = ABS(a->size);

        while (--i >= 0 && a->digits[i] == b->digits[i]);
        if (i < 0)
            sign = 0;
        else {
            sign = (snum) a->digits[i] - (snum) b->digits[i];
            if (a->size < 0)
                sign = -sign;
        }
    }
    return sign < 0 ? -1 : sign > 0 ? 1 : 0;
}


int number_slice(Number *in,int n){
    int idx = 0;
    for(int i=0;n>0&&i<ABS(in->size);i++){
        num d = in->digits[i];
        idx += d&((1<<n)-1);
        n -= NUM_SHIFT;
    }
    return idx;
}

Number * number_copy(Number *a){
	Number *r = number_new(a->size);
	memcpy(r->digits, a->digits, sizeof(num)*ABS(a->size));
	return r;
}

void number_clear(Number *a) {
	if((a->size)>0){
		free(a->digits);
	}
    free(a);
}
