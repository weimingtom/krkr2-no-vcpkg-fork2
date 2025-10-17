//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS Math class implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsMath.h"
#include <ctime>

#ifdef _WIN32
#include <corecrt_math_defines.h>
#ifndef TJS_NO_MASK_MATHERR
#include <cfloat>
#endif
#ifdef _MSC_VER
#define _USERENTRY
#endif

#else
#define _USE_MATH_DEFINES
#include <cmath>
#endif

#if defined(__MINGW32__)
#define M_E         2.71828182845904523536
#define M_LOG2E     1.44269504088896340736
#define M_LOG10E    0.434294481903251827651
#define M_LN2       0.693147180559945309417
#define M_LN10      2.30258509299404568402
#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.785398163397448309616
#define M_1_PI      0.318309886183790671538
#define M_2_PI      0.636619772367581343076
#define M_1_SQRTPI  0.564189583547756286948
#define M_2_SQRTPI  1.12837916709551257390
#define M_SQRT2     1.41421356237309504880
#define M_SQRT_2    0.707106781186547524401

#define EDOM    33      /* Math argument */
#define ERANGE  34      /* Result too large */
//https://blog.csdn.net/benpaobagzb/article/details/50396301
#endif

//---------------------------------------------------------------------------
// matherr and matherrl function
//---------------------------------------------------------------------------
// these functions invalidate the mathmarical error
// (other exceptions, like divide-by-zero error, are not to be caught
// here)
#if defined(__WIN32__) && !defined(__GNUC__)
#ifndef TJS_NO_MASK_MATHERR
int _USERENTRY _matherr(struct _exception *e) { return 1; }
int _USERENTRY _matherrl(struct _exception *e) { return 1; }
#endif
#endif
//---------------------------------------------------------------------------

namespace TJS {
    //---------------------------------------------------------------------------
    // tTJSXorshift
    //---------------------------------------------------------------------------
    // generates random number using Xorshift
    class tTJSXorshift {
    public:
        static void init(tjs_uint32 seed) {
            for(tjs_uint32 i = 0; i < 4; ++i) {
                seeds[i] = seed = 1812433253 * (seed ^ (seed >> 30)) + i;
            }
        }

        // generates a random number in the range [0,1)
        static tTVReal random_real() {
            TJSSetFPUE();
            return (tTVReal)((tTVReal)random() / ((tTVReal)MAX + 1));
        }

    private:
        static tjs_uint32 seeds[4];
        const static tjs_uint32 MAX = (2LL << 31) - 1;

        static tjs_uint32 random() {
            tjs_uint32 t = seeds[0] ^ (seeds[0] << 11);
            seeds[0] = seeds[1];
            seeds[1] = seeds[2];
            seeds[2] = seeds[3];
            seeds[3] = (seeds[3] ^ (seeds[3] >> 19)) ^ (t ^ (t >> 8));
            return seeds[3];
        }
    };

    tjs_uint32 tTJSXorshift::seeds[4] = { 123456789, 362436069, 521288629,
                                          88675123 };

    //---------------------------------------------------------------------------
    // tTJSNC_Math : TJS Native Class : Math
    //---------------------------------------------------------------------------
    tjs_uint32 tTJSNC_Math::ClassID = (tjs_uint32)-1;

    tTJSNC_Math::tTJSNC_Math() : tTJSNativeClass(TJS_W("Math")) {
        // constructor
        time_t time_num;
        time(&time_num);
        tTJSXorshift::init((tjs_uint32)time_num);

        /*
            TJS2 cannot promise that the sequence of generated random
           numbers are unique. since Math.RandomGenerator provides
           Mersenne Twister high-quality random generator.
        */

        TJSSetFPUE();

        TJS_BEGIN_NATIVE_MEMBERS(/*TJS class name*/ Math)
        TJS_DECL_EMPTY_FINALIZE_METHOD
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL_NO_INSTANCE(
            /*TJS class name*/ Math) {
            return TJS_S_OK;
        }
        TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/ Math)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ abs) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = fabs(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ abs)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ acos) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = acos(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ acos)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ asin) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = asin(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ asin)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ atan) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = atan(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ atan)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ atan2) {
            if(numparams < 2)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = atan2(param[0]->AsReal(), param[1]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ atan2)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ ceil) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = ceil(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ ceil)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ exp) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = exp(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ exp)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ floor) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = floor(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ floor)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ log) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = log(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ log)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ pow) {
            if(numparams < 2)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = pow(param[0]->AsReal(), param[1]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ pow)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ max) {
            if(result) {
                TJSSetFPUE();

                tjs_real r;
                *(tjs_uint64 *)&r = TJS_IEEE_D_N_INF;
                for(tjs_int i = 0; i < numparams; ++i) {
                    tTVReal v = param[i]->AsReal();
                    auto *ui64 = (tjs_uint64 *)&v;
                    if(TJS_IEEE_D_IS_NaN(*ui64)) {
                        tjs_real d;
                        *(tjs_uint64 *)&d = TJS_IEEE_D_P_NaN;
                        *result = d;
                        return TJS_S_OK;
                    } else if(*ui64 == 0) {
                        // v is positive-zero
                        // check r is negative
                        if(TJS_IEEE_D_GET_SIGN(
                               *(tjs_uint64 *)(&r))) // true if negative
                        {
                            // r is negative and v is positive-zero
                            r = v;
                        }
                    } else if(r < v) {
                        r = v;
                    }
                }

                *result = r;
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ max)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ min) {
            if(result) {
                TJSSetFPUE();

                tjs_real r;
                *(tjs_uint64 *)&r = TJS_IEEE_D_P_INF;
                for(tjs_int i = 0; i < numparams; ++i) {
                    tTVReal v = param[i]->AsReal();
                    auto *ui64 = (tjs_uint64 *)&v;
                    if(TJS_IEEE_D_IS_NaN(*ui64)) {
                        tjs_real d;
                        *(tjs_uint64 *)&d = TJS_IEEE_D_P_NaN;
                        *result = d;
                        return TJS_S_OK;
                    } else if(*ui64 == (0 | TJS_IEEE_D_SIGN_MASK)) {
                        // v is nagative-zero
                        // note that 0|TJS_IEEE_D_SIGN_MASK is a
                        // presentation value of nagative-zero. check
                        // r is positive
                        if(!TJS_IEEE_D_GET_SIGN(
                               *(tjs_uint64 *)(&r))) // false if positive
                        {
                            // v is negative-zero and r is positive
                            r = v;
                        }
                    } else if(v < r) {
                        r = v;
                    }
                }

                *result = r;
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ min)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ random) {
            if(result) {
                *result = tTJSXorshift::random_real();
            }
            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ random)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ round) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = ::floor(param[0]->AsReal() + 0.5f);
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ round)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ sin) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = ::sin(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ sin)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ cos) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = ::cos(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ cos)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ sqrt) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = ::sqrt(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ sqrt)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ tan) {
            if(numparams < 1)
                return TJS_E_BADPARAMCOUNT;

            if(result) {
                TJSSetFPUE();
                *result = ::tan(param[0]->AsReal());
            }

            return TJS_S_OK;
        }
        TJS_END_NATIVE_STATIC_METHOD_DECL(/*func. name*/ tan)
        //----------------------------------------------------------------------
        TJS_BEGIN_NATIVE_PROP_DECL(E){
            TJS_BEGIN_NATIVE_PROP_GETTER{ *result = M_E;
        return TJS_S_OK;
    }
    TJS_END_NATIVE_PROP_GETTER

    TJS_DENY_NATIVE_PROP_SETTER
} // namespace TJS
TJS_END_NATIVE_STATIC_PROP_DECL(E)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(LOG2E){
    TJS_BEGIN_NATIVE_PROP_GETTER{ *result = M_LOG2E;
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(LOG2E)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(LOG10E){
    TJS_BEGIN_NATIVE_PROP_GETTER{ *result = M_LOG10E;
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(LOG10E)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(LN10){
    TJS_BEGIN_NATIVE_PROP_GETTER{ *result = M_LN10;
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(LN10)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(LN2){ TJS_BEGIN_NATIVE_PROP_GETTER{ *result = M_LN2;
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(LN2)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(PI){ TJS_BEGIN_NATIVE_PROP_GETTER{ *result = M_PI;
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(PI)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(SQRT1_2){
    TJS_BEGIN_NATIVE_PROP_GETTER{ *result = (M_SQRT2 / 2);
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(SQRT1_2)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(SQRT2){
    TJS_BEGIN_NATIVE_PROP_GETTER{ *result = M_SQRT2;
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(SQRT2)
//---------------------------------------------------------------------------

TJS_END_NATIVE_MEMBERS
} // tTJSNC_Math::tTJSNC_Math()
//---------------------------------------------------------------------------
} // namespace TJS
