# CMake definitions for building doxygen API documentation
# ========================================================
# $Id$


# Configure doxygen targets
# -------------------------

find_program(DOXYGEN "doxygen")
if(DOXYGEN)
  set(KLF_DOXYGEN_SF "")
  execute_process(COMMAND "${CMAKE_COMMAND}" -E "make_directory" "${CMAKE_CURRENT_BINARY_DIR}/apidoc")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.klfbackend.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klfbackend"
    IMMEDIATE @ONLY)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.klflib.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klflib"
    IMMEDIATE @ONLY)
  set(KLF_DOXYGEN_SF "_sf")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.klfbackend.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klfbackend.sfweb"
    IMMEDIATE @ONLY)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.klflib.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klflib.sfweb"
    IMMEDIATE @ONLY)
  add_custom_target(doc_klfbackend
    COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klfbackend"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Doxygen Documenation for klfbackend"
    VERBATIM
  )
  add_custom_target(doc_klflib
    COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klflib"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Doxygen Documenation for klflib"
    VERBATIM
  )
  add_custom_target(doc_klfbackend_sfweb
    COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klfbackend.sfweb"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Doxygen Documenation for klfbackend (sfweb)"
    VERBATIM
  )
  add_custom_target(doc_klflib_sfweb
    COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klflib.sfweb"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Doxygen Documenation for klflib (sfweb)"
    VERBATIM
  )

  add_custom_target(doc
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/apidoc/index.html" "index.html"
    COMMAND tar cvfj "klf-apidoc-${KLF_VERSION}.tar.bz2" klfbackend/ klflib/ index.html
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/apidoc"
    COMMENT "Creating API doc archive"
    VERBATIM
  )
  add_dependencies(doc_klflib  doc_klfbackend) # depends on klfbackend.tag
  add_dependencies(doc  doc_klfbackend doc_klflib)
  
  message(STATUS "doxygen developer API documentation can be generated with 'make doc'")
else(DOXYGEN)
  message(STATUS "doxygen not found; developer API documentation cannot be generated
  (if unsure, this warning is safe to ignore)")
endif(DOXYGEN)

