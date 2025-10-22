10-22 09:50:58.482: D/com.netease.nemu_vapi_android.thread.HttpServer(1130): http server response data: {"errcode":0,"message":"OK"}
10-22 09:50:58.483: V/libnb(2888): enter native_bridge2_getTrampoline Java_org_libsdl_app_SDLActivity_onNativeOrientationChanged
10-22 09:50:58.488: D/com.netease.nemu_vapi_android.thread.HttpServer(1130): http server response data: {"errcode":0,"message":"OK"}
10-22 09:50:58.488: V/libnb(2888): enter native_bridge2_isSupported /data/app/org.cocos2dx.hellocpp-1/lib/arm/libcpp_empty_test.so
10-22 09:50:58.488: D/libnb(2888): enter native_bridge2_loadLibrary /data/app/org.cocos2dx.hellocpp-1/lib/arm/libcpp_empty_test.so
10-22 09:50:58.597: D/houdini(2888): [2888] Added shared library /data/app/org.cocos2dx.hellocpp-1/lib/arm/libcpp_empty_test.so for ClassLoader by Native Bridge.
10-22 09:50:58.597: V/libnb(2888): enter native_bridge2_getTrampoline JNI_OnLoad
10-22 09:50:58.597: D/main(2888): JNI_OnLoad 1
10-22 09:50:58.597: D/main(2888): JNI_OnLoad 2
10-22 09:50:58.598: D/main(2888): cocos_android_app_init
10-22 09:50:58.598: D/main(2888): JNI_OnLoad 3
10-22 09:50:58.601: D/Cocos2dxHelper(2888): isSupportLowLatency:true
10-22 09:50:58.601: V/libnb(2888): enter native_bridge2_getTrampoline Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetContext
10-22 09:50:58.601: V/libnb(2888): enter native_bridge2_getTrampoline Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetContext__Landroid_content_Context_2Landroid_content_res_AssetManager_2
10-22 09:50:58.602: V/libnb(2888): enter native_bridge2_getTrampoline Java_org_cocos2dx_lib_Cocos2dxHelper_nativeSetContext
10-22 09:50:58.603: V/libnb(2888): enter native_bridge2_getTrampoline Java_org_cocos2dx_lib_Cocos2dxActivity_getGLContextAttrs
10-22 09:50:58.603: V/libnb(2888): enter native_bridge2_getTrampoline Java_org_cocos2dx_lib_Cocos2dxActivity_getGLContextAttrs__
10-22 09:50:58.603: V/libnb(2888): enter native_bridge2_getTrampoline Java_org_cocos2dx_lib_Cocos2dxActivity_getGLContextAttrs
10-22 09:50:58.610: V/libnb(2888): enter native_bridge2_getTrampoline Java_org_tvp_kirikiri2_KR2Activity_nativeGetHideSystemButton
10-22 09:50:58.610: V/libnb(2888): enter native_bridge2_getTrampoline Java_org_tvp_kirikiri2_KR2Activity_nativeGetHideSystemButton__
10-22 09:50:58.610: V/libnb(2888): enter native_bridge2_getTrampoline Java_org_tvp_kirikiri2_KR2Activity_nativeGetHideSystemButton
10-22 09:50:58.610: V/libnb(2888): enter native_bridge2_getTrampoline Java_org_tvp_kirikiri2_KR2Activity_nativeGetHideSystemButton__
10-22 09:50:58.610: E/art(2888): No implementation found for boolean org.tvp.kirikiri2.KR2Activity.nativeGetHideSystemButton() (tried Java_org_tvp_kirikiri2_KR2Activity_nativeGetHideSystemButton and Java_org_tvp_kirikiri2_KR2Activity_nativeGetHideSystemButton__)
10-22 09:50:58.610: D/AndroidRuntime(2888): Shutting down VM
10-22 09:50:58.610: E/AndroidRuntime(2888): FATAL EXCEPTION: SDLActivity
10-22 09:50:58.610: E/AndroidRuntime(2888): Process: org.cocos2dx.hellocpp, PID: 2888
10-22 09:50:58.610: E/AndroidRuntime(2888): java.lang.UnsatisfiedLinkError: No implementation found for boolean org.tvp.kirikiri2.KR2Activity.nativeGetHideSystemButton() (tried Java_org_tvp_kirikiri2_KR2Activity_nativeGetHideSystemButton and Java_org_tvp_kirikiri2_KR2Activity_nativeGetHideSystemButton__)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at org.tvp.kirikiri2.KR2Activity.nativeGetHideSystemButton(Native Method)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at org.tvp.kirikiri2.KR2Activity.hideSystemUI(KR2Activity.java:365)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at org.tvp.kirikiri2.KR2Activity.onCreateView(KR2Activity.java:104)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at org.cocos2dx.lib.Cocos2dxActivity.init(Cocos2dxActivity.java:283)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at org.cocos2dx.lib.Cocos2dxActivity.onCreate(Cocos2dxActivity.java:151)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at org.tvp.kirikiri2.KR2Activity.onCreate(KR2Activity.java:96)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at android.app.Activity.performCreate(Activity.java:6394)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at android.app.Instrumentation.callActivityOnCreate(Instrumentation.java:1111)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at android.app.ActivityThread.performLaunchActivity(ActivityThread.java:2499)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at android.app.ActivityThread.handleLaunchActivity(ActivityThread.java:2606)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at android.app.ActivityThread.-wrap11(ActivityThread.java)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at android.app.ActivityThread$H.handleMessage(ActivityThread.java:1444)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at android.os.Handler.dispatchMessage(Handler.java:102)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at android.os.Looper.loop(Looper.java:148)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at android.app.ActivityThread.main(ActivityThread.java:5654)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at java.lang.reflect.Method.invoke(Native Method)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at com.android.internal.os.ZygoteInit$MethodAndArgsCaller.run(ZygoteInit.java:782)
10-22 09:50:58.610: E/AndroidRuntime(2888): 	at com.android.internal.os.ZygoteInit.main(ZygoteInit.java:672)
10-22 09:50:58.612: W/ActivityManager(733):   Force finishing activity org.cocos2dx.hellocpp/org.tvp.kirikiri2.KR2Activity
10-22 09:50:58.614: D/ActivityManager(733): Nemu don't show crash dialog of ProcessRecord{2dc0e26 2888:org.cocos2dx.hellocpp/u0a107}
