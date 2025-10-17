#pragma once
#include "tjs.h"
#include "tjsHashSearch.h"
#include <functional>
#if 0
#include <freetype/freetype.h>
#else
#include "ft2build.h"
#include FT_FREETYPE_H
#endif

const FT_Library TVPGetFontLibrary();

void TVPInitFontNames();
int TVPEnumFontsProc(const ttstr &FontPath);
const ttstr &TVPGetDefaultFontName();
tTJSBinaryStream *TVPCreateFontStream(const ttstr &fontname);
struct TVPFontNamePathInfo {
    ttstr Path;
    std::function<tTJSBinaryStream *(TVPFontNamePathInfo *)> Getter;
    int Index{};
};
TVPFontNamePathInfo *TVPFindFont(const ttstr &name);

//---------------------------------------------------------------------------
// font enumeration and existence check
//---------------------------------------------------------------------------
class tTVPttstrHash {
public:
    static tjs_uint32 Make(const ttstr &val);
};
extern tTJSHashTable<ttstr, TVPFontNamePathInfo, tTVPttstrHash> TVPFontNames;
