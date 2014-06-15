# ##################################################### #
# CMake extra definitions for klatexformula on Mac OS X
# ##################################################### #
# $Id$
# ##################################################### #

macro(KLFMessageBundleExtraDeps var textname)
  if(${var})
    message(STATUS "Will bundle library and framework dependencies into ${textname} (${var})")
  else(${var})
    message(STATUS "Will not bundle library and framework dependencies into ${textname} (${var})")
  endif(${var})
endmacro(KLFMessageBundleExtraDeps var textname)

if(APPLE AND KLF_MACOSX_BUNDLES)
  option(KLF_MACOSX_BUNDLE_EXTRAS
	"Bundle dependency libraries and frameworks into bundles (general setting)"  ON)
  KLFMessageBundleExtraDeps(KLF_MACOSX_BUNDLE_EXTRAS
	"bundles in general, unless bundle-specific settings specifies otherwise")
endif(APPLE AND KLF_MACOSX_BUNDLES)

set(KLF_BUNDLE_QT_FRAMEWORKS ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTXML_LIBRARY}
	      ${QT_QTSQL_LIBRARY} ${QT_QTDBUS_LIBRARY} CACHE STRING "Qt Frameworks on Mac OS X")
string(REGEX REPLACE "([^;]+)/([a-zA-Z0-9_-]+.framework)"
		     "${KLF_MACOSX_BUNDLE_PATH}/Contents/Frameworks/\\2"
	 klf_local_qt_frameworks "${KLF_BUNDLE_QT_FRAMEWORKS}")
set(KLF_LOCAL_QT_FRAMEWORKS "${klf_local_qt_frameworks}" CACHE INTERNAL "Qt frameworks relative to bundle")

#message("klf-qt-frameworks: ${KLF_BUNDLE_QT_FRAMEWORKS}")
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

#mark_as_advanced(KLF_BUNDLE_QT_FRAMEWORKS KLF_QT_PLUGINS)


add_custom_target(bundleclean
  # Each bundle/framework will add a rule to remove itself, adding a
  # dependency of bundleclean on their rule.
  COMMENT "Removing bundles"
)



# LOCAL/FILE will be the location inside the bundle.app/Contents/ directory
#   the split of loca path between LOCAL and FILE is irrelevant except for messages
#   displayed during build.
# FULLLOCATION is the source location.
macro(KLFMBundlePrivateImport TGT FILE FULLLOCATION LOCAL)

  # Add this to the list of sources
  list(APPEND ${TGT}_BUNDLEXTRA "${klfbundlelocation_${TGT}}/Contents/${LOCAL}/${FILE}")

  set(localtarget "${klfbundlelocation_${TGT}}/Contents/${LOCAL}/${FILE}")
  string(REGEX REPLACE "(.*)/[^/]+$" "\\1" dirnamelocaltarget ${localtarget})

  # add the command to copy the file or directory into the bundle
  add_custom_command(TARGET ${TGT}_maclibpacked PRE_BUILD
	# cmake -E ... doesn't work well enough for us, plus anyway mac OS X is a unix-like platform
	COMMAND mkdir -p "${dirnamelocaltarget}"
	COMMAND rm -Rf "${klfbundlelocation_${TGT}}/Contents/${LOCAL}/${FILE}"
	COMMAND cp -Rf "${FULLLOCATION}"
		  "${klfbundlelocation_${TGT}}/Contents/${LOCAL}/${FILE}"
        # needed for homebrew Qt with bad permissions
	COMMAND chmod -R u+w "${klfbundlelocation_${TGT}}/Contents/${LOCAL}/${FILE}"
	COMMENT "Importing ${FILE} into Mac OS X bundle"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	VERBATIM
  )
  
endmacro()

macro(KLFMInstallNameToolID TGT LIBRELPATH)
  add_custom_command(TARGET ${TGT}_maclibpacked PRE_BUILD
    COMMAND "install_name_tool" -id "@executable_path/../${LIBRELPATH}" "${klfbundlelocation_${TGT}}/Contents/${LIBRELPATH}"
    COMMENT "Updating framework or library ID of ${LIBRELPATH}"
    VERBATIM
  )
endmacro()

macro(KLFMInstallNameToolChange TGT CHANGEFILENAMEREL DEPRELPATH DEPFULLPATH)
  string(REGEX REPLACE "^.*/([A-Za-z0-9_-]+)\\.framework" "\\1.framework"
	 klfINTC_reltolibdir "${DEPFULLPATH}")
  set(klflibnamepath "${CHANGEFILENAMEREL}")
  if(NOT IS_ABSOLUTE "${klflibnamepath}")
    set(klflibnamepath "${klfbundlelocation_${TGT}}/Contents/${CHANGEFILENAMEREL}")
  endif(NOT IS_ABSOLUTE "${klflibnamepath}")
  add_custom_command(TARGET ${TGT}_maclibpacked POST_BUILD
    COMMAND "install_name_tool" -change "${DEPFULLPATH}" "@executable_path/../${DEPRELPATH}" "${klflibnamepath}"
    COMMAND "install_name_tool" -change "${klfINTC_reltolibdir}" "@executable_path/../${DEPRELPATH}" "${klflibnamepath}"
    COMMENT "Updating ${CHANGEFILENAMEREL}'s dependency on ${DEPFULLPATH} to @executable_path/../${DEPRELPATH}"
    VERBATIM
  )
endmacro()

macro(KLFMBundlePrivateLibUpdateQtDep TGT LIBNAME QTDEPS)
  foreach(dep ${QTDEPS})
    string(TOUPPER ${dep} dep_upper)
    set(qtdepfullpath "${QT_QT${dep_upper}_LIBRARY}")
    KLFMInstallNameToolChange(${TGT} ${LIBNAME}
      "Frameworks/Qt${dep}.framework/Versions/4/Qt${dep}"
      ${qtdepfullpath}/Versions/4/Qt${dep})
    KLFMInstallNameToolChange(${TGT} ${LIBNAME}
      "Frameworks/Qt${dep}.framework/Versions/Current/Qt${dep}"
      ${qtdepfullpath}/Versions/Current/Qt${dep})
  endforeach()
endmacro()

macro(KLFMBundlePrivateImportQtLib TGT QTLIBBASENAME DEPS)
  string(TOUPPER ${QTLIBBASENAME} QTLIBBASENAMEUPPER)
  set(QTLIBFULLPATH "${QT_QT${QTLIBBASENAMEUPPER}_LIBRARY}")
  KLFMBundlePrivateImport(${TGT} Qt${QTLIBBASENAME}.framework
			 "${QTLIBFULLPATH}" Frameworks)
  KLFMInstallNameToolID(${TGT}
      Frameworks/Qt${QTLIBBASENAME}.framework/Versions/4/Qt${QTLIBBASENAME})
  KLFMInstallNameToolID(${TGT}
      Frameworks/Qt${QTLIBBASENAME}.framework/Versions/Current/Qt${QTLIBBASENAME})
  KLFMBundlePrivateLibUpdateQtDep(${TGT}
      Frameworks/Qt${QTLIBBASENAME}.framework/Versions/4/Qt${QTLIBBASENAME}  "${DEPS}")
  KLFMBundlePrivateLibUpdateQtDep(${TGT}
      Frameworks/Qt${QTLIBBASENAME}.framework/Versions/Current/Qt${QTLIBBASENAME}  "${DEPS}")
endmacro()


macro(KLFMInstFrameworkUpdateId INSTALLEDLIB)
  install(CODE "execute_process(COMMAND \"install_name_tool\" -id \"${INSTALLEDLIB}\" \"$ENV{DESTDIR}${INSTALLEDLIB}\")")
endmacro()

macro(KLFMInstFrameworkUpdateLibChange INSTALLEDBIN OLDLIBID NEWLIBID)
  string(REGEX REPLACE "^.*/([A-Za-z0-9_-]+)\\.framework" "\\1.framework"
	 klfIFULC_reltolibdir "${OLDLIBID}")
  install(CODE "execute_process(COMMAND \"install_name_tool\" -change \"${OLDLIBID}\" \"${NEWLIBID}\" \"$ENV{DESTDIR}${INSTALLEDBIN}\")")
  # in case the library is in /Library/Frameworks or other system path, it does not
  # have the full path:
  install(CODE "execute_process(COMMAND \"install_name_tool\" -change \"${klfIFULC_reltolibdir}\" \"${NEWLIBID}\" \"$ENV{DESTDIR}${INSTALLEDBIN}\")")
endmacro()

macro(KLFMMakeBundle TGT)
  set_target_properties(${TGT} PROPERTIES
    MACOSX_BUNDLE	true
  )

  KLFGetTargetLocation(klfbundleexetarget ${TGT})
  set(klfbundleexetarget_${TGT} "${klfbundleexetarget}" CACHE INTERNAL "${TGT} target name")
  string(REGEX REPLACE "/Contents/MacOS/[^/]+$" "" klfbundlelocation "${klfbundleexetarget_${TGT}}")
  set(klfbundlelocation_${TGT} "${klfbundlelocation}" CACHE INTERNAL "${TGT} bundle location (path to incl. .app dir)")
  KLFGetTargetFileName(klfbundlename ${TGT})
  set(klfbundlename_${TGT} "${klfbundlename}" CACHE INTERNAL "${TGT} bundle name (abc.app)")

  # Add Proper bundle clean target 
  add_custom_target(${TGT}_clean
	COMMAND "${CMAKE_COMMAND}" -E remove_directory "${klfbundlelocation_${TGT}}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT "Removing bundle ${TGT}"
	VERBATIM
  )
  add_dependencies(bundleclean ${TGT}_clean)

  add_custom_target(${TGT}_maclibpacked
	DEPENDS ${TGT}
	COMMENT "Packaging non-system libraries into bundle ${TGT}"
  )
  
endmacro()

macro(KLFMBundleMkPrivateDir TGT SUBDIR)
  add_custom_command(TARGET ${TGT} POST_BUILD
	COMMAND mkdir -p "${SUBDIR}"
	WORKING_DIRECTORY "${klfbundlelocation_${TGT}}"
	COMMENT "Creating subdir ${SUBDIR} in ${TGT} bundle"
	VERBATIM
  )
endmacro()

macro(KLFMBundlePackage TGT)
  #KLFDeclareCacheVarOptionFollow(specificoption genericoption cachestring)
  KLFDeclareCacheVarOptionFollow(KLF_MACOSX_BUNDLE_EXTRAS_${TGT} KLF_MACOSX_BUNDLE_EXTRAS
	"Bundle dependency frameworks into bundle ${TGT}")
  KLFMessageBundleExtraDeps(KLF_MACOSX_BUNDLE_EXTRAS_${TGT} "${TGT}")

  add_dependencies(${TGT}_maclibpacked ${${TGT}_BUNDLEXTRA})
  
  if(KLF_MACOSX_BUNDLE_EXTRAS_${TGT})
    # perform ${TGT}_maclibpacked in ALL. Just make it depend on a target that is built in ALL
    add_custom_target(${TGT}_maclibpacked_all ALL )
    add_dependencies(${TGT}_maclibpacked_all ${TGT}_maclibpacked)
  endif(KLF_MACOSX_BUNDLE_EXTRAS_${TGT})
endmacro()

macro(KLFMMakeMacExtraBundledTarget TGT BUNDLETARGET DEPS)
  add_custom_target(${TGT}_maclibpacked DEPENDS ${DEPS})
  set(klfbundlelocation_${TGT} ${klfbundlelocation_${BUNDLETARGET}}
    CACHE INTERNAL "target ${TGT} to be included in ${BUNDLETARGET}")
  set(klfbundleexetarget_${TGT} ${klfbundleexetarget_${BUNDLETARGET}}
    CACHE INTERNAL "target ${TGT} to be included in ${BUNDLETARGET}")
  set(klfbundlename_${TGT} ${klfbundlename_${BUNDLETARGET}}
    CACHE INTERNAL "target ${TGT} to be included in ${BUNDLETARGET}")
endmacro(KLFMMakeMacExtraBundledTarget)

macro(KLFMMakeFramework TGT HEADERS)

  set_target_properties(${TGT} PROPERTIES
	FRAMEWORK	TRUE
  )

  KLFGetTargetLocation(klffwlibtarget ${TGT})
  set(klffwlibtarget_${TGT} "${klffwlibtarget}" CACHE INTERNAL "${TGT} target name")
  string(REGEX REPLACE "/Contents/MacOS/[^/]+$" "" klffwlocation "${klffwexetarget_${TGT}}")
  set(klffwlocation_${TGT} "${klffwlocation}" CACHE INTERNAL "${TGT} fw location (path to incl. .app dir)")
  KLFGetTargetFileName(klffwname ${TGT})
  set(klffwname_${TGT} "${klffwname}" CACHE INTERNAL "${TGT} fw name (abc.app)")

  # Add Proper bundle clean target 
  add_custom_target(${TGT}_clean
	COMMAND "${CMAKE_COMMAND}" -E remove_directory "${klffwlocation_${TGT}}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	COMMENT "Removing Framework ${TGT}"
	VERBATIM
  )
  add_dependencies(bundleclean ${TGT}_clean)

endmacro()

