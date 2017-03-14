//
// Created by lxd on 2017/3/9.
//


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "number.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))


#define DIGIT_SHIFT 15
#define DIGIT_BASE ((digit)1<<DIGIT_SHIFT)

#define DIGIT_MASK ((digit)DIGIT_BASE-1)


Number *number_new(int size) {
    Number *n = malloc(sizeof(Number));
    n->size = size;
    digit *ds = calloc((size_t) ABS(size), sizeof(digit));
    n->digits = ds;
    return n;
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
        r += (((long) bn->digits[i]) << (i * DIGIT_SHIFT));
    }
    return (bn->size < 0) ? -r : r;
}

Number *number_from_long(long l) {
    long x = ABS(l);
    long y = x;
    int size = 0;
    while (y > 0) {
        y >>= DIGIT_SHIFT;
        size++;
    }

    Number *z = number_new(size);
    for (int i = 0; i < size; i++) {
        z->digits[i] = (digit) (x & DIGIT_MASK);
        x >>= DIGIT_SHIFT;
    }
    z->size = (l < 0) ? -size : size;
    return z;
}

void number_dump(Number *x) {
    if (x->size < 0) {
        printf("-");
    }
    int size = ABS(x->size);
    for (int i = size - 1; i >= 0; i--) {
        printf("%x", x->digits[i]);
    }
    printf("\n");
    return;
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

    digit carry = 0;
    int i;
    for (i = 0; i < size_b; i++) {
        carry += a->digits[i] + b->digits[i];
        z->digits[i] = (digit) (carry & DIGIT_MASK);
        carry >>= DIGIT_SHIFT;
    }
    for (; i < size_a; i++) {
        carry += a->digits[i];
        z->digits[i] = (digit) (carry & DIGIT_MASK);
        carry >>= DIGIT_SHIFT;
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
    digit borrow = 0;
    for (i = 0; i < size_b; i++) {
        borrow = a->digits[i] - b->digits[i] - borrow;
        z->digits[i] = (digit) (borrow & DIGIT_MASK);
        borrow >>= DIGIT_SHIFT;
        borrow &= 1;
    }
    for (; i < size_a; i++) {
        borrow = a->digits[i] - borrow;
        z->digits[i] = (digit) (borrow & DIGIT_MASK);
        borrow >>= DIGIT_SHIFT;
        borrow &= 1;
    }
    if (sign < 0)
        z->size = -z->size;
    return number_normalize(z);

}

Number *x_mul(Number *a, Number *b) {
    int size_a = ABS(a->size), size_b = ABS(b->size);
    Number *z = number_new(size_a + size_b);
    int i;
    int j;

    for (i = 0; i < size_a; i++) {
        twodigits carry = 0;
        twodigits ai = a->digits[i];
        for (j = 0; j < size_b; j++) {
            twodigits bj = b->digits[j];
            carry += z->digits[i + j] + ai * bj;
            z->digits[i + j] = (digit) (carry & DIGIT_MASK);
            carry >>= DIGIT_SHIFT;
        }
        if (carry) {
            z->digits[i + j] += (digit) (carry & DIGIT_MASK);
        }
    }
    return number_normalize(z);
}


const unsigned char BitLengthTable[32] = {
        0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

int bits_in_digit(digit d) {
    int d_bits = 0;
    while (d >= 32) {
        d_bits += 6;
        d >>= 6;
    }
    d_bits += (int) BitLengthTable[d];
    return d_bits;
}

digit v_lshift(digit *z, digit *a, int m, int d) {
    digit carry = 0;
    for (int i = 0; i < m; i++) {
        twodigits acc = (twodigits) a[i] << d | carry;
        z[i] = (digit) ((digit) acc & DIGIT_MASK);
        carry = (digit) (acc >> DIGIT_SHIFT);
    }
    return carry;
}

digit v_rshift(digit *z, digit *a, int m, int d) {
    int i;
    digit carry = 0;
    digit mask = (digit) (((digit) 1 << d) - 1U);

    for (i = m; i-- > 0;) {
        twodigits acc = (twodigits) carry << DIGIT_SHIFT | a[i];
        carry = (digit) acc & mask;
        z[i] = (digit) (acc >> d);
    }
    return carry;
}

#define ARITHMETIC_RIGHT_SHIFT(I, J) \
    ((I) < 0 ? -1-((-1-(I)) >> (J)) : (I) >> (J))

Number *x_divrem(Number *v1, Number *w1, Number **prem) {
    Number *v, *w, *a;
    int i, k, size_v, size_w;
    int d;
    digit wm1, wm2, carry, q, r, vtop, *v0, *vk, *w0, *ak;
    twodigits vv;
    sdigit zhi;
    stwodigits z;

    /* We follow Knuth [The Art of Computer Programming, Vol. 2 (3rd
       edn.), section 4.3.1, Algorithm D], except that we don't explicitly
       handle the special case when the initial estimate q for a quotient
       digit is >= PyLong_BASE: the max value for q is PyLong_BASE+1, and
       that won't overflow a digit. */

    /* allocate space; w will also be used to hold the final remainder */
    size_v = ABS(v1->size);
    size_w = ABS(w1->size);
    assert(size_v >= size_w && size_w >= 2); /* Assert checks by div() */
    v = number_new(size_v + 1);
    w = number_new(size_w);

    /* normalize: shift w1 left so that its top digit is >= PyLong_BASE/2.
       shift v1 left by the same amount.  Results go into w and v. */
    d = DIGIT_SHIFT - bits_in_digit(w1->digits[size_w - 1]);
    carry = v_lshift(w->digits, w1->digits, size_w, d);
    assert(carry == 0);
    carry = v_lshift(v->digits, v1->digits, size_v, d);
    if (carry != 0 || v->digits[size_v - 1] >= w->digits[size_w - 1]) {
        v->digits[size_v] = carry;
        size_v++;
    }

    /* Now v->ob_digit[size_v-1] < w->ob_digit[size_w-1], so quotient has
       at most (and usually exactly) k = size_v - size_w digits. */
    k = size_v - size_w;
    assert(k >= 0);
    a = number_new(k);
    v0 = v->digits;
    w0 = w->digits;
    wm1 = w0[size_w - 1];
    wm2 = w0[size_w - 2];
    for (vk = v0 + k, ak = a->digits + k; vk-- > v0;) {
        /* inner loop: divide vk[0:size_w+1] by w0[0:size_w], giving
           single-digit quotient q, remainder in vk[0:size_w]. */

        /* estimate quotient digit q; may overestimate by 1 (rare) */
        vtop = vk[size_w];
        assert(vtop <= wm1);
        vv = ((twodigits) vtop << DIGIT_SHIFT) | vk[size_w - 1];
        q = (digit) (vv / wm1);
        r = (digit) (vv - (twodigits) wm1 * q); /* r = vv % wm1 */
        while ((twodigits) wm2 * q > (((twodigits) r << DIGIT_SHIFT)
                                      | vk[size_w - 2])) {
            --q;
            r += wm1;
            if (r >= DIGIT_BASE)
                break;
        }
        assert(q <= DIGIT_BASE);

        /* subtract q*w0[0:size_w] from vk[0:size_w+1] */
        zhi = 0;
        for (i = 0; i < size_w; ++i) {
            /* invariants: -PyLong_BASE <= -q <= zhi <= 0;
               -PyLong_BASE * q <= z < PyLong_BASE */
            z = (sdigit) vk[i] + zhi -
                (stwodigits) q * (stwodigits) w0[i];
            vk[i] = (digit) ((digit) z & DIGIT_MASK);
            zhi = (sdigit) ARITHMETIC_RIGHT_SHIFT(z, DIGIT_SHIFT);
        }

        /* add w back if q was too large (this branch taken rarely) */
        assert((sdigit) vtop + zhi == -1 || (sdigit) vtop + zhi == 0);
        if ((sdigit) vtop + zhi < 0) {
            carry = 0;
            for (i = 0; i < size_w; ++i) {
                carry += vk[i] + w0[i];
                vk[i] = (digit) (carry & DIGIT_MASK);
                carry >>= DIGIT_SHIFT;
            }
            --q;
        }

        /* store quotient digit */
        assert(q < DIGIT_BASE);
        *--ak = q;
    }

    /* unshift remainder; we reuse w to store the result */
    carry = v_rshift(w0, v0, size_w, d);
    assert(carry == 0);

    *prem = number_normalize(w);
    return number_normalize(a);
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

Number *number_mul(Number *a, Number *b) {
    Number *n3 = x_mul(a, b);
    if ((a->size ^ b->size) < 0)
        n3->size = -n3->size;
    return n3;
}


Number *x_divrem1(Number *a, digit n, digit *prem) {
    int size = ABS(a->size);
    Number *z;

    z = number_new(size);

    twodigits rem = 0;


    assert(n > 0 && n < DIGIT_MASK);

    digit *pin = a->digits;
    digit *pout = z->digits;

    pin += size;
    pout += size;

    while (--size >= 0) {
        digit hi;
        rem = (rem << DIGIT_SHIFT) | *--pin;
        *--pout = hi = (digit) (rem / n);
        rem -= (twodigits) hi * n;
    }

    *prem = (digit) rem;
    return number_normalize(z);
}

int number_divmod(Number *a, Number *b, Number **pdiv, Number **prem) {
    int size_a = ABS(a->size), size_b = ABS(b->size);
    Number *z;
    if (size_b == 0) {
        return -1;
    }
    if (size_a < size_b ||
        (size_a == size_b &&
         a->digits[size_a - 1] < b->digits[size_b - 1])) {
        *pdiv = number_from_long(0);

        *prem = number_from_long(number_as_long(a));

        return 0;
    }
    if (size_b == 1) {
        digit rem = 0;
        z = x_divrem1(a, b->digits[0], &rem);
        *prem = number_from_long((long) rem);

    } else {
        z = x_divrem(a, b, prem);
    }
    if ((a->size < 0) != (b->size < 0))
        z->size = -(z->size);
    if (a->size < 0 && (*prem)->size != 0)
        (*prem)->size = -((*prem)->size);
    *pdiv = z;
    return 0;
}

int number_compare(Number *a, Number *b) {
    int sign;
    if (a->size != b->size) {
        sign = a->size - b->size;
    } else {
        int i = ABS(a->size);

        while (--i >= 0 && a->digits[i] == b->digits[i]);
        if (i < 0)
            sign = 0;
        else {
            sign = (sdigit) a->digits[i] - (sdigit) b->digits[i];
            if (a->size < 0)
                sign = -sign;
        }
    }
    return sign < 0 ? -1 : sign > 0 ? 1 : 0;
}