
#ifndef __FONT_RASTERIZER_H__
#define __FONT_RASTERIZER_H__

#include "tjsTypes.h"
#include "tjsString.h"

class FontRasterizer {

public:
    virtual ~FontRasterizer() = default;
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual void ApplyFont(class tTVPNativeBaseBitmap *bmp, bool force) = 0;
    virtual void ApplyFont(const struct tTVPFont &font) = 0;
    virtual void GetTextExtent(tjs_char ch, tjs_int &w, tjs_int &h) = 0;
    virtual tjs_int GetAscentHeight() = 0;
    virtual class tTVPCharacterData *
    GetBitmap(const struct tTVPFontAndCharacterData &font, tjs_int aofsx,
              tjs_int aofsy) = 0;
    virtual void GetGlyphDrawRect(const TJS::ttstr &text,
                                  struct tTVPRect &area) = 0;
};

#endif // __FREE_TYPE_FONT_RASTERIZER_H__
