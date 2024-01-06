cmake_minimum_required(VERSION 3.5)

project(GeotiffConverter_2 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Program Files (x86)/libpng;C:/Program Files (x86)/tiff;C:/Program Files (x86)/Project;C:/Program Files (x86)/lua")

include(FindZLIB)
include(FindPNG)
include(FindTIFF)
include(FindSQLite3)
include(FindLua)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Lua REQUIRED)

# # # sol3 generated single header library
add_library(sol2_single INTERFACE)
add_library(sol2::sol2::single ALIAS sol2_single)
set_target_properties(sol2_single
        PROPERTIES
        EXPORT_NAME sol2::sol2::single
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/include")
add_dependencies(sol2_single sol2_single_header_generator)

set(interface
    tifffunctions.h
    qtfunctions.h
    commonfunctions.h
    pngfunctions.h
    configurergbform.h
    conversionparameters.h
    geotiffwindow.h
    csvwindow.h
    geojsonwindow.h
    shapes.h
    previewtask.h
    imageconverter.h
    geopackagewindow.h
    tabledata.h
    consts.h
    luacodewindow.h
    newcsvwindow.h
    newgeojsonwindow.h
    newgeopackagewindow.h
)
set(src
    tifffunctions.cpp
    qtfunctions.cpp
    commonfunctions.cpp
    configurergbform.cpp
    geotiffwindow.cpp
    csvwindow.cpp
    geojsonwindow.cpp
    shapes.cpp
    imageconverter.cpp
    geopackagewindow.cpp
    tabledata.cpp
    luacodewindow.cpp
    newcsvwindow.cpp
    newgeojsonwindow.cpp
    newgeopackagewindow.cpp
)
set(uis
    configurergbform.ui
    geotiffwindow.ui
    csvwindow.ui
    geojsonwindow.ui
    geopackagewindow.ui
    luacodewindow.ui
    newcsvwindow.ui
    newgeojsonwindow.ui
    newgeopackagewindow.ui
)

set(all_src
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        ${interface}
        ${src}
        ${uis}
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(GeotiffConverter_2
        MANUAL_FINALIZATION
        ${all_src}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET GeotiffConverter_2 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(GeotiffConverter_2 SHARED
            ${all_src}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(GeotiffConverter_2
            ${all_src}
        )
    endif()
endif()

target_include_directories(GeotiffConverter_2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    ${PNG_INCLUDE_DIRS}
    ${TIFF_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
)
target_link_libraries(GeotiffConverter_2 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Threads::Threads ZLIB::ZLIB ${PNG_LIBRARIES} ${TIFF_LIBRARIES} ${SQLite3_LIBRARIES} ${LUA_LIBRARIES} sol2_single)

set_target_properties(GeotiffConverter_2 PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS GeotiffConverter_2
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(GeotiffConverter_2)
endif()
