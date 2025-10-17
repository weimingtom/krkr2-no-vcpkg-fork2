//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// configuration
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifndef tjsConfigH
#define tjsConfigH

#include <string>
#include <cwchar>

#include "tjsTypes.h"

namespace TJS {
    //---------------------------------------------------------------------------

    /*
            many settings can be changed here.

            tjsCommHead.h includes most common headers that will be
       needed to compile the entire TJS program.

            configuration about Critical Section for multithreading
       support is there in tjsUtils.cpp/h.
    */

    TJS_EXP_FUNC_DEF(tjs_int, TJS_atoi, (const tjs_char *s));

    TJS_EXP_FUNC_DEF(tjs_char *, TJS_int_to_str,
                     (tjs_int value, tjs_char *string));

    TJS_EXP_FUNC_DEF(tjs_char *, TJS_tTVInt_to_str,
                     (tjs_int64 value, tjs_char *string));

    TJS_EXP_FUNC_DEF(tjs_int, TJS_strnicmp,
                     (const tjs_char *s1, const tjs_char *s2, size_t maxlen));

    TJS_EXP_FUNC_DEF(tjs_int, TJS_stricmp,
                     (const tjs_char *s1, const tjs_char *s2));

    TJS_EXP_FUNC_DEF(void, TJS_strcpy_maxlen,
                     (tjs_char * d, const tjs_char *s, size_t len));

    TJS_EXP_FUNC_DEF(void, TJS_strcpy, (tjs_char * d, const tjs_char *s));

    TJS_EXP_FUNC_DEF(size_t, TJS_strlen, (const tjs_char *d));

    int TJS_strcmp(const tjs_char *src, const tjs_char *dst);

    int TJS_strncmp(const tjs_char *first, const tjs_char *last, size_t count);

    tjs_char *TJS_strncpy(tjs_char *dest, const tjs_char *source, size_t count);

    tjs_char *TJS_strcat(tjs_char *dst, const tjs_char *src);

    tjs_char *TJS_strstr(const tjs_char *wcs1, const tjs_char *wcs2);

    tjs_char *TJS_strchr(const tjs_char *string, tjs_char ch);

    void *TJS_malloc(size_t n);

    void *TJS_realloc(void *buf, size_t n);

    void TJS_free(void *buf);

#define TJS_nsprintf sprintf
#define TJS_nstrcpy strcpy
#define TJS_nstrcat strcat
#define TJS_nstrlen strlen
#define TJS_nstrstr strstr

    size_t TJS_strftime(tjs_char *wstring, size_t maxsize,
                        const tjs_char *wformat, const tm *timeptr);

#define TJS_octetcpy memcpy
#define TJS_octetcmp memcmp

    double TJS_strtod(const tjs_char *pString, tjs_char **endPtr);

    TJS_EXP_FUNC_DEF(tjs_int64, TJS_atoll, (const tjs_char *s));

    extern size_t TJS_mbstowcs(tjs_char *pwcs, const tjs_nchar *s, size_t n);

    extern size_t TJS_wcstombs(tjs_nchar *s, const tjs_char *pwcs, size_t n);

#define TJS_strncpy_s(d, dl, s, sl) TJS_strncpy(d, s, sl)
#if defined(_MSC_VER)
#define TJS_cdecl __cdecl
#define TJS_timezone _timezone
#else
#define TJS_cdecl
#define TJS_timezone timezone
#endif

#define TJS_narrowtowidelen(X)                                                 \
    TJS_mbstowcs(nullptr, (X), 0) // narrow->wide (if) converted length
#define TJS_narrowtowide TJS_mbstowcs

#ifdef TJS_DEBUG_TRACE
#define TJS_D(x) TJS_debug_out x;
#define TJS_F_TRACE(x) tTJSFuncTrace ___trace(TJS_W(x));
#else
#define TJS_D(x)
#define TJS_F_TRACE(x)
#endif
    class tTJSString;
    void TVPConsoleLog(const tTJSString &str);

    extern void TJSNativeDebuggerBreak();

    extern void TJSSetFPUE();

//---------------------------------------------------------------------------
// elapsed time profiler
//---------------------------------------------------------------------------
#ifdef TJS_DEBUG_PROFILE_TIME
    extern tjs_uint TJSGetTickCount();
    class tTJSTimeProfiler {
        tjs_uint &timevar;
        tjs_uint start;

    public:
        tTJSTimeProfiler(tjs_uint &tv) : timevar(tv) {
            start = TJSGetTickCount();
        }

        ~tTJSTimeProfiler() { timevar += TJSGetTickCount() - start; }
    };
#endif
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    // function tracer
    //---------------------------------------------------------------------------
    class tTJSFuncTrace {
        const std::string funcname;

    public:
        tTJSFuncTrace(const tjs_char *p);

        ~tTJSFuncTrace();
    };
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    // tTJSNarrowStringHolder : converts wide -> narrow, and holds it
    // until be destroyed
    //---------------------------------------------------------------------------
    struct tTJSNarrowStringHolder {
        bool Allocated;
        tjs_nchar *Buf;
        tTJSNarrowStringHolder(const tjs_char *wide);

        ~tTJSNarrowStringHolder();

        operator const tjs_nchar *() const { return Buf; }
    };
    //---------------------------------------------------------------------------

} // namespace TJS

#endif
