#!/bin/sh
set -e
# Build ali-welcome .deb for Debian trixie amd64
VER=1.3.4
rm -rf build
mkdir -p build/DEBIAN build/usr/bin build/usr/share/applications build/usr/share/icons/hicolor/scalable/apps
make
cp ali-welcome build/usr/bin/
cp ali-welcome.desktop build/usr/share/applications/
cp ../ali-style/icons/ali-welcome.svg build/usr/share/icons/hicolor/scalable/apps/
cat > build/DEBIAN/control <<EOF
Package: ali-welcome
Version: $VER
Section: x11
Priority: optional
Architecture: amd64
Depends: libgtk-3-0
Maintainer: ALI Linux <ali@localhost>
Description: ALI Welcome - first-run wizard
 Bilingual greeting (TR/EN): WiFi, updates, Center, tour, login toggle.
EOF
dpkg-deb --build build "ali-welcome_${VER}_amd64.deb"
echo "built ali-welcome_${VER}_amd64.deb"
