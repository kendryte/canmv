#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "nncaseruntime" for configuration "Release"
set_property(TARGET nncaseruntime APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(nncaseruntime PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libnncase.runtime.a"
  )

list(APPEND _cmake_import_check_targets nncaseruntime )
list(APPEND _cmake_import_check_files_for_nncaseruntime "${_IMPORT_PREFIX}/lib/libnncase.runtime.a" )

# Import target "kendryte" for configuration "Release"
set_property(TARGET kendryte APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(kendryte PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libkendryte.a"
  )

list(APPEND _cmake_import_check_targets kendryte )
list(APPEND _cmake_import_check_files_for_kendryte "${_IMPORT_PREFIX}/lib/libkendryte.a" )

# Import target "nncase_rt_modules_k210" for configuration "Release"
set_property(TARGET nncase_rt_modules_k210 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(nncase_rt_modules_k210 PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libnncase.rt_modules.k210.a"
  )

list(APPEND _cmake_import_check_targets nncase_rt_modules_k210 )
list(APPEND _cmake_import_check_files_for_nncase_rt_modules_k210 "${_IMPORT_PREFIX}/lib/libnncase.rt_modules.k210.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
