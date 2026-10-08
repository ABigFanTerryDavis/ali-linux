#!/bin/sh
set -e
# Build ali-shrine .deb for Debian trixie amd64
VER=1.3.6
rm -rf build
mkdir -p build/DEBIAN build/usr/bin
make
cp ali-shrine build/usr/bin/
cat > build/DEBIAN/control <<EOF
Package: ali-shrine
Version: $VER
Section: x11
Priority: optional
Architecture: amd64
Depends: libgtk-3-0
Maintainer: ALI Linux <ali@localhost>
Description: ALI Shrine - idle oracle screensaver
 Fullscreen oracle words on black, wakes on any key.
EOF
dpkg-deb --build build "ali-shrine_${VER}_amd64.deb"
echo "built ali-shrine_${VER}_amd64.deb"
