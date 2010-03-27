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
string(REGEX REPLACE ";[a-zA-Z0-9_/-]+_debug\\.dylib" "" klf_qtplugins_nodebug "${klf_qtplugins}")
set(KLF_QT_PLUGINS ${klf_qtplugins_nodebug} CACHE STRING "Qt plugins to include in application bundle")
string(REGEX REPLACE "([^;]+)/([a-zA-Z0-9_-]+.dylib)$" "plugins/\\1/\\2"
	klf_local_qtplugins "${KLF_QT_PLUGINS}")
set(KLF_LOCAL_QT_PLUGINS ${klf_local_qtplugins_nodebug} CACHE INTERNAL "Qt plugins relative to bundle")

#mark_as_advanced(KLF_QT_FRAMEWORKS KLF_QT_PLUGINS)


macro(KLFBundlePrivateImport TGT BUNDLE FILE FULLLOCATION LOCAL)
  message("Custom command for ${BUNDLE}/Contents/${LOCAL}/${FILE}")
  add_custom_command(OUTPUT "${BUNDLE}/Contents/${LOCAL}/${FILE}"
	COMMAND "${CMAKE_EXECUTABLE}" -E copy_directory "${FULLLOCATION}"
		  "${BUNDLE}/Contents/${LOCAL}/${FILE}"
	COMMENT "Importing ${FILE} into Mac OS X bundle"
	VERBATIM
  )
endmacro()

macro(KLFMakeBundle TGT BUNDLE)
  set_target_properties(${TGT} PROPERTIES
	MACOSX_BUNDLE	true
  )

  add_custom_command(TARGET ${TGT} PRE_BUILD
	COMMAND "${CMAKE_EXECUTABLE}" -E remove_directory "${BUNDLE}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	VERBATIM
  )
  add_custom_command(TARGET ${TGT} POST_BUILD
	COMMAND mkdir -p Contents/Resources/rccresources
#	COMMAND mkdir -p Contents/plugins
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

endmacro()

