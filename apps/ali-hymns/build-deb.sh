#!/bin/sh
set -e
# Build ali-hymns .deb for Debian trixie amd64
VER=1.3.7
rm -rf build
mkdir -p build/DEBIAN build/usr/bin build/usr/share/applications build/usr/share/icons/hicolor/scalable/apps
make
cp ali-hymns build/usr/bin/
cp ali-hymns.desktop ali-hymns-tr.desktop build/usr/share/applications/
cp ../ali-style/icons/ali-hymns.svg build/usr/share/icons/hicolor/scalable/apps/
cat > build/DEBIAN/control <<EOF
Package: ali-hymns
Version: $VER
Section: sound
Priority: optional
Architecture: amd64
Depends: libgtk-3-0, beep | sox
Maintainer: ALI Linux <ali@localhost>
Description: ALI Hymns - TempleOS-tribute chiptune player
 Three original chiptunes (Temple Morning, Oracle's Dance, 640x480).
EOF
dpkg-deb --build build "ali-hymns_${VER}_amd64.deb"
echo "built ali-hymns_${VER}_amd64.deb"
