#!/bin/sh
set -e
# Build ali-center .deb for Debian trixie amd64
VER=1.3.1
rm -rf build
mkdir -p build/DEBIAN build/usr/bin build/usr/share/applications
make
cp ali-center build/usr/bin/
cp ali-center.desktop ali-center-tr.desktop build/usr/share/applications/
cat > build/DEBIAN/control <<EOF
Package: ali-center
Version: $VER
Section: x11
Priority: optional
Architecture: amd64
Depends: libgtk-3-0, xfce4-terminal, thunar
Maintainer: ALI Linux <ali@localhost>
Description: ALI Center - system control panel
 XFCE-native settings hub (System/Appearance/Apps/Install/Status/Security/About).
EOF
dpkg-deb --build build "ali-center_${VER}_amd64.deb"
echo "built ali-center_${VER}_amd64.deb"
