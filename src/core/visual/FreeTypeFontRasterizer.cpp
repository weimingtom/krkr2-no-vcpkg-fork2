
#include "FreeTypeFontRasterizer.h"
#include "LayerBitmapIntf.h"
#include "FreeType.h"
#if _WIN32 && !defined(__MINGW32__)
#include <corecrt_math_defines.h>
#else
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#endif
#include "MsgIntf.h"
#include "FontSystem.h"
#include <complex>

#if defined(__MINGW32__)
#define M_PI 3.14159265358979323846
#endif 

extern void TVPUninitializeFreeFont();
extern FontSystem *TVPFontSystem;
extern const ttstr &TVPGetDefaultFontName();
void FreeTypeFontRasterizer::ApplyFallbackFace() {
    if(!FaceFallback && Face &&
       Face->GetFontName() != TVPGetDefaultFontName()) {
        FaceFallback = new tFreeTypeFace(TVPGetDefaultFontName(), 0);
    }
    if(!FaceFallback)
        return;
    FaceFallback->SetHeight(CurrentFont.Height < 0 ? -CurrentFont.Height
                                                   : CurrentFont.Height);
    if(CurrentFont.Flags & TVP_TF_ITALIC) {
        FaceFallback->SetOption(TVP_TF_ITALIC);
    } else {
        FaceFallback->ClearOption(TVP_TF_ITALIC);
    }
    if(CurrentFont.Flags & TVP_TF_BOLD) {
        FaceFallback->SetOption(TVP_TF_BOLD);
    } else {
        FaceFallback->ClearOption(TVP_TF_BOLD);
    }
    if(CurrentFont.Flags & TVP_TF_UNDERLINE) {
        FaceFallback->SetOption(TVP_TF_UNDERLINE);
    } else {
        FaceFallback->ClearOption(TVP_TF_UNDERLINE);
    }
    if(CurrentFont.Flags & TVP_TF_STRIKEOUT) {
        FaceFallback->SetOption(TVP_TF_STRIKEOUT);
    } else {
        FaceFallback->ClearOption(TVP_TF_STRIKEOUT);
    }
}

FreeTypeFontRasterizer::FreeTypeFontRasterizer() :
    RefCount(0), Face(nullptr), LastBitmap(nullptr) {
    AddRef();
}
FreeTypeFontRasterizer::~FreeTypeFontRasterizer() {

    delete Face;
    Face = nullptr;
    if(FaceFallback) {
        delete FaceFallback;
        FaceFallback = nullptr;
    }
    TVPUninitializeFreeFont();
}
void FreeTypeFontRasterizer::AddRef() { RefCount++; }
//---------------------------------------------------------------------------
void FreeTypeFontRasterizer::Release() {
    RefCount--;
    LastBitmap = nullptr;
    if(RefCount == 0) {

        delete Face;
        Face = nullptr;
        if(FaceFallback) {
            delete FaceFallback;
            FaceFallback = nullptr;
        }
        delete this;
    }
}
//---------------------------------------------------------------------------
void FreeTypeFontRasterizer::ApplyFont(class tTVPNativeBaseBitmap *bmp,
                                       bool force) {
    if(bmp != LastBitmap || force) {
        ApplyFont(bmp->GetFont());
        LastBitmap = bmp;
    }
}
//---------------------------------------------------------------------------
void FreeTypeFontRasterizer::ApplyFont(const tTVPFont &font) {
    CurrentFont = font;
    ttstr stdname = TVPFontSystem->GetBeingFont(font.Face);
    // TVP_FACE_OPTIONS_NO_ANTIALIASING
    // TVP_FACE_OPTIONS_NO_HINTING
    // TVP_FACE_OPTIONS_FORCE_AUTO_HINTING
    tjs_uint32 opt = 0;
    opt |= (font.Flags & TVP_TF_ITALIC) ? TVP_TF_ITALIC : 0;
    opt |= (font.Flags & TVP_TF_BOLD) ? TVP_TF_BOLD : 0;
    opt |= (font.Flags & TVP_TF_UNDERLINE) ? TVP_TF_UNDERLINE : 0;
    opt |= (font.Flags & TVP_TF_STRIKEOUT) ? TVP_TF_STRIKEOUT : 0;
    opt |= (font.Flags & TVP_TF_FONTFILE) ? TVP_FACE_OPTIONS_FILE : 0;
    bool recreate = false;
    if(Face) {
        if(Face->GetFontName() != stdname) {
            delete Face;
            Face = new tFreeTypeFace(stdname, opt);
            recreate = true;
        }
    } else {
        Face = new tFreeTypeFace(stdname, opt);
        recreate = true;
    }
    Face->SetHeight(font.Height < 0 ? -font.Height : font.Height);
    if(recreate == false) {
        if(font.Flags & TVP_TF_ITALIC) {
            Face->SetOption(TVP_TF_ITALIC);
        } else {
            Face->ClearOption(TVP_TF_ITALIC);
        }
        if(font.Flags & TVP_TF_BOLD) {
            Face->SetOption(TVP_TF_BOLD);
        } else {
            Face->ClearOption(TVP_TF_BOLD);
        }
        if(font.Flags & TVP_TF_UNDERLINE) {
            Face->SetOption(TVP_TF_UNDERLINE);
        } else {
            Face->ClearOption(TVP_TF_UNDERLINE);
        }
        if(font.Flags & TVP_TF_STRIKEOUT) {
            Face->SetOption(TVP_TF_STRIKEOUT);
        } else {
            Face->ClearOption(TVP_TF_STRIKEOUT);
        }
    }
    LastBitmap = nullptr;
}
//---------------------------------------------------------------------------
void FreeTypeFontRasterizer::GetTextExtent(tjs_char ch, tjs_int &w,
                                           tjs_int &h) {
    if(Face) {
        tGlyphMetrics metrics{};
        if(Face->GetGlyphSizeFromCharcode(ch, metrics)) {
            w = metrics.CellIncX;
            h = metrics.CellIncY;
        } else {
            w = Face->GetHeight();
            h = w;
        }
    }
}
//---------------------------------------------------------------------------
tjs_int FreeTypeFontRasterizer::GetAscentHeight() {
    if(Face)
        return Face->GetAscent();
    return 0;
}
static bool isUnicodeSpace(char16_t ch) {
    return (ch >= 0x0009 && ch <= 0x000D) || ch == 0x0020 || ch == 0x0085 ||
        ch == 0x00A0 || ch == 0x1680 || (ch >= 0x2000 && ch <= 0x200A) ||
        ch == 0x2028 || ch == 0x2029 || ch == 0x202F || ch == 0x205F ||
        ch == 0x3000;
}
//---------------------------------------------------------------------------
tTVPCharacterData *
FreeTypeFontRasterizer::GetBitmap(const tTVPFontAndCharacterData &font,
                                  tjs_int aofsx, tjs_int aofsy) {
    if(font.Antialiased) {
        Face->ClearOption(TVP_FACE_OPTIONS_NO_ANTIALIASING);
    } else {
        Face->SetOption(TVP_FACE_OPTIONS_NO_ANTIALIASING);
    }
    if(font.Hinting) {
        Face->ClearOption(TVP_FACE_OPTIONS_NO_HINTING);
        // Face->SetOption( TVP_FACE_OPTIONS_FORCE_AUTO_HINTING );
    } else {
        Face->SetOption(TVP_FACE_OPTIONS_NO_HINTING);
        // Face->ClearOption( TVP_FACE_OPTIONS_FORCE_AUTO_HINTING );
    }
    tTVPCharacterData *data = Face->GetGlyphFromCharcode(font.Character);
    if(!data && !isUnicodeSpace(font.Character)) {
        ApplyFallbackFace();
        if(FaceFallback) {
            data = FaceFallback->GetGlyphFromCharcode(font.Character);
        }
    }
    if(data == nullptr) {
        data = Face->GetGlyphFromCharcode(Face->GetDefaultChar());
    }
    if(data == nullptr) {
        data = Face->GetGlyphFromCharcode(Face->GetFirstChar());
    }
    if(data == nullptr) {
        TVPThrowExceptionMessage(TVPFontRasterizeError);
    }

    int cx = data->Metrics.CellIncX;
    int cy = data->Metrics.CellIncY;
    if(font.Font.Angle == 0) {
        data->Metrics.CellIncX = cx;
        data->Metrics.CellIncY = 0;
    } else if(font.Font.Angle == 2700) {
        data->Metrics.CellIncX = 0;
        data->Metrics.CellIncY = cx;
    } else {
        double angle = font.Font.Angle * (M_PI / 1800);
        data->Metrics.CellIncX = static_cast<tjs_int>(std::cos(angle) * cx);
        data->Metrics.CellIncY = static_cast<tjs_int>(-std::sin(angle) * cx);
    }

    data->Antialiased = font.Antialiased;
    data->FullColored = false;
    data->Blured = font.Blured;
    data->BlurWidth = font.BlurWidth;
    data->BlurLevel = font.BlurLevel;
    data->OriginX += aofsx; // for vertical text
                            //	data->OriginY += aofsy;

    // apply blur
    if(font.Blured)
        data->Blur(); // nasty ...
    return data;
}
//---------------------------------------------------------------------------
void FreeTypeFontRasterizer::GetGlyphDrawRect(const ttstr &text,
                                              tTVPRect &area) {
    // アンチエイリアスとヒンティングは有効にする
    Face->ClearOption(TVP_FACE_OPTIONS_NO_ANTIALIASING);
    Face->ClearOption(TVP_FACE_OPTIONS_NO_HINTING);

    area.left = area.top = area.right = area.bottom = 0;
    tjs_int offsetx = 0;
    tjs_int offsety = 0;
    tjs_uint len = text.length();
    for(tjs_uint i = 0; i < len; i++) {
        tjs_char ch = text[i];
        tjs_int ax, ay;
        tTVPRect rt(0, 0, 0, 0);
        bool result = Face->GetGlyphRectFromCharcode(rt, ch, ax, ay);
        if(result == false)
            result = Face->GetGlyphRectFromCharcode(rt, Face->GetDefaultChar(),
                                                    ax, ay);
        if(result == false)
            result = Face->GetGlyphRectFromCharcode(rt, Face->GetFirstChar(),
                                                    ax, ay);
        if(result) {
            rt.add_offsets(offsetx, offsety);
            if(i != 0) {
                area.do_union(rt);
            } else {
                area = rt;
            }
        }
        offsetx += ax;
        offsety = 0;
    }
}
