#include "math.h"

double sqrt(double x) {
    double res;
    __asm__("fsqrt" : "=t"(res) : "0"(x));
    return res;
}

double log2(double x) {
    double ret;
    __asm__("fld1\n"
            "fxch %%st(1)\n"
            "fyl2x"
            : "=t"(ret)
            : "0"(x));
    return ret;
}

double log10(double x) {
    double ret;
    __asm__("fldlg2\n"
            "fxch %%st(1)\n"
            "fyl2x\n"
            : "=t"(ret)
            : "0"(x));
    return ret;
}

double fabs(double x) {
    __asm__("fabs" : "+t"(x));
    return x;
}

double exp2(double exponent) {
    double res;
    __asm__("fld1\n"
            "fld %%st(1)\n"
            "fprem\n"
            "f2xm1\n"
            "faddp\n"
            "fscale\n"
            "fstp %%st(1)"
            : "=t"(res)
            : "0"(exponent));
    return res;
}

double pow(double x, double y) { return exp2(y * log2(x)); }

double sin(double angle) {
    double ret;
    __asm__("fsin" : "=t"(ret) : "0"(angle));
    return ret;
}

double cos(double angle) {
    double ret;
    __asm__("fcos" : "=t"(ret) : "0"(angle));
    return ret;
}

double atan2(double y, double x) {
    double ret;
    __asm__("fpatan" : "=t"(ret) : "0"(x), "u"(y) : "st(1)");
    return ret;
}
