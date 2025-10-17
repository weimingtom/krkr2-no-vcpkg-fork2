//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Definition of Messages and Message Related Utilities
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "MsgIntf.h"
#include "MsgImpl.h"
#include "PluginImpl.h"

#include "CharacterSet.h"
// #include "resource.h"

//---------------------------------------------------------------------------
// version retrieving
//---------------------------------------------------------------------------
void TVPGetVersion() {
    static bool DoGet = true;
    if(DoGet) {
        DoGet = false;

        TVPVersionMajor = 2;
        TVPVersionMinor = 32;
        TVPVersionRelease = 2;
        TVPVersionBuild = 426;
#if 0
        TVPGetFileVersionOf(ExePath().c_str(), TVPVersionMajor, TVPVersionMinor,
            TVPVersionRelease, TVPVersionBuild);
#endif
    }
}

//---------------------------------------------------------------------------
// about string retrieving
//---------------------------------------------------------------------------
extern const tjs_char *TVPCompileDate;
extern const tjs_char *TVPCompileTime;

ttstr TVPReadAboutStringFromResource() {
    return TJS_W("Kirikiri2 Runtime Core version %1(TJS version %2)");
#if 0
    HMODULE hModule = ::GetModuleHandle(nullptr);
    const char *buf = nullptr;
    unsigned int size = 0;
    HRSRC hRsrc = ::FindResource(nullptr, MAKEINTRESOURCE(IDR_LICENSE_TEXT), TEXT("TEXT"));
    if( hRsrc != nullptr ) {
        size = ::SizeofResource( hModule, hRsrc );
        HGLOBAL hGlobal = ::LoadResource( hModule, hRsrc );
        if( hGlobal != nullptr ) {
            buf = reinterpret_cast<const char*>(::LockResource(hGlobal));
        }
    }
    if( buf == nullptr ) ttstr(L"Resource Read Error.");

    // UTF-8 to UTF-16
    size_t len = TVPUtf8ToWideCharString( buf, size, nullptr );
    if( len < 0 ) return ttstr(L"Resource Read Error.");
    wchar_t* tmp = new wchar_t[len+1];
    ttstr ret;
    if( tmp ) {
        try {
            len = TVPUtf8ToWideCharString( buf, size, tmp );
        } catch(...) {
            delete[] tmp;
            throw;
        }
        tmp[len] = 0;

        size_t datelen = TJS_strlen( TVPCompileDate );
        size_t timelen = TJS_strlen( TVPCompileTime );

        // CR to CR-LF, %DATE% and %TIME% to compile data and time
        std::vector<wchar_t> tmp2;
        tmp2.reserve( len * 2 + datelen + timelen );
        for( size_t i = 0; i < len; i++ ) {
            if( tmp[i] == '%' && (i+6) < len && tmp[i+1] == 'D' && tmp[i+2] == 'A' && tmp[i+3] == 'T' && tmp[i+4] == 'E' && tmp[i+5] == '%' ) {
                for( size_t j = 0; j < datelen; j++ ) {
                    tmp2.push_back( TVPCompileDate[j] );
                }
                i += 5;
            } else if( tmp[i] == '%' && (i+6) < len && tmp[i+1] == 'T' && tmp[i+2] == 'I' && tmp[i+3] == 'M' && tmp[i+4] == 'E' && tmp[i+5] == '%' ) {
                for( size_t j = 0; j < timelen; j++ ) {
                    tmp2.push_back( TVPCompileTime[j] );
                }
                i += 5;
            } else if( tmp[i] != L'\n' ) {
                tmp2.push_back( tmp[i] );
            } else {
                tmp2.push_back( L'\r' );
                tmp2.push_back( L'\n' );
            }
        }
        tmp2.push_back( 0 );
        ret = ttstr( &(tmp2[0]) );
        delete[] tmp;
    }
    return ret;
#endif
}
