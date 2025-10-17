#cmake_minimum_required(VERSION 3.19)
cmake_minimum_required(VERSION 3.6)
project(krkr2plugin)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PUBLIC
#    ${CMAKE_CURRENT_SOURCE_DIR}/scriptsEx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/win32dialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dirlist.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/csvParser.cpp
#    ${CMAKE_CURRENT_SOURCE_DIR}/layerExMovie.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/varfile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/saveStruct.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/getabout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/addFont.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/wutcwf.cpp
#    ${CMAKE_CURRENT_SOURCE_DIR}/windowEx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/getSample.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/layerExPerspective.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fftgraph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LayerExBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xp3filter.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${KRKR2CORE_PATH}/
    ${KRKR2CORE_PATH}/base
    ${KRKR2CORE_PATH}/base/win32
    ${KRKR2CORE_PATH}/environ
    ${KRKR2CORE_PATH}/environ/win32
    ${KRKR2CORE_PATH}/environ/android
    ${KRKR2CORE_PATH}/environ/sdl
    ${KRKR2CORE_PATH}/msg
    ${KRKR2CORE_PATH}/msg/win32
    ${KRKR2CORE_PATH}/extension
    ${KRKR2CORE_PATH}/sound
    ${KRKR2CORE_PATH}/sound/win32
    ${KRKR2CORE_PATH}/tjs2
    ${KRKR2CORE_PATH}/utils
    ${KRKR2CORE_PATH}/utils/win32
    ${KRKR2CORE_PATH}/visual
    ${KRKR2CORE_PATH}/visual/ARM
    ${KRKR2CORE_PATH}/visual/win32
    ${KRKR2CORE_PATH}/visual/RenderScript/rs
)

add_definitions(-D__STDC_CONSTANT_MACROS -DTJS_TEXT_OUT_CRLF)

target_include_directories(${PROJECT_NAME} PUBLIC ${KRKR2PLUGIN_PATH} ${KRKR2PLUGIN_PATH}/simplebinder})
target_link_libraries(${PROJECT_NAME} PUBLIC krkr2core)

#add_subdirectory(psdfile)
#add_subdirectory(psbfile)
#add_subdirectory(layerex_draw)
#add_subdirectory(motionplayer)

##add_subdirectory(json)
#add_subdirectory(fstat)
##add_subdirectory(steam)
##add_subdirectory(DrawDeviceForSteam)
#target_link_libraries(${PROJECT_NAME} PUBLIC
#        psdfile
#        psbfile
#        layerExDraw
#        motionplayer
#        fstat
##        json steam DrawDeviceForSteam
#)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)
