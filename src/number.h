//
// Created by lxd on 2017/3/9.
//

#ifndef CINTSET_BIGNUM_H
#define CINTSET_BIGNUM_H


#endif //CINTSET_BIGNUM_H


typedef uint16_t digit;
typedef int16_t sdigit;
typedef uint32_t twodigits;
typedef int32_t stwodigits;

typedef struct {
    int size;
    digit *digits;
} Number;


void number_dump(Number *x);

Number *number_from_long(long x);

long number_as_long(Number *bn);

Number *number_add(Number *a, Number *b);

Number *number_sub(Number *n1, Number *n2);

Number *number_mul(Number *a, Number *b);

int number_divmod(Number *a, Number *b, Number **pdiv, Number **prem);

int number_compare(Number *a, Number *b);
