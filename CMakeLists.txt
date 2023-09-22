cmake_minimum_required(VERSION 3.1.0)

set(PROJECT "HeatMapPlugin")

PROJECT(${PROJECT})

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /EHsc /MP /permissive- /Zc:__cplusplus ")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif(MSVC)

file(TO_CMAKE_PATH $ENV{HDPS_INSTALL_DIR} INSTALL_DIR)

find_package(Qt6 6.3.1 COMPONENTS Widgets WebEngineWidgets REQUIRED)

qt6_add_resources(RESOURCE_FILES res/heatmap_resources.qrc)

set(SOURCES
    src/HeatMapPlugin.h
    src/HeatMapPlugin.cpp
    src/HeatMapWidget.h
    src/HeatMapWidget.cpp
    src/HeatMapPlugin.json
)

set (RESOURCES
    res/heatmap_resources.qrc
)

set(PLUGIN_MOC_HEADERS
    src/HeatMapPlugin.h
    src/HeatMapWidget.h
)

source_group( Plugin FILES ${SOURCES})
source_group( Resources FILES ${RESOURCES})
source_group( Resources\\generated FILES ${CMAKE_CURRENT_BINARY_DIR}/qrc_heatmap_resources.cpp)

add_library(${PROJECT} SHARED ${SOURCES} ${RESOURCES} ${RESOURCE_FILES})

qt_wrap_cpp(HEATMAPPLUGIN_MOC ${PLUGIN_MOC_HEADERS} TARGET ${PROJECT})
target_sources(${PROJECT} PRIVATE ${HEATMAPPLUGIN_MOC})

target_include_directories(${PROJECT} PRIVATE "$ENV{HDPS_INSTALL_DIR}/$<CONFIGURATION>/include/")

target_compile_features(${PROJECT} PRIVATE cxx_std_17)

target_link_libraries(${PROJECT} PRIVATE Qt6::Widgets)
target_link_libraries(${PROJECT} PRIVATE Qt6::WebEngineWidgets)

set(HDPS_LINK_PATH "${INSTALL_DIR}/$<CONFIGURATION>/lib")
set(PLUGIN_LINK_PATH "${INSTALL_DIR}/$<CONFIGURATION>/$<IF:$<CXX_COMPILER_ID:MSVC>,lib,Plugins>")
set(HDPS_LINK_SUFFIX $<IF:$<CXX_COMPILER_ID:MSVC>,${CMAKE_LINK_LIBRARY_SUFFIX},${CMAKE_SHARED_LIBRARY_SUFFIX}>)

set(HDPS_LINK_LIBRARY "${HDPS_LINK_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}HDPS_Public${HDPS_LINK_SUFFIX}")
set(POINTDATA_LINK_LIBRARY "${PLUGIN_LINK_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}PointData${HDPS_LINK_SUFFIX}") 
set(CLUSTERDATA_LINK_LIBRARY "${PLUGIN_LINK_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}ClusterData${HDPS_LINK_SUFFIX}") 

target_link_libraries(${PROJECT} PRIVATE "${HDPS_LINK_LIBRARY}")
target_link_libraries(${PROJECT} PRIVATE "${POINTDATA_LINK_LIBRARY}")
target_link_libraries(${PROJECT} PRIVATE "${CLUSTERDATA_LINK_LIBRARY}")

install(TARGETS ${PROJECT}
    RUNTIME DESTINATION Plugins COMPONENT PLUGINS # Windows .dll
    LIBRARY DESTINATION Plugins COMPONENT PLUGINS # Linux/Mac .so
)

add_custom_command(TARGET ${PROJECT} POST_BUILD
	COMMAND "${CMAKE_COMMAND}"
	--install ${CMAKE_CURRENT_BINARY_DIR}
	--config $<CONFIGURATION>
	--prefix ${INSTALL_DIR}/$<CONFIGURATION>
)

# Automatically set the debug environment (command + working directory) for MSVC
if(MSVC)
    set_property(TARGET ${PROJECT} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<IF:$<CONFIG:DEBUG>,${INSTALL_DIR}/debug,${INSTALL_DIR}/release>)
    set_property(TARGET ${PROJECT} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,${INSTALL_DIR}/debug/HDPS.exe,${INSTALL_DIR}/release/HDPS.exe>)
endif()
