/****************************************************************************
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

#include "platform/CCPlatformConfig.h"
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

#include "platform/android/CCApplication-android.h"
#include "platform/android/CCGLViewImpl-android.h"
#include "base/CCDirector.h"
#include "base/CCEventCustom.h"
#include "base/CCEventType.h"
#include "base/CCEventDispatcher.h"
#include "renderer/CCGLProgramCache.h"
#include "renderer/CCTextureCache.h"
#include "renderer/ccGLStateCache.h"
#include "2d/CCDrawingPrimitives.h"
#include "platform/android/jni/JniHelper.h"
#include "platform/CCDataManager.h"
#include "network/CCDownloader-android.h"
#include <unistd.h>
#include <android/log.h>
#include <android/api-level.h>
#include <jni.h>

#define  LOG_TAG    "main"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

//#if !MY_USE_MINLIB
//This function have to be weak, otherwise linked failed
//But need to be careful, decaration of funcion cocos_android_app_init must be very same, see main.cpp
//This function is changed in different versions of cocos2d-x
void cocos_android_app_init(JNIEnv* env) __attribute__((weak));
//#else
//(x, not need) For Stop making #00 pc 00000000  <unknown> runtime error
//(x, not need) void cocos_android_app_init(JNIEnv* env);
//#endif

void cocos_audioengine_focus_change(int focusChange);

using namespace cocos2d;

extern "C"
{

// ndk break compatibility, refer to https://github.com/cocos2d/cocos2d-x/issues/16267 for detail information
// should remove it when using NDK r13 since NDK r13 will add back bsd_signal()
#if __ANDROID_API__ > 19
#include <signal.h>
#include <dlfcn.h>
    typedef __sighandler_t (*bsd_signal_func_t)(int, __sighandler_t);
    bsd_signal_func_t bsd_signal_func = NULL;

    __sighandler_t bsd_signal(int s, __sighandler_t f) {
        if (bsd_signal_func == NULL) {
            // For now (up to Android 7.0) this is always available 
            bsd_signal_func = (bsd_signal_func_t) dlsym(RTLD_DEFAULT, "bsd_signal");

            if (bsd_signal_func == NULL) {
                __android_log_assert("", "bsd_signal_wrapper", "bsd_signal symbol not found!");
            }
        }
        return bsd_signal_func(s, f);
    }
#endif // __ANDROID_API__ > 19

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
LOGD("%s", "JNI_OnLoad 1");	
    JniHelper::setJavaVM(vm);
LOGD("%s", "JNI_OnLoad 2");
//#if !MY_USE_MINLIB	
    cocos_android_app_init(JniHelper::getEnv());
//#else
//	//skip
//#endif
LOGD("%s", "JNI_OnLoad 3");
    return JNI_VERSION_1_4;
}

JNIEXPORT void Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeInit(JNIEnv*  env, jobject thiz, jint w, jint h)
{
    DataManager::setProcessID(getpid());
    DataManager::setFrameSize(w, h);

    auto director = cocos2d::Director::getInstance();
    auto glview = director->getOpenGLView();
    if (!glview)
    {
        glview = cocos2d::GLViewImpl::create("Android app");
        glview->setFrameSize(w, h);
        director->setOpenGLView(glview);

        cocos2d::Application::getInstance()->run();
    }
    else
    {
        cocos2d::GL::invalidateStateCache();
        cocos2d::GLProgramCache::getInstance()->reloadDefaultGLPrograms();
        cocos2d::DrawPrimitives::init();
        cocos2d::VolatileTextureMgr::reloadAllTextures();

        cocos2d::EventCustom recreatedEvent(EVENT_RENDERER_RECREATED);
        director->getEventDispatcher()->dispatchEvent(&recreatedEvent);
        director->setGLDefaultValues();
    }
#if !MY_USE_MINLIB	
    cocos2d::network::_preloadJavaDownloaderClass();
#endif
}

JNIEXPORT jintArray Java_org_cocos2dx_lib_Cocos2dxActivity_getGLContextAttrs(JNIEnv*  env, jobject thiz)
{
//#if !MY_USE_MINLIB	
//	//skip
//#else
//    cocos_android_app_init(JniHelper::getEnv());
//#endif

    cocos2d::Application::getInstance()->initGLContextAttrs(); 
    GLContextAttrs _glContextAttrs = GLView::getGLContextAttrs();
    
    int tmp[7] = {_glContextAttrs.redBits, _glContextAttrs.greenBits, _glContextAttrs.blueBits,
                           _glContextAttrs.alphaBits, _glContextAttrs.depthBits, _glContextAttrs.stencilBits, _glContextAttrs.multisamplingCount};


    jintArray glContextAttrsJava = env->NewIntArray(7);
        env->SetIntArrayRegion(glContextAttrsJava, 0, 7, tmp);
    
    return glContextAttrsJava;
}

JNIEXPORT void Java_org_cocos2dx_lib_Cocos2dxAudioFocusManager_nativeOnAudioFocusChange(JNIEnv* env, jobject thiz, jint focusChange)
{
    cocos_audioengine_focus_change(focusChange);
}

JNIEXPORT void Java_org_cocos2dx_lib_Cocos2dxRenderer_nativeOnSurfaceChanged(JNIEnv*  env, jobject thiz, jint w, jint h)
{
    cocos2d::Application::getInstance()->applicationScreenSizeChanged(w, h);
}

}

#endif // CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID

