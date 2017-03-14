//
// Created by lxd on 2017/3/9.
//

#ifndef NUMBER_H
#define NUMBER_H
typedef uint16_t digit;
typedef int16_t sdigit;
typedef uint32_t twodigits;
typedef int32_t stwodigits;


typedef struct {
    int size;
    digit *digits;
} Number;


#endif



void number_dump(Number *x);

Number *number_from_long(long x);

long number_as_long(Number *bn);

Number *number_add(Number *a, Number *b);

Number *number_sub(Number *n1, Number *n2);

Number *number_mul(Number *a, Number *b);

int number_divmod(Number *a, Number *b, Number **pdiv, Number **prem);

int number_cmp(Number *a, Number *b);

void number_clear(Number *a);

Number * number_copy(Number *a);
