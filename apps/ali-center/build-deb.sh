#!/bin/sh
set -e
# Build ali-center .deb for Debian trixie amd64
VER=1.4.5
rm -rf build
mkdir -p build/DEBIAN build/usr/bin build/usr/share/applications
make
cp ali-center build/usr/bin/
cp ali-center.desktop ali-center-tr.desktop build/usr/share/applications/
mkdir -p build/usr/share/icons/hicolor/scalable/apps
cp ../ali-style/icons/ali-center.svg build/usr/share/icons/hicolor/scalable/apps/
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
