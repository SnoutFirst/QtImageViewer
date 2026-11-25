cmake_minimum_required(VERSION 3.21...3.31)

project(CmakeConfigPackageTests
  VERSION ${PROJECT_VERSION}
  LANGUAGES CXX)

# ---- Test as standalone project the exported config package ----

if(PROJECT_IS_TOP_LEVEL OR TEST_INSTALLED_VERSION)
  enable_testing()

  find_package(QtImageViewer CONFIG REQUIRED) # for the options/warnings targets

  if(NOT TARGET QtImageViewer_options)
    message(FATAL_ERROR "Required config package not found!")
  endif()
endif()

# ---- Dependencies ----

# CPM is pulled in by the top level Dependencies.cmake; the tests can also be
# configured on their own, so make sure it is there.
if(NOT COMMAND cpmaddpackage)
  include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/CPM.cmake)
endif()

if(NOT TARGET Catch2::Catch2WithMain)
  cpmaddpackage(
    NAME Catch2
    VERSION 3.12.0
    GITHUB_REPOSITORY "catchorg/Catch2"
    SYSTEM YES)
endif()

include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)

# ---- Tests ----

# The playlist and image-scanning code is the part with the interesting rules
# ("every image once before repeating", recursion, duplicate suppression), so it
# is the part that is unit tested.
add_executable(tests main.cpp playlist_tests.cpp imagefiles_tests.cpp)
target_link_libraries(
  tests
  PRIVATE QtImageViewer::QtImageViewer_options
          QtImageViewer::QtImageViewer_warnings
          QtImageViewer::Core
          Catch2::Catch2WithMain)

QtImageViewer_apply_static_analysers(tests)
QtImageViewer_configure_linker(tests)

if(WIN32 AND BUILD_SHARED_LIBS)
  add_custom_command(
    TARGET tests
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:tests> $<TARGET_FILE_DIR:tests>
    COMMAND_EXPAND_LISTS)
endif()

# automatically discover the test cases defined in catch based test files
catch_discover_tests(
  tests
  TEST_PREFIX
  "unittests."
  REPORTER
  XML
  OUTPUT_DIR
  .
  OUTPUT_PREFIX
  "unittests."
  OUTPUT_SUFFIX
  .xml)

# CMake-script regression tests for cmake/Sanitizers.cmake (issue #141).
add_test(
  NAME cmake.sanitizers.apple_excludes_leak
  COMMAND ${CMAKE_COMMAND} -P
          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_sanitizers_apple_excludes_leak.cmake)
add_test(
  NAME cmake.sanitizers.non_apple_includes_leak
  COMMAND ${CMAKE_COMMAND} -P
          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/test_sanitizers_non_apple_includes_leak.cmake)

# The presets have to disable the analysers on Windows with the project's own
# option names, or a from scratch build fails there.
add_test(
  NAME presets.windows_disables_static_analysis
  COMMAND
    ${CMAKE_COMMAND}
    -DPRESETS_FILE=${CMAKE_SOURCE_DIR}/CMakePresets.json
    -P ${CMAKE_CURRENT_SOURCE_DIR}/test_cmake_presets.cmake)

# The command line is part of the program: the version it reports has to be the
# version it was built as, and it has to be able to explain itself.
#
# A window system is not needed for either, but a QApplication still wants a Qt
# platform plugin, hence QT_QPA_PLATFORM=offscreen.
if(NOT WIN32)
  add_test(NAME cli.reports_version COMMAND QtImageViewer --version)
  set_tests_properties(cli.reports_version PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
                                                         PASS_REGULAR_EXPRESSION "${PROJECT_VERSION}")

  add_test(NAME cli.explains_itself COMMAND QtImageViewer --help)
  set_tests_properties(cli.explains_itself PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
                                                        PASS_REGULAR_EXPRESSION "--no-recurse")

  # No images at all is a usage error, not a window with nothing in it.
  add_test(NAME cli.rejects_empty_command_line COMMAND QtImageViewer)
  set_tests_properties(cli.rejects_empty_command_line PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
                                                                WILL_FAIL TRUE)
endif()
