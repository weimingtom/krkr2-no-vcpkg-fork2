#cmake_minimum_required(VERSION 3.28)
cmake_minimum_required(VERSION 3.6)
project(tjs2)

add_library(${PROJECT_NAME} STATIC)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:_DEBUG>)

set(GEN_CODE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)

if(WINDOWS)
    target_compile_options(${PROJECT_NAME} PRIVATE "/FS")
endif()

if(NOT EXISTS ${GEN_CODE_BINARY_DIR})
    file(MAKE_DIRECTORY ${GEN_CODE_BINARY_DIR})
endif()

#===============================================================#
#======================= generate parser =======================#
#===============================================================#
#find_program(BISON_EXECUTABLE NAMES win_bison.exe bison.exe bison)

#set(BISON_FILES
#    ${CMAKE_CURRENT_LIST_DIR}/bison/tjs.y
#    ${CMAKE_CURRENT_LIST_DIR}/bison/tjsdate.y
#    ${CMAKE_CURRENT_LIST_DIR}/bison/tjspp.y
#)
#set(PARSE_TAB_CPP_FILES)
#foreach(BISON_FILE ${BISON_FILES})
#    get_filename_component(FILE_NAME ${BISON_FILE} NAME_WE)
#
#    execute_process(
#        TIMEOUT 5000
#        COMMAND ${BISON_EXECUTABLE} ${BISON_FILE}
#        WORKING_DIRECTORY ${GEN_CODE_BINARY_DIR}
#        COMMAND_ECHO STDOUT
#    )
#
#    add_custom_command(
#        OUTPUT
#            ${GEN_CODE_BINARY_DIR}/${FILE_NAME}.tab.cpp
#            ${GEN_CODE_BINARY_DIR}/${FILE_NAME}.tab.hpp
#        DEPENDS ${BISON_FILE}
#        COMMAND ${BISON_EXECUTABLE} ${BISON_FILE}
#        WORKING_DIRECTORY ${GEN_CODE_BINARY_DIR}
#        COMMENT "Generate Bison File From ${BISON_FILE}"
#    )
#
#    list(APPEND PARSE_TAB_CPP_FILES ${GEN_CODE_BINARY_DIR}/${FILE_NAME}.tab.cpp)
#endforeach()

#add_custom_target(GENERATE_CPP_FILES DEPENDS ${PARSE_TAB_CPP_FILES})
#add_dependencies(${PROJECT_NAME} GENERATE_CPP_FILES)

#install(DIRECTORY "${GEN_CODE_BINARY_DIR}/"
#    DESTINATION "include/tjs2/parser"
#    FILES_MATCHING PATTERN "*.tab.hpp"
#)

#FIXME:added
set(PARSE_TAB_CPP_FILES
    ${CMAKE_CURRENT_LIST_DIR}/tjs.tab.cpp
    ${CMAKE_CURRENT_LIST_DIR}/tjsdate.tab.cpp
    ${CMAKE_CURRENT_LIST_DIR}/tjspp.tab.cpp
)

#===============================================================#
#==================== generate date word =======================#
#===============================================================#
find_package(Python3)

execute_process(
    TIMEOUT 5000
    COMMAND ${Python3_EXECUTABLE} create_world_map.py ${GEN_CODE_BINARY_DIR}/tjsDateWordMap.inc
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/script
    COMMAND_ECHO STDOUT
)

target_sources(${PROJECT_NAME} PRIVATE
    ${PARSE_TAB_CPP_FILES}
    tjsLex.cpp
    tjsNative.cpp
    tjsRandomGenerator.cpp
    tjsDebug.cpp
    tjsRegExp.cpp
    tjsBinarySerializer.cpp
    tjsInterCodeGen.cpp
    tjsObject.cpp
    tjsConstArrayData.cpp
    tjsUtils.cpp
    tjsException.cpp
    tjsDictionary.cpp
    tjsInterCodeExec.cpp
    tjsVariantString.cpp
    tjsScriptBlock.cpp
    tjsMath.cpp
    tjs.cpp
    tjsMessage.cpp
    tjsDisassemble.cpp
    tjsDate.cpp
    tjsError.cpp
    tjsInterface.cpp
    tjsMT19937ar-cok.cpp
    tjsArray.cpp
    tjsByteCodeLoader.cpp
    tjsDateParser.cpp
    tjsGlobalStringMap.cpp
    tjsOctPack.cpp
    tjsNamespace.cpp
    tjsScriptCache.cpp
    tjsConfig.cpp
    tjsCompileControl.cpp
    tjsObjectExtendable.cpp
    tjsVariant.cpp
    tjsString.cpp
#FIXME:
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/ascii.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/big5.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/cp1251.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/euc_jp.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/euc_kr.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/euc_tw.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/gb18030.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_1.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_2.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_3.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_4.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_5.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_6.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_7.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_8.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_9.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_10.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_11.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_13.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_14.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_15.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/iso8859_16.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/koi8.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/koi8_r.c
#    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/mktable.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/sjis.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/unicode.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/utf8.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/utf16_be.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/utf16_le.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/utf32_be.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/enc/utf32_le.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/regcomp.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/regenc.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/regerror.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/regexec.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/regext.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/reggnu.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/regparse.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/regposerr.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/regposix.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/regsyntax.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/regtrav.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/regversion.c
    ${CMAKE_CURRENT_LIST_DIR}/../external/onig/st.c
#FIXME:added, not need, -DONIG_EXTERN=extern -DNOT_RUBY -DEXPORT
)

install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/"
    DESTINATION "include/tjs2"
    FILES_MATCHING PATTERN "*.h"
)

#FIXME:added ../external/onig
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<BUILD_INTERFACE:${GEN_CODE_BINARY_DIR}>
	${CMAKE_CURRENT_LIST_DIR}/../external/onig
    INTERFACE
        $<INSTALL_INTERFACE:include/tjs2>
        $<INSTALL_INTERFACE:include/tjs2/parser>
)

find_package(Boost COMPONENTS locale REQUIRED NO_MODULE)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
#find_package(oniguruma CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Boost::locale
    fmt::fmt
    spdlog::spdlog
#    oniguruma::onig
)