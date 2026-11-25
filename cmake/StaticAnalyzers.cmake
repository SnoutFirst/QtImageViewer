# The analysers are configured once, and then switched on *per target* with
# QtImageViewer_apply_static_analysers(). Setting CMAKE_CXX_CLANG_TIDY or
# CMAKE_CXX_CPPCHECK instead would apply them to everything built afterwards,
# including the third party dependencies CPM downloads, which are not ours to
# fix and cannot pass a warnings-as-errors policy.

macro(QtImageViewer_enable_cppcheck WARNINGS_AS_ERRORS CPPCHECK_OPTIONS)
  find_program(CPPCHECK cppcheck)
  if(CPPCHECK)

    if(CMAKE_GENERATOR MATCHES ".*Visual Studio.*")
      set(CPPCHECK_TEMPLATE "vs")
    else()
      set(CPPCHECK_TEMPLATE "gcc")
    endif()

    if("${CPPCHECK_OPTIONS}" STREQUAL "")
      # Enable all warnings that are actionable by the user of this toolset
      # style should enable the other 3, but we'll be explicit just in case
      set(SUPPRESS_DIR "*:${CMAKE_CURRENT_BINARY_DIR}/_deps/*.h")
      set(cppcheck_options
          ${CPPCHECK}
          --template=${CPPCHECK_TEMPLATE}
          --enable=style,performance,warning,portability
          --inline-suppr
          # We cannot act on a bug/missing feature of cppcheck
          --suppress=cppcheckError
          --suppress=internalAstError
          # if a file does not have an internalAstError, we get an unmatchedSuppression error
          --suppress=unmatchedSuppression
          # noisy and incorrect sometimes
          --suppress=passedByValue
          # ignores code that cppcheck thinks is invalid C++
          --suppress=syntaxError
          --suppress=preprocessorErrorDirective
          # ignores static_assert type failures
          --suppress=knownConditionTrueFalse
          # analysing every branch is the point of running it at all
          --check-level=exhaustive
          --inconclusive
          --suppress=${SUPPRESS_DIR})
    else()
      # if the user provides a CPPCHECK_OPTIONS with a template specified, it will override this template
      set(cppcheck_options ${CPPCHECK} --template=${CPPCHECK_TEMPLATE} ${CPPCHECK_OPTIONS})
    endif()

    if(NOT
       "${CMAKE_CXX_STANDARD}"
       STREQUAL
       "")
      list(APPEND cppcheck_options --std=c++${CMAKE_CXX_STANDARD})
    endif()
    if(${WARNINGS_AS_ERRORS})
      list(APPEND cppcheck_options --error-exitcode=2)
    endif()

    # Stored for the per-target property below, *not* in CMAKE_CXX_CPPCHECK:
    # that variable would apply cppcheck to every target created from then on,
    # Catch2 included, and third party code cannot pass a warnings-as-errors
    # policy we did not write.
    set(QtImageViewer_CPPCHECK_OPTIONS
        "${cppcheck_options}"
        CACHE INTERNAL "cppcheck command line used by QtImageViewer_apply_static_analysers")
  else()
    message(WARNING "cppcheck requested but executable not found")
  endif()
endmacro()

macro(QtImageViewer_enable_clang_tidy target WARNINGS_AS_ERRORS)

  find_program(CLANGTIDY clang-tidy)
  if(CLANGTIDY)
    if(NOT
       CMAKE_CXX_COMPILER_ID
       MATCHES
       ".*Clang")

      get_target_property(TARGET_PCH ${target} INTERFACE_PRECOMPILE_HEADERS)

      if("${TARGET_PCH}" STREQUAL "TARGET_PCH-NOTFOUND")
        get_target_property(TARGET_PCH ${target} PRECOMPILE_HEADERS)
      endif()

      if(NOT ("${TARGET_PCH}" STREQUAL "TARGET_PCH-NOTFOUND"))
        message(
          SEND_ERROR
            "clang-tidy cannot be enabled with non-clang compiler and PCH, clang-tidy fails to handle gcc's PCH file")
      endif()
    endif()

    # construct the clang-tidy command line
    set(CLANG_TIDY_OPTIONS
        ${CLANGTIDY}
        -extra-arg=-Wno-unknown-warning-option
        -extra-arg=-Wno-ignored-optimization-argument
        -extra-arg=-Wno-unused-command-line-argument
        -p)
    # set standard
    if(NOT
       "${CMAKE_CXX_STANDARD}"
       STREQUAL
       "")
      if("${CLANG_TIDY_OPTIONS_DRIVER_MODE}" STREQUAL "cl")
        set(CLANG_TIDY_OPTIONS ${CLANG_TIDY_OPTIONS} -extra-arg=/std:c++${CMAKE_CXX_STANDARD})
      else()
        set(CLANG_TIDY_OPTIONS ${CLANG_TIDY_OPTIONS} -extra-arg=-std=c++${CMAKE_CXX_STANDARD})
      endif()
    endif()

    # set warnings as errors
    if(${WARNINGS_AS_ERRORS})
      list(APPEND CLANG_TIDY_OPTIONS -warnings-as-errors=*)
    endif()

    set(QtImageViewer_CLANG_TIDY_OPTIONS
        "${CLANG_TIDY_OPTIONS}"
        CACHE INTERNAL "clang-tidy command line used by QtImageViewer_apply_static_analysers")
  else()
    message(WARNING "clang-tidy requested but executable not found")
  endif()
endmacro()

macro(QtImageViewer_enable_include_what_you_use)
  find_program(INCLUDE_WHAT_YOU_USE include-what-you-use)
  if(INCLUDE_WHAT_YOU_USE)
    set(QtImageViewer_INCLUDE_WHAT_YOU_USE_OPTIONS
        "${INCLUDE_WHAT_YOU_USE}"
        CACHE INTERNAL "include-what-you-use command line used by QtImageViewer_apply_static_analysers")
  else()
    message(WARNING "include-what-you-use requested but executable not found")
  endif()
endmacro()

#! Switches on whichever analysers were enabled, for \a target and its sources only.
macro(QtImageViewer_apply_static_analysers target)
  if(QtImageViewer_CLANG_TIDY_OPTIONS)
    set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${QtImageViewer_CLANG_TIDY_OPTIONS}")
  endif()
  if(QtImageViewer_CPPCHECK_OPTIONS)
    set_target_properties(${target} PROPERTIES CXX_CPPCHECK "${QtImageViewer_CPPCHECK_OPTIONS}")
  endif()
  if(QtImageViewer_INCLUDE_WHAT_YOU_USE_OPTIONS)
    set_target_properties(${target} PROPERTIES CXX_INCLUDE_WHAT_YOU_USE "${QtImageViewer_INCLUDE_WHAT_YOU_USE_OPTIONS}")
  endif()
endmacro()
