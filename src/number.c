//
// Created by lxd on 2017/3/9.
//


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "number.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))


Number *number_new(int size) {
    Number *n = malloc(sizeof(Number));
    n->size = size;
    d_digit *ds = calloc((size_t) ABS(size), sizeof(d_digit));
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
        z->digits[i] = (d_digit) (x & DIGIT_MASK);
        x >>= DIGIT_SHIFT;
    }
    z->size = (l < 0) ? -size : size;
    return z;
}

void number_dump(Number *x) {
	if(x->size==0){
		 printf("0");
	}else{
		if (x->size < 0) {
			printf("-");
		}
	   int size = ABS(x->size);
		for (int i = size - 1; i >= 0; i--) {
			printf("%x", x->digits[i]);
		}
	}
	printf("\n");
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

    d_digit carry = 0;
    int i;
    for (i = 0; i < size_b; i++) {
        carry += a->digits[i] + b->digits[i];
        z->digits[i] = (d_digit) (carry & DIGIT_MASK);
        carry >>= DIGIT_SHIFT;
    }
    for (; i < size_a; i++) {
        carry += a->digits[i];
        z->digits[i] = (d_digit) (carry & DIGIT_MASK);
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
    d_digit borrow = 0;
    for (i = 0; i < size_b; i++) {
        borrow = a->digits[i] - b->digits[i] - borrow;
        z->digits[i] = (d_digit) (borrow & DIGIT_MASK);
        borrow >>= DIGIT_SHIFT;
        borrow &= 1;
    }
    for (; i < size_a; i++) {
        borrow = a->digits[i] - borrow;
        z->digits[i] = (d_digit) (borrow & DIGIT_MASK);
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
        d_twodigits carry = 0;
        d_twodigits ai = a->digits[i];
        for (j = 0; j < size_b; j++) {
            d_twodigits bj = b->digits[j];
            carry += z->digits[i + j] + ai * bj;
            z->digits[i + j] = (d_digit) (carry & DIGIT_MASK);
            carry >>= DIGIT_SHIFT;
        }
        if (carry) {
            z->digits[i + j] += (d_digit) (carry & DIGIT_MASK);
        }
    }
    return number_normalize(z);
}


const unsigned char BitLengthTable[32] = {
        0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};

int bits_in_digit(d_digit d) {
    int d_bits = 0;
    while (d >= 32) {
        d_bits += 6;
        d >>= 6;
    }
    d_bits += (int) BitLengthTable[d];
    return d_bits;
}

d_digit v_lshift(d_digit *z, d_digit *a, int m, int d) {
    d_digit carry = 0;
    for (int i = 0; i < m; i++) {
        d_twodigits acc = (d_twodigits) a[i] << d | carry;
        z[i] = (d_digit) ((d_digit) acc & DIGIT_MASK);
        carry = (d_digit) (acc >> DIGIT_SHIFT);
    }
    return carry;
}

d_digit v_rshift(d_digit *z, d_digit *a, int m, int d) {
    int i;
    d_digit carry = 0;
    d_digit mask = (d_digit) (((d_digit) 1 << d) - 1U);

    for (i = m; i-- > 0;) {
        d_twodigits acc = (d_twodigits) carry << DIGIT_SHIFT | a[i];
        carry = (d_digit) acc & mask;
        z[i] = (d_digit) (acc >> d);
    }
    return carry;
}

#define ARITHMETIC_RIGHT_SHIFT(I, J) \
    ((I) < 0 ? -1-((-1-(I)) >> (J)) : (I) >> (J))

Number *x_divrem(Number *v1, Number *w1, Number **prem) {
    Number *v, *w, *a;
    int i, k, size_v, size_w;
    int d;
    d_digit wm1, wm2, carry, q, r, vtop, *v0, *vk, *w0, *ak;
    d_twodigits vv;
    d_sdigit zhi;
    d_stwodigits z;

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
        vv = ((d_twodigits) vtop << DIGIT_SHIFT) | vk[size_w - 1];
        q = (d_digit) (vv / wm1);
        r = (d_digit) (vv - (d_twodigits) wm1 * q); /* r = vv % wm1 */
        while ((d_twodigits) wm2 * q > (((d_twodigits) r << DIGIT_SHIFT)
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
            z = (d_sdigit) vk[i] + zhi -
                (d_stwodigits) q * (d_stwodigits) w0[i];
            vk[i] = (d_digit) ((d_digit) z & DIGIT_MASK);
            zhi = (d_sdigit) ARITHMETIC_RIGHT_SHIFT(z, DIGIT_SHIFT);
        }

        /* add w back if q was too large (this branch taken rarely) */
        assert((d_sdigit) vtop + zhi == -1 || (d_sdigit) vtop + zhi == 0);
        if ((d_sdigit) vtop + zhi < 0) {
            carry = 0;
            for (i = 0; i < size_w; ++i) {
                carry += vk[i] + w0[i];
                vk[i] = (d_digit) (carry & DIGIT_MASK);
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


Number *x_divrem1(Number *a, d_digit n, d_digit *prem) {
    int size = ABS(a->size);
    Number *z;

    z = number_new(size);

    d_twodigits rem = 0;


    assert(n > 0 && n < DIGIT_MASK);

    d_digit *pin = a->digits;
    d_digit *pout = z->digits;

    pin += size;
    pout += size;

    while (--size >= 0) {
        d_digit hi;
        rem = (rem << DIGIT_SHIFT) | *--pin;
        *--pout = hi = (d_digit) (rem / n);
        rem -= (d_twodigits) hi * n;
    }

    *prem = (d_digit) rem;
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
        d_digit rem = 0;
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

int number_cmp(Number *a, Number *b) {
    int sign;
    if (a->size != b->size) {
        sign = a->size - b->size;
    } else {
        int i = ABS(a->size);

        while (--i >= 0 && a->digits[i] == b->digits[i]);
        if (i < 0)
            sign = 0;
        else {
            sign = (d_sdigit) a->digits[i] - (d_sdigit) b->digits[i];
            if (a->size < 0)
                sign = -sign;
        }
    }
    return sign < 0 ? -1 : sign > 0 ? 1 : 0;
}


Number * number_copy(Number *a){
	Number *r = number_new(a->size);
	memcpy(r->digits, a->digits, sizeof(d_digit)*ABS(a->size));
	return r;
}

void number_clear(Number *a) {
	if((a->size)>0){
		free(a->digits);
	}
    free(a);
}
