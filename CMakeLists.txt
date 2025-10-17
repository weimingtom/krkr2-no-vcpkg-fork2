#/****************************************************************************
# Copyright (c) 2013 cocos2d-x.org
# Copyright (c) 2014 martell malone
# Copyright (c) 2015-2017 Chukong Technologies Inc.
#
# http://www.cocos2d-x.org
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# ****************************************************************************/

# build engine library and all tests

cmake_minimum_required(VERSION 3.6)

#project(Cocos2d-x)
################
#set(APP_NAME cpp-empty-test)
set(APP_NAME krkr2)
#set(APP_NAME kirikiroid2)

project(${APP_NAME})

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
################


# cocos2dx root path
set(COCOS2DX_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${COCOS2DX_ROOT_PATH}/cmake/Modules/)

# prevent in-source-build
include(PreventInSourceBuilds)
AssureOutOfSourceBuilds()

# works before build libcocos2d
include(CocosBuildSet)

# build options
option(BUILD_TESTS "Build tests" ON)

# default tests include lua, js test project, so we set those option on to build libs
set(BUILD_LUA_LIBS ON)
set(BUILD_JS_LIBS ON)

add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos ${ENGINE_BINARY_PATH}/cocos/core)

# prevent tests project to build "cocos2d-x/cocos" again
set(BUILD_ENGINE_DONE ON)

####################

set(KRKR2CORE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/core)
set(KRKR2PLUGIN_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins)

#if(MSVC)
#    add_link_options("/ignore:4099")
#    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#endif()

if(NOT DEFINED BUILD_ENGINE_DONE)
    set(COCOS2DX_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../..)
    set(CMAKE_MODULE_PATH ${COCOS2DX_ROOT_PATH}/cmake/Modules/)

    include(CocosBuildSet)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos ${ENGINE_BINARY_PATH}/cocos/core)
endif()

# record sources, headers, resources...
set(GAME_SOURCE)
set(GAME_HEADER)

set(GAME_RES_FOLDER
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
    )
if(APPLE OR VS)
    cocos_mark_multi_resources(cc_common_res RES_TO "Resources" FOLDERS ${GAME_RES_FOLDER})
endif()

list(APPEND GAME_HEADER
     Classes/AppMacros.h
     Classes/HelloWorldScene.h
     Classes/AppDelegate.h
     )
list(APPEND GAME_SOURCE
#     Classes/AppDelegate.cpp
     Classes/HelloWorldScene.cpp
     )

#if(ANDROID)
#    add_library(${PROJECT_NAME} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/android/cpp/krkr2_android.cpp)
#elseif(LINUX)
#    add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/linux/main.cpp)
#elseif(WINDOWS)
#    add_executable(${PROJECT_NAME} 
#        ${CMAKE_CURRENT_SOURCE_DIR}/windows/main.cpp
#        ${CMAKE_CURRENT_SOURCE_DIR}/windows/game.rc
#    )
#endif()
if(ANDROID)
    # change APP_NAME to the share library name for Android, it's value depend on AndroidManifest.xml
    set(APP_NAME cpp_empty_test)
    list(APPEND GAME_SOURCE
         platforms/android/app/jni/main.cpp
         )
elseif(LINUX)
    list(APPEND GAME_SOURCE
         platforms/linux/main.cpp
         )
elseif(WINDOWS)
    list(APPEND GAME_HEADER
         platforms/windows/main.h
         )
    list(APPEND GAME_SOURCE
         platforms/windows/main.cpp
         ${cc_common_res}
         )
elseif(APPLE)
    if(IOS)
        list(APPEND GAME_HEADER
             proj.ios/AppController.h
             proj.ios/RootViewController.h
             )
        set(APP_UI_RES
            proj.ios/LaunchScreen.storyboard
            proj.ios/LaunchScreenBackground.png
            proj.ios/Images.xcassets
            )
        list(APPEND GAME_SOURCE
             proj.ios/main.m
             proj.ios/AppController.mm
             proj.ios/RootViewController.mm
             ${APP_UI_RES}
             )
    elseif(MACOSX)
        set(APP_UI_RES
            platforms/apple/macos/Icon.icns
            platforms/apple/macos/Info.plist
            platforms/apple/macos/en.lproj/MainMenu.xib
            platforms/apple/macos/en.lproj/InfoPlist.strings
            )
        list(APPEND GAME_SOURCE
             platforms/apple/macos/main.cpp
             ${APP_UI_RES}
             )
    endif()
    list(APPEND GAME_SOURCE ${cc_common_res})
endif()

set(all_code_files
    ${GAME_HEADER}
    ${GAME_SOURCE}
    )

# mark app complie info
if(NOT ANDROID)
    add_executable(${APP_NAME} ${all_code_files})
else()
    add_library(${APP_NAME} SHARED ${all_code_files})
    add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos/platform/android ${ENGINE_BINARY_PATH}/cocos/platform)
    target_link_libraries(${APP_NAME} -Wl,--whole-archive cpp_android_spec -Wl,--no-whole-archive)
endif()


target_include_directories(${PROJECT_NAME} PUBLIC ${KRKR2CORE_PATH}/environ/cocos2d)
# build library
add_subdirectory(${KRKR2CORE_PATH})
add_subdirectory(${KRKR2PLUGIN_PATH})
target_link_libraries(${PROJECT_NAME} PUBLIC
    krkr2plugin krkr2core
#FIXME:added
    cocos2d GLU GL jpeg
#FIXME:added
    fmt
#    avcodec avformat avfilter swscale swresample
#    avutil
#pkg-config --libs opencv4
    opencv_imgproc opencv_core
#    turbojpeg
    lz4
    spdlog
    openal
#pkg-config --libs gtk+-2.0
    gtk-x11-2.0 gdk-x11-2.0 pangocairo-1.0 atk-1.0 cairo gdk_pixbuf-2.0 gio-2.0 pangoft2-1.0 pango-1.0 gobject-2.0 glib-2.0 harfbuzz fontconfig freetype
    archive
    opusfile
    webp
    vorbisfile vorbis ogg
    SDL2
    uchardet
)
#if(NOT ANDROID)
#    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CocosBuildHelpers.cmake")
#
#    set(GAME_RES_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/ui/cocos-studio")
#
#    if(WINDOWS)
#        cocos_mark_multi_resources(common_res_files RES_TO "Resources" FOLDERS ${GAME_RES_FOLDER})
#    endif()
#endif()
#################target_link_libraries(${APP_NAME} cocos2d)

target_include_directories(${APP_NAME} PRIVATE Classes)

#FIXME:added
#target_include_directories(${APP_NAME} PRIVATE /usr/include/GLFW)
#FIXME:removed, see upper target_link_libraries
#target_link_libraries(${APP_NAME} GLU)
#target_link_libraries(${APP_NAME} GL)
#target_link_libraries(${APP_NAME} jpeg)

#if(NOT ANDROID)
#    # mark app resources
#    setup_cocos_app_config(${APP_NAME})
#endif()    
# mark app resources
setup_cocos_app_config(${APP_NAME})



if(MSVC)
  target_compile_options(${PROJECT_NAME} PUBLIC "/ZI")
  target_link_options(${PROJECT_NAME} PUBLIC "/INCREMENTAL")
endif()

#if(NOT ANDROID)
#    if(WINDOWS)
#        target_sources(${PROJECT_NAME} PRIVATE ${common_res_files})
#        cocos_copy_target_dll(${APP_NAME})
#    endif()
#endif()
if(APPLE)
    set_target_properties(${APP_NAME} PROPERTIES RESOURCE "${APP_UI_RES}")
    if(MACOSX)
        set_target_properties(${APP_NAME} PROPERTIES
                              MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/proj.mac/Info.plist"
                              )
    elseif(IOS)
        cocos_pak_xcode(${APP_NAME} INFO_PLIST "iOSBundleInfo.plist.in")
        set_xcode_property(${APP_NAME} ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon-${APP_NAME}")
        set_xcode_property(${APP_NAME} CODE_SIGN_IDENTITY "iPhone Developer")
        set_xcode_property(${APP_NAME} DEVELOPMENT_TEAM "")
        set_xcode_property(${APP_NAME} CODE_SIGN_IDENTITY "iPhone Developer")
    endif()
elseif(WINDOWS)
    cocos_copy_target_dll(${APP_NAME})
endif()

#if(NOT ANDROID)
#    if(LINUX OR WINDOWS)
#        cocos_get_resource_path(APP_RES_DIR ${APP_NAME})
#        cocos_copy_target_res(${APP_NAME} LINK_TO ${APP_RES_DIR} FOLDERS ${GAME_RES_FOLDER})
#    endif()
#endif()
if(LINUX OR WINDOWS)
    set(APP_RES_DIR "$<TARGET_FILE_DIR:${APP_NAME}>/Resources")
    cocos_copy_target_res(${APP_NAME} COPY_TO ${APP_RES_DIR} FOLDERS ${GAME_RES_FOLDER})
endif()





