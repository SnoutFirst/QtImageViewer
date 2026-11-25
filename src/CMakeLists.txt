# The Qt application. Everything the cmake_template provides (the warning set,
# sanitizers, hardening, linker flags, static analysers) is reused through the
# QtImageViewer_options / QtImageViewer_warnings interface targets.
#
# The sources are plain C++20 + Qt: the same files build against Qt 5.15 and
# Qt 6.x, and Dependencies.cmake picks the one to use.

# qt_standard_project_setup() (Qt 6) and the plain AUTOMOC below turn the Q_OBJECT
# classes in the headers into the meta object code they need.
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
  qt_standard_project_setup()
else()
  set(CMAKE_AUTOMOC ON)
endif()

# ---- The part that is not a widget, shared with the unit tests --------------
add_library(QtImageViewerCore OBJECT playlist.cpp playlist.h imagefiles.cpp imagefiles.h keymap.cpp keymap.h)
add_library(QtImageViewer::Core ALIAS QtImageViewerCore)

target_include_directories(QtImageViewerCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
QtImageViewer_apply_static_analysers(QtImageViewerCore)

target_link_libraries(
  QtImageViewerCore
  PUBLIC QtImageViewer::QtImageViewer_options QtImageViewer::QtImageViewer_warnings ${QTIMAGEVIEWER_QT_GUI})

# ---- The widgets, shared with the GUI tests ---------------------------------
add_library(
  QtImageViewerWidgets OBJECT imageview.cpp imageview.h framelesswindow.cpp framelesswindow.h mainwindow.cpp
                           mainwindow.h)
add_library(QtImageViewer::Widgets ALIAS QtImageViewerWidgets)

target_include_directories(QtImageViewerWidgets PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
QtImageViewer_apply_static_analysers(QtImageViewerWidgets)

target_link_libraries(
  QtImageViewerWidgets
  PUBLIC QtImageViewer::QtImageViewer_options QtImageViewer::QtImageViewer_warnings QtImageViewer::Core
         ${QTIMAGEVIEWER_QT_WIDGETS})

# ---- The program -------------------------------------------------------------
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
  qt_add_executable(QtImageViewer MANUAL_FINALIZATION main.cpp)
else()
  add_executable(QtImageViewer main.cpp)
endif()

# Object libraries do not hand their objects on through another object library,
# so both of them are named here.
target_link_libraries(
  QtImageViewer
  PRIVATE QtImageViewer::QtImageViewer_options
          QtImageViewer::QtImageViewer_warnings
          QtImageViewer::Core
          QtImageViewer::Widgets
          ${QTIMAGEVIEWER_QT_WIDGETS})

QtImageViewer_apply_static_analysers(QtImageViewer)
QtImageViewer_configure_linker(QtImageViewer)

# CMake-configured headers (project name / version / git sha)
target_include_directories(QtImageViewer PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include/internal_use_only")

# A GUI app on Windows (no console window) and a proper .app bundle on macOS, so
# that windeployqt / macdeployqt have something sensible to work on.
set_target_properties(
  QtImageViewer
  PROPERTIES WIN32_EXECUTABLE ON
             MACOSX_BUNDLE ON
             MACOSX_BUNDLE_BUNDLE_NAME "QtImageViewer"
             MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.snoutfirst.QtImageViewer"
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})

# Resolves the plugins of the Qt modules this target links (a no-op before Qt 6).
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
  qt_finalize_executable(QtImageViewer)
endif()
