//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Conditional Compile Control
//---------------------------------------------------------------------------

#ifndef tjsCompileControlH
#define tjsCompileControlH

#include "tjsString.h"
#include "tjspp.tab.hpp"

using namespace TJS;

namespace TJSPP {
    //---------------------------------------------------------------------------

    class tTJSPPExprParser {
    public:
        tTJSPPExprParser(tTJS *tjs, const tjs_char *script);

        ~tTJSPPExprParser();

        tjs_int32 Parse();

        tTJS *TJS;

        tjs_int GetNext(tjs_int &value);

        tTJS *GetTJS() { return TJS; }

        [[nodiscard]] const tjs_char *GetString(tjs_int idx) const;

        tjs_int32 Result{};

    private:
        std::vector<ttstr> IDs;

        const tjs_char *Script;
        const tjs_char *Current{};
    };
    int yylex(parser::value_type *yylex, tTJSPPExprParser *ptr);
} // namespace TJSPP

#endif
