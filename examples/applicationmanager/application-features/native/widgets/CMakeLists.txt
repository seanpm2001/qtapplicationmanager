
cmake_minimum_required(VERSION 3.16)
project(widgets LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/applicationmanager/application-features/apps/widgets")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS AppManApplicationMainPrivate)

qt_add_executable(widgets
    main.cpp
)

# This define flags us as a "launcher" and allows us to link against the AppMan's private libraries
target_compile_definitions(widgets PRIVATE AM_COMPILING_LAUNCHER)

set_target_properties(widgets PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    RUNTIME_OUTPUT_DIRECTORY ../../apps/widgets
)
target_link_libraries(widgets PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::AppManApplicationMainPrivate
)

install(TARGETS widgets
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
