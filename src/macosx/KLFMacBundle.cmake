# ##################################################### #
# CMake extra definitions for klatexformula on Mac OS X
# ##################################################### #
# $Id: CMakeLists.txt 341 2010-03-26 20:09:49Z philippe $
# ##################################################### #

macro(KLFMakeBundle TGT BUNDLE)
  set_target_properties(${TGT} PROPERTIES
	MACOSX_BUNDLE	true
  )
  
  add_custom_command(TARGET ${TGT} POST_BUILD
	COMMAND mkdir -p Frameworks
	COMMAND mkdir -p Resources/rccresources
	COMMAND mkdir -p plugins
	COMMAND "/bin/bash" "${KLF_MACOSXDIR}/macsetupbundle.sh"
			"${CMAKE_CURRENT_BINARY_DIR}/${BUNDLE}/Contents/MacOS/${BUNDLE}"
			"${KLF_MACOSXDIR}/macChangeLibRef.pl"
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

