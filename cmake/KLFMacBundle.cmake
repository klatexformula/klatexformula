# ##################################################### #
# CMake extra definitions for klatexformula on Mac OS X
# ##################################################### #
# $Id$
# ##################################################### #

set(KLF_QT_FRAMEWORKS ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY}
	      ${QT_QTSQL_LIBRARY} ${QT_QTDBUS_LIBRARY} CACHE STRING "Qt Frameworks on Mac OS X")
string(REGEX REPLACE "([^;]+)/([a-zA-Z0-9_-]+.framework)"
		     "${KLF_MACOSX_BUNDLE_PATH}/Contents/Frameworks/\\2"
	 klf_local_qt_frameworks "${KLF_QT_FRAMEWORKS}")
set(KLF_LOCAL_QT_FRAMEWORKS "${klf_local_qt_frameworks}" CACHE INTERNAL "Qt frameworks relative to bundle")

#message("klf-qt-frameworks: ${KLF_QT_FRAMEWORKS}")
#message("klf-local-qt-frameworks: ${KLF_LOCAL_QT_FRAMEWORKS}")

file(GLOB klf_qtimageformatplugins "${QT_PLUGINS_DIR}/imageformats/*.dylib")
file(GLOB klf_qtcodecsplugins "${QT_PLUGINS_DIR}/codecs/*.dylib")
file(GLOB klf_sqldriversplugins "${QT_PLUGINS_DIR}/sqldrivers/*.dylib")
set(klf_qtplugins ${klf_qtimageformatplugins} ${klf_qtcodecsplugins} ${klf_sqldriversplugins})
# string(REGEX REPLACE ";[a-zA-Z0-9_/-]+_debug\\.dylib" "" klf_qtplugins_nodebug "${klf_qtplugins}")
set(klf_qtplugins_rel "")
foreach (p ${klf_qtplugins})
#  message("p is ${p}")
  if (p MATCHES "_debug.dylib")
  else(p MATCHES "_debug.dylib")
    file(RELATIVE_PATH r "${QT_PLUGINS_DIR}" ${p})
    list(APPEND klf_qtplugins_rel "${r}")
  endif(p MATCHES "_debug.dylib")
endforeach()
#message("After relative: ${klf_qtplugins_rel}")
set(KLF_QT_PLUGINS ${klf_qtplugins_rel} CACHE STRING "Qt plugins to include in application bundle")
set(klf_local_qtplugins "")
foreach (p ${KLF_QT_PLUGINS})
  list(APPEND klf_local_qtplugins "plugins/${p}")
endforeach()
# string(REGEX REPLACE "([^;]+)/([a-zA-Z0-9_-]+.dylib)$" "plugins/\\1/\\2"
# 	klf_local_qtplugins "${KLF_QT_PLUGINS}")
set(KLF_LOCAL_QT_PLUGINS ${klf_local_qtplugins} CACHE INTERNAL "Qt plugins relative to bundle")

#message("Qt Plugins to package ${QT_PLUGINS_DIR} : ${KLF_QT_PLUGINS}, ... ${KLF_LOCAL_QT_PLUGINS}")

#mark_as_advanced(KLF_QT_FRAMEWORKS KLF_QT_PLUGINS)


add_custom_target(bundleclean
  # Each bundle/framework will add a rule to remove itself, adding a
  # dependency of bundleclean on their rule.
  COMMENT "Removing bundles"
)



macro(KLFBundlePrivateImport TGT BUNDLE VAR_SRCS FILE FULLLOCATION LOCAL)

  # Add this to the list of sources
  list(APPEND ${VAR_SRCS} "${BUNDLE}/Contents/${LOCAL}/${FILE}")

  set(localtarget "${BUNDLE}/Contents/${LOCAL}/${FILE}")
  string(REGEX REPLACE "(.*)/[^/]+$" "\\1" dirnamelocaltarget ${localtarget})

  # add the command to copy the file or directory into the bundle
  add_custom_command(TARGET ${TGT}_maclibpacked PRE_BUILD
#	OUTPUT "${BUNDLE}/Contents/${LOCAL}/${FILE}"
	COMMAND mkdir -p "${dirnamelocaltarget}"
	COMMAND rm -Rf "${BUNDLE}/Contents/${LOCAL}/${FILE}"
	COMMAND cp -Rf "${FULLLOCATION}"
		  "${BUNDLE}/Contents/${LOCAL}/${FILE}"
	COMMENT "Importing ${FILE} into Mac OS X bundle"
	VERBATIM
  )
  
endmacro()

macro(KLFInstallNameToolID TGT BUNDLE LIBRELPATH)
  add_custom_command(TARGET ${TGT}_maclibpacked PRE_BUILD
    COMMAND "install_name_tool" -id "@executable_path/../${LIBRELPATH}" "${BUNDLE}/Contents/${LIBRELPATH}"
    COMMENT "Updating framework or library ID of ${LIBRELPATH}"
    VERBATIM
  )
endmacro()

macro(KLFInstallNameToolChange TGT CHANGEFILENAMEREL BUNDLEPATH DEPRELPATH DEPFULLPATH)
  add_custom_command(TARGET ${TGT}_maclibpacked POST_BUILD
    COMMAND "install_name_tool" -change "${DEPFULLPATH}" "@executable_path/../${DEPRELPATH}" "${BUNDLEPATH}/Contents/${CHANGEFILENAMEREL}"
    COMMENT "Updating ${CHANGEFILENAMEREL}'s dependency on ${DEPFULLPATH} to @executable_path/../${DEPRELPATH}"
    VERBATIM
  )
endmacro()

macro(KLFBundlePrivateLibUpdateQtDep TGT BUNDLE LIBNAME QTDEPS)
  foreach(dep ${QTDEPS})
    string(TOUPPER ${dep} dep_upper)
    set(qtdepfullpath "${QT_QT${dep_upper}_LIBRARY}")
    KLFInstallNameToolChange(${TGT} ${LIBNAME}
	"${BUNDLE}" "Frameworks/Qt${dep}.framework/Versions/4/Qt${dep}"
	${qtdepfullpath}/Versions/4/Qt${dep})
  endforeach()
endmacro()

macro(KLFBundlePrivateImportQtLib TGT BUNDLE VAR_SRCS QTLIBBASENAME DEPS)
  string(TOUPPER ${QTLIBBASENAME} QTLIBBASENAMEUPPER)
  set(QTLIBFULLPATH "${QT_QT${QTLIBBASENAMEUPPER}_LIBRARY}")
  KLFBundlePrivateImport(${TGT} ${BUNDLE} ${VAR_SRCS} Qt${QTLIBBASENAME}.framework
			 ${QTLIBFULLPATH} Frameworks)
  KLFInstallNameToolID(${TGT} ${BUNDLE}
      Frameworks/Qt${QTLIBBASENAME}.framework/Versions/4/Qt${QTLIBBASENAME})
  KLFBundlePrivateLibUpdateQtDep(${TGT} ${BUNDLE}
      Frameworks/Qt${QTLIBBASENAME}.framework/Versions/4/Qt${QTLIBBASENAME}  "${DEPS}")
endmacro()


macro(KLFInstFrameworkUpdateId INSTALLEDLIB)
  install(CODE "execute_process(COMMAND \"install_name_tool\" -id \"${INSTALLEDLIB}\" \"${INSTALLEDLIB}\")")
endmacro()

macro(KLFInstFrameworkUpdateLibChange INSTALLEDBIN OLDLIBID NEWLIBID)
  install(CODE "execute_process(COMMAND \"install_name_tool\" -change \"${OLDLIBID}\" \"${NEWLIBID}\" \"${INSTALLEDBIN}\")")
endmacro()

macro(KLFMakeBundle TGT BUNDLE)
  set_target_properties(${TGT} PROPERTIES
	MACOSX_BUNDLE	true
  )

  # Add Proper bundle clean target 
  add_custom_target(${TGT}_clean
	COMMAND "${CMAKE_COMMAND}" -E remove_directory "${BUNDLE}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT "Removing bundle ${TGT}"
	VERBATIM
  )
  add_dependencies(bundleclean ${TGT}_clean)

  add_custom_target(${TGT}_maclibpacked ALL
	DEPENDS ${TGT}
	COMMENT "Packaging non-system libraries into bundle ${TGT}"
  )
  
  add_custom_command(TARGET ${TGT} POST_BUILD
	COMMAND mkdir -p Contents/Resources/rccresources
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${BUNDLE}"
	COMMENT "Setting up bundled target ${BUNDLE}"
	VERBATIM
  )
endmacro()

macro(KLFBundlePackage TGT BUNDLEXTRA)
  message("Extras: ${BUNDLEXTRA}")
  add_dependencies(${TGT}_maclibpacked ${BUNDLEXTRA})
endmacro()

macro(KLFMakeFramework TGT HEADERS)

  set_target_properties(${TGT} PROPERTIES
	FRAMEWORK	TRUE
  )

  # Add Proper bundle clean target 
  add_custom_target(${TGT}_clean
	COMMAND "${CMAKE_COMMAND}" -E remove_directory "${TGT}.framework"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT "Removing Framework ${TGT}"
	VERBATIM
  )
  add_dependencies(bundleclean ${TGT}_clean)

endmacro()

