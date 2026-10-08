#!/bin/sh
set -e
VER=1.2.8
rm -rf build
mkdir -p build/DEBIAN build/usr/bin build/usr/share/applications
make
cp ali-notepad build/usr/bin/
cp ali-notepad.desktop build/usr/share/applications/
cat > build/DEBIAN/control <<EOF
Package: ali-notepad
Version: $VER
Section: editors
Priority: optional
Architecture: amd64
Depends: libgtk-3-0
Maintainer: ALI Linux <ali@localhost>
Description: ALI Notepad - minimal text editor
 C + GTK3 notepad for ALI Linux.
EOF
dpkg-deb --build build "ali-notepad_${VER}_amd64.deb"
echo "built ali-notepad_${VER}_amd64.deb"
