cmake_minimum_required(VERSION 3.17)

# -----------------------------------------------------------------------------
# HeatMap Viewer Plugin
# -----------------------------------------------------------------------------
set(PROJECT "HeatMapPlugin")
PROJECT(${PROJECT})

# -----------------------------------------------------------------------------
# CMake Options
# -----------------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DWIN32 /EHsc /MP /permissive- /Zc:__cplusplus ")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
endif(MSVC)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
find_package(Qt6 COMPONENTS Widgets WebEngineWidgets REQUIRED)

find_package(ManiVault COMPONENTS Core PointData ClusterData CONFIG)

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------
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

qt6_add_resources(RESOURCE_FILES res/heatmap_resources.qrc)

# -----------------------------------------------------------------------------
# CMake Target
# -----------------------------------------------------------------------------
add_library(${PROJECT} SHARED ${SOURCES} ${RESOURCES} ${RESOURCE_FILES})

# -----------------------------------------------------------------------------
# Target include directories
# -----------------------------------------------------------------------------
target_include_directories(${PROJECT} PRIVATE "${ManiVault_INCLUDE_DIR}")

# -----------------------------------------------------------------------------
# Target properties
# -----------------------------------------------------------------------------
target_compile_features(${PROJECT} PRIVATE cxx_std_17)

# -----------------------------------------------------------------------------
# Target library linking
# -----------------------------------------------------------------------------
target_link_libraries(${PROJECT} PRIVATE Qt6::Widgets)
target_link_libraries(${PROJECT} PRIVATE Qt6::WebEngineWidgets)

target_link_libraries(${PROJECT} PRIVATE ManiVault::Core)
target_link_libraries(${PROJECT} PRIVATE ManiVault::PointData)
target_link_libraries(${PROJECT} PRIVATE ManiVault::ClusterData)

# -----------------------------------------------------------------------------
# Target installation
# -----------------------------------------------------------------------------
install(TARGETS ${PROJECT}
    RUNTIME DESTINATION Plugins COMPONENT PLUGINS # Windows .dll
    LIBRARY DESTINATION Plugins COMPONENT PLUGINS # Linux/Mac .so
)

add_custom_command(TARGET ${PROJECT} POST_BUILD
	COMMAND "${CMAKE_COMMAND}"
	--install ${CMAKE_CURRENT_BINARY_DIR}
	--config $<CONFIGURATION>
	--prefix ${ManiVault_INSTALL_DIR}/$<CONFIGURATION>
)

# -----------------------------------------------------------------------------
# Miscellaneous
# -----------------------------------------------------------------------------
# Automatically set the debug environment (command + working directory) for MSVC
if(MSVC)
    set_property(TARGET ${PROJECT} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY $<IF:$<CONFIG:DEBUG>,${ManiVault_INSTALL_DIR}/debug,${ManiVault_INSTALL_DIR}/release>)
    set_property(TARGET ${PROJECT} PROPERTY VS_DEBUGGER_COMMAND $<IF:$<CONFIG:DEBUG>,"${ManiVault_INSTALL_DIR}/debug/ManiVault Studio.exe","${ManiVault_INSTALL_DIR}/release/ManiVault Studio.exe">)
endif()
