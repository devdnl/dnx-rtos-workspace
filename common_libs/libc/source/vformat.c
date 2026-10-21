/*==============================================================================
@file   vformat.c

@author Daniel Zorychta

@brief  Formatted output core used by the printf() family.

        The implementation follows the POSIX (IEEE Std 1003.1) / C99
        specification of fprintf():

        - flags:      '-', '+', ' ', '#', '0' and '\'' (no effect in C locale)
        - width:      decimal number or '*' (negative means '-' flag)
        - precision:  '.' followed by decimal number or '*' (negative means
                      precision omitted)
        - length:     hh, h, l, ll, j, z, t, L
        - conversion: d, i, o, u, x, X, f, F, e, E, g, G, a, A, c, s, p, n, %

        The return value is the number of characters produced (also these not
        stored by snprintf() because of the buffer size), or -1 on error.

        Limitations:
        - positional arguments (%n$) are not supported (XSI extension); such a
          conversion specification is printed as is,
        - wide characters (%lc, %ls) are supported only for ASCII (C locale),
          other characters cause EILSEQ error,
        - floating point conversion is computed with double-double arithmetic
          (~106 bits). Up to 17 significant digits are always exact, digits
          up to the 30th are exact in practice, next digits are printed as 0.
          The long double (L) argument is converted to double,
        - EOVERFLOW is not defined in dnx RTOS, so ERANGE is set if the result
          is longer than INT_MAX.

        Note: the double-double algorithms require that the compiler does not
        contract floating point expressions (-ffp-contract=off). It is the
        default for ISO C mode (-std=c18).

        MIT License

        Copyright (C) 2026 Daniel Zorychta <daniel.zorychta@gmail.com>

        Permission is hereby granted, free of charge, to any person obtaining
        a copy of this software and associated documentation files (the
        "Software"), to deal in the Software without restriction, including
        without limitation the rights to use, copy, modify, merge, publish,
        distribute, sublicense, and/or sell copies of the Software, and to
        permit persons to whom the Software is furnished to do so, subject to
        the following conditions:

        The above copyright notice and this permission notice shall be included
        in all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
        OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
        FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
        THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
        OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
        ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
        OTHER DEALINGS IN THE SOFTWARE.

==============================================================================*/

/*==============================================================================
  Include files
==============================================================================*/
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <libc/source/vformat.h>

/*==============================================================================
  Local macros
==============================================================================*/
#define FL_LEFT  0x01 //!< '-' flag
#define FL_PLUS  0x02 //!< '+' flag
#define FL_SPACE 0x04 //!< ' ' flag
#define FL_ALT   0x08 //!< '#' flag
#define FL_ZERO  0x10 //!< '0' flag

#define FP_DIGITS     30 //!< number of generated significant digits
#define CHUNK_LEN     16 //!< size of local buffers used to batch output
#define MAX_FIELD_LEN (INT_MAX - 1024)

// functions with large frames are not inlined to keep stack usage of the
// format loop low (the printf() family is used by programs with small stacks)

/*==============================================================================
  Local object types
==============================================================================*/
typedef enum {
        LEN_NONE,
        LEN_HH,
        LEN_H,
        LEN_L,
        LEN_LL,
        LEN_J,
        LEN_Z,
        LEN_T,
        LEN_LD,
} len_mod_t;

typedef struct {
        uint8_t flags; //!< FL_* flags
        int width;     //!< minimal field width
        int prec;      //!< precision, -1 if omitted
} spec_t;

typedef struct {
        double hi;
        double lo;
} dd_t;

typedef struct {
        char dig[FP_DIGITS]; //!< significant digits, dig[0] is the most significant
        int n;               //!< number of valid digits, 0 means value 0
        int e10;             //!< decimal exponent of dig[0]
        bool sticky;         //!< nonzero digits exist beyond dig[]
} fpdec_t;

/*==============================================================================
  Local function prototypes
==============================================================================*/

/*==============================================================================
  Local objects
==============================================================================*/
/** powers of 10 that are exact in double */
static const double POW10[] = {
        1e0,
        1e1,
        1e2,
        1e3,
        1e4,
        1e5,
        1e6,
        1e7,
        1e8,
        1e9,
        1e10,
        1e11,
        1e12,
        1e13,
        1e14,
        1e15,
        1e16,
        1e17,
        1e18,
        1e19,
        1e20,
        1e21,
        1e22,
};

/*==============================================================================
  Exported objects
==============================================================================*/

/*==============================================================================
  External objects
==============================================================================*/

/*==============================================================================
  Function definitions
==============================================================================*/

//==============================================================================
/**
 * @brief  Pass text to the output sink.
 *
 * @param  out          output sink
 * @param  str          text
 * @param  len          text length
 */
//==============================================================================
static void out_write(_libc_fmt_out_t *out, const char *str, size_t len)
{
        if (len > 0) {
                out->count += len;

                if (!out->error && !out->write(out, str, len)) {
                        out->error = true;
                }
        }
}

//==============================================================================
/**
 * @brief  Pass single character to the output sink.
 *
 * @param  out          output sink
 * @param  c            character
 */
//==============================================================================
static void out_char(_libc_fmt_out_t *out, char c)
{
        out_write(out, &c, 1);
}

//==============================================================================
/**
 * @brief  Pass repeated character to the output sink.
 *
 * @param  out          output sink
 * @param  c            character
 * @param  n            number of characters (may be negative = nothing)
 */
//==============================================================================
static void out_pad(_libc_fmt_out_t *out, char c, int n)
{
        char buf[CHUNK_LEN];
        memset(buf, c, sizeof(buf));

        while ((n > 0) && !out->error) {
                int len = (n < CHUNK_LEN) ? n : CHUNK_LEN;
                out_write(out, buf, len);
                n -= len;
        }

        if (n > 0) {
                out->count += n; // count characters not written because of error
        }
}

//==============================================================================
/**
 * @brief  Print string field (%s, %c and special values).
 *
 * @param  out          output sink
 * @param  sp           conversion specification
 * @param  str          string
 * @param  len          string length
 */
//==============================================================================
static void fmt_str(_libc_fmt_out_t *out, const spec_t *sp, const char *str, size_t len)
{
        int pad = (len < (size_t)sp->width) ? sp->width - (int)len : 0;

        if (!(sp->flags & FL_LEFT)) {
                out_pad(out, ' ', pad);
        }

        out_write(out, str, len);

        if (sp->flags & FL_LEFT) {
                out_pad(out, ' ', pad);
        }
}

//==============================================================================
/**
 * @brief  Print integer field (%d, %i, %o, %u, %x, %X, %p).
 *
 * @param  out          output sink
 * @param  sp           conversion specification
 * @param  conv         conversion character
 * @param  val          absolute value
 * @param  neg          value is negative
 */
//==============================================================================
static void fmt_int(_libc_fmt_out_t *out, const spec_t *sp, char conv, uintmax_t val, bool neg)
{
        char buf[24]; // 22 octal digits of 64-bit value
        char *end = buf + sizeof(buf);
        char *p = end;

        const char *digits = (conv == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
        unsigned base = (conv == 'o') ? 8 : ((conv == 'x') || (conv == 'X') || (conv == 'p')) ? 16 : 10;

        bool nonzero = (val != 0);

        while (val) {
                *--p = digits[val % base];
                val /= base;
        }

        int ndig = end - p;
        int prec = sp->prec;
        bool zero_pad = (sp->flags & FL_ZERO) && !(sp->flags & FL_LEFT) && (prec < 0);

        if (prec < 0) {
                prec = 1;
        }

        char prefix[2];
        int plen = 0;

        if ((conv == 'd') || (conv == 'i')) {
                if (neg) {
                        prefix[plen++] = '-';
                } else if (sp->flags & FL_PLUS) {
                        prefix[plen++] = '+';
                } else if (sp->flags & FL_SPACE) {
                        prefix[plen++] = ' ';
                }

        } else if ((conv == 'o') && (sp->flags & FL_ALT)) {
                if (prec <= ndig) {
                        prec = ndig + 1; // force the first digit to be zero
                }

        } else if (((conv == 'x') || (conv == 'X')) && (sp->flags & FL_ALT) && nonzero) {
                prefix[plen++] = '0';
                prefix[plen++] = conv;

        } else if (conv == 'p') {
                prefix[plen++] = '0';
                prefix[plen++] = 'x';
        }

        int zeros = (prec > ndig) ? prec - ndig : 0;
        int len = plen + zeros + ndig;
        int pad = (sp->width > len) ? sp->width - len : 0;

        if (zero_pad) {
                zeros += pad;
                pad = 0;
        }

        if (!(sp->flags & FL_LEFT)) {
                out_pad(out, ' ', pad);
        }

        out_write(out, prefix, plen);
        out_pad(out, '0', zeros);
        out_write(out, p, ndig);

        if (sp->flags & FL_LEFT) {
                out_pad(out, ' ', pad);
        }
}

//==============================================================================
/**
 * @brief  Sum of two doubles (|a| >= |b|) as double-double.
 *
 * @param  a            value
 * @param  b            value
 *
 * @return a + b without rounding error.
 */
//==============================================================================
static dd_t fast_two_sum(double a, double b)
{
        dd_t r;
        r.hi = a + b;
        r.lo = b - (r.hi - a);
        return r;
}

//==============================================================================
/**
 * @brief  Product of two doubles as double-double (Dekker's algorithm).
 *
 * @param  a            value
 * @param  b            value
 *
 * @return a * b without rounding error.
 */
//==============================================================================
static dd_t two_prod(double a, double b)
{
        const double split = 134217729.0; // 2^27 + 1

        double c = split * a;
        double ah = c - (c - a);
        double al = a - ah;

        c = split * b;
        double bh = c - (c - b);
        double bl = b - bh;

        dd_t r;
        r.hi = a * b;
        r.lo = ((ah * bh - r.hi) + ah * bl + al * bh) + al * bl;
        return r;
}

//==============================================================================
/**
 * @brief  Multiply double-double by double.
 *
 * @param  a            double-double value
 * @param  b            double value
 *
 * @return a * b
 */
//==============================================================================
static dd_t dd_mul(dd_t a, double b)
{
        dd_t p = two_prod(a.hi, b);
        return fast_two_sum(p.hi, p.lo + a.lo * b);
}

//==============================================================================
/**
 * @brief  Divide double-double by double.
 *
 * @param  a            double-double value
 * @param  b            double value
 *
 * @return a / b
 */
//==============================================================================
static dd_t dd_div(dd_t a, double b)
{
        double q1 = a.hi / b;
        dd_t p = two_prod(q1, b);
        double r = ((a.hi - p.hi) - p.lo) + a.lo;
        return fast_two_sum(q1, r / b);
}

//==============================================================================
/**
 * @brief  Multiply value by power of 10 in double-double arithmetic.
 *
 * @param  a            value
 * @param  k            exponent
 *
 * @return a * 10^k
 */
//==============================================================================
static dd_t dd_scale(double a, int k)
{
        // a value close to DBL_MAX would overflow in the division steps, so it
        // is scaled down by 2^64 (exact) and the result is scaled back at the end
        const double two64 = 18446744073709551616.0;
        bool big = (a > 1e300);

        dd_t r = {big ? a / two64 : a, 0.0};

        while (k > 22) {
                r = dd_mul(r, 1e22);
                k -= 22;
        }

        while (k < -22) {
                r = dd_div(r, 1e22);
                k += 22;
        }

        if (k > 0) {
                r = dd_mul(r, POW10[k]);
        } else if (k < 0) {
                r = dd_div(r, POW10[-k]);
        }

        if (big) {
                r.hi *= two64;
                r.lo *= two64;
        }

        return r;
}

//==============================================================================
/**
 * @brief  Convert positive, finite, nonzero double to decimal digits.
 *
 * @param  a            value
 * @param  be           biased binary exponent of value
 * @param  mant         mantissa bits of value
 * @param  d            result
 */
//==============================================================================
__attribute__((noinline)) static void fp_decompose(double a, int be, uint64_t mant, fpdec_t *d)
{
        int e2;

        if (be == 0) {
                int b = 51;
                while (!(mant >> b)) {
                        b--;
                }
                e2 = -1074 + b;
        } else {
                e2 = be - 1023;
        }

        // estimate of floor(log10(a)), can differ by 1
        int t = e2 * 1233;
        int est = (t >= 0) ? (t >> 12) : -((-t + 4095) >> 12);

        // x is in range [1e14, 1e18), so it has 15..18 integer digits
        dd_t x = dd_scale(a, 16 - est);

        uint64_t ip = (uint64_t)x.hi;
        double f = (x.hi - (double)ip) + x.lo;
        int64_t fl = (int64_t)f;
        if ((double)fl > f) {
                fl--;
        }
        ip = (uint64_t)((int64_t)ip + fl);
        f -= (double)fl; // f in range [0, 1)

        char tmp[20];
        int len = 0;
        do {
                tmp[len++] = '0' + (ip % 10);
                ip /= 10;
        } while (ip);

        for (int i = 0; i < len; i++) {
                d->dig[i] = tmp[len - 1 - i];
        }

        int rest = FP_DIGITS - len;
        double s = f * POW10[rest];
        uint64_t frac = (uint64_t)s;
        d->sticky = (s != (double)frac);

        if (frac >= (uint64_t)POW10[rest]) {
                frac = (uint64_t)POW10[rest] - 1;
                d->sticky = true;
        }

        for (int i = FP_DIGITS - 1; i >= len; i--) {
                d->dig[i] = '0' + (frac % 10);
                frac /= 10;
        }

        d->n = FP_DIGITS;
        d->e10 = est + len - 17;
}

//==============================================================================
/**
 * @brief  Round decimal digits to selected number of significant digits
 *         (round half to even).
 *
 * @param  d            digits
 * @param  nsig         number of significant digits (can be 0 or negative)
 */
//==============================================================================
static void fp_round(fpdec_t *d, int nsig)
{
        if (nsig >= d->n) {
                return;
        }

        if (nsig < 0) {
                d->n = 0;
                d->sticky = false;
                return;
        }

        bool up;
        char r = d->dig[nsig];

        if (r > '5') {
                up = true;
        } else if (r < '5') {
                up = false;
        } else {
                bool rest = d->sticky;
                for (int i = nsig + 1; (i < d->n) && !rest; i++) {
                        rest = (d->dig[i] != '0');
                }

                up = rest || ((nsig > 0) && ((d->dig[nsig - 1] - '0') & 1));
        }

        d->n = nsig;
        d->sticky = false;

        if (up) {
                int i = nsig - 1;
                while ((i >= 0) && (d->dig[i] == '9')) {
                        d->dig[i--] = '0';
                }

                if (i >= 0) {
                        d->dig[i]++;
                } else {
                        d->dig[0] = '1';
                        d->n = (nsig > 0) ? nsig : 1;
                        d->e10++;
                }
        }
}

//==============================================================================
/**
 * @brief  Return digit at selected position (0 is the most significant).
 *
 * @param  d            digits
 * @param  pos          position
 *
 * @return Digit character.
 */
//==============================================================================
static char fp_digit(const fpdec_t *d, int pos)
{
        return ((pos >= 0) && (pos < d->n)) ? d->dig[pos] : '0';
}

//==============================================================================
/**
 * @brief  Print digits from selected positions.
 *
 * @param  out          output sink
 * @param  d            digits
 * @param  pos          first position
 * @param  count        number of digits
 */
//==============================================================================
static void fp_out_digits(_libc_fmt_out_t *out, const fpdec_t *d, int pos, int count)
{
        char buf[CHUNK_LEN];

        while ((count > 0) && !out->error) {
                // positions behind generated digits are zeros
                if (pos >= d->n || pos + count <= 0) {
                        out_pad(out, '0', count);
                        return;
                }

                int len = 0;
                while ((len < CHUNK_LEN) && (count > 0)) {
                        buf[len++] = fp_digit(d, pos++);
                        count--;
                }

                out_write(out, buf, len);
        }

        if (count > 0) {
                out->count += count;
        }
}

//==============================================================================
/**
 * @brief  Print hexadecimal floating point field (%a, %A).
 *
 * @param  out          output sink
 * @param  sp           conversion specification
 * @param  upper        upper case
 * @param  sign         sign character, 0 if none
 * @param  be           biased binary exponent
 * @param  mant         mantissa bits
 */
//==============================================================================
__attribute__((noinline)) static void fmt_hexfloat(_libc_fmt_out_t *out, const spec_t *sp, bool upper,
        char sign, int be, uint64_t mant)
{
        const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";

        int lead = (be == 0) ? 0 : 1;
        int exp = (be == 0) ? ((mant == 0) ? 0 : -1022) : be - 1023;
        int ndig = 13;
        int extra = 0;

        if ((sp->prec >= 0) && (sp->prec < 13)) {
                int shift = (13 - sp->prec) * 4;
                uint64_t rem = mant & ((UINT64_C(1) << shift) - 1);
                uint64_t half = UINT64_C(1) << (shift - 1);
                mant >>= shift;

                bool odd = (sp->prec > 0) ? (mant & 1) : (lead & 1);

                if ((rem > half) || ((rem == half) && odd)) {
                        mant++;
                        if ((mant >> (sp->prec * 4)) != 0) {
                                lead++;
                                mant &= (UINT64_C(1) << (sp->prec * 4)) - 1;
                        }
                }

                ndig = sp->prec;

        } else if (sp->prec < 0) {
                while ((ndig > 0) && !(mant & 0xF)) {
                        mant >>= 4;
                        ndig--;
                }
        } else {
                extra = sp->prec - 13;
        }

        char ebuf[8];
        int elen = 0;
        unsigned uexp = (exp < 0) ? -exp : exp;
        do {
                ebuf[sizeof(ebuf) - 1 - elen++] = '0' + (uexp % 10);
                uexp /= 10;
        } while (uexp);
        ebuf[sizeof(ebuf) - 1 - elen++] = (exp < 0) ? '-' : '+';
        ebuf[sizeof(ebuf) - 1 - elen++] = upper ? 'P' : 'p';

        bool dot = (ndig + extra > 0) || (sp->flags & FL_ALT);
        int len = (sign ? 1 : 0) + 3 + (dot ? 1 : 0) + ndig + extra + elen;
        int pad = (sp->width > len) ? sp->width - len : 0;
        bool zero_pad = (sp->flags & FL_ZERO) && !(sp->flags & FL_LEFT);

        if (!(sp->flags & FL_LEFT) && !zero_pad) {
                out_pad(out, ' ', pad);
        }

        if (sign) {
                out_char(out, sign);
        }

        out_write(out, upper ? "0X" : "0x", 2);

        if (zero_pad) {
                out_pad(out, '0', pad);
        }

        out_char(out, digits[lead]);

        if (dot) {
                out_char(out, '.');
        }

        char buf[13];
        for (int i = 0; i < ndig; i++) {
                buf[i] = digits[(mant >> (4 * (ndig - 1 - i))) & 0xF];
        }
        out_write(out, buf, ndig);
        out_pad(out, '0', extra);
        out_write(out, ebuf + sizeof(ebuf) - elen, elen);

        if (sp->flags & FL_LEFT) {
                out_pad(out, ' ', pad);
        }
}

//==============================================================================
/**
 * @brief  Print floating point field (%f, %F, %e, %E, %g, %G, %a, %A).
 *
 * @param  out          output sink
 * @param  sp           conversion specification
 * @param  conv         conversion character
 * @param  val          value
 */
//==============================================================================
__attribute__((noinline)) static void fmt_float(_libc_fmt_out_t *out, const spec_t *sp, char conv, double val)
{
        uint64_t bits;
        memcpy(&bits, &val, sizeof(bits));

        bool upper = (conv >= 'A') && (conv <= 'Z');
        char lconv = conv | 0x20;
        bool alt = sp->flags & FL_ALT;
        int be = (bits >> 52) & 0x7FF;
        uint64_t mant = bits & ((UINT64_C(1) << 52) - 1);

        char sign = 0;
        if (bits >> 63) {
                sign = '-';
                val = -val;
        } else if (sp->flags & FL_PLUS) {
                sign = '+';
        } else if (sp->flags & FL_SPACE) {
                sign = ' ';
        }

        if (be == 0x7FF) {
                char buf[4];
                int len = 0;
                if (sign) {
                        buf[len++] = sign;
                }
                memcpy(&buf[len], mant ? (upper ? "NAN" : "nan") : (upper ? "INF" : "inf"), 3);
                spec_t s = *sp;
                s.prec = -1;
                fmt_str(out, &s, buf, len + 3);
                return;
        }

        if (lconv == 'a') {
                fmt_hexfloat(out, sp, upper, sign, be, mant);
                return;
        }

        fpdec_t d;
        if (val == 0.0) {
                d.n = 0;
                d.e10 = 0;
                d.sticky = false;
        } else {
                fp_decompose(val, be, mant, &d);
        }

        int prec = (sp->prec < 0) ? 6 : ((sp->prec > MAX_FIELD_LEN) ? MAX_FIELD_LEN : sp->prec);
        bool estyle;

        if (lconv == 'f') {
                fp_round(&d, d.e10 + 1 + prec);
                estyle = false;

        } else if (lconv == 'e') {
                fp_round(&d, prec + 1);
                estyle = true;

        } else { // 'g'
                int p = (prec == 0) ? 1 : prec;
                fp_round(&d, p);

                int x = d.n ? d.e10 : 0;

                if ((p > x) && (x >= -4)) {
                        estyle = false;
                        prec = p - 1 - x;
                } else {
                        estyle = true;
                        prec = p - 1;
                }

                if (!alt) {
                        int base = estyle ? 0 : d.e10;
                        while ((prec > 0) && (fp_digit(&d, base + prec) == '0')) {
                                prec--;
                        }
                }
        }

        bool dot = (prec > 0) || alt;
        int len = (sign ? 1 : 0) + (dot ? 1 : 0) + prec;

        char ebuf[6];
        int elen = 0;

        if (estyle) {
                int exp = d.n ? d.e10 : 0;
                unsigned uexp = (exp < 0) ? -exp : exp;
                do {
                        ebuf[sizeof(ebuf) - 1 - elen++] = '0' + (uexp % 10);
                        uexp /= 10;
                } while (uexp || (elen < 2));
                ebuf[sizeof(ebuf) - 1 - elen++] = (exp < 0) ? '-' : '+';
                ebuf[sizeof(ebuf) - 1 - elen++] = upper ? 'E' : 'e';
                len += 1 + elen;
        } else {
                len += (d.e10 >= 0) ? d.e10 + 1 : 1;
        }

        int pad = (sp->width > len) ? sp->width - len : 0;
        bool zero_pad = (sp->flags & FL_ZERO) && !(sp->flags & FL_LEFT);

        if (!(sp->flags & FL_LEFT) && !zero_pad) {
                out_pad(out, ' ', pad);
        }

        if (sign) {
                out_char(out, sign);
        }

        if (zero_pad) {
                out_pad(out, '0', pad);
        }

        if (estyle) {
                fp_out_digits(out, &d, 0, 1);
                if (dot) {
                        out_char(out, '.');
                }
                fp_out_digits(out, &d, 1, prec);
                out_write(out, ebuf + sizeof(ebuf) - elen, elen);

        } else {
                if (d.e10 >= 0) {
                        fp_out_digits(out, &d, 0, d.e10 + 1);
                } else {
                        out_char(out, '0');
                }

                if (dot) {
                        out_char(out, '.');
                }

                fp_out_digits(out, &d, d.e10 + 1, prec);
        }

        if (sp->flags & FL_LEFT) {
                out_pad(out, ' ', pad);
        }
}

//==============================================================================
/**
 * @brief  Print wide string or character (%ls, %lc) in C locale.
 *
 * @param  out          output sink
 * @param  sp           conversion specification
 * @param  ws           wide string
 * @param  max          maximum number of wide characters to scan
 *
 * @return On success true is returned. On illegal character false is returned.
 */
//==============================================================================
__attribute__((noinline)) static bool fmt_wstr(_libc_fmt_out_t *out, const spec_t *sp, const wchar_t *ws,
        size_t max)
{
        size_t len = 0;
        while ((len < max) && ws[len]) {
                if ((uint32_t)ws[len] > 0x7F) { // also negative values if wchar_t is signed
                        errno = EILSEQ;
                        return false;
                }
                len++;
        }

        int pad = (len < (size_t)sp->width) ? sp->width - (int)len : 0;

        if (!(sp->flags & FL_LEFT)) {
                out_pad(out, ' ', pad);
        }

        char buf[CHUNK_LEN];
        size_t pos = 0;
        while (pos < len) {
                size_t n = 0;
                while ((n < CHUNK_LEN) && (pos < len)) {
                        buf[n++] = (char)ws[pos++];
                }
                out_write(out, buf, n);
        }

        if (sp->flags & FL_LEFT) {
                out_pad(out, ' ', pad);
        }

        return true;
}

//==============================================================================
/**
 * @brief  Parse decimal number of width or precision.
 *
 * @param  fmt          format string pointer (updated)
 *
 * @return Parsed value (saturated).
 */
//==============================================================================
static int parse_num(const char **fmt)
{
        int val = 0;

        while ((**fmt >= '0') && (**fmt <= '9')) {
                int digit = *(*fmt)++ - '0';
                val = (val > (MAX_FIELD_LEN - digit) / 10) ? MAX_FIELD_LEN : val * 10 + digit;
        }

        return val;
}

//==============================================================================
/**
 * @brief  Format text according to POSIX printf() rules.
 *
 * @param  out          output sink
 * @param  format       format string
 * @param  arg          arguments
 *
 * @return Number of produced characters (also these not accepted by the
 *         sink because of size limit). On error -1 is returned and errno
 *         is set.
 */
//==============================================================================
int _libc_vformat(_libc_fmt_out_t *out, const char *format, va_list arg)
{
        va_list ap;
        va_copy(ap, arg);

        out->count = 0;
        out->error = false;

        const char *fmt = format;

        while (*fmt && !out->error) {

                if (*fmt != '%') {
                        const char *begin = fmt;
                        while (*fmt && (*fmt != '%')) {
                                fmt++;
                        }
                        out_write(out, begin, fmt - begin);
                        continue;
                }

                const char *spec_begin = fmt++;

                spec_t sp = {.flags = 0, .width = 0, .prec = -1};

                // flags
                for (bool flag = true; flag;) {
                        switch (*fmt) {
                        case '-':
                                sp.flags |= FL_LEFT;
                                break;
                        case '+':
                                sp.flags |= FL_PLUS;
                                break;
                        case ' ':
                                sp.flags |= FL_SPACE;
                                break;
                        case '#':
                                sp.flags |= FL_ALT;
                                break;
                        case '0':
                                sp.flags |= FL_ZERO;
                                break;
                        case '\'':
                                break; // thousands grouping: no effect in C locale
                        default:
                                flag = false;
                                continue;
                        }
                        fmt++;
                }

                // width
                if (*fmt == '*') {
                        int w = va_arg(ap, int);
                        if (w < 0) {
                                sp.flags |= FL_LEFT;
                                w = (w < -MAX_FIELD_LEN) ? MAX_FIELD_LEN : -w;
                        }
                        sp.width = (w > MAX_FIELD_LEN) ? MAX_FIELD_LEN : w;
                        fmt++;
                } else {
                        sp.width = parse_num(&fmt);
                }

                // precision
                if (*fmt == '.') {
                        fmt++;
                        if (*fmt == '*') {
                                int p = va_arg(ap, int);
                                sp.prec = (p < 0) ? -1 : ((p > MAX_FIELD_LEN) ? MAX_FIELD_LEN : p);
                                fmt++;
                        } else {
                                sp.prec = parse_num(&fmt);
                        }
                }

                // length modifier
                len_mod_t len = LEN_NONE;
                switch (*fmt) {
                case 'h':
                        fmt++;
                        len = (*fmt == 'h') ? (fmt++, LEN_HH) : LEN_H;
                        break;
                case 'l':
                        fmt++;
                        len = (*fmt == 'l') ? (fmt++, LEN_LL) : LEN_L;
                        break;
                case 'j':
                        fmt++;
                        len = LEN_J;
                        break;
                case 'z':
                        fmt++;
                        len = LEN_Z;
                        break;
                case 't':
                        fmt++;
                        len = LEN_T;
                        break;
                case 'L':
                        fmt++;
                        len = LEN_LD;
                        break;
                default:
                        break;
                }

                char conv = *fmt;
                if (conv == '\0') {
                        out_write(out, spec_begin, fmt - spec_begin);
                        break;
                }
                fmt++;

                switch (conv) {
                case '%':
                        out_char(out, '%');
                        break;

                case 'd':
                case 'i': {
                        intmax_t v;
                        switch (len) {
                        case LEN_HH:
                                v = (signed char)va_arg(ap, int);
                                break;
                        case LEN_H:
                                v = (short)va_arg(ap, int);
                                break;
                        case LEN_L:
                                v = va_arg(ap, long);
                                break;
                        case LEN_LL:
                                v = va_arg(ap, long long);
                                break;
                        case LEN_J:
                                v = va_arg(ap, intmax_t);
                                break;
                        case LEN_Z:
                                v = va_arg(ap, ptrdiff_t);
                                break; // signed type of size_t size
                        case LEN_T:
                                v = va_arg(ap, ptrdiff_t);
                                break;
                        default:
                                v = va_arg(ap, int);
                                break;
                        }

                        uintmax_t u = (v < 0) ? (uintmax_t)0 - (uintmax_t)v : (uintmax_t)v;
                        fmt_int(out, &sp, conv, u, v < 0);
                        break;
                }

                case 'o':
                case 'u':
                case 'x':
                case 'X': {
                        uintmax_t v;
                        switch (len) {
                        case LEN_HH:
                                v = (unsigned char)va_arg(ap, unsigned int);
                                break;
                        case LEN_H:
                                v = (unsigned short)va_arg(ap, unsigned int);
                                break;
                        case LEN_L:
                                v = va_arg(ap, unsigned long);
                                break;
                        case LEN_LL:
                                v = va_arg(ap, unsigned long long);
                                break;
                        case LEN_J:
                                v = va_arg(ap, uintmax_t);
                                break;
                        case LEN_Z:
                                v = va_arg(ap, size_t);
                                break;
                        case LEN_T:
                                v = (size_t)va_arg(ap, ptrdiff_t);
                                break;
                        default:
                                v = va_arg(ap, unsigned int);
                                break;
                        }

                        fmt_int(out, &sp, conv, v, false);
                        break;
                }

                case 'p': {
                        void *ptr = va_arg(ap, void *);
                        if (ptr) {
                                sp.flags &= ~(FL_PLUS | FL_SPACE);
                                fmt_int(out, &sp, 'p', (uintptr_t)ptr, false);
                        } else {
                                sp.prec = -1;
                                fmt_str(out, &sp, "(nil)", 5);
                        }
                        break;
                }

                case 'c':
                        if (len == LEN_L) {
                                wchar_t wc[2] = {(wchar_t)va_arg(ap, int), 0};
                                if (wc[0] == 0) {
                                        char c = '\0';
                                        fmt_str(out, &sp, &c, 1);
                                } else if (!fmt_wstr(out, &sp, wc, 1)) {
                                        out->error = true;
                                }
                        } else {
                                char c = (char)va_arg(ap, int);
                                fmt_str(out, &sp, &c, 1);
                        }
                        break;

                case 's':
                        if (len == LEN_L) {
                                const wchar_t *ws = va_arg(ap, const wchar_t *);
                                if (!ws) {
                                        ws = L"(null)";
                                }
                                if (!fmt_wstr(out, &sp, ws, (sp.prec < 0) ? SIZE_MAX : (size_t)sp.prec)) {
                                        out->error = true;
                                }
                        } else {
                                const char *s = va_arg(ap, const char *);
                                if (!s) {
                                        s = "(null)";
                                }

                                // the array does not need to be null-terminated if precision is given
                                size_t n = 0;
                                if (sp.prec < 0) {
                                        n = strlen(s);
                                } else {
                                        while ((n < (size_t)sp.prec) && s[n]) {
                                                n++;
                                        }
                                }

                                fmt_str(out, &sp, s, n);
                        }
                        break;

                case 'f':
                case 'F':
                case 'e':
                case 'E':
                case 'g':
                case 'G':
                case 'a':
                case 'A': {
                        double v = (len == LEN_LD) ? (double)va_arg(ap, long double) : va_arg(ap, double);
                        fmt_float(out, &sp, conv, v);
                        break;
                }

                case 'n':
                        switch (len) {
                        case LEN_HH:
                                *va_arg(ap, signed char *) = (signed char)out->count;
                                break;
                        case LEN_H:
                                *va_arg(ap, short *) = (short)out->count;
                                break;
                        case LEN_L:
                                *va_arg(ap, long *) = (long)out->count;
                                break;
                        case LEN_LL:
                                *va_arg(ap, long long *) = (long long)out->count;
                                break;
                        case LEN_J:
                                *va_arg(ap, intmax_t *) = (intmax_t)out->count;
                                break;
                        case LEN_Z:
                                *va_arg(ap, size_t *) = out->count;
                                break;
                        case LEN_T:
                                *va_arg(ap, ptrdiff_t *) = (ptrdiff_t)out->count;
                                break;
                        default:
                                *va_arg(ap, int *) = (int)out->count;
                                break;
                        }
                        break;

                default:
                        // unknown conversion specification is printed as is
                        out_write(out, spec_begin, fmt - spec_begin);
                        break;
                }
        }

        va_end(ap);

        if (out->error) {
                return -1;
        }

        if (out->count > INT_MAX) {
                errno = ERANGE; // EOVERFLOW is not defined in dnx RTOS
                return -1;
        }

        return (int)out->count;
}

/*==============================================================================
  End of file
==============================================================================*/
