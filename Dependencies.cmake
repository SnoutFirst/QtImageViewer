# Qt is not fetched: it is a system dependency, provided by the distribution
# (qt6-base-dev or qtbase5-dev on Debian and Ubuntu), by Homebrew on macOS, or
# by the Qt online installer / aqtinstall on Windows. There are no other
# dependencies; the unit tests fetch Catch2 through CPM inside test/CMakeLists.txt.

# Which Qt to build against. "AUTO" takes Qt 6 when it can be found and falls
# back to Qt 5; set it to 5 or 6 to build against exactly that major version.
set(QTIMAGEVIEWER_QT_VERSION
    "AUTO"
    CACHE STRING "Qt major version to build against: AUTO, 5 or 6")
set_property(CACHE QTIMAGEVIEWER_QT_VERSION PROPERTY STRINGS AUTO 5 6)

if(QTIMAGEVIEWER_QT_VERSION STREQUAL "AUTO")
  # Only picks the major version; the per-module files (and with them the
  # qt_* functions used below) are loaded by the find_package() after it.
  find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
elseif(QTIMAGEVIEWER_QT_VERSION MATCHES "^[56]$")
  set(QT_VERSION_MAJOR ${QTIMAGEVIEWER_QT_VERSION})
else()
  message(FATAL_ERROR "QTIMAGEVIEWER_QT_VERSION must be AUTO, 5 or 6, not \"${QTIMAGEVIEWER_QT_VERSION}\"")
endif()

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# NOTE: find_package(Qt...) has to run at *directory* scope, not inside a
# function() or a macro(): it sets variables such as QT_CMAKE_EXPORT_NAMESPACE
# that are still needed when a target is finalized at the end of its own
# CMakeLists.txt, and those die with the function scope.

# Widgets pulls in Gui and Core, so all three are available. The non-widget part
# of the program (playlist, file scanning) only needs Gui: it decodes images with
# QImageReader, which needs no window system.
set(QTIMAGEVIEWER_QT_CORE Qt${QT_VERSION_MAJOR}::Core)
set(QTIMAGEVIEWER_QT_GUI Qt${QT_VERSION_MAJOR}::Gui)
set(QTIMAGEVIEWER_QT_WIDGETS Qt${QT_VERSION_MAJOR}::Widgets)

message(STATUS "QtImageViewer: building against Qt ${Qt${QT_VERSION_MAJOR}_VERSION}")
