# ALI Notepad v2 - separate app (C + GTK3)

Tabbed notepad: New / Open (each in its own tab) / Save / Save As /
Close Tab, Find + Find Next, Ctrl+T/O/S/W/F shortcuts, Turkish via
`--tr` / `ALI_LANG=tr`. Window + dialogs all say ALI Notepad.

## Build on Debian trixie
```bash
sudo apt install libgtk-3-dev pkg-config make gcc
make
./ali-notepad
./ali-notepad somefile.txt
```

## Install
```bash
sudo make install DESTDIR=/
```

## .deb (for ISO batch, NOT yet)
```bash
./build-deb.sh
# -> ali-notepad_1.0.0_amd64.deb
```
