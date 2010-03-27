#!/bin/bash

SOURCE_DIR=${1-"."}
BINARY_DIR=${2-"."}

if [ ! -e "$SOURCE_DIR/VERSION" ]; then
    echo >&2
    echo >&2 "Bad source dir: `$SOURCE_DIR'."
    echo >&2 "Usage: $0 [source_dir [binary_dir]]"
    echo >&2
    exit 255
fi

VERSION=`cat "$SOURCE_DIR/VERSION"`

if [ ! -e "klatexformula.app" ] ; then
    echo >&2 "Can't find bundle klatexformula.app"
    exit 255
fi
if [ -e "klatexformula-$VERSION.app" ]; then
    echo >&2 "Please remove preexisting bundle klatexformula-$VERSION.app"
    exit 255
fi

if [ -z "$QTDIR" ]; then
    echo >&2 "Please set QTDIR to a relevant value."
    exit 255
fi

BUNDLE="klatexformula-$VERSION.app"

cp -R klatexformula.app "$BUNDLE"

mkdir -p "$BUNDLE/Contents/Frameworks"
mkdir -p "$BUNDLE/Contents/Resources/rccresources"
mkdir -p "$BUNDLE/Contents/plugins"
# TODO :.......Icon klficon.icns, PkgInfo file............
cp "$SOURCE_DIR/src/macosx/qt.conf" "$BUNDLE/Contents/Resources/qt.conf"

# copy Qt libraries locally
# and update their internal cross-references
for lib in Core Gui Xml DBus Sql
  do
  cp -R "$QTDIR/lib/Qt$lib.framework" "$BUNDLE/Contents/Frameworks"
  # don't take up megabytes and remove the debug libraries.
  rm "$BUNDLE/Contents/Frameworks/Qt$lib.framework/Qt${lib}_debug" \
      "$BUNDLE/Contents/Frameworks/Qt$lib.framework/Qt${lib}_debug.prl" \
      "$BUNDLE/Contents/Frameworks/Qt$lib.framework/Versions/4/Qt${lib}_debug"
  
  # update copied library's ID
  install_name_tool -id "@executable_path/../Frameworks/Qt$lib.framework/Versions/4/Qt$lib" \
      "$BUNDLE/Contents/Frameworks/Qt$lib.framework/Versions/4/Qt$lib"
    
  # adjust internal Qt modules inter-dependencies:
  # dependence on QtCore if $lib!=Core
  if [ "$lib" != "Core" ] ; then
      install_name_tool -change "$QTDIR/lib/QtCore.framework/Versions/4/QtCore" \
	  "@executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore" \
	  "$BUNDLE/Contents/Frameworks/Qt$lib.framework/Versions/4/Qt$lib"
  fi
  # dependence of QtDBus on QtXml
  if [ "$lib" == "DBus" ] ; then
      install_name_tool -change "$QTDIR/lib/QtXml.framework/Versions/4/QtXml" \
	  "@executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml" \
	  "$BUNDLE/Contents/Frameworks/QtDBus.framework/Versions/4/QtDBus"
  fi
done

# update the dependency of target(s) to qt libraries
#   klatexformula executable:
for lib in Core Gui Xml DBus Sql
do
  install_name_tool -change "$QTDIR/lib/Qt$lib.framework/Versions/4/Qt$lib" \
      "@executable_path/../Frameworks/Qt$lib.framework/Versions/4/Qt$lib" \
      "$BUNDLE/Contents/MacOS/klatexformula"
done

# copy Qt (imageformat) plugins locally
for subdir in accessible codecs imageformats
do
  mkdir -p "$BUNDLE/Contents/plugins/$subdir"
  for plugin in `ls "$QTDIR/plugins/$subdir/"*.dylib | grep -v '_debug'`
  do
    cp -R "$plugin" "$BUNDLE/Contents/plugins/$subdir/"
    pluginbase=`basename "$plugin"`
    for lib in Core Gui Xml
      do
      install_name_tool -change "$QTDIR/lib/Qt$lib.framework/Versions/4/Qt$lib" \
	  "@executable_path/../Frameworks/Qt$lib.framework/Versions/4/Qt$lib" \
	  "$BUNDLE/Contents/plugins/$subdir/$pluginbase"
      done
  done
done

for plugin in skin systrayicon
  do
  for lib in Core Gui Xml Sql
    do
    install_name_tool -change "$QTDIR/lib/Qt$lib.framework/Versions/4/Qt$lib" \
	"@executable_path/../Frameworks/Qt$lib.framework/Versions/4/Qt$lib" \
	"src/plugins/lib${plugin}.so"
  done
done

# klfbaseplugins.rcc will be added by the CMake script after compiling the plugins.

echo "Done."
echo

exit 0