check down to up src\core\tjs2\bison\tjs.y
need to sync .y and .cpp
C:\work\krkr2-no-vcpkg-fork2\src\core\environ\linux\Platform.cpp
C:\work\krkr2-no-vcpkg-fork2\src\core\environ\android\AndroidUtils.cpp


----
old:
void tTVPBasicDrawDevice::Show() {
    if(Window) {
        iWindowLayer *form = Window->GetForm();
        if(form && !Managers.empty()) {
            iTVPBaseBitmap *buf = Managers.back()->GetDrawBuffer();
            if(buf)
---->                ;
            form->UpdateDrawBuffer(buf->GetTexture());
        }
    }
---