//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "Window" TJS Class implementation
//---------------------------------------------------------------------------

#ifndef WindowImplH
#define WindowImplH

// #include <myWindows/StdAfx.h>
#include "WindowIntf.h"
#include "TVPWindow.h"

#define WM_USER 0x0400

/*[*/
//---------------------------------------------------------------------------
// window message receivers
//---------------------------------------------------------------------------
// struct tTVPWindowMessage
//{
//	unsigned int Msg; // window message
//	WPARAM WParam;  // WPARAM
//	LPARAM LParam;  // LPARAM
//	LRESULT Result;  // result
//};
// typedef bool * tTVPWindowMessageReceiver
//	(void *userdata, tTVPWindowMessage *Message);
//
// #define TVP_WM_DETACH (WM_USER+106)  // before re-generating the
// window #define TVP_WM_ATTACH (WM_USER+107)  // after re-generating
// the window #define TVP_WM_FULLSCREEN_CHANGING (WM_USER+108)  //
// before full-screen or
// window changing #define TVP_WM_FULLSCREEN_CHANGED  (WM_USER+109) //
// after full-screen or window changing

/*]*/
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
/*
struct tTVP_devicemodeA {
        // copy of DEVMODE, to avoid windows platform SDK version
mismatch #pragma pack(push, 1) BYTE   dmDeviceName[CCHDEVICENAME];
        WORD dmSpecVersion;
        WORD dmDriverVersion;
        WORD dmSize;
        WORD dmDriverExtra;
        DWORD dmFields;
        union {
          struct {
                short dmOrientation;
                short dmPaperSize;
                short dmPaperLength;
                short dmPaperWidth;
          };
          POINTL dmPosition;
        };
        short dmScale;
        short dmCopies;
        short dmDefaultSource;
        short dmPrintQuality;
        short dmColor;
        short dmDuplex;
        short dmYResolution;
        short dmTTOption;
        short dmCollate;
        BYTE   dmFormName[CCHFORMNAME];
        WORD   dmLogPixels;
        DWORD  dmBitsPerPel;
        DWORD  dmPelsWidth;
        DWORD  dmPelsHeight;
        union {
                DWORD  dmDisplayFlags;
                DWORD  dmNup;
        };
        DWORD  dmDisplayFrequency;
        DWORD  dmICMMethod;
        DWORD  dmICMIntent;
        DWORD  dmMediaType;
        DWORD  dmDitherType;
        DWORD  dmReserved1;
        DWORD  dmReserved2;
#pragma pack(pop)
};
*/
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Mouse Cursor Management
//---------------------------------------------------------------------------
extern tjs_int TVPGetCursor(const ttstr &name);
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Utility functions
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(tjs_uint32, TVPGetCurrentShiftKeyState, ());

// implement for Application
#if 0
TJS_EXP_FUNC_DEF(void, TVPRegisterAcceleratorKey, (HWND hWnd, char virt, short key, short cmd) );
TJS_EXP_FUNC_DEF(void, TVPUnregisterAcceleratorKey, (HWND hWnd, short cmd));
TJS_EXP_FUNC_DEF(void, TVPDeleteAcceleratorKeyTable, (HWND hWnd));
HWND TVPGetModalWindowOwnerHandle();
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// Color Format Detection
//---------------------------------------------------------------------------
extern tjs_int TVPDisplayColorFormat;
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// Screen Mode management
//---------------------------------------------------------------------------

//! @brief		Structure for monitor screen mode
struct tTVPScreenMode
{
	tjs_int Width; //!< width of screen in pixel
	tjs_int Height; //!< height of screen in pixel
	tjs_int BitsPerPixel; //!< bits per pixel (0 = unspecified)

	ttstr Dump() const
	{
		return
			DumpHeightAndWidth() +
			TJS_W(", BitsPerPixel=") + (BitsPerPixel?ttstr(BitsPerPixel):ttstr(TJS_W("unspecified")));
	}

	ttstr DumpHeightAndWidth() const
	{
		return
			TJS_W("Width=") + ttstr(Width) +
			TJS_W(", Height=") + ttstr(Height);
	}

	bool operator < (const tTVPScreenMode & rhs) const {
		tjs_int area_this = Width * Height;
		tjs_int area_rhs  = rhs.Width * rhs.Height;
		if(area_this < area_rhs) return true;
		if(area_this > area_rhs) return false;
		if(BitsPerPixel < rhs.BitsPerPixel) return true;
		if(BitsPerPixel > rhs.BitsPerPixel) return false;
		return false;
	}
	bool operator == (const tTVPScreenMode & rhs) const {
		return ( Width == rhs.Width && Height == rhs.Height && BitsPerPixel == rhs.BitsPerPixel );
	}
};

//! @brief		Structure for monitor screen mode candidate
struct tTVPScreenModeCandidate : tTVPScreenMode
{
	tjs_int ZoomNumer; //!< zoom ratio numer
	tjs_int ZoomDenom; //!< zoom ratio denom
	tjs_int RankZoomIn;
	tjs_int RankBPP;
	tjs_int RankZoomBeauty;
	tjs_int RankSize; //!< candidate preference priority (lower value is higher preference)

	ttstr Dump() const
	{
		return tTVPScreenMode::Dump() +
			TJS_W(", ZoomNumer=") + ttstr(ZoomNumer) +
			TJS_W(", ZoomDenom=") + ttstr(ZoomDenom) +
			TJS_W(", RankZoomIn=") + ttstr(RankZoomIn) +
			TJS_W(", RankBPP=") + ttstr(RankBPP) +
			TJS_W(", RankZoomBeauty=") + ttstr(RankZoomBeauty) +
			TJS_W(", RankSize=") + ttstr(RankSize);
	}

	bool operator < (const tTVPScreenModeCandidate & rhs) const{
		if(RankZoomIn < rhs.RankZoomIn) return true;
		if(RankZoomIn > rhs.RankZoomIn) return false;
		if(RankBPP < rhs.RankBPP) return true;
		if(RankBPP > rhs.RankBPP) return false;
		if(RankZoomBeauty < rhs.RankZoomBeauty) return true;
		if(RankZoomBeauty > rhs.RankZoomBeauty) return false;
		if(RankSize < rhs.RankSize) return true;
		if(RankSize > rhs.RankSize) return false;
		return false;
	}
};
struct IDirect3D9;
extern void TVPTestDisplayMode(tjs_int w, tjs_int h, tjs_int & bpp);
extern void TVPSwitchToFullScreen(HWND window, tjs_int w, tjs_int h, class iTVPDrawDevice* drawdevice);
extern void TVPRecalcFullScreen( tjs_int w, tjs_int h );
extern void TVPRevertFromFullScreen(HWND window,tjs_uint w,tjs_uint h, class iTVPDrawDevice* drawdevice);
TJS_EXP_FUNC_DEF(void, TVPEnsureDirect3DObject, ());
void TVPDumpDirect3DDriverInformation();
extern tTVPScreenModeCandidate TVPFullScreenMode;
/*[*/
//---------------------------------------------------------------------------
// Direct3D former declaration
//---------------------------------------------------------------------------
#ifndef DIRECT3D_VERSION
struct IDirect3D9;
#endif

/*]*/
TJS_EXP_FUNC_DEF(IDirect3D9 *,  TVPGetDirect3DObjectNoAddRef, ());
extern void TVPMinimizeFullScreenWindowAtInactivation();
extern void TVPRestoreFullScreenWindowAtActivation();
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNI_Window : Window Native Instance
//---------------------------------------------------------------------------
typedef iWindowLayer TTVPWindowForm;
class iTVPDrawDevice;
class tTJSNI_BaseLayer;
class tTJSNI_Window : public tTJSNI_BaseWindow {
    TTVPWindowForm *Form;
    //	class tTVPVSyncTimingThread *VSyncTimingThread;

public:
    tTJSNI_Window();
    tjs_error Construct(tjs_int numparams, tTJSVariant **param,
                        iTJSDispatch2 *tjs_obj) override;
    void Invalidate() override;
    bool CloseFlag;

public:
    [[nodiscard]] bool CanDeliverEvents()
        const override; // tTJSNI_BaseWindow::CanDeliverEvents override

public:
    [[nodiscard]] TTVPWindowForm *GetForm() const override { return Form; }
    void NotifyWindowClose();
    void SendCloseMessage();
    void TickBeat();

private:
    bool GetWindowActive() override;
    void UpdateVSyncThread() override;
    /**
     * フルスクリーン時に操作できない値を変えようとした時に確認のため呼び出し、フルスクリーンの時例外を出す
     */
    void FullScreenGuard() const;

public:
    //-- draw device
    void ResetDrawDevice() override;

    //-- event control
    virtual void PostInputEvent(const ttstr &name, iTJSDispatch2 *params);

    //-- interface to layer manager
    void NotifySrcResize() override; // is called from primary layer

    void SetDefaultMouseCursor() override; // set window mouse cursor to default
    void SetMouseCursor(tjs_int handle) override; // set window mouse cursor
    void GetCursorPos(tjs_int &x, tjs_int &y) override;
    void SetCursorPos(tjs_int x, tjs_int y) override;
    void WindowReleaseCapture() override;
    void SetHintText(iTJSDispatch2 *sender, const ttstr &text) override;
    void SetAttentionPoint(tTJSNI_BaseLayer *layer, tjs_int l,
                           tjs_int t) override;
    void DisableAttentionPoint() override;
    void SetImeMode(tTVPImeMode mode) override;
    void SetDefaultImeMode(tTVPImeMode mode);
    [[nodiscard]] tTVPImeMode GetDefaultImeMode() const override;
    void ResetImeMode() override;

    //-- update managment
    void BeginUpdate(const tTVPComplexRect &rects) override;
    void EndUpdate() override;

    //-- interface to VideoOverlay object
public:
#if 0
	HWND GetSurfaceWindowHandle();
	HWND GetWindowHandle();
#endif
    void GetVideoOffset(tjs_int &ofsx, tjs_int &ofsy);

    void ReadjustVideoRect();
    void WindowMoved();
    void DetachVideoOverlay();

    //-- interface to plugin
    void ZoomRectangle(tjs_int &left, tjs_int &top, tjs_int &right,
                       tjs_int &bottom);
#if 0
	HWND GetWindowHandleForPlugin();
#endif
    void RegisterWindowMessageReceiver(tTVPWMRRegMode mode, void *proc,
                                       const void *userdata);

    //-- methods
    void Close();
    void OnCloseQueryCalled(bool b);

#ifdef USE_OBSOLETE_FUNCTIONS
    void BeginMove();
#endif

    void BringToFront();
    void Update(tTVPUpdateType);

    void ShowModal();

    void HideMouseCursor();

    //-- properties
    [[nodiscard]] bool GetVisible() const;
    void SetVisible(bool s);

    void GetCaption(ttstr &v) const;
    void SetCaption(const ttstr &v);

    void SetWidth(tjs_int w);
    [[nodiscard]] tjs_int GetWidth() const;
    void SetHeight(tjs_int h);
    [[nodiscard]] tjs_int GetHeight() const;
    void SetSize(tjs_int w, tjs_int h);

    void SetMinWidth(int v);
    [[nodiscard]] int GetMinWidth() const;
    void SetMinHeight(int v);
    [[nodiscard]] int GetMinHeight() const;
    void SetMinSize(int w, int h);

    void SetMaxWidth(int v);
    [[nodiscard]] int GetMaxWidth() const;
    void SetMaxHeight(int v);
    [[nodiscard]] int GetMaxHeight() const;
    void SetMaxSize(int w, int h);

    void SetLeft(tjs_int l);
    [[nodiscard]] tjs_int GetLeft() const;
    void SetTop(tjs_int t);
    [[nodiscard]] tjs_int GetTop() const;
    void SetPosition(tjs_int l, tjs_int t);

#ifdef USE_OBSOLETE_FUNCTIONS
    void SetLayerLeft(tjs_int l);
    [[nodiscard]] tjs_int GetLayerLeft() const;
    void SetLayerTop(tjs_int t);
    [[nodiscard]] tjs_int GetLayerTop() const;
    void SetLayerPosition(tjs_int l, tjs_int t);

    void SetInnerSunken(bool b);
    [[nodiscard]] bool GetInnerSunken() const;
#endif

    void SetInnerWidth(tjs_int w);
    [[nodiscard]] tjs_int GetInnerWidth() const;
    void SetInnerHeight(tjs_int h);
    [[nodiscard]] tjs_int GetInnerHeight() const;

    void SetInnerSize(tjs_int w, tjs_int h);

    void SetBorderStyle(tTVPBorderStyle st);
    [[nodiscard]] tTVPBorderStyle GetBorderStyle() const;

    void SetStayOnTop(bool b);
    [[nodiscard]] bool GetStayOnTop() const;

#ifdef USE_OBSOLETE_FUNCTIONS
    void SetShowScrollBars(bool b);
    [[nodiscard]] bool GetShowScrollBars() const;
#endif

    void SetFullScreen(bool b);
    [[nodiscard]] bool GetFullScreen() const;

    void SetUseMouseKey(bool b);
    [[nodiscard]] bool GetUseMouseKey() const;

    void SetTrapKey(bool b);
    [[nodiscard]] bool GetTrapKey() const;

    void SetMaskRegion(tjs_int threshold);
    void RemoveMaskRegion();

    void SetMouseCursorState(tTVPMouseCursorState mcs);
    [[nodiscard]] tTVPMouseCursorState GetMouseCursorState() const;

    void SetFocusable(bool b);
    bool GetFocusable();

    void SetZoom(tjs_int numer, tjs_int denom);
    void SetZoomNumer(tjs_int n);
    [[nodiscard]] tjs_int GetZoomNumer() const;
    void SetZoomDenom(tjs_int n);
    [[nodiscard]] tjs_int GetZoomDenom() const;

    void SetTouchScaleThreshold(tjs_real threshold);
    [[nodiscard]] tjs_real GetTouchScaleThreshold() const;
    void SetTouchRotateThreshold(tjs_real threshold);
    [[nodiscard]] tjs_real GetTouchRotateThreshold() const;

    tjs_real GetTouchPointStartX(tjs_int index);
    tjs_real GetTouchPointStartY(tjs_int index);
    tjs_real GetTouchPointX(tjs_int index);
    tjs_real GetTouchPointY(tjs_int index);
    tjs_real GetTouchPointID(tjs_int index);
    tjs_int GetTouchPointCount();
    bool GetTouchVelocity(tjs_int id, float &x, float &y, float &speed) const;
    bool GetMouseVelocity(float &x, float &y, float &speed) const;
    void ResetMouseVelocity();

    void SetHintDelay(tjs_int delay);
    [[nodiscard]] tjs_int GetHintDelay() const;

    void SetEnableTouch(bool b);
    [[nodiscard]] bool GetEnableTouch() const;

    int GetDisplayOrientation();
    int GetDisplayRotate();

    bool WaitForVBlank(tjs_int *in_vblank, tjs_int *delayed);

    void OnTouchUp(tjs_real x, tjs_real y, tjs_real cx, tjs_real cy,
                   tjs_uint32 id) override;

public: // for iTVPLayerTreeOwner
        // LayerManager -> LTO
    /*
    implements on tTJSNI_BaseWindow
    virtual void RegisterLayerManager( iTVPLayerManager*
    manager ); virtual void UnregisterLayerManager( class
    iTVPLayerManager* manager );
    */

    void StartBitmapCompletion(iTVPLayerManager *manager) override;
    void NotifyBitmapCompleted(iTVPLayerManager *manager, tjs_int x, tjs_int y,
                               tTVPBaseTexture *bmp, const tTVPRect &cliprect,
                               tTVPLayerType type, tjs_int opacity) override;
    void EndBitmapCompletion(iTVPLayerManager *manager) override;

    void SetMouseCursor(iTVPLayerManager *manager, tjs_int cursor) override;
    void GetCursorPos(iTVPLayerManager *manager, tjs_int &x,
                      tjs_int &y) override;
    void SetCursorPos(iTVPLayerManager *manager, tjs_int x, tjs_int y) override;
    void ReleaseMouseCapture(iTVPLayerManager *manager) override;

    void SetHint(iTVPLayerManager *manager, iTJSDispatch2 *sender,
                 const ttstr &hint) override;

    void NotifyLayerResize(iTVPLayerManager *manager) override;
    void NotifyLayerImageChange(iTVPLayerManager *manager) override;

    void SetAttentionPoint(iTVPLayerManager *manager, tTJSNI_BaseLayer *layer,
                           tjs_int x, tjs_int y) override;
    void DisableAttentionPoint(iTVPLayerManager *manager) override;

    void SetImeMode(iTVPLayerManager *manager,
                    tjs_int mode) override; // mode == tTVPImeMode
    void ResetImeMode(iTVPLayerManager *manager) override;

protected:
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#endif
