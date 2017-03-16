//
// Created by lxd on 2017/3/9.
//


#ifndef NUMBER_H
#define NUMBER_H

#ifdef UINT64_MAX
#define DIGIT_SHIFT 30
typedef uint32_t d_digit;
typedef int32_t d_sdigit;
typedef uint64_t d_twodigits;
typedef int64_t d_stwodigits;
#else
#define DIGIT_SHIFT 15
typedef uint16_t d_digit;
typedef int16_t d_sdigit;
typedef uint32_t d_twodigits;
typedef int32_t d_stwodigits;
#endif

#define DIGIT_BASE ((d_digit)1<<DIGIT_SHIFT)
#define DIGIT_MASK ((d_digit)DIGIT_BASE-1)


typedef struct {
    int size;
    d_digit *digits;
} Number;


#endif


Number *number_new(int size);

void number_dump(Number *x);

Number *number_from_long(long x);

long number_as_long(Number *bn);

Number *number_add(Number *a, Number *b);

Number *number_sub(Number *n1, Number *n2);

Number *number_mul(Number *a, Number *b);

int number_divmod(Number *a, Number *b, Number **pdiv, Number **prem);

int number_cmp(Number *a, Number *b);

void number_clear(Number *a);

Number *number_copy(Number *a);

