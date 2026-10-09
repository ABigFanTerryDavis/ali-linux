#!/bin/sh
set -e
# Build ali-fail .deb for Debian trixie amd64
VER=1.5.0
rm -rf build
mkdir -p build/DEBIAN build/usr/bin build/usr/lib/systemd/system
make
cp ali-fail ali-fail-screen build/usr/bin/
cp ali-fail@.service build/usr/lib/systemd/system/
cat > build/DEBIAN/control <<EOF
Package: ali-fail
Version: $VER
Section: admin
Priority: optional
Architecture: amd64
Depends: libgtk-3-0
Maintainer: ALI Linux <ali@localhost>
Description: ALI Fail - the honest blue screen
 Fullscreen failure alert per daemon, with restart/log/continue.
EOF
dpkg-deb --build build "ali-fail_${VER}_amd64.deb"
echo "built ali-fail_${VER}_amd64.deb"
