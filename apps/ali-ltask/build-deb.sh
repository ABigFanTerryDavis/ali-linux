#!/bin/sh
set -e
# Build ali-ltask .deb (pure Python port, no compile step)
VER=1.2.8
rm -rf build
mkdir -p build/DEBIAN build/usr/bin build/usr/share/ali-ltask build/usr/share/applications
cp ltaskmanager.py build/usr/share/ali-ltask/
cp ali-ltask build/usr/bin/
cp ali-ltask.desktop build/usr/share/applications/
cat > build/DEBIAN/control <<EOF
Package: ali-ltask
Version: $VER
Section: utils
Priority: optional
Architecture: all
Depends: python3, python3-pyqt6, python3-psutil, python3-pyqtgraph
Maintainer: ALI Linux <ali@localhost>
Description: ALI Task Manager - process + performance GUI
 Port of LTaskManager (MerixCipher, MIT) with ALI branding.
 Processes tab (end/suspend/tree, details) + Performance graphs.
EOF
dpkg-deb --build build "ali-ltask_${VER}_all.deb"
echo "built ali-ltask_${VER}_all.deb"
