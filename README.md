# ALI Linux 1.1.6 - Debian trixie based, XFCE, amd64 (+ Pardus variant)

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
- 1.1.6 - PUSH: Pardus keyring + repos enabled + pardus CI job (2 ISOs)

## Pre-push checklist (1.1.6 push)
```powershell
git add .
git status --short
git diff --cached --stat
# review, then ONE commit + push = ONE 30-40 min workflow run
git commit -m "ALI Linux 1.1.6 batch: 1.0.7-1.1.6 + Pardus variant"
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
3. Storage -> mount `ali-linux-1.1.6-amd64.hybrid.iso`
4. Start -> Try Live -> user `ali`, then double-click Debian Installer to install to disk.

QEMU (faster check):
```powershell
qemu-system-x86_64 -m 4096 -cdrom ali-linux-1.1.6-amd64.hybrid.iso -boot d -enable-hvm
# with UEFI:
qemu-system-x86_64 -m 4096 -bios "C:\Program Files\qemu\share\OVMF.fd" -cdrom ali-linux-1.1.6-amd64.hybrid.iso -boot d
```

Verify branding:
```
cat /etc/os-release
cat /etc/issue
fastfetch
```

## Structure
- `debian13/auto/config` - Debian trixie live-build (the image CI builds)
- `pardus/` - Pardus 25 variant scaffold (EXPERIMENTAL, no CI yet - see `pardus/README-VARIANT.md`)
- `apps/` - shared ALI apps (Center, Notepad, Terminal, commands, Odysseus)
- `debian13/config/package-lists/ali-xfce.list.chroot` - XFCE pkgs
- `debian13/config/includes.chroot/etc/` - os-release, hostname, motd
- `debian13/config/hooks/normal/0099-ali-branding.hook.chroot` - enforces branding
- `debian13/config/hooks/normal/0990-ali-bootmenu.hook.binary` - boot menu + Normal/Performance
- `.github/workflows/build.yml` - GHA builder :.github/workflows/build.yml:1
