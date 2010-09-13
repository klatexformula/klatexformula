# CMake definitions for building doxygen API documentation
# ========================================================
# $Id$


# Configure doxygen targets
# -------------------------

find_program(DOXYGEN "doxygen")
if(DOXYGEN)

  # A non-cache variable holding the directory in which we will build the documentation
  # NOTE: Doxyfile.*.in do NOT want a trailing slash '/'.
  # NOTE: Doxyfile.*.in assume that concatenating KLF_APIDOC_DIR with KLF_DOXYGEN_SF gives the right apidoc dir.
  set(KLF_APIDOC_DIR "${CMAKE_CURRENT_BINARY_DIR}/apidoc")   # General API documentation
  set(KLF_APIDOCSF_DIR "${CMAKE_CURRENT_BINARY_DIR}/apidoc_sf")  # For my Sourceforge pages
  # Instructions to create those directories
  add_custom_target(doc_mkpath_apidoc
    COMMAND "${CMAKE_COMMAND}" -E "make_directory" "${KLF_APIDOC_DIR}"
    )
  add_custom_target(doc_mkpath_apidoc_sfweb
    COMMAND "${CMAKE_COMMAND}" -E "make_directory" "${KLF_APIDOCSF_DIR}"
    )

  # HTML doxygen API documentation (general)
  set(KLF_DOXYGEN_SF "")
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
  # HTML doxygen API documentation (for my sourceforge pages)
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

  macro(KLFMakeDoxygenTarget apidocdir targetbasename doxsuffix targetsuffix othertargetdeps)
   add_custom_command(OUTPUT "${apidocdir}/${targetbasename}.tag"
      COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.${targetbasename}${doxsuffix}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Building Doxygen Documenation for ${targetbasename}${targetsuffix}"
      VERBATIM
      )
    add_custom_target(doc_${targetbasename}${targetsuffix}
      COMMAND "${DOXYGEN}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.${targetbasename}${doxsuffix}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Building Doxygen Documenation for ${targetbasename}${targetsuffix} (forced rebuild)"
      VERBATIM
      )
    add_custom_target(doc_noautorebuild_${targetbasename}${targetsuffix} DEPENDS "${apidocdir}/${targetbasename}.tag")
    add_dependencies(doc_${targetbasename}${targetsuffix}  doc_mkpath_apidoc${targetsuffix})
    add_dependencies(doc_noautorebuild_${targetbasename}${targetsuffix}  doc_mkpath_apidoc${targetsuffix})
    foreach(dep ${othertargetdeps})
      add_dependencies(doc_${targetbasename}${targetsuffix}  doc_${dep}${targetsuffix})
      add_dependencies(doc_noautorebuild_${targetbasename}${targetsuffix}  doc_noautorebuild_${dep}${targetsuffix})
    endforeach(dep)
  endmacro(KLFMakeDoxygenTarget)

  KLFMakeDoxygenTarget("${KLF_APIDOC_DIR}" klfbackend "" ""  "")
  KLFMakeDoxygenTarget("${KLF_APIDOC_DIR}" klftools "" ""  "klfbackend")
  KLFMakeDoxygenTarget("${KLF_APIDOC_DIR}" klfapp "" ""  "klfbackend;klftools")

  KLFMakeDoxygenTarget("${KLF_APIDOCSF_DIR}" klfbackend ".sfweb" "_sfweb"  "")
  KLFMakeDoxygenTarget("${KLF_APIDOCSF_DIR}" klftools ".sfweb" "_sfweb"  "klfbackend")
  KLFMakeDoxygenTarget("${KLF_APIDOCSF_DIR}" klfapp ".sfweb" "_sfweb"  "klfbackend;klftools")


  set(klf_tar_dirname "klatexformula-apidoc-${KLF_VERSION}")
  get_filename_component(klfapidocdirname "${KLF_APIDOC_DIR}" NAME)
  add_custom_target(doc
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/apidoc/index.html" "${KLF_APIDOC_DIR}/index.html"
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/apidoc/f.gif" "${KLF_APIDOC_DIR}/f.gif"
    COMMAND "${CMAKE_COMMAND}" -E create_symlink "${klfapidocdirname}" "${klf_tar_dirname}"
    COMMAND tar cvhfj "${klf_tar_dirname}.tar.bz2" "${klf_tar_dirname}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
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





# Install Doxygen API Documentation?
# ----------------------------------
#
#KLFDeclareCacheVarOptionCondition(specificoption cachetype cachestring updatenoticestring condition forcedvalue defaultvalue)
KLFDeclareCacheVarOptionCondition(KLF_INSTALL_APIDOC_DIR
  STRING "Install API documentation to this location (relative to CMAKE_INSTALL_PREFIX or absolute)" #cachetype/string
  "Cannot install developer doxygen documentation as doxygen is not available!" # updatenotice
  "DOXYGEN" # condition
  "" # forced value
  "" # default value
  )
if(KLF_INSTALL_APIDOC_DIR)
  message(STATUS "API documentation will be installed to ${KLF_INSTALL_APIDOC_DIR} (KLF_INSTALL_APIDOC_DIR)")

  # Make sure that make all will generate doxygen doc
  add_custom_target(doc_all ALL)
  add_dependencies(doc_all  doc_noautorebuild_klfbackend doc_noautorebuild_klftools doc_noautorebuild_klfapp)

  install(DIRECTORY "${KLF_APIDOC_DIR}/klfbackend" "${KLF_APIDOC_DIR}/klftools" "${KLF_APIDOC_DIR}/klfapp"
    DESTINATION "${KLF_INSTALL_APIDOC_DIR}"
    FILES_MATCHING REGEX "\\.(html|css|png|gif|jpe?g)$")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/apidoc/index.html" "${CMAKE_CURRENT_SOURCE_DIR}/apidoc/f.gif"
    "${KLF_APIDOC_DIR}/klfbackend.tag" "${KLF_APIDOC_DIR}/klftools.tag" "${KLF_APIDOC_DIR}/klfapp.tag"
    DESTINATION "${KLF_INSTALL_APIDOC_DIR}")

else(KLF_INSTALL_APIDOC_DIR)
  message(STATUS "Developer API documentation will not be installed (KLF_INSTALL_APIDOC_DIR)")
endif(KLF_INSTALL_APIDOC_DIR)


