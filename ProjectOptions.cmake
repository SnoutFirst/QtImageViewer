include(cmake/LibFuzzer.cmake)
include(CMakeDependentOption)
include(CheckCXXCompilerFlag)


include(CheckCXXSourceCompiles)


macro(QtImageViewer_supports_sanitizers)
  # Emscripten doesn't support sanitizers
  if(EMSCRIPTEN)
    set(SUPPORTS_UBSAN OFF)
    set(SUPPORTS_ASAN OFF)
  elseif((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND NOT WIN32)

    message(STATUS "Sanity checking UndefinedBehaviorSanitizer, it should be supported on this platform")
    set(TEST_PROGRAM "int main() { return 0; }")

    # Check if UndefinedBehaviorSanitizer works at link time
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=undefined")
    check_cxx_source_compiles("${TEST_PROGRAM}" HAS_UBSAN_LINK_SUPPORT)

    if(HAS_UBSAN_LINK_SUPPORT)
      message(STATUS "UndefinedBehaviorSanitizer is supported at both compile and link time.")
      set(SUPPORTS_UBSAN ON)
    else()
      message(WARNING "UndefinedBehaviorSanitizer is NOT supported at link time.")
      set(SUPPORTS_UBSAN OFF)
    endif()
  else()
    set(SUPPORTS_UBSAN OFF)
  endif()

  if((CMAKE_CXX_COMPILER_ID MATCHES ".*Clang.*" OR CMAKE_CXX_COMPILER_ID MATCHES ".*GNU.*") AND WIN32)
    set(SUPPORTS_ASAN OFF)
  else()
    if (NOT WIN32)
      message(STATUS "Sanity checking AddressSanitizer, it should be supported on this platform")
      set(TEST_PROGRAM "int main() { return 0; }")

      # Check if AddressSanitizer works at link time
      set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
      set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
      check_cxx_source_compiles("${TEST_PROGRAM}" HAS_ASAN_LINK_SUPPORT)

      if(HAS_ASAN_LINK_SUPPORT)
        message(STATUS "AddressSanitizer is supported at both compile and link time.")
        set(SUPPORTS_ASAN ON)
      else()
        message(WARNING "AddressSanitizer is NOT supported at link time.")
        set(SUPPORTS_ASAN OFF)
      endif()
    else()
      set(SUPPORTS_ASAN ON)
    endif()
  endif()
endmacro()

macro(QtImageViewer_setup_options)
  option(QtImageViewer_ENABLE_HARDENING "Enable hardening" ON)
  option(QtImageViewer_ENABLE_COVERAGE "Enable coverage reporting" OFF)
  cmake_dependent_option(
    QtImageViewer_ENABLE_GLOBAL_HARDENING
    "Attempt to push hardening options to built dependencies"
    ON
    QtImageViewer_ENABLE_HARDENING
    OFF)

  QtImageViewer_supports_sanitizers()

  if(NOT PROJECT_IS_TOP_LEVEL OR QtImageViewer_PACKAGING_MAINTAINER_MODE)
    option(QtImageViewer_ENABLE_IPO "Enable IPO/LTO" OFF)
    option(QtImageViewer_WARNINGS_AS_ERRORS "Treat Warnings As Errors" OFF)
    option(QtImageViewer_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    option(QtImageViewer_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(QtImageViewer_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" OFF)
    option(QtImageViewer_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(QtImageViewer_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(QtImageViewer_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    option(QtImageViewer_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(QtImageViewer_ENABLE_INCLUDE_WHAT_YOU_USE "Enable include-what-you-use" OFF)
    option(QtImageViewer_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(QtImageViewer_ENABLE_PCH "Enable precompiled headers" OFF)
    option(QtImageViewer_ENABLE_CACHE "Enable ccache" OFF)
  else()
    option(QtImageViewer_ENABLE_IPO "Enable IPO/LTO" ON)
    option(QtImageViewer_WARNINGS_AS_ERRORS "Treat Warnings As Errors" ON)
    option(QtImageViewer_ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" ${SUPPORTS_ASAN})
    option(QtImageViewer_ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    option(QtImageViewer_ENABLE_SANITIZER_UNDEFINED "Enable undefined sanitizer" ${SUPPORTS_UBSAN})
    option(QtImageViewer_ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    option(QtImageViewer_ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    option(QtImageViewer_ENABLE_UNITY_BUILD "Enable unity builds" OFF)
    # The analysers are OFF by default: the template's .clang-tidy asks for every
    # check with warnings as errors, which is not a useful signal for code that
    # includes Qt headers (and clang-tidy 19 segfaults on it), and cppcheck takes
    # several minutes per translation unit here. Turn them on per build with
    # -DQtImageViewer_ENABLE_CLANG_TIDY=ON / -DQtImageViewer_ENABLE_CPPCHECK=ON;
    # the build itself is warnings-as-errors either way.
    option(QtImageViewer_ENABLE_CLANG_TIDY "Enable clang-tidy" OFF)
    option(QtImageViewer_ENABLE_INCLUDE_WHAT_YOU_USE "Enable include-what-you-use" OFF)
    option(QtImageViewer_ENABLE_CPPCHECK "Enable cpp-check analysis" OFF)
    option(QtImageViewer_ENABLE_PCH "Enable precompiled headers" OFF)
    option(QtImageViewer_ENABLE_CACHE "Enable ccache" ON)
  endif()

  if(NOT PROJECT_IS_TOP_LEVEL)
    mark_as_advanced(
      QtImageViewer_ENABLE_IPO
      QtImageViewer_WARNINGS_AS_ERRORS
      QtImageViewer_ENABLE_SANITIZER_ADDRESS
      QtImageViewer_ENABLE_SANITIZER_LEAK
      QtImageViewer_ENABLE_SANITIZER_UNDEFINED
      QtImageViewer_ENABLE_SANITIZER_THREAD
      QtImageViewer_ENABLE_SANITIZER_MEMORY
      QtImageViewer_ENABLE_UNITY_BUILD
      QtImageViewer_ENABLE_CLANG_TIDY
      QtImageViewer_ENABLE_INCLUDE_WHAT_YOU_USE
      QtImageViewer_ENABLE_CPPCHECK
      QtImageViewer_ENABLE_COVERAGE
      QtImageViewer_ENABLE_PCH
      QtImageViewer_ENABLE_CACHE)
  endif()

  QtImageViewer_check_libfuzzer_support(LIBFUZZER_SUPPORTED)
  if(LIBFUZZER_SUPPORTED AND (QtImageViewer_ENABLE_SANITIZER_ADDRESS OR QtImageViewer_ENABLE_SANITIZER_THREAD OR QtImageViewer_ENABLE_SANITIZER_UNDEFINED))
    set(DEFAULT_FUZZER ON)
  else()
    set(DEFAULT_FUZZER OFF)
  endif()

  option(QtImageViewer_BUILD_FUZZ_TESTS "Enable fuzz testing executable" ${DEFAULT_FUZZER})

endmacro()

macro(QtImageViewer_global_options)
  if(QtImageViewer_ENABLE_IPO)
    include(cmake/InterproceduralOptimization.cmake)
    QtImageViewer_enable_ipo()
  endif()

  QtImageViewer_supports_sanitizers()

  if(QtImageViewer_ENABLE_HARDENING AND QtImageViewer_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR QtImageViewer_ENABLE_SANITIZER_UNDEFINED
       OR QtImageViewer_ENABLE_SANITIZER_ADDRESS
       OR QtImageViewer_ENABLE_SANITIZER_THREAD
       OR QtImageViewer_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    message("${QtImageViewer_ENABLE_HARDENING} ${ENABLE_UBSAN_MINIMAL_RUNTIME} ${QtImageViewer_ENABLE_SANITIZER_UNDEFINED}")
    QtImageViewer_enable_hardening(QtImageViewer_options ON ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()
endmacro()

macro(QtImageViewer_local_options)
  if(PROJECT_IS_TOP_LEVEL)
    include(cmake/StandardProjectSettings.cmake)
  endif()

  add_library(QtImageViewer_warnings INTERFACE)
  add_library(QtImageViewer_options INTERFACE)

  include(cmake/CompilerWarnings.cmake)
  QtImageViewer_set_project_warnings(
    QtImageViewer_warnings
    ${QtImageViewer_WARNINGS_AS_ERRORS}
    ""
    ""
    ""
    "")

  include(cmake/Linker.cmake)
  # Must configure each target with linker options, we're avoiding setting it globally for now

  if(NOT EMSCRIPTEN)
    include(cmake/Sanitizers.cmake)
    QtImageViewer_enable_sanitizers(
      QtImageViewer_options
      ${QtImageViewer_ENABLE_SANITIZER_ADDRESS}
      ${QtImageViewer_ENABLE_SANITIZER_LEAK}
      ${QtImageViewer_ENABLE_SANITIZER_UNDEFINED}
      ${QtImageViewer_ENABLE_SANITIZER_THREAD}
      ${QtImageViewer_ENABLE_SANITIZER_MEMORY})
  endif()

  set_target_properties(QtImageViewer_options PROPERTIES UNITY_BUILD ${QtImageViewer_ENABLE_UNITY_BUILD})

  if(QtImageViewer_ENABLE_PCH)
    target_precompile_headers(
      QtImageViewer_options
      INTERFACE
      <vector>
      <string>
      <utility>)
  endif()

  if(QtImageViewer_ENABLE_CACHE)
    include(cmake/Cache.cmake)
    QtImageViewer_enable_cache()
  endif()

  include(cmake/StaticAnalyzers.cmake)
  if(QtImageViewer_ENABLE_CLANG_TIDY)
    QtImageViewer_enable_clang_tidy(QtImageViewer_options ${QtImageViewer_WARNINGS_AS_ERRORS})
  endif()

  if(QtImageViewer_ENABLE_INCLUDE_WHAT_YOU_USE)
    QtImageViewer_enable_include_what_you_use()
  endif()

  if(QtImageViewer_ENABLE_CPPCHECK)
    QtImageViewer_enable_cppcheck(${QtImageViewer_WARNINGS_AS_ERRORS} "" # override cppcheck options
    )
  endif()

  if(QtImageViewer_ENABLE_COVERAGE)
    include(cmake/Tests.cmake)
    QtImageViewer_enable_coverage(QtImageViewer_options)
  endif()

  if(QtImageViewer_WARNINGS_AS_ERRORS)
    check_cxx_compiler_flag("-Wl,--fatal-warnings" LINKER_FATAL_WARNINGS)
    if(LINKER_FATAL_WARNINGS)
      # This is not working consistently, so disabling for now
      # target_link_options(QtImageViewer_options INTERFACE -Wl,--fatal-warnings)
    endif()
  endif()

  if(QtImageViewer_ENABLE_HARDENING AND NOT QtImageViewer_ENABLE_GLOBAL_HARDENING)
    include(cmake/Hardening.cmake)
    if(NOT SUPPORTS_UBSAN 
       OR QtImageViewer_ENABLE_SANITIZER_UNDEFINED
       OR QtImageViewer_ENABLE_SANITIZER_ADDRESS
       OR QtImageViewer_ENABLE_SANITIZER_THREAD
       OR QtImageViewer_ENABLE_SANITIZER_LEAK)
      set(ENABLE_UBSAN_MINIMAL_RUNTIME FALSE)
    else()
      set(ENABLE_UBSAN_MINIMAL_RUNTIME TRUE)
    endif()
    QtImageViewer_enable_hardening(QtImageViewer_options OFF ${ENABLE_UBSAN_MINIMAL_RUNTIME})
  endif()

endmacro()
