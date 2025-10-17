# krkr2-no-vcpkg-fork2
[WIP and not recommended] The second fork of building krkr2 kirikiroid2 vcpkg version without vcpkg, with apt install instead,
Code base:  
https://github.com/2468785842/krkr2/tree/c08acbc134bbc26d62fc11b5c355efaed64d466b  
**Different from 2468785842/krkr2, currently no windowEx.dll support, you can refer to 2468785842/krkr2 for windowEx.dll support**

## Bugs
* Android: Exit/Save will cause loop and no response, krkr2-no-vcpkg_v19_android_exit_save_loop.7z  
* (Not sure) File manager UI scrolls not well
* No windowEx.dll support (No System.getMonitorInfo implementation), just able to run a very simple demo _testdata/data.xp3  

## TODO
* Port to msys2, cocos2d-x-2.2.6_msys_v1.7z, cocos2d-x-2.2.6_mingw.7z
* Linux build not tested
* Some code is changed to mine (see readme_android.txt, like core/environ/android/AndroidUtils.cpp and core/sound/win32/WaveMixer.cpp), not original code, need to be synchronized  
* https://github.com/weimingtom/krkr2-no-vcpkg/blob/master/src/core/environ/android/AndroidUtils.cpp  
* https://github.com/weimingtom/krkr2-no-vcpkg/blob/master/src/core/sound/win32/WaveMixer.cpp  
* Modify https://github.com/weimingtom/krkr2-no-vcpkg/blob/master/jni/main.cpp, include "../src/core/environ/cocos2d/AppDelegate.h"  
* Remove console.bat NDK_MODULE_PATH

## Recommended Development Environment for good audio music output 
* Xubuntu 20.04 desktop amd64, in VMWare or VirtualBox  

## For Xubuntu 20.04 and Xubuntu 25.04, desktop amd64, in VMWare (20.04 also support VirtualBox)  
* $ sudo apt update
* $ sudo apt install lftp gedit pkg-config make gcc g++ cmake
* $ sudo apt install libglew-dev libfreetype-dev libglfw3-dev libsdl2-dev libvorbis-dev libwebp-dev  libboost-locale-dev libfmt-dev libopencv-dev liblz4-dev libspdlog-dev libopenal-dev libgtk2.0-dev libarchive-dev libopusfile-dev libminizip-dev libjpeg-dev libuchardet-dev libogg-dev libopus-dev
* $ make clean && make -j8 && make test
* $ mkdir build && cd build && cmake .. && make -j8 && ./bin/krkr2/krkr2 && cd ..

