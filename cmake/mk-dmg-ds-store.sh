
# usage:  mk-dmg-ds-store <klf-version>

#
# Utility to create a .DS_Store file for klatexformula bundle. This is not part of the
# cmake/cpack build process, you need to do this manually.
#
# Usage: ./mk-dmg-ds-store.sh <klf-version>
#
# The script fiddles a bit, does lots of hacks, and creates a file "DS_Store.created".
# To integrate it into the cmake/cpack dmg package generation, rename the file to
# DS_Store. Note that DS_Store is in SVN version control for convenience.
#
# Note: you have to be in this directory so that this script gets the paths right.
#

#
# creates a temp disk image, sets up the DS_Store, and then extracts it and saves it as
# DS_Store.
#
# Based on http://stackoverflow.com/a/1513578/1694896 . Check Step #1 there on your system!!
#

VERSION=${1-VERSIONMISSING}
source=KLatexFormula-${VERSION}
title=${source}

applicationName=klatexformula.app

size=10000 # kB

tempdmg=pack.temp.dmg

# start the job

if [ -e "${source}" -o -e "${tempdmg}" ]; then
    echo >&2 "Source ${source} and/or DMG ${tempdmg} exist! Remove them first, please."
    exit 255
fi

if [ -e "/Volumes/${title}" ]; then
    echo >&2 "Another image is already mounted as /Volumes/${title}. Please unmount it first."
    exit 255
fi


#
# Create a dummy klatexformula.app, but with correct icon etc., so that it can
# be placed and everything.
#
mkdir -p ${source}/${applicationName}/Contents/MacOS
mkdir -p ${source}/${applicationName}/Contents/Resources
# valid binary file
cp -a /bin/echo ${source}/${applicationName}/Contents/MacOS/klatexformula
chmod +x ${source}/${applicationName}/Contents/MacOS/klatexformula
echo '<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
  <dict>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleExecutable</key>
    <string>klatexformula</string>
    <key>CFBundleIdentifier</key>
    <string>org.klatexformula.klatexformula</string>
    <key>CFBundleName</key>
    <string>klatexformula</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>'"${VERSION}"'</string>
    <key>CFBundleSignature</key>
    <string>????</string>
    <key>CFBundleVersion</key>
    <string>'"${VERSION}"'</string>
    <key>CFBundleIconFile</key>
    <string>klatexformula.icns</string>
  </dict>
</plist>
' >${source}/${applicationName}/Contents/Info.plist
cp -a ../src/macosx/klatexformula.icns ${source}/${applicationName}/Contents/Resources

hdiutil create -srcfolder "${source}" -volname "${title}" -fs HFS+ \
      -fsargs "-c c=64,a=16,e=16" -format UDRW -size ${size}k ${tempdmg}



device=$(hdiutil attach -readwrite -noverify -noautoopen "${tempdmg}" | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')

echo "Device is ${device}"

sleep 5

mkdir -p /Volumes/"${title}"/.background
cp installer_dragndrop_bg.png /Volumes/"${title}"/.background/background.png

echo '
   tell application "Finder"
     tell disk "'"${title}"'"
           open
           set current view of container window to icon view
           set toolbar visible of container window to false
           set statusbar visible of container window to false
           set the bounds of container window to {400, 100, 1007, 455}
           set theViewOptions to the icon view options of container window
           set arrangement of theViewOptions to not arranged
           set icon size of theViewOptions to 120
           set background picture of theViewOptions to file ".background:background.png"
           make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
           close
           open
           -- set position of item "background.png" of container window to {1000, 1000}
           set position of item "'"${applicationName}"'" of container window to {170, 150}
           set position of item "Applications" of container window to {450, 150}
           close
           open
           update without registering applications
           delay 15
           -- do shell script "cp .DS_Store '"`pwd`/DS_Store.created"'" -- somehow this is not an up to date file
           eject
     end tell
   end tell
' | osascript



#chmod -Rf go-w /Volumes/"${title}"
sync
sync

#cp /Volumes/"${title}"/.DS_Store ./DS_Store.created

hdiutil detach ${device}
#hdiutil convert "/${tempdmg}" -format UDZO -imagekey zlib-level=9 -o "${finalDMGName}"
#rm -f /${tempdmg}

sleep 2

# re-mount the device to get the .DS_Store file

device=$(hdiutil attach -readwrite -noverify -noautoopen "${tempdmg}" | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')

cp /Volumes/"${title}"/.DS_Store "`pwd`/DS_Store.created"

hdiutil detach ${device}

echo "Done, your DS_Store is ready and saved as DS_Store.created"
