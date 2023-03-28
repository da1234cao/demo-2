cmake_minimum_required(VERSION 3.11)

project(json_test)

# boost-json
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.75 REQUIRED 
    COMPONENTS json)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found")
endif()
include_directories((${Boost_INCLUDE_DIRS}))

add_executable(boost_json_test boost_json_test.cpp)
target_link_libraries(boost_json_test PRIVATE ${Boost_LIBRARIES})

# qt-json
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES qt_json_test.cpp)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(qt_json_test
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(qt_json_test SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(qt_json_test
            ${PROJECT_SOURCES}
        )
    endif()
endif()
target_link_libraries(qt_json_test PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(qt_json_test PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(qt_json_test)
endif()