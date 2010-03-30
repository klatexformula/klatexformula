# ##################################################### #
# CMake extra definitions for klatexformula on Mac OS X
# ##################################################### #
# $Id: CMakeLists.txt 341 2010-03-26 20:09:49Z philippe $
# ##################################################### #

set(KLF_QT_FRAMEWORKS ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY}
	      ${QT_QTSQL_LIBRARY} ${QT_QTDBUS_LIBRARY} CACHE STRING "Qt Frameworks on Mac OS X")
string(REGEX REPLACE "([^;]+)/([a-zA-Z0-9_-]+.framework)"
		     "${KLF_MACOSX_BUNDLE_PATH}/Contents/Frameworks/\\2"
	 klf_local_qt_frameworks "${KLF_QT_FRAMEWORKS}")
set(KLF_LOCAL_QT_FRAMEWORKS "${klf_local_qt_frameworks}" CACHE INTERNAL "Qt frameworks relative to bundle")

message("klf-qt-frameworks: ${KLF_QT_FRAMEWORKS}")
message("klf-local-qt-frameworks: ${KLF_LOCAL_QT_FRAMEWORKS}")

file(GLOB klf_qtimageformatplugins RELATIVE "${QT_PLUGINS_DIR}" "imageformats/*.dylib")
file(GLOB klf_qtcodecsplugins RELATIVE "${QT_PLUGINS_DIR}" "codecs/*.dylib")
file(GLOB klf_sqldriversplugins RELATIVE "${QT_PLUGINS_DIR}" "sqldrivers/*.dylib")
set(klf_qtplugins ${klf_qtimageformatplugins} ${klf_qtcodecsplugins} ${klf_sqldriersplugins})
#string(REGEX REPLACE ";[a-zA-Z0-9_/-]+_debug\\.dylib" "" klf_qtplugins_nodebug "${klf_qtplugins}")
set(klf_qtplugins_nodebug "${klf_qtplugins}") # keep debug libraries & plugins...
set(KLF_QT_PLUGINS ${klf_qtplugins_nodebug} CACHE STRING "Qt plugins to include in application bundle")
string(REGEX REPLACE "([^;]+)/([a-zA-Z0-9_-]+.dylib)$" "plugins/\\1/\\2"
	klf_local_qtplugins "${KLF_QT_PLUGINS}")
set(KLF_LOCAL_QT_PLUGINS ${klf_local_qtplugins_nodebug} CACHE INTERNAL "Qt plugins relative to bundle")

#mark_as_advanced(KLF_QT_FRAMEWORKS KLF_QT_PLUGINS)


add_custom_target(bundleclean
  # Each bundle/framework will add a rule to remove itself, adding a
  # dependency of bundleclean on their rule.
  COMMENT "Removing bundles"
)



macro(KLFBundlePrivateImport TGT BUNDLE VAR_SRCS FILE FULLLOCATION LOCAL)

  # Add this to the list of sources
  list(APPEND ${VAR_SRCS} "${BUNDLE}/Contents/${LOCAL}/${FILE}")

  message("Custom command for ${BUNDLE}/Contents/${LOCAL}/${FILE}")
  # Determine if the file is a regular file or a directory
  if(IS_DIRECTORY "${FULLLOCATION}")
    set(copycmd "copy_directory")
  else(IS_DIRECTORY "${FULLLOCATION}")
    set(copycmd "copy")
  endif(IS_DIRECTORY "${FULLLOCATION}")
  # And add the command to copy the file or directory into the bundle
  add_custom_command(OUTPUT "${BUNDLE}/Contents/${LOCAL}/${FILE}"
	COMMAND "${CMAKE_EXECUTABLE}" -E "${copycmd}" "${FULLLOCATION}"
		  "${BUNDLE}/Contents/${LOCAL}/${FILE}"
	COMMENT "Importing ${FILE} into Mac OS X bundle"
	VERBATIM
  )
  
endmacro()

macro(KLFInstallNameToolID TGT BUNDLE LIBRELPATH)
  add_custom_target(TARGET ${TGT} POST_BUILD
    COMMAND "install_name_tool -id \"@executable_path/../${LIBRELPATH}\" \"${BUNDLE}/Contents/${LIBRELPATH}\""
    COMMENT "Updating framework or library ID of ${LIBRELPATH}"
    VERBATIM
  )
endmacro()

macro(KLFInstallNameToolChange TGT CHANGEFILENAME BUNDLEPATH DEPRELPATH DEPFULLPATH)
  add_custom_target(TARGET ${TGT} POST_BUILD
    COMMAND "install_name_tool -change \"${DEPFULLPATH}\" \"@executable_path/../${DEPRELPATH}\" \"${CHANGEFILENAME}\""
    COMMENT "Updating ${CHANGEFILENAME}'s dependency on ${DEPFULLPATH} to @executable_path/../${DEPRELPATH}"
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
  string(TO_UPPER ${QTLIBBASENAME} QTLIBBASENAMEUPPER)
  set(QTLIBFULLPATH "${QT_QT${QTLIBBASENAMEUPPER}_LIBRARY}")
  KLFBundlePrivateImport(${TGT} ${BUNDLE} ${VAR_SRCS} Qt${QTLIBBASENAME}.framework
			 ${QTLIBFULLPATH} Frameworks)
  KLFInstallNameToolID(${TGT} ${BUNDLE}
      Frameworks/Qt${QTLIBBASENAME}.framework/Versions/4/Qt${QTLIBBASENAME})
  KLFBundlePrivateLibUpdateQtDep(${TGT} ${BUNDLE}
      Frameworks/Qt${QTLIBBASENAME}.framework/Versions/4/Qt${QTLIBBASENAME}  ${DEPS})
endmacro()


macro(KLFInstFrameworkUpdateId INSTALLEDLIB)
  install(CODE "execute_process(COMMAND \"install_name_tool -id \\\"${INSTALLEDLIB}\\\" \\\"${INSTALLEDLIB}\\\"\")")
endmacro()

macro(KLFInstFrameworkUpdateLibChange INSTALLEDBIN OLDLIBID NEWLIBID)
  install(CODE "execute_process(COMMAND \"install_name_tool -change \\\"${OLDLIBID}\\\" \\\"${NEWLIBID}\\\" \\\"${INSTALLEDBIN}\\\"\")")
endmacro()

macro(KLFMakeBundle TGT BUNDLE)
  set_target_properties(${TGT} PROPERTIES
	MACOSX_BUNDLE	true
  )

  # Add Proper bundle clean target 
  add_custom_target(${TGT}_clean
	COMMAND "${CMAKE_EXECUTABLE}" -E remove_directory "${BUNDLE}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	VERBATIM
  )
  add_dependencies(bundleclean ${TGT}_clean)
  
  add_custom_command(TARGET ${TGT} POST_BUILD
	COMMAND mkdir -p Contents/Resources/rccresources
#	COMMAND "/bin/bash" "${KLF_MACOSXDIR}/macsetupbundle.sh"
#			"${CMAKE_CURRENT_BINARY_DIR}/${BUNDLE}/Contents/MacOS/${BUNDLE}"
#			"${KLF_MACOSXDIR}/macChangeLibRef.pl"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${BUNDLE}"
	COMMENT "Setting up bundled target ${BUNDLE}"
	VERBATIM
  )
endmacro()

macro(KLFMakeFramework TGT HEADERS)

  set_target_properties(${TGT} PROPERTIES
	FRAMEWORK	true
  )

  # Add Proper bundle clean target 
  add_custom_target(${TGT}_clean
	COMMAND "${CMAKE_EXECUTABLE}" -E remove_directory "${TGT}.framework"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	VERBATIM
  )
  add_dependencies(bundleclean ${TGT}_clean)

endmacro()

