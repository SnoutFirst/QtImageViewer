# The Qt application. Everything the cmake_template provides (the warning set,
# sanitizers, hardening, linker flags, static analysers) is reused through the
# QtImageViewer_options / QtImageViewer_warnings interface targets.
#
# The sources are plain C++20 + Qt: the same files build against Qt 5.15 and
# Qt 6.x, and Dependencies.cmake picks the one to use.

# ---- The part that is not a widget, shared with the unit tests --------------
add_library(QtImageViewerCore OBJECT playlist.cpp playlist.h imagefiles.cpp imagefiles.h)
add_library(QtImageViewer::Core ALIAS QtImageViewerCore)

target_include_directories(QtImageViewerCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
QtImageViewer_apply_static_analysers(QtImageViewerCore)

target_link_libraries(
  QtImageViewerCore
  PUBLIC QtImageViewer::QtImageViewer_options QtImageViewer::QtImageViewer_warnings ${QTIMAGEVIEWER_QT_GUI})

# ---- The viewer --------------------------------------------------------------
# qt_standard_project_setup() (Qt 6) and the plain AUTOMOC below turn the Q_OBJECT
# classes in mainwindow.h and imageview.h into the meta object code they need.
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
  qt_standard_project_setup()
else()
  set(CMAKE_AUTOMOC ON)
endif()

set(QTIMAGEVIEWER_APP_SOURCES main.cpp mainwindow.cpp mainwindow.h imageview.cpp imageview.h)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
  qt_add_executable(QtImageViewer MANUAL_FINALIZATION ${QTIMAGEVIEWER_APP_SOURCES})
else()
  add_executable(QtImageViewer ${QTIMAGEVIEWER_APP_SOURCES})
endif()

target_link_libraries(
  QtImageViewer
  PRIVATE QtImageViewer::QtImageViewer_options
          QtImageViewer::QtImageViewer_warnings
          QtImageViewer::Core
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
