

#ifndef BitmapIntfH
#define BitmapIntfH

#include "tjsNative.h"
class tTVPBitmap;
class tTVPBaseBitmap;
class tTJSNI_Bitmap : public tTJSNativeInstance {
    typedef tTJSNativeInstance inherited;

protected:
    iTJSDispatch2 *Owner;
    tTVPBaseBitmap *Bitmap;
    bool Loading;

public:
    tTJSNI_Bitmap();
    ~tTJSNI_Bitmap() override;
    tjs_error Construct(tjs_int numparams, tTJSVariant **param,
                        iTJSDispatch2 *tjs_obj) override;
    void Invalidate() override;

public:
    tTVPBaseBitmap *GetBitmap() { return Bitmap; }
    [[nodiscard]] const tTVPBaseBitmap *GetBitmap() const { return Bitmap; }

    [[nodiscard]] tjs_uint32 GetPixel(tjs_int x, tjs_int y) const;
    void SetPixel(tjs_int x, tjs_int y, tjs_uint32 color);

    [[nodiscard]] tjs_int GetMaskPixel(tjs_int x, tjs_int y) const;
    void SetMaskPixel(tjs_int x, tjs_int y, tjs_int mask);

    void Independ(bool copy = true);

    iTJSDispatch2 *Load(const ttstr &name, tjs_uint32 colorkey);
    void LoadAsync(const ttstr &name);
    void Save(const ttstr &name, const ttstr &type,
              iTJSDispatch2 *meta = nullptr);

    void SetSize(tjs_uint width, tjs_uint height, bool keepimage = true);
    // for async load
    // @param bits :
    // tTVPBitmapBitsAlloc::Alloc‚ÅŠm•Û‚µ‚½‚à‚Ì‚ðŽg—p‚·‚é‚±‚Æ
    void SetSizeAndImageBuffer(tTVPBitmap *bmp);

    void SetWidth(tjs_uint width);
    [[nodiscard]] tjs_uint GetWidth() const;
    void SetHeight(tjs_uint height);
    [[nodiscard]] tjs_uint GetHeight() const;

    [[nodiscard]] const void *GetPixelBuffer() const;
    void *GetPixelBufferForWrite();
    [[nodiscard]] tjs_int GetPixelBufferPitch() const;

    // copy on wirte
    void CopyFrom(const tTJSNI_Bitmap *src);

    [[nodiscard]] bool IsLoading() const { return Loading; }

    // for internal
    void CopyFrom(const class iTVPBaseBitmap *src);
    virtual void SetLoading(bool load) { Loading = load; }
};

class tTJSNC_Bitmap : public tTJSNativeClass {
    typedef tTJSNativeClass inherited;

public:
    tTJSNC_Bitmap();
    static tjs_uint32 ClassID;

protected:
    tTJSNativeInstance *CreateNativeInstance() override;
};

extern tTJSNativeClass *TVPCreateNativeClass_Bitmap();
extern iTJSDispatch2 *TVPCreateBitmapObject();
#endif // BitmapIntfH
