#!/bin/sh
set -e
# Build ali-packages .deb for Debian trixie amd64
VER=1.4.7
rm -rf build
mkdir -p build/DEBIAN build/usr/bin build/usr/share/applications
make
cp ali-packages build/usr/bin/
cp ali-packages.desktop ali-packages-tr.desktop build/usr/share/applications/
cat > build/DEBIAN/control <<EOF
Package: ali-packages
Version: $VER
Section: admin
Priority: optional
Architecture: amd64
Depends: libgtk-3-0, apt
Maintainer: ALI Linux <ali@localhost>
Description: ALI Packages - apt face for ALI Linux
 Search, details, install/remove, ALI family pinned, updates header.
EOF
dpkg-deb --build build "ali-packages_${VER}_amd64.deb"
echo "built ali-packages_${VER}_amd64.deb"
