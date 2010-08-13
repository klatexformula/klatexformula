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
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.klftools.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klftools"
    IMMEDIATE @ONLY)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.klfapp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klfapp"
    IMMEDIATE @ONLY)
  set(KLF_DOXYGEN_SF "_sf")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.klfbackend.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klfbackend.sfweb"
    IMMEDIATE @ONLY)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.klftools.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klftools.sfweb"
    IMMEDIATE @ONLY)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.klfapp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klfapp.sfweb"
    IMMEDIATE @ONLY)

  add_custom_target(doc_klfbackend
    COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klfbackend"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Doxygen Documenation for klfbackend"
    VERBATIM
  )
  add_custom_target(doc_klftools
    COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klftools"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Doxygen Documenation for klftools"
    VERBATIM
  )
  add_custom_target(doc_klfapp
    COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klfapp"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Doxygen Documenation for klfapp"
    VERBATIM
  )
  add_custom_target(doc_klfbackend_sfweb
    COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klfbackend.sfweb"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Doxygen Documenation for klfbackend (sfweb)"
    VERBATIM
  )
  add_custom_target(doc_klftools_sfweb
    COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klftools.sfweb"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Doxygen Documenation for klftools (sfweb)"
    VERBATIM
  )
  add_custom_target(doc_klfapp_sfweb
    COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.klfapp.sfweb"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Doxygen Documenation for klfapp (sfweb)"
    VERBATIM
  )

  add_custom_target(doc
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/apidoc/index.html" "index.html"
    COMMAND tar cvfj "klf-apidoc-${KLF_VERSION}.tar.bz2" klfbackend/ klftools/ klfapp/ index.html
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/apidoc"
    COMMENT "Creating API doc archive"
    VERBATIM
  )

  add_dependencies(doc_klfapp  doc_klftools  doc_klfbackend) # depends on klfbackend.tag and klftools.tag
  add_dependencies(doc_klftools  doc_klfbackend) # depends on klfbackend.tag

  add_dependencies(doc  doc_klfbackend doc_klftools doc_klfapp)

  # and the sourceforge-hosted docs (internal...)
  add_dependencies(doc_klfapp_sfweb
		   doc_klftools_sfweb doc_klfbackend_sfweb) # depends on klfbackend.tag and klftools.tag
  add_dependencies(doc_klftools_sfweb doc_klfbackend_sfweb) # depends on klfbackend.tag

  message(STATUS "doxygen developer API documentation can be generated with 'make doc'")
else(DOXYGEN)
  message(STATUS "doxygen not found; developer API documentation cannot be generated
   (if unsure, this warning is safe to ignore)")
endif(DOXYGEN)

