//
// Created by lxd on 2017/3/9.
//

#include<Python.h>
#include <longintrepr.h>

#ifndef NUMBER_H
#define NUMBER_H


typedef struct {
    int size;
    digit *digits;
    int islong;
} Number;


#endif


Number *number_new(int size);

void number_dump(Number *x);

Number *number_from_long(long x);

Number* number_get_small(int x);

long number_as_long(Number *bn);

Number *number_add(Number *a, Number *b);

Number *number_sub(Number *n1, Number *n2);

int number_splice(Number *in, int n);

int number_cmp(Number *a, Number *b);

void number_clear(Number *a);

Number *number_copy(Number *a);

