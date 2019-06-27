/*************************************************************************
    > File Name: number_test.c
    > Author:
    > Mail:
    > Created Time: 四  6/27 13:27:10 2019
 ************************************************************************/

#include<stdio.h>
#include <assert.h>
#include "number.h"


void test_number_cmp(){
    Number* a;
    Number* b;


    a = number_from_long(((long)1)<<32);
    b = number_from_long(((long)1)<<33);
    assert(number_cmp(a, b) < 0);
}

int main(){
    test_number_cmp();
    return 0;
}
