#!/bin/bash

VERSION=`cat VERSION`

if [ ! -e klatexformula.app ] ; then
    echo >&2 "Can't find bundle klatexformula.app"
    exit 255
fi
if [ -e klatexformula-$VERSION.app ]; then
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

# make sure Qt won't try to find plugins in global installation of Qt on current machine
qtconf="$BUNDLE/Contents/Resources/qt.conf"
echo >>"$qtconf"
echo "[Paths]" >>"$qtconf"
echo "Plugins = plugins" >>"$qtconf"

# copy Qt libraries locally
# and update their internal cross-references
for lib in Core Gui Xml DBus
  do
  cp -R "$QTDIR/lib/Qt$lib.framework" "$BUNDLE/Contents/Frameworks"
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
for lib in Core Gui Xml DBus
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
  for lib in Core Gui Xml
    do
    install_name_tool -change "$QTDIR/lib/Qt$lib.framework/Versions/4/Qt$lib" \
	"@executable_path/../Frameworks/Qt$lib.framework/Versions/4/Qt$lib" \
	"src/plugins/lib${plugin}.dylib"
  done
done
rcc -binary -o "$BUNDLE/Contents/Resources/rccresources/klfbaseplugins.rcc" "src/plugins/klfbaseplugins_mac.qrc"


echo "Done."
echo

exit 0