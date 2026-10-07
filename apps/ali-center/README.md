# ALI Center v1 - separate app (C + GTK3)

XFCE-native control panel. Shows up inside XFCE Settings Manager
via `Categories=X-XFCE-Settings`.

Tabs: System / Appearance / Apps / Install / About.
v1 launches helpers only, writes nothing (visible layer safe).

## Build on Debian trixie
```bash
sudo apt install libgtk-3-dev pkg-config make gcc thunar xfce4-terminal firefox-esr
make
./ali-center
```

## Install to system
```bash
sudo make install DESTDIR=/
# or staged: make install DESTDIR=/tmp/ali-test
```

## .deb (for ISO later)
```bash
./build-deb.sh
# -> ali-center_1.0.0_amd64.deb
```

## Add to ISO (1.0.1 batch, NOT yet)
- add `ali-center_*.deb` to `debian13/config/packages.chroot/` (CI does this automatically)
- add dep `libgtk-3-0` to `config/package-lists/ali-xfce.list.chroot`
- desktop file auto-appears in Settings Manager, no fork needed
