//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Timer Object Implementation
//---------------------------------------------------------------------------
#ifndef TimerImplH
#define TimerImplH

#include "TimerIntf.h"

//---------------------------------------------------------------------------
// tTJSNI_Timer : Timer Native Instance
//---------------------------------------------------------------------------
class tTVPTimerThread;

class tTJSNI_Timer : public tTJSNI_BaseTimer {
    typedef tTJSNI_BaseTimer inherited;

    friend class tTVPTimerThread;

    tjs_uint64 Interval;
    tjs_uint64 NextTick;
    tjs_int PendingCount;
    bool Enabled;

public:
    tTJSNI_Timer();

    tjs_error Construct(tjs_int numparams, tTJSVariant **param,
                        iTJSDispatch2 *tjs_obj) override;

    void Invalidate() override;

    void InternalSetInterval(tjs_uint64 n) { Interval = n; }

    void SetInterval(tjs_uint64 n);

    [[nodiscard]] tjs_uint64 GetInterval() const;
    // { return Interval; }

    void ZeroPendingCount() { PendingCount = 0; }

    void SetNextTick(tjs_uint64 n) { NextTick = n; }

    [[nodiscard]] tjs_uint64 GetNextTick() const;
    // { return NextTick; }
    // bcc 5.5.1 has an inliner bug of bad returning of int64...

    void InternalSetEnabled(bool b) { Enabled = b; }

    void SetEnabled(bool b);

    [[nodiscard]] bool GetEnabled() const { return Enabled; }

    void Trigger(tjs_uint n);

    void FirePendingEventsAndClear();

private:
    void CancelTrigger();
};
//---------------------------------------------------------------------------
#endif
