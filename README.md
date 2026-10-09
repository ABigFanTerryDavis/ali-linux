# ALI Linux 1.4.9 - Debian trixie based, XFCE, amd64

Learning project. Visible rebrand only - underneath 100% Debian.

## Versions
- 1.0 - base bootable XFCE live
- 1.0.1 - (planned) Turkish KB available + ALI Center v1 separate app
- 1.0.2 - (staged) ALI Notepad v1 + ALI Terminal rebrand + full ALI string sweep
- 1.0.3 - (staged) apps baked into ISO via CI .deb + upload-artifact v5, QEMU job dropped
- 1.0.4 - (staged) XFCE defaults + GRUB/Plymouth text branding + welcome page + version bump
- 1.0.5 - (staged) live autologin + sudo nopasswd + ALI Installer label, mousepad dropped, fastfetch logo, debs 1.0.5
- 1.0.6 - released: version to 1.0.6 + single push (one workflow run)
- 1.0.7 - (staged, no push until 1.1.6) Normal + Performance boot menu entries
- 1.0.8 - (staged, no push until 1.1.6) real wallpapers (default.jpg + simple.jpg), default wired
- 1.0.9 - (staged, no push until 1.1.6) 4 new commands: ali-update, ali-info, ali-perf, ali-normal
- 1.1.0 - (staged, no push until 1.1.6) Odysseus: first-up last-down task
- 1.1.1 - (staged, no push until 1.1.6) Odysseus v2: boot-mode enforcer, watchdog, safe-rm guard
- 1.1.2 - (staged, no push until 1.1.6) Center Status tab, update notifier, us+tr keyboard, GRUB background
- 1.1.3 - (staged, no push until 1.1.6) ufw firewall, unattended-upgrades, ali-update-version (no wine)
- 1.1.4 - (staged, no push until 1.1.6) debian13/+pardus/ split, shared apps/, Pardus 25 scaffold (repos disabled, no CI yet)
- 1.1.5 - (staged, no push until 1.1.6) pardus fully ALI: synced updater, Pardus welcome/logo, fastfetch+ufw verified
- 1.1.6 - PUSHED: Pardus keyring + repos enabled + pardus CI job (2 ISOs) + b43 pin fix
- 1.2.0 - (staged, PUSHED in 1.2.8) ALI Sentinel: IDS alerts + IPS auto-block, Center Security tab, ali-sentinel CLI
- 1.2.1 - (staged, PUSHED in 1.2.8) Odysseus v4: Sentinel memory (archive+restore blocks), boot/shutdown timing, Sentinel watchdog
- 1.2.2 - (staged, PUSHED in 1.2.8) terrydavis QoL daemon: Janitor cleanup, Town crier notifications, USB announcer
- 1.2.3 - (staged, PUSHED in 1.2.8) terrydavis v2: Game mode, WiFi watchdog, Night light, Download sorter
- 1.2.4 - (staged, PUSHED in 1.2.8) templeos tribute daemon: Oracle, ali-retro 640x480 mode, Daily verse (+ Center About verse)
- 1.2.5 - (staged, PUSHED in 1.2.8) Turkish pass (Center+CLI+launchers), terrydavis v3 (Coffee+Crash), ALI Hymns app
- 1.2.6 - (staged, PUSHED in 1.2.8) ALI Task Manager: ported LTaskManager (PyQt6) as .deb + deps + menu entry
- 1.2.7 - (staged, PUSHED in 1.2.8) ALI look: mac-style dock bar (docklike), clean desktop, traffic-light windows, ALI menu logo
- 1.2.8 - PUSHED: Odysseus v5 network navigator, Pardus REMOVED, updater refresh
- 1.2.9 - (staged, PUSHED in 1.3.9) Terry's Dice security base: dice rolls, ali-passgen, ali-vault (AES-256, Sentinel ALERTs)
- 1.3.0 - (staged, PUSHED in 1.3.9) Boot identity (Plymouth splash + GRUB theme), Welcome wizard, Dice in Center, Battery brain
- 1.3.1 - (staged, PUSHED in 1.3.9) Session look (terminal theme+billboard, Conky HUD), Welcome-back ping, daily-driver pack, ltask Turkish
- 1.3.2 - (staged, PUSHED in 1.3.9) ALI-everywhere sweep, ltask true daemon names, Oracle security daemon (verdicts+lots)
- 1.3.3 - (staged, PUSHED in 1.3.9) Controllers: ltask Services tab, Center firewall, Oracle pings, zram, wallpaper rotation
- 1.3.4 - (staged, PUSHED in 1.3.9) GUI rehaul: shared dark theme, Center sidebar, per-app icons, ltask palette, Welcome banner
- 1.3.5 - (staged, PUSHED in 1.3.9) Hymns v2 (real audio + 3 songs), ltask Startup tab, Notepad v2 (TR+find), ali-backup, boot report
- 1.3.6 - (staged, PUSHED in 1.3.9) Shrine screensaver, Updates tab, Thunar actions, ali-info v2, game corner
- 1.3.7 - (staged, PUSHED in 1.3.9) Power pack: ali-mirrors, Services boot toggles, governor buttons, preload, Notepad tabs
- 1.3.8 - (staged, PUSHED in 1.3.9) Hardening: pre-push audit, Notepad print, Hymns queue, updater dry-run
- 1.3.9 - PUSHED: what's-new tour page + Welcome button, full version sweep
- 1.4.0 - (staged, PUSHED in 1.4.5) Official-ness: Flatpak+Flathub, 20 man pages, LUKS splash prompt, CLI Turkish mop-up, phone link
- 1.4.1 - (staged, PUSHED in 1.4.5) Comfort: Night Light switch, screenshot key, disk tool
- 1.4.2 - (staged, PUSHED in 1.4.5) Soul pack: lock screen, backup nanny, ali-game+MangoHud, hymn alarm, Fenn the ferret
- 1.4.3 - (staged, PUSHED in 1.4.5) THE dice logo: canonical logo.jpg wired into welcome page + wizard banner
- 1.4.4 - (staged, PUSHED in 1.4.5) Dressed in dice: dice fastfetch + About art, mini-dice menu button, greeter logo
- 1.4.5 - PUSHED: chime, CSV export, SSH limit, recents, transparency + dice logo everywhere
- 1.4.6 - (staged) TCC ships with the distro, just like gcc
- 1.4.7 - (staged) risestothrone farewell guardian + ALI Packages app
- 1.4.8 - (staged) hcc HolyC compiler + terry farewell link + memorial wallpapers
- 1.4.9 - (staged) abigfanterrydavis identity guardian + linustorvalds kernel voice

## Push checklist (1.4.5 push - THIS IS IT)
```powershell
git add .
git status --short
git diff --cached --stat
# review, then ONE commit + push = ONE 30-40 min workflow run
git commit -m "ALI Linux 1.4.x batch: 1.4.0-1.4.5"
git push -u origin main
```

## Push checklist (1.3.9 push - THIS IS IT)
```powershell
git add .
git status --short
git diff --cached --stat
# review, then ONE commit + push = ONE 30-40 min workflow run
git commit -m "ALI Linux 1.3.x batch: 1.2.9-1.3.9"
git push -u origin main
```

## 1. Push to GitHub
```powershell
cd C:\Users\Meric1\Downloads\ALIIII
git init
git add .
git commit -m "ALI Linux 1.0"
git branch -M main
git remote add origin https://github.com/<you>/ali-linux.git
git push -u origin main
```

## 2. Build
Actions tab -> `Build ALI Linux ISO` -> Run workflow.
Wait 20-40 min. Download artifact `ali-linux-iso`.

## 3. Test

VirtualBox:
1. New -> Linux Debian 64-bit, 4GB RAM, 20GB VDI
2. Settings -> System -> EFI: ON (we build grub-efi)
3. Storage -> mount `ali-linux-1.2.8-amd64.hybrid.iso`
4. Start -> Try Live -> user `ali`, then double-click Debian Installer to install to disk.

QEMU (faster check):
```powershell
qemu-system-x86_64 -m 4096 -cdrom ali-linux-1.2.8-amd64.hybrid.iso -boot d -enable-hvm
# with UEFI:
qemu-system-x86_64 -m 4096 -bios "C:\Program Files\qemu\share\OVMF.fd" -cdrom ali-linux-1.2.8-amd64.hybrid.iso -boot d
```

Verify branding:
```
cat /etc/os-release
cat /etc/issue
fastfetch
```

## Structure
- `debian13/auto/config` - Debian trixie live-build (the image CI builds)
- `apps/` - shared ALI apps (Center, Notepad, Hymns, Task Manager, Terminal, commands, Odysseus, Sentinel, terrydavis, templeos)
- `debian13/config/package-lists/ali-xfce.list.chroot` - XFCE pkgs
- `debian13/config/includes.chroot/etc/` - os-release, hostname, motd
- `debian13/config/hooks/normal/0099-ali-branding.hook.chroot` - enforces branding
- `debian13/config/hooks/normal/0990-ali-bootmenu.hook.binary` - boot menu + Normal/Performance
- `.github/workflows/build.yml` - GHA builder :.github/workflows/build.yml:1
