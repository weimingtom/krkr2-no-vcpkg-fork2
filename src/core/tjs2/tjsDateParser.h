//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Date/time string parser
//---------------------------------------------------------------------------
#ifndef tjsDateParserH
#define tjsDateParserH

#include "tjsdate.tab.hpp"

namespace TJSDate {

    //---------------------------------------------------------------------------
    // tTJSDateParser : A date/time parser class
    //---------------------------------------------------------------------------
    class tTJSDateParser {
        bool YearSet;
        int Year;

        bool MonthSet;
        int Month;

        bool MDaySet;
        int MDay;

        bool HourSet;
        int Hour;

        bool MinSet;
        int Min;

        bool SecSet;
        int Sec;

        bool AMPMSet;
        bool AMPM; // pm:true am:false

        bool TimeZoneSet;
        int TimeZone;

        bool TimeZoneOffsetSet;
        int TimeZoneOffset;

        const tjs_char *Input;
        const tjs_char *InputPointer;

        tjs_int64 Time; // time from 1970-01-01 00:00:00.00 GMT

    public:
        tTJSDateParser(const tjs_char *in);

        ~tTJSDateParser() = default;

        tjs_int64 GetTime();

        void SkipToRightParenthesis();

        void SetMDay(int v);

        void SetMonth(int v);

        void SetYear(int v);

        void SetHours(int v);

        void SetMin(int v);

        void SetSec(int v);

        void SetAMPM(bool is_pm);

        void SetTimeZone(int v);

        void SetTimeZoneOffset(int v);

        int lex(parser::value_type *yylex);
    };
    int yylex(parser::value_type *yylex, tTJSDateParser *ptr);
    //---------------------------------------------------------------------------
} // namespace TJSDate
#endif
