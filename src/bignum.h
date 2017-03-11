
#ifndef _BIGNUM_H
#define _BIGNUM_H
#endif


typedef struct{
    unsigned long *digits;
    int used_digits;
    int signbit;
}BigNum;
