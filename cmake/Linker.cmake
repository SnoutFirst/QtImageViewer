macro(QtImageViewer_configure_linker project_name)
  set(QtImageViewer_USER_LINKER_OPTION
    "DEFAULT"
      CACHE STRING "Linker to be used")
    set(QtImageViewer_USER_LINKER_OPTION_VALUES "DEFAULT" "SYSTEM" "LLD" "GOLD" "BFD" "MOLD" "SOLD" "APPLE_CLASSIC" "MSVC")
  set_property(CACHE QtImageViewer_USER_LINKER_OPTION PROPERTY STRINGS ${QtImageViewer_USER_LINKER_OPTION_VALUES})
  list(
    FIND
    QtImageViewer_USER_LINKER_OPTION_VALUES
    ${QtImageViewer_USER_LINKER_OPTION}
    QtImageViewer_USER_LINKER_OPTION_INDEX)

  if(${QtImageViewer_USER_LINKER_OPTION_INDEX} EQUAL -1)
    message(
      STATUS
        "Using custom linker: '${QtImageViewer_USER_LINKER_OPTION}', explicitly supported entries are ${QtImageViewer_USER_LINKER_OPTION_VALUES}")
  endif()

  set_target_properties(${project_name} PROPERTIES LINKER_TYPE "${QtImageViewer_USER_LINKER_OPTION}")
endmacro()
