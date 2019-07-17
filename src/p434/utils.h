/********************************************************************************************
* SIDH: an efficient supersingular isogeny cryptography library
*
* Abstract: internal header file for P434
*********************************************************************************************/

#ifndef UTILS_H_
#define UTILS_H_

#include <stddef.h>
#include <sike/sike.h>

// Conversion macro from number of bits to number of bytes
#define BITS_TO_BYTES(nbits)      (((nbits)+7)/8)

// Bit size of the field
#define BITS_FIELD              434
// Byte size of the field
#define FIELD_BYTESZ            BITS_TO_BYTES(BITS_FIELD)
// Number of 64-bit words of a 224-bit element
#define NBITS_ORDER             224
#define NWORDS64_ORDER          ((NBITS_ORDER+63)/64)
// Number of elements in Alice's strategy
#define A_max                   108
// Number of elements in Bob's strategy
#define B_max                   137
// Word size size
#define RADIX                   sizeof(crypto_word_t)*8
// Byte size of a limb
#define LSZ                     sizeof(crypto_word_t)

#if defined(CPU_64_BIT)
    typedef uint64_t crypto_word_t;
    // Number of words of a 434-bit field element
    #define NWORDS_FIELD    7
    // Number of "0" digits in the least significant part of p434 + 1
    #define ZERO_WORDS 3
    // U64_TO_WORDS expands |x| for a |crypto_word_t| array literal.
    #define U64_TO_WORDS(x) UINT64_C(x)
#else
    typedef uint32_t crypto_word_t;
    // Number of words of a 434-bit field element
    #define NWORDS_FIELD    14
    // Number of "0" digits in the least significant part of p434 + 1
    #define ZERO_WORDS 6
    // U64_TO_WORDS expands |x| for a |crypto_word_t| array literal.
    #define U64_TO_WORDS(x) \
        (uint32_t)(UINT64_C(x) & 0xffffffff), (uint32_t)(UINT64_C(x) >> 32)
#endif

// Extended datatype support
#if !defined(HAS_UINT128)
    typedef uint64_t uint128_t[2];
#endif

// The following functions return 1 (TRUE) if condition is true, 0 (FALSE) otherwise
// Digit multiplication
#define MUL(multiplier, multiplicand, hi, lo) digit_x_digit((multiplier), (multiplicand), &(lo));

// If mask |x|==0xff.ff set |x| to 1, otherwise 0
#define M2B(x) ((x)>>(RADIX-1))

// Digit addition with carry
#define ADDC(carryIn, addend1, addend2, carryOut, sumOut)                   \
do {                                                                        \
  crypto_word_t tempReg = (addend1) + (crypto_word_t)(carryIn);             \
  (sumOut) = (addend2) + tempReg;                                           \
  (carryOut) = M2B(ct_uint_lt(tempReg, (crypto_word_t)(carryIn)) |  \
                   ct_uint_lt((sumOut), tempReg));                  \
} while(0)

// Digit subtraction with borrow
#define SUBC(borrowIn, minuend, subtrahend, borrowOut, differenceOut)           \
do {                                                                            \
    crypto_word_t tempReg = (minuend) - (subtrahend);                           \
    crypto_word_t borrowReg = M2B(ct_uint_lt((minuend), (subtrahend))); \
    borrowReg |= ((borrowIn) & ct_uint_eq(tempReg, 0));               \
    (differenceOut) = tempReg - (crypto_word_t)(borrowIn);                      \
    (borrowOut) = borrowReg;                                                    \
} while(0)

/* Old GCC 4.9 (jessie) doesn't implement {0} initialization properly,
   which violates C11 as described in 6.7.9, 21 (similarily C99, 6.7.8).
   Defines below are used to work around the bug, and provide a way
   to initialize f2elem_t and point_proj_t structs.
   Bug has been fixed in GCC6 (debian stretch).
*/
#define F2ELM_INIT {{ {0}, {0} }}
#define POINT_PROJ_INIT {{ F2ELM_INIT, F2ELM_INIT }}

// Datatype for representing 434-bit field elements (448-bit max.)
// Elements over GF(p434) are encoded in 63 octets in little endian format
// (i.e., the least significant octet is located in the lowest memory address).
typedef crypto_word_t felm_t[NWORDS_FIELD];

// An element in F_{p^2}, is composed of two coefficients from F_p, * i.e.
// Fp2 element = c0 + c1*i in F_{p^2}
// Datatype for representing double-precision 2x434-bit field elements (448-bit max.)
// Elements (a+b*i) over GF(p434^2), where a and b are defined over GF(p434), are
// encoded as {a, b}, with a in the lowest memory portion.
typedef struct {
    felm_t c0;
    felm_t c1;
} fp2;

// Our F_{p^2} element type is a pointer to the struct.
typedef fp2 f2elm_t[1];

// Datatype for representing double-precision 2x434-bit
// field elements in contiguous memory.
typedef crypto_word_t dfelm_t[2*NWORDS_FIELD];

// Constants used during SIKE computation.
struct params_t {
    // Stores a prime
    const crypto_word_t prime[NWORDS_FIELD];
    // Stores prime + 1
    const crypto_word_t prime_p1[NWORDS_FIELD];
    // Stores prime * 2
    const crypto_word_t prime_x2[NWORDS_FIELD];
    // Alice's generator values {XPA0 + XPA1*i, XQA0 + XQA1*i, XRA0 + XRA1*i}
    // in GF(prime^2), expressed in Montgomery representation
    const crypto_word_t A_gen[6*NWORDS_FIELD];
    // Bob's generator values {XPB0 + XPB1*i, XQB0 + XQB1*i, XRB0 + XRB1*i}
    // in GF(prime^2), expressed in Montgomery representation
    const crypto_word_t B_gen[6*NWORDS_FIELD];
    // Montgomery constant mont_R2 = (2^448)^2 mod prime
    const crypto_word_t mont_R2[NWORDS_FIELD];
    // Value 'one' in Montgomery representation
    const crypto_word_t mont_one[NWORDS_FIELD];
    // Value '6' in Montgomery representation
    const crypto_word_t mont_six[NWORDS_FIELD];
    // Fixed parameters for isogeny tree computation
    const unsigned int A_strat[A_max-1];
    const unsigned int B_strat[B_max-1];
};

// Point representation in projective XZ Montgomery coordinates.
typedef struct {
    f2elm_t X;
    f2elm_t Z;
} point_proj;
typedef point_proj point_proj_t[1];

// Checks whether two words are equal. Returns 1 in case it is,
// otherwise 0.
static inline crypto_word_t ct_uint_eq(crypto_word_t x, crypto_word_t y)
{
    // if x==y then t = 0
    crypto_word_t t = x ^ y;
    // if x!=y t will have first bit set
    t = (t >> 1) - t;
    // return MSB - 1 in case x==y, otherwise 0
    return ((~t) >> (RADIX-1));
}
// Constant time select.
// if pick == 1 (out = in1)
// if pick == 0 (out = in2)
// else out is undefined
static inline uint8_t ct_select_8(uint8_t flag, uint8_t in1, uint8_t in2) {
    uint8_t mask = ((int8_t)(flag << 7))>>7;
    return (in1&mask) | (in2&(~mask));
}

// Constant time memcmp. Returns 1 if p==q, otherwise 0
static inline int ct_mem_eq(const void *p, const void *q, size_t n)
{
  const uint8_t *pp = (uint8_t*)p, *qq = (uint8_t*)q;
  uint8_t a = 0;

  while (n--) a |= *pp++ ^ *qq++;
  return (ct_uint_eq(a, 0));
}

/*
// Returns 1 if x<y, otherwise 0
static inline crypto_word_t ct_uint_lt(crypto_word_t x, crypto_word_t y) {
  const crypto_word_t t1 = x^y;
  const crypto_word_t t2 = x - y;
  const crypto_word_t tt = x ^ (t1 | (t2^y));
  return (tt >> (RADIX-1));
}
*/

/// OZAPTF: coppied from boringssl
static inline crypto_word_t constant_time_msb_w(crypto_word_t a) {
  return 0u - (a >> (sizeof(a) * 8 - 1));
}

// constant_time_lt_w returns 0xff..f if a < b and 0 otherwise.
static inline crypto_word_t ct_uint_lt(crypto_word_t x, crypto_word_t y)
{
  /*
  const crypto_word_t t1 = x^y;
  const crypto_word_t t2 = x - y;
  const crypto_word_t tt = x ^ (t1 | (t2^y));
  return (tt >> (RADIX-1));
  */
  // Consider the two cases of the problem:
  //   msb(a) == msb(b): a < b iff the MSB of a - b is set.
  //   msb(a) != msb(b): a < b iff the MSB of b is set.
  //
  // If msb(a) == msb(b) then the following evaluates as:
  //   msb(a^((a^b)|((a-b)^a))) ==
  //   msb(a^((a-b) ^ a))       ==   (because msb(a^b) == 0)
  //   msb(a^a^(a-b))           ==   (rearranging)
  //   msb(a-b)                      (because ∀x. x^x == 0)
  //
  // Else, if msb(a) != msb(b) then the following evaluates as:
  //   msb(a^((a^b)|((a-b)^a))) ==
  //   msb(a^(𝟙 | ((a-b)^a)))   ==   (because msb(a^b) == 1 and 𝟙
  //                                  represents a value s.t. msb(𝟙) = 1)
  //   msb(a^𝟙)                 ==   (because ORing with 1 results in 1)
  //   msb(b)
  //
  //
  // Here is an SMT-LIB verification of this formula:
  //
  // (define-fun lt ((a (_ BitVec 32)) (b (_ BitVec 32))) (_ BitVec 32)
  //   (bvxor a (bvor (bvxor a b) (bvxor (bvsub a b) a)))
  // )
  //
  // (declare-fun a () (_ BitVec 32))
  // (declare-fun b () (_ BitVec 32))
  //
  // (assert (not (= (= #x00000001 (bvlshr (lt a b) #x0000001f)) (bvult a b))))
  // (check-sat)
  // (get-model)
  return constant_time_msb_w(x^((x^y)|((x-y)^x)));
}
#endif // UTILS_H_
