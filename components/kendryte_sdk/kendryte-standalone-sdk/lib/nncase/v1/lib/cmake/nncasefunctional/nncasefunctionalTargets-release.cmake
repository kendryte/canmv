#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "nncasefunctional" for configuration "Release"
set_property(TARGET nncasefunctional APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(nncasefunctional PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libnncase.functional.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS nncasefunctional )
list(APPEND _IMPORT_CHECK_FILES_FOR_nncasefunctional "${_IMPORT_PREFIX}/lib/libnncase.functional.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
