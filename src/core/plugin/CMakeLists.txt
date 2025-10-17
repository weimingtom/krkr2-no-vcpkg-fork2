set(PLUGIN_PATH ${CMAKE_CURRENT_LIST_DIR})
set(PLUGIN_SOURCE_FILES
        ${PLUGIN_PATH}/ncbind.cpp
        ${PLUGIN_PATH}/PluginIntf.cpp
        ${PLUGIN_PATH}/PluginImpl.cpp

    PARENT_SCOPE
)

set(PLUGIN_HEADERS_DIR
        ${PLUGIN_PATH}/

    PARENT_SCOPE
)
