#cmake_minimum_required(VERSION 3.28)
cmake_minimum_required(VERSION 3.6)
project(krkr2core)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

#add_library(${PROJECT_NAME} INTERFACE)

add_subdirectory(tjs2)
add_subdirectory(base)
add_subdirectory(environ)
add_subdirectory(extension)
add_subdirectory(plugin)
add_subdirectory(movie)
add_subdirectory(sound)
add_subdirectory(visual)
add_subdirectory(utils)

# Create the static library
add_library(${PROJECT_NAME} STATIC
    ${BASE_SOURCE_FILES}
    ${UTILS_SOURCE_FILES}
    ${ENVIRON_SOURCE_FILES}
    ${EXTENSION_SOURCE_FILES}
    ${MOVIE_SOURCE_FILES}
#    ${MSG_SOURCE_FILES}
    ${SOUND_SOURCE_FILES}
    ${VISUAL_SOURCE_FILES}
    ${PLUGIN_SOURCE_FILES}
)

# Common include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${BASE_HEADERS_DIR}
    ${UTILS_HEADERS_DIR}
    ${ENVIRON_HEADERS_DIR}
    ${EXTENSION_HEADERS_DIR}
    ${MOVIE_HEADERS_DIR}
#    ${MSG_HEADERS_DIR}
    ${SOUND_HEADERS_DIR}
    ${VISUAL_HEADERS_DIR}
    ${PLUGIN_HEADERS_DIR}
    ${KRKR2CORE_PATH}/
    ${KRKR2CORE_PATH}/../plugins
    ${KRKR2CORE_PATH}/../../external
    ${KRKR2CORE_PATH}/../../external/unzip
    ${KRKR2CORE_PATH}/../../cocos
    ${KRKR2CORE_PATH}/../..
    ${KRKR2CORE_PATH}/../../extensions
    ${KRKR2CORE_PATH}/../../cocos/editor-support
    /usr/include/gtk-2.0
    /usr/include/glib-2.0 
    /usr/lib/x86_64-linux-gnu/glib-2.0/include 
    /usr/include/cairo 
    /usr/include/pango-1.0
    /usr/include/harfbuzz
    /usr/lib/x86_64-linux-gnu/gtk-2.0/include
    /usr/include/gdk-pixbuf-2.0 
    /usr/include/atk-1.0
    /usr/include/GLFW
    /usr/include/opus
    /usr/include/webp
    /usr/include/opencv4
    /usr/include/freetype2
    /usr/include/uchardet
#FIXME:for Fedora    
    PUBLIC /usr/lib64/glib-2.0/include
    PUBLIC /usr/lib64/gtk-2.0/include    
#FIXME:Raspberry Pi 4, FIXME: two places
    PUBLIC /usr/lib/arm-linux-gnueabihf/glib-2.0/include
    PUBLIC /usr/lib/arm-linux-gnueabihf/gtk-2.0/include  
#FIXME:VisionFive2, FIXME: two places    
    PUBLIC /usr/lib/riscv64-linux-gnu/glib-2.0/include  
    PUBLIC /usr/lib/riscv64-linux-gnu/gtk-2.0/include  
)
#FIXME:added, ../../external, ../../cocos

target_compile_definitions(${PROJECT_NAME} PUBLIC
    -DTJS_TEXT_OUT_CRLF
    -D__STDC_CONSTANT_MACROS
    -DUSE_UNICODE_FSTRING
)

#############
#FIXME:TODO:export from ../../cocos/CMakeLists.txt
#FIXME:added
add_definitions(-DCC_ENABLE_BOX2D_INTEGRATION=0 -DCC_USE_PHYSICS=0)
add_definitions(-DCC_USE_3D_PHYSICS=0 -DCC_ENABLE_BULLET_INTEGRATION=0 -DCC_USE_NAVMESH=0)
add_definitions(-DCC_USE_WEBP=0 -DCC_USE_TIFF=0)

#FIXME:added
add_definitions(-DGLFW_DLL)
add_definitions(-DLINUX)
add_definitions(-DCOCOS2D_DEBUG=1)

#FIXME:added
add_definitions(-DMY_USE_FREESERIF=1)
add_definitions(-DMY_USE_UIBUTTON_SETTITLECOLOR=1)
add_definitions(-DMY_USE_UNRARSRC=0)
add_definitions(-DMY_USE_LIBBPG=0)
add_definitions(-DMY_USE_LIBJXR=0)
add_definitions(-DMY_USE_LIB7ZIP=0)
add_definitions(-DMY_USE_PARTICLE3D=0)
#############
add_definitions(-DMY_USE_FFMPEG=0)

#FIXME:added
target_link_libraries(${PROJECT_NAME} PUBLIC tjs2 minizip)

if(NOT APPLE)
    # 启用 OpenMP 支持
    find_package(OpenMP REQUIRED)

    target_link_libraries(${PROJECT_NAME} INTERFACE OpenMP::OpenMP_CXX)
    target_compile_options(${PROJECT_NAME} INTERFACE ${OpenMP_CXX_FLAGS})
endif ()

if(ANDROID)
    target_link_libraries(${PROJECT_NAME} INTERFACE
        log
        android
        EGL
        GLESv2
        GLESv1_CM
        OpenSLES
    )
endif()

# cocos2dx
#find_package(cocos2dx CONFIG REQUIRED)

## Link libraries
target_link_libraries(${PROJECT_NAME} INTERFACE
    # cocos2d
#    cocos2dx::cocos2d
#    $<$<BOOL:${ANDROID}>:$<LINK_LIBRARY:WHOLE_ARCHIVE,cocos2dx::cpp_android_spec>>
)

#FIXME:added, for cocos2d-x
add_definitions(-DLINUX)

