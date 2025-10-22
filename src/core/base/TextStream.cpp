#if !MY_USE_MINLIB




#include <cstdint>
#if !MY_USE_MINLIB
#include <uchardet.h>
#endif
#include <zlib.h>
#include <optional>

#include "TextStream.h"

#if !MY_USE_MINLIB
#include <opencv2/core/hal/interface.h>
#include <spdlog/spdlog.h>
#endif

#include "MsgIntf.h"
#include "UtilStreams.h"
#include "tjsError.h"
#include "CharacterSet.h"
#include "BinaryStream.h"

static std::string G_DefaultReadEncoding = "UTF-8";

std::string checkTextEncoding(const void *buf, size_t size,
                              std::uint8_t &bomSize) {
    auto raw = static_cast<const unsigned char *>(buf);
    std::string encoding;
    // --- 检查 BOM ---
    if(size >= 2 && raw[0] == 0xFF && raw[1] == 0xFE) {
        // UTF-16LE BOM
        bomSize = 2;
        encoding = "UTF-16LE";
    } else if(size >= 2 && raw[0] == 0xFE && raw[1] == 0xFF) {
        // UTF-16BE BOM
        bomSize = 2;
        encoding = "UTF-16BE";
    } else if(size >= 3 && raw[0] == 0xEF && raw[1] == 0xBB && raw[2] == 0xBF) {
        // UTF-8 BOM
        bomSize = 3;
        encoding = "UTF-8";
    } else if(size >= 4 && raw[0] == 0xFF && raw[1] == 0xFE && raw[2] == 0x00 &&
              raw[3] == 0x00) {
        // UTF-32LE BOM
        bomSize = 4;
        encoding = "UTF-32LE";
    } else if(size >= 4 && raw[0] == 0x00 && raw[1] == 0x00 && raw[2] == 0xFE &&
              raw[3] == 0xFF) {
        // UTF-32BE BOM
        bomSize = 4;
        encoding = "UTF-32BE";
    } else {
#if !MY_USE_MINLIB	
        // ---------- 普通文本：用 uchardet 检测编码 ----------
        uchardet_t ud = uchardet_new();
        uchardet_handle_data(ud, reinterpret_cast<const char *>(raw), size);
        uchardet_data_end(ud);
        encoding = uchardet_get_charset(ud);
        uchardet_delete(ud);
        if(encoding == "SHIFT_JIS") {
            encoding = "cp932";
        } else if(encoding == "WINDOWS-1252") {
            encoding = "ASCII";
        }
#else
            encoding = "ASCII";
#endif		
    }

    return encoding;
}

/*
 *  note: encryption of mode 0 or 1 ( simple crypt ) does never
 *  intend data pretection security.
 */
class tTVPTextReadStream : public iTJSTextReadStream {
    std::unique_ptr<tTJSBinaryStream> _stream{};
    std::u16string _buffer; // 全部文本，UTF-16
    size_t _pos = 0; // 当前读取位置

public:
    tTVPTextReadStream(const ttstr &name, const ttstr &mode) {
        _stream.reset(TVPCreateStream(name, TJS_BS_READ));
        size_t ofs = parseModeNumber(mode.c_str(), TJS_W('o'), 255, 0).value();
        _stream->SetPosition(ofs);

        auto size = static_cast<size_t>(_stream->GetSize() - ofs);
        std::vector<std::uint8_t> raw(size);
        _stream->ReadBuffer(raw.data(), size);

        // ---------- 检查是否加密/压缩 ----------
        if(size >= 3 && raw[0] == 0xFE && raw[1] == 0xFE) {
            std::uint8_t m = raw[2];
            if(m == 0 || m == 1) {
                // 解密 UTF-16 数据
                const auto *src =
                    reinterpret_cast<const char16_t *>(raw.data() + 4);
                size_t len = (size - 4) / 2;
                _buffer.resize(len);
                for(size_t i = 0; i < len; i++) {
                    char16_t ch = src[i];
                    if(m == 0) {
                        if(ch >= 0x20)
                            ch ^= (((ch & 0xfe) << 8) ^ 1);
                    } else if(m == 1) {
                        ch =
                            ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
                    }
                    _buffer[i] = ch;
                }
                return;
            }
            if(m == 2) {
                // 压缩流
                if(size < 3 + 2 + 16)
                    TVPThrowExceptionMessage(TVPUnsupportedCipherMode, name);

                // 读压缩大小和解压大小
                std::uint8_t *ptr = raw.data() + 5;
                std::uint64_t compressed =
                    *reinterpret_cast<std::uint64_t *>(ptr);
                ptr += 8;
                std::uint64_t uncompressed =
                    *reinterpret_cast<std::uint64_t *>(ptr);
                ptr += 8;

                std::vector<std::uint8_t> compBuf(compressed);
                memcpy(compBuf.data(), ptr, compressed);

                std::vector<std::uint8_t> uncompBuf(uncompressed);
                auto destLen = static_cast<unsigned long>(uncompressed);
                int ret = uncompress(uncompBuf.data(), &destLen, compBuf.data(),
                                     static_cast<unsigned long>(compressed));
                if(ret != Z_OK || destLen != uncompressed)
                    TVPThrowExceptionMessage(TVPUnsupportedCipherMode, name);

                // 解压得到 UTF-16 数据
                _buffer.assign(reinterpret_cast<char16_t *>(uncompBuf.data()),
                               reinterpret_cast<char16_t *>(uncompBuf.data() +
                                                            uncompressed));
                return;
            }
            TVPThrowExceptionMessage(TVPUnsupportedCipherMode, name);
        }
        std::uint8_t bomSize = 0;
        std::string encoding = checkTextEncoding(raw.data(), size, bomSize);
        raw.erase(raw.begin(), raw.begin() + bomSize);

        if(encoding.empty())
            encoding = G_DefaultReadEncoding; // 默认回退

        if(encoding == "ASCII") {
            _buffer.assign(raw.data(), raw.data() + size);
            return;
        }
#if !MY_USE_MINLIB
        if(encoding == "UTF-8") {
            _buffer = boost::locale::conv::utf_to_utf<char16_t>(
                reinterpret_cast<const char *>(raw.data()),
                reinterpret_cast<const char *>(raw.data() + size));
            return;
        }
#endif		

        if(encoding == "UTF-16" || encoding == "UTF-16LE" ||
           encoding == "UTF-16BE") {
            _buffer.assign(
                reinterpret_cast<const char16_t *>(raw.data()),
                reinterpret_cast<const char16_t *>(raw.data() + raw.size()));

            if(encoding == "UTF-16BE") {
                size_t len = raw.size() / 2;
                _buffer.resize(len);
                auto src = reinterpret_cast<const char16_t *>(raw.data());
                for(size_t i = 0; i < len; i++) {
                    char16_t ch = src[i];
                    _buffer[i] = (ch >> 8) | (ch << 8);
                }
            }

            return;
        }

#if !MY_USE_MINLIB
        if(encoding == "UTF-32" || encoding == "UTF-32LE" ||
           encoding == "UTF-32BE") {
            _buffer = boost::locale::conv::utf_to_utf<char16_t>(
                reinterpret_cast<const char32_t *>(raw.data()),
                reinterpret_cast<const char32_t *>(raw.data() + size));
            return;
        }

        // 其他文本字符
        try {
            std::wstring wide = boost::locale::conv::to_utf<wchar_t>(
                reinterpret_cast<const char *>(raw.data()),
                reinterpret_cast<const char *>(raw.data() + raw.size()),
                encoding);
            _buffer = boost::locale::conv::utf_to_utf<char16_t>(wide);
        } catch(const std::exception &e) {
            spdlog::error(e.what());
            TVPThrowExceptionMessage(TJSNarrowToWideConversionError);
        }
#endif		
    }

    ~tTVPTextReadStream() override = default;

    tjs_uint Read(tTJSString &targ, tjs_uint size) override {
        static_assert(sizeof(tjs_char) == sizeof(char16_t),
                      "Char size mismatch");
        if(_pos >= _buffer.size()) {
            targ.Clear();
            return 0;
        }
        size_t remain = _buffer.size() - _pos;
        size_t n = size ? size : remain;
        tjs_char *buf = targ.AllocBuffer(n);
        std::copy_n(_buffer.data() + _pos, n, buf);
        buf[n] = 0;
        _pos += n;
        targ.FixLen();
        return n;
    }

    void Destruct() override { delete this; }
};


class tTVPTextWriteStream : public iTJSTextWriteStream {
    // TODO: 32bit wchar_t support

    static constexpr size_t COMPRESSION_BUFFER_SIZE = 1024 * 1024;

    std::unique_ptr<tTJSBinaryStream> _stream{};
    tjs_int _cryptMode{};
    // -1 for no-crypt
    // 0: (unused)	(old buggy crypt mode)
    // 1: simple crypt
    // 2: complessed
    int _compressionLevel{}; // compression level of zlib

    std::unique_ptr<z_stream_s> _zStream{};
    tjs_uint _compressionSizePosition{ 0 };
    std::vector<Bytef> _compressionBuffer =
        std::vector<Bytef>(COMPRESSION_BUFFER_SIZE);
    bool _compressionFailed{ false };

public:
    tTVPTextWriteStream(const ttstr &name, const ttstr &mode) {
        // mode supports following modes:
        // dN: deflate(compress) at mode N ( currently not implemented
        // ) cN: write in cipher at mode N ( currently n is ignored )
        // zN: write with compress at mode N ( N is compression level
        // ) oN: write from binary offset N (in bytes)

        // check c/z mode
        _cryptMode =
            parseModeNumber(mode.c_str(), TJS_W('c'), 1, -1).value_or(1);

        if(auto z = parseModeNumber(mode.c_str(), TJS_W('z'), 1,
                                    Z_DEFAULT_COMPRESSION)) {
            _compressionLevel = z.value();
        } else {
            _cryptMode = 2;
        }

        if(_cryptMode != -1 && _cryptMode != 1 && _cryptMode != 2)
            TVPThrowExceptionMessage(TVPUnsupportedModeString,
                                     TJS_W("unsupported cipher mode"));

        // check o mode
        int ofs = parseModeNumber(mode.c_str(), TJS_W('o'), 255, 0).value();
        if(ofs != 0) {
            _stream.reset(TVPCreateStream(name, TJS_BS_UPDATE));
            _stream->SetPosition(ofs);
        } else {
            _stream.reset(TVPCreateStream(name, TJS_BS_WRITE));
        }

        if(_cryptMode == 1 || _cryptMode == 2) {
            // simple crypt or compressed
            tjs_uint8 crypt_mode_sig[4];
            crypt_mode_sig[0] = crypt_mode_sig[1] = 0xfe;
            crypt_mode_sig[2] = static_cast<tjs_uint8>(_cryptMode);
            crypt_mode_sig[3] = 0;
            _stream->WriteBuffer(crypt_mode_sig, 3);
        }

        // now output text stream will write unicode texts
        static tjs_uint8 bommark[2] = { 0xff, 0xfe };
        _stream->WriteBuffer(bommark, 2);

        if(_cryptMode == 2) {
            // allocate and initialize zlib straem
            _zStream.reset(new z_stream_s());
            _zStream->zalloc = Z_NULL;
            _zStream->zfree = Z_NULL;
            _zStream->opaque = Z_NULL;
            if(deflateInit(_zStream.get(), _compressionLevel) != Z_OK) {
                _compressionFailed = true;
                TVPThrowExceptionMessage(TVPCompressionFailed);
            }

            _zStream->next_in = nullptr;
            _zStream->avail_in = 0;
            _zStream->next_out = _compressionBuffer.data();
            _zStream->avail_out = COMPRESSION_BUFFER_SIZE;

            // Compression Size (write dummy)
            _compressionSizePosition =
                static_cast<tjs_uint>(_stream->GetPosition());
            WriteI64LE(0);
            WriteI64LE(0);
        }
    }

    ~tTVPTextWriteStream() override {
        if(_cryptMode == 2) {

            if(!_compressionFailed) {
                try {
                    // close zlib stream
                    int result = 0;
                    do {
                        result = deflate(_zStream.get(), Z_FINISH);
                        if(result != Z_OK && result != Z_STREAM_END) {
                            TVPThrowExceptionMessage(TVPCompressionFailed);
                        }
                        _stream->WriteBuffer(_compressionBuffer.data(),
                                             COMPRESSION_BUFFER_SIZE -
                                                 _zStream->avail_out);
                        _zStream->next_out = _compressionBuffer.data();
                        _zStream->avail_out = COMPRESSION_BUFFER_SIZE;
                    } while(result != Z_STREAM_END);

                    // rollback and write compression size.
                    _stream->SetPosition(_compressionSizePosition);
                    WriteI64LE(_zStream->total_out);
                    WriteI64LE(_zStream->total_in);
                } catch(...) {
                    // delete zlib compress stream
                    if(_zStream) {
                        deflateEnd(_zStream.get());
                    }
                    throw;
                }
            }
            // delete zlib compress stream
            if(_zStream) {
                deflateEnd(_zStream.get());
            }
        }
    }

    void WriteI64LE(tjs_uint64 v) {
        // write 64bit little endian value to the file.
        tjs_uint8 buf[8];
        for(int i = 0; i < 8; i++) {
            buf[i] = static_cast<tjs_uint8>(v >> (i * 8));
        }
        _stream->WriteBuffer(buf, 8);
    }

    void Write(const ttstr &targ) override {
        tjs_int len = targ.GetLen();
        auto buf = std::make_unique<tjs_uint16[]>(len + 1);
        const tjs_char *src = targ.c_str();
        tjs_int i;
        for(i = 0; i < len; i++) {
            buf[i] = src[i];
        }
        buf[i] = 0;

        if(_cryptMode == 1) {
            // simple crypt
            if(tjs_uint16 *p = buf.get()) {
                while(*p) {
                    tjs_char ch = *p;
                    ch = (ch & 0xaaaaaaaa) >> 1 | (ch & 0x55555555) << 1;
                    *p = ch;
                    p++;
                }
            }

            WriteRawData(buf.get(), len * sizeof(tjs_uint16));
        } else {
            WriteRawData(buf.get(), len * sizeof(tjs_uint16));
        }
    }

    void WriteRawData(void *ptr, size_t size) {
        if(_cryptMode == 2) {
            // compressed with zlib stream.
            _zStream->next_in = static_cast<Bytef *>(ptr);
            _zStream->avail_in = size;

            while(_zStream->avail_in > 0) {
                int result = deflate(_zStream.get(), Z_NO_FLUSH);
                if(result != Z_OK) {
                    _compressionFailed = true;
                    TVPThrowExceptionMessage(TVPCompressionFailed);
                }
                if(_zStream->avail_out == 0) {
                    _stream->WriteBuffer(_compressionBuffer.data(),
                                         COMPRESSION_BUFFER_SIZE);
                    _zStream->next_out = _compressionBuffer.data();
                    _zStream->avail_out = COMPRESSION_BUFFER_SIZE;
                }
            }
        } else {
            _stream->WriteBuffer(ptr, size); // write directly
        }
    }

    void Destruct() override { delete this; }
};

iTJSTextReadStream *TVPCreateTextStreamForRead(const ttstr &name,
                                               const ttstr &mode) {
    return new tTVPTextReadStream(name, mode);
}

iTJSTextWriteStream *TVPCreateTextStreamForWrite(const ttstr &name,
                                                 const ttstr &mode) {
    return new tTVPTextWriteStream(name, mode);
}

//---------------------------------------------------------------------------
void TVPSetDefaultReadEncoding(const ttstr &encoding) {
    ttstr codestr = encoding;
    codestr.ToLowerCase();
    if(codestr == TJS_W("sjis") || codestr == TJS_W("shiftjis") ||
       codestr == TJS_W("shift_jis") || codestr == TJS_W("shift-jis")) {
        G_DefaultReadEncoding = "cp932";
    } else if(codestr == TJS_W("utf8") || codestr == TJS_W("utf-8")) {
        G_DefaultReadEncoding = "UTF-8";
    } else {
        G_DefaultReadEncoding = encoding.AsStdString();
    }
}

//---------------------------------------------------------------------------
const tjs_char *TVPGetDefaultReadEncoding() {
    return ttstr{ G_DefaultReadEncoding }.c_str();
}



































































#else /*MY_USE_MINLIB*/


//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Text read/write stream
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <zlib.h>
#include "TextStream.h"
#include "MsgIntf.h"
#include "DebugIntf.h"
#include "EventIntf.h"
#include "UtilStreams.h"
#include "tjsError.h"
#include "CharacterSet.h"

extern "C" int sjis_mbtowc(unsigned short *wc, const unsigned char *s);
extern "C" int gbk_mbtowc(unsigned short *wc, const unsigned char *s);

int utf8_mbtowc(unsigned short *pwc, const unsigned char *s) {
    unsigned char c = s[0];

    if(c < 0x80) {
        *pwc = c;
        return 1;
    } else if(c < 0xc2) {
        return -1;
    } else if(c < 0xe0) {
        if(!((s[1] ^ 0x80) < 0x40))
            return -1;
        *pwc =
            ((unsigned short)(c & 0x1f) << 6) | (unsigned short)(s[1] ^ 0x80);
        return 2;
    } else if(c < 0xf0) {
        if(!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40 &&
             (c >= 0xe1 || s[1] >= 0xa0)))
            return -1;
        *pwc = ((unsigned short)(c & 0x0f) << 12) |
            ((unsigned short)(s[1] ^ 0x80) << 6) |
            (unsigned short)(s[2] ^ 0x80);
        return 3;
    } else
        return -1;
}

int (*mbtowc_for_text_stream)(unsigned short *wc,
                              const unsigned char *s) = nullptr;

static size_t _TextStream_mbstowcs(int (*func_mbtowc)(unsigned short *,
                                                      const unsigned char *),
                                   tjs_char *pwcs, const tjs_nchar *s,
                                   size_t n) {
    if(!s)
        return -1;
    if(pwcs && n == 0)
        return 0;

    size_t count = 0;
    int cl;
    if(!pwcs) {
        while(*s) {
            unsigned short wc;
            cl = func_mbtowc(&wc, (const unsigned char *)s);
            if(cl <= 0)
                return -1;
            s += cl;
            ++count;
        }
    } else {
        while(*s && n > 0) {
            cl = func_mbtowc((unsigned short *)pwcs, (const unsigned char *)s);
            if(cl <= 0)
                return -1;
            n--;
            s += cl;
            pwcs++;
            ++count;
        }
    }
    return count;
}

/*
        Text stream is used by TJS's Array.save, Dictionary.saveStruct
   etc. to input/output text files.
*/

extern size_t TextStream_mbstowcs(tjs_char *pwcs, const tjs_nchar *s,
                                  size_t n) {
    if(mbtowc_for_text_stream) {
        return _TextStream_mbstowcs(mbtowc_for_text_stream, pwcs, s, n);
    }
    // trying every encoding available
    size_t ret = _TextStream_mbstowcs(sjis_mbtowc, pwcs, s, n);
    if(ret == (size_t)-1) {
        ret = _TextStream_mbstowcs(utf8_mbtowc, pwcs, s, n);
        if(ret != (size_t)-1) {
            mbtowc_for_text_stream = utf8_mbtowc;
            return ret;
        }
        ret = _TextStream_mbstowcs(gbk_mbtowc, pwcs, s, n);
        if(ret != (size_t)-1) {
            mbtowc_for_text_stream = gbk_mbtowc;
            return ret;
        }
    }
    return ret;
}

static ttstr enc_utf8 = TJS_W("utf8"), enc_utf8_2 = TJS_W("utf-8"),
             enc_utf16 = TJS_W("utf16"), enc_utf16_2 = TJS_W("utf-16"),
             enc_gbk = TJS_W("gbk"), enc_jis = TJS_W("sjis"),
             enc_jis_2 = TJS_W("shiftjis"), enc_jis_3 = TJS_W("shift_jis"),
             enc_jis_4 = TJS_W("shift-jis");

bool TVPStringDecode(const void *p, int len, ttstr &result,
                     ttstr encoding /*= "utf8"*/) {
    if(encoding == enc_utf8 || encoding == enc_utf8_2) {
        int n = (int)TJS_mbstowcs(nullptr, (char *)p, len);
        if(n == -1)
            return false;
        TJS_mbstowcs(result.AllocBuffer(n), (char *)p, len);
    } else if(encoding == enc_utf16 || encoding == enc_utf16_2) {
        memcpy(result.AllocBuffer(len / 2), p, len);
        result.FixLen();
    } else if(encoding == enc_jis || encoding == enc_jis_2 ||
              encoding == enc_jis_3 || encoding == enc_jis_4) {
        int n = _TextStream_mbstowcs(sjis_mbtowc, nullptr, (char *)p, len);
        if(n == -1)
            return false;
        _TextStream_mbstowcs(sjis_mbtowc, result.AllocBuffer(n), (char *)p,
                             len);
    } else if(encoding == enc_gbk) {
        int n = _TextStream_mbstowcs(gbk_mbtowc, nullptr, (char *)p, len);
        if(n == -1)
            return false;
        _TextStream_mbstowcs(gbk_mbtowc, result.AllocBuffer(n), (char *)p, len);
    } else {
        return false;
    }
    return true;
}

bool TVPStringEncode(const ttstr &src, std::string &result,
                     ttstr encoding /*= "utf8"*/) {
    if(encoding == enc_utf8 || encoding == enc_utf8_2) {
        result = src.AsStdString();
    } else if(encoding == enc_utf16 || encoding == enc_utf16_2) {
        result.resize(src.length() * 2);
        memcpy((char *)result.c_str(), src.c_str(), src.length() * 2);
        // 	} else if (encoding == enc_jis || encoding == enc_jis_2 ||
        // encoding == enc_jis_3 || encoding == enc_jis_4) { 	} else
        // if (encoding == enc_gbk) {
        // // unsupported yet
    } else {
        return false;
    }
    return true;
}

#ifdef TVP_TEXT_READ_ANSI_MBCS
static ttstr DefaultReadEncoding = TJS_W("Shift_JIS");
#else
static ttstr DefaultReadEncoding = TJS_W("UTF-8");
#endif
//---------------------------------------------------------------------------
// Interface to tTJSTextStream
//---------------------------------------------------------------------------
/*
        note: encryption of mode 0 or 1 ( simple crypt ) does never
   intend data pretection security.
*/
class tTVPTextReadStream : public iTJSTextReadStream {
    tTJSBinaryStream *Stream;
    bool DirectLoad;
    tjs_char *Buffer;
    size_t BufferLen;
    tjs_char *BufferPtr;
    tjs_int CryptMode;

public:
    tTVPTextReadStream(const ttstr &name, const ttstr &modestr) {
        // following syntax of modestr is currently supported.
        // oN: read from binary offset N (in bytes)

#ifndef TVP_NO_CHECK_WIDE_CHAR_SIZE
        if(sizeof(tjs_char) != 2)
            TVPThrowExceptionMessage(TVPTheHostIsNotA16BitUnicodeSystem);
#endif

        Stream = nullptr;
        Buffer = nullptr;
        DirectLoad = false;
        CryptMode = -1;

        // check o mode
        Stream = TVPCreateStream(name, TJS_BS_READ);

        tjs_uint64 ofs = 0;
        const tjs_char *o_ofs;
        o_ofs = TJS_strchr(modestr.c_str(), TJS_W('o'));
        if(o_ofs != nullptr) {
            // seek to offset
            o_ofs++;
            tjs_char buf[256];
            int i;
            for(i = 0; i < 255; i++) {
                if(o_ofs[i] >= TJS_W('0') && o_ofs[i] <= TJS_W('9'))
                    buf[i] = o_ofs[i];
                else
                    break;
            }
            buf[i] = 0;
            ofs = ttstr(buf).AsInteger();
            Stream->SetPosition(ofs);
        }

        // check first of the file - whether the file is unicode
        try {
            tjs_uint8 mark[3] = { 0, 0 };
            Stream->Read(mark, 3);
            if(mark[0] == 0xff && mark[1] == 0xfe) {
                // unicode
                Stream->SetPosition(ofs + 2);
                DirectLoad = true;
            } else if(mark[0] == 0xfe && mark[1] == 0xfe) {
                // ciphered text or compressed
                tjs_uint8 mode = mark[2];
                if(mode != 0 && mode != 1 && mode != 2)
                    TVPThrowExceptionMessage(TVPUnsupportedCipherMode, name);
                // currently only mode0 and mode1, and compressed
                // (mode2) are supported
                CryptMode = mode;
                DirectLoad = CryptMode != 2;

                Stream->Read(mark,
                             2); // original bom code comes here (is
                                 // not compressed)
                if(mark[0] != 0xff || mark[1] != 0xfe)
                    TVPThrowExceptionMessage(TVPUnsupportedCipherMode, name);

                if(CryptMode == 2) {
                    // compressed text stream
                    tjs_uint64 compressed = Stream->ReadI64LE();
                    tjs_uint64 uncompressed = Stream->ReadI64LE();
                    if(compressed != (unsigned long)compressed ||
                       uncompressed != (unsigned long)uncompressed)
                        TVPThrowExceptionMessage(TVPUnsupportedCipherMode,
                                                 name);
                    // too large stream
                    unsigned long destlen;
                    tjs_uint8 *nbuf =
                        new tjs_uint8[(unsigned long)compressed + 1];
                    try {
                        Stream->ReadBuffer(nbuf, (unsigned long)compressed);
                        Buffer =
                            new tjs_char[(BufferLen = (destlen = (unsigned long)
                                                           uncompressed) /
                                              2) +
                                         1];
                        int result = uncompress(/* uncompress from zlib */
                                                (unsigned char *)Buffer,
                                                &destlen, (unsigned char *)nbuf,
                                                (unsigned long)compressed);
                        if(result != Z_OK ||
                           destlen != (unsigned long)uncompressed)
                            TVPThrowExceptionMessage(TVPUnsupportedCipherMode,
                                                     name);
                        // uncompression failed.
                    } catch(...) {
                        delete[] nbuf;
                        throw;
                    }
                    delete[] nbuf;
                    Buffer[BufferLen] = 0;
                    BufferPtr = Buffer;
                }
            } else {
                // check UTF-8 BOM
                if(mark[0] == 0xef && mark[1] == 0xbb && mark[2] == 0xbf) {
                    // UTF-8 BOM
                    tjs_uint size = (tjs_uint)(Stream->GetSize() - 3) - ofs;
                    tjs_uint8 *nbuf = new tjs_uint8[size + 1];
                    try {
                        Stream->ReadBuffer(nbuf, size);
                        nbuf[size] = 0; // terminater
                        BufferLen = TVPUtf8ToWideCharString((const char *)nbuf,
                                                            nullptr);
                        if(BufferLen == (size_t)-1)
                            TVPThrowExceptionMessage(
                                TJSNarrowToWideConversionError);
                        Buffer = new tjs_char[BufferLen + 1];
                        TVPUtf8ToWideCharString((const char *)nbuf, Buffer);
                    } catch(...) {
                        delete[] nbuf;
                        throw;
                    }
                    delete[] nbuf;
                    Buffer[BufferLen] = 0;
                    BufferPtr = Buffer;
                } else {
                    // ansi/mbcs
                    // read whole and hold it
                    Stream->SetPosition(ofs);
                    tjs_uint size = (tjs_uint)(Stream->GetSize()) - ofs;
                    tjs_uint8 *nbuf = new tjs_uint8[size + 1];
                    try {
                        Stream->ReadBuffer(nbuf, size);
                        nbuf[size] = 0; // terminater
                        BufferLen =
                            TextStream_mbstowcs(nullptr, (tjs_nchar *)nbuf, 0);
                        if(BufferLen == (size_t)-1) {
                            ttstr msg(
                                TVPGetMessageByLocale("err_narrow_to_wide"));
                            TVPThrowExceptionMessage(msg.c_str());
                        }
                        Buffer = new tjs_char[BufferLen + 1];
                        TextStream_mbstowcs(Buffer, (tjs_nchar *)nbuf,
                                            BufferLen);
                    } catch(...) {
                        delete[] nbuf;
                        throw;
                    }
                    delete[] nbuf;
                    Buffer[BufferLen] = 0;
                    BufferPtr = Buffer;
                }
            }
        } catch(...) {
            delete Stream;
            Stream = nullptr;
            throw;
        }
    }

    ~tTVPTextReadStream() {
        if(Stream)
            delete Stream;
        if(Buffer)
            delete[] Buffer;
    }

    tjs_uint Read(tTJSString &targ, tjs_uint size) {
        if(DirectLoad) {
            if(sizeof(tjs_char) == 2) {
                if(size == 0)
                    size = static_cast<tjs_uint>(Stream->GetSize() -
                                                 Stream->GetPosition());
                if(!size) {
                    targ.Clear();
                    return 0;
                }
                tjs_char *buf = targ.AllocBuffer(size);
                tjs_uint read =
                    Stream->Read(buf, size * 2); // 2 = BMP unicode size
                read /= 2;
#if TJS_HOST_IS_BIG_ENDIAN
                // re-order input
                for(tjs_uint i = 0; i < read; i++) {
                    tjs_char ch = buf[i];
                    buf[i] = ((ch >> 8) & 0xff) + ((ch & 0xff) << 8);
                }
#endif
                if(CryptMode == 0) {
                    // simple crypt
                    for(tjs_uint i = 0; i < read; i++) {
                        tjs_char ch = buf[i];
                        if(ch >= 0x20)
                            buf[i] = ch ^ (((ch & 0xfe) << 8) ^ 1);
                    }
                } else if(CryptMode == 1) {
                    // simple crypt
                    for(tjs_uint i = 0; i < read; i++) {
                        tjs_char ch = buf[i];
                        ch =
                            ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
                        buf[i] = ch;
                    }
                }
                buf[read] = 0;
                targ.FixLen();
                return read;
            } else {
                // sizeof(tjs_char) is 4
                // FIXME: NOT TESTED CODE
                if(size == 0)
                    size = static_cast<tjs_uint>(Stream->GetSize() -
                                                 Stream->GetPosition());
                tjs_uint16 *buf = new tjs_uint16[size / 2];
                tjs_uint read;
                try {
                    read = Stream->Read(buf, size * 2); // 2 = BMP unicode size
                    read /= 2;
#if TJS_HOST_IS_BIG_ENDIAN
                    // re-order input
                    for(tjs_uint i = 0; i < read; i++) {
                        tjs_char ch = buf[i];
                        buf[i] = ((ch >> 8) & 0xff) + ((ch & 0xff) << 8);
                    }
#endif
                    if(CryptMode == 0) {
                        // simple crypt (buggy version)
                        for(tjs_uint i = 0; i < read; i++) {
                            tjs_char ch = buf[i];
                            if(ch >= 0x20)
                                buf[i] = ch ^ (((ch & 0xfe) << 8) ^ 1);
                        }
                    } else if(CryptMode == 1) {
                        // simple crypt
                        for(tjs_uint i = 0; i < read; i++) {
                            tjs_char ch = buf[i];
                            ch = ((ch & 0xaaaaaaaa) >> 1) |
                                ((ch & 0x55555555) << 1);
                            buf[i] = ch;
                        }
                    }
                    buf[read] = 0;
                } catch(...) {
                    delete[] buf;
                    throw;
                }
                targ = TVPStringFromBMPUnicode(buf);
                delete[] buf;
                return read;
            }
        } else {
            if(size == 0)
                size = (tjs_uint)BufferLen;
            if(size) {
                tjs_char *buf = targ.AllocBuffer(size);
                TJS_strncpy(buf, BufferPtr, size);
                buf[size] = 0;
                BufferPtr += size;
                BufferLen -= size;
                targ.FixLen();
            } else {
                targ.Clear();
            }
            return size;
        }
    }

    void Destruct() { delete this; }
};

//---------------------------------------------------------------------------
class tTVPTextWriteStream : public iTJSTextWriteStream {
    // TODO: 32bit wchar_t support

    static const tjs_uint COMPRESSION_BUFFER_SIZE = 1024 * 1024;

    tTJSBinaryStream *Stream;
    tjs_int CryptMode;
    // -1 for no-crypt
    // 0: (unused)	(old buggy crypt mode)
    // 1: simple crypt
    // 2: complessed
    int CompressionLevel; // compression level of zlib

    z_stream_s *ZStream;
    tjs_uint CompressionSizePosition;
    tjs_nchar *CompressionBuffer;
    bool CompressionFailed;

public:
    tTVPTextWriteStream(const ttstr &name, const ttstr &modestr) {
        // modestr supports following modes:
        // dN: deflate(compress) at mode N ( currently not implemented
        // ) cN: write in cipher at mode N ( currently n is ignored )
        // zN: write with compress at mode N ( N is compression level
        // ) oN: write from binary offset N (in bytes)
        Stream = nullptr;
        CryptMode = -1;
        CompressionLevel = Z_DEFAULT_COMPRESSION;
        ZStream = nullptr;
        CompressionSizePosition = 0;
        CompressionBuffer = nullptr;
        CompressionFailed = false;

        // check c/z mode
        const tjs_char *p;
        if((p = TJS_strchr(modestr.c_str(), TJS_W('c'))) != nullptr) {
            CryptMode = 1; // simple crypt
            if(p[1] >= TJS_W('0') && p[1] <= TJS_W('9'))
                CryptMode = p[1] - TJS_W('0');
        }

        if((p = TJS_strchr(modestr.c_str(), TJS_W('z'))) != nullptr) {
            CryptMode = 2; // compressed (cannot be with 'c')
            if(p[1] >= TJS_W('0') && p[1] <= TJS_W('9'))
                CompressionLevel = p[1] - TJS_W('0');
        }

        if(CryptMode != -1 && CryptMode != 1 && CryptMode != 2)
            TVPThrowExceptionMessage(TVPUnsupportedModeString,
                                     TJS_W("unsupported cipher mode"));

        // check o mode
        const tjs_char *o_ofs;
        o_ofs = TJS_strchr(modestr.c_str(), TJS_W('o'));
        if(o_ofs != nullptr) {
            // seek to offset
            o_ofs++;
            tjs_char buf[256];
            int i;
            for(i = 0; i < 255; i++) {
                if(o_ofs[i] >= TJS_W('0') && o_ofs[i] <= TJS_W('9'))
                    buf[i] = o_ofs[i];
                else
                    break;
            }
            buf[i] = 0;
            tjs_uint64 ofs = ttstr(buf).AsInteger();
            Stream = TVPCreateStream(name, TJS_BS_UPDATE);
            Stream->SetPosition(ofs);
        } else {
            Stream = TVPCreateStream(name, TJS_BS_WRITE);
        }

        if(CryptMode == 1 || CryptMode == 2) {
            // simple crypt or compressed
            tjs_uint8 crypt_mode_sig[4];
            crypt_mode_sig[0] = crypt_mode_sig[1] = 0xfe;
            crypt_mode_sig[2] = (tjs_uint8)CryptMode;
            crypt_mode_sig[3] = 0;
            Stream->WriteBuffer(crypt_mode_sig, 3);
        }

        // now output text stream will write unicode texts
        static tjs_uint8 bommark[4] = { 0xff, 0xfe, 0x00,
                                        0x00 /*dummy 2bytes*/ };
        Stream->WriteBuffer(bommark, 2);

        if(CryptMode == 2) {
            // allocate and initialize zlib straem
            ZStream = new z_stream_s();
            ZStream->zalloc = Z_NULL;
            ZStream->zfree = Z_NULL;
            ZStream->opaque = Z_NULL;
            if(deflateInit(ZStream, CompressionLevel) != Z_OK) {
                CompressionFailed = true;
                TVPThrowExceptionMessage(TVPCompressionFailed);
            }

            CompressionBuffer = new tjs_nchar[COMPRESSION_BUFFER_SIZE];

            ZStream->next_in = nullptr;
            ZStream->avail_in = 0;
            ZStream->next_out = reinterpret_cast<Bytef *>(CompressionBuffer);
            ZStream->avail_out = COMPRESSION_BUFFER_SIZE;

            // Compression Size (write dummy)
            CompressionSizePosition =
                static_cast<tjs_uint>(Stream->GetPosition());
            WriteI64LE((tjs_uint64)0);
            WriteI64LE((tjs_uint64)0);
        }
    }

    ~tTVPTextWriteStream() {
        if(CryptMode == 2) {

            if(!CompressionFailed) {
                try {
                    // close zlib stream
                    int result = 0;
                    do {
                        result = deflate(ZStream, Z_FINISH);
                        if(result != Z_OK && result != Z_STREAM_END) {
                            TVPThrowExceptionMessage(TVPCompressionFailed);
                        }
                        Stream->WriteBuffer(CompressionBuffer,
                                            COMPRESSION_BUFFER_SIZE -
                                                ZStream->avail_out);
                        ZStream->next_out =
                            reinterpret_cast<Bytef *>(CompressionBuffer);
                        ZStream->avail_out = COMPRESSION_BUFFER_SIZE;
                    } while(result != Z_STREAM_END);

                    // rollback and write compression size.
                    Stream->SetPosition(CompressionSizePosition);
                    WriteI64LE((tjs_uint64)ZStream->total_out);
                    WriteI64LE((tjs_uint64)ZStream->total_in);
                } catch(...) {
                    // delete zlib compress stream
                    if(ZStream) {
                        deflateEnd(ZStream);
                        delete ZStream;
                    }
                    delete[] CompressionBuffer;
                    delete Stream;
                    throw;
                }
            }
            // delete zlib compress stream
            if(ZStream) {
                deflateEnd(ZStream);
                delete ZStream;
            }
            delete[] CompressionBuffer;
        }

        if(Stream)
            delete Stream;
    }

    void WriteI64LE(tjs_uint64 v) {
        // write 64bit little endian value to the file.
        tjs_uint8 buf[8];
        buf[0] = (tjs_uint8)(v >> (0 * 8));
        buf[1] = (tjs_uint8)(v >> (1 * 8));
        buf[2] = (tjs_uint8)(v >> (2 * 8));
        buf[3] = (tjs_uint8)(v >> (3 * 8));
        buf[4] = (tjs_uint8)(v >> (4 * 8));
        buf[5] = (tjs_uint8)(v >> (5 * 8));
        buf[6] = (tjs_uint8)(v >> (6 * 8));
        buf[7] = (tjs_uint8)(v >> (7 * 8));

        Stream->WriteBuffer(buf, 8);
    }

    void Write(const ttstr &targ) {
        tjs_uint16 *buf;
        tjs_int len = targ.GetLen();
        buf = new tjs_uint16[len + 1];
        try {
            const tjs_char *src = targ.c_str();
            tjs_int i;
            for(i = 0; i < len; i++) {
                if(src[i] < 0x10000)
                    buf[i] = src[i];
                else
                    buf[i] = TJS_W('?');
            }
            buf[i] = 0;

#if TJS_HOST_IS_BIG_ENDIAN
            tjs_uint16 *p;
            if(CryptMode == 1) {
                // simple crypt
                p = buf;
                if(p) {
                    while(*p) {
                        tjs_char ch = *p;
                        ch =
                            ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
                        *p = ch;
                        p++;
                    }
                }
            }

            // re-order to little endian unicode text
            p = buf;
            if(p) {
                while(*p) {
                    *p = ((*p >> 8) & 0xff) + ((*p & 0xff) << 8);
                    p++;
                }
            }

            WriteRawData(str.c_str(), len * sizeof(tjs_char));
#else
            if(CryptMode == 1) {
                // simple crypt
                tjs_uint16 *p;
                p = buf;
                if(p) {
                    while(*p) {
                        tjs_char ch = *p;
                        ch =
                            ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
                        *p = ch;
                        p++;
                    }
                }

                WriteRawData(buf, len * sizeof(tjs_uint16));

            } else {
                WriteRawData(buf, len * sizeof(tjs_uint16));
            }
#endif
        } catch(...) {
            delete[] buf;
            throw;
        }
        delete[] buf;
    }

    void WriteRawData(void *ptr, size_t size) {
        if(CryptMode == 2) {
            // compressed with zlib stream.
            ZStream->next_in = (Bytef *)ptr;
            ZStream->avail_in = (uInt)size;

            while(ZStream->avail_in > 0) {
                int result = deflate(ZStream, Z_NO_FLUSH);
                if(result != Z_OK) {
                    CompressionFailed = true;
                    TVPThrowExceptionMessage(TVPCompressionFailed);
                }
                if(ZStream->avail_out == 0) {
                    Stream->WriteBuffer(CompressionBuffer,
                                        COMPRESSION_BUFFER_SIZE);
                    ZStream->next_out =
                        reinterpret_cast<Bytef *>(CompressionBuffer);
                    ZStream->avail_out = COMPRESSION_BUFFER_SIZE;
                }
            }
        } else {
            Stream->WriteBuffer(ptr,
                                (tjs_uint)size); // write directly
        }
    }

    void Destruct() { delete this; }
};

//---------------------------------------------------------------------------
iTJSTextReadStream *TVPCreateTextStreamForRead(const ttstr &name,
                                               const ttstr &modestr) {
    return new tTVPTextReadStream(name, modestr);
}

//---------------------------------------------------------------------------
iTJSTextReadStream *TVPCreateTextStreamForReadByEncoding(const ttstr &name,
                                                         const ttstr &modestr) {
    return new tTVPTextReadStream(name, modestr);
}

//---------------------------------------------------------------------------
iTJSTextWriteStream *TVPCreateTextStreamForWrite(const ttstr &name,
                                                 const ttstr &modestr) {
    return new tTVPTextWriteStream(name, modestr);
}

//---------------------------------------------------------------------------
void TVPSetDefaultReadEncoding(const ttstr &encoding) {
    DefaultReadEncoding = encoding;
    ttstr codestr = encoding;
    codestr.ToLowerCase();
    if(codestr == enc_gbk) {
        mbtowc_for_text_stream = gbk_mbtowc;
    } else if(codestr == enc_utf8 || codestr == enc_utf8_2) {
        mbtowc_for_text_stream = utf8_mbtowc;
    } else if(codestr == enc_jis || codestr == enc_jis_2 ||
              codestr == enc_jis_3 || codestr == enc_jis_4) {
        mbtowc_for_text_stream = sjis_mbtowc;
    } else {
        TVPThrowExceptionMessage(TVPUnsupportedEncoding, encoding);
    }
}

//---------------------------------------------------------------------------
const tjs_char *TVPGetDefaultReadEncoding() {
    return DefaultReadEncoding.c_str();
}
//---------------------------------------------------------------------------

#endif /*MY_USE_MINLIB*/
