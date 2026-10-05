# ALI Linux 1.0.6 - Debian trixie based, XFCE, amd64

Learning project. Visible rebrand only - underneath 100% Debian.

## Versions
- 1.0 - base bootable XFCE live
- 1.0.1 - (planned) Turkish KB available + ALI Center v1 separate app
- 1.0.2 - (staged) ALI Notepad v1 + ALI Terminal rebrand + full ALI string sweep
- 1.0.3 - (staged) apps baked into ISO via CI .deb + upload-artifact v5, QEMU job dropped
- 1.0.4 - (staged) XFCE defaults + GRUB/Plymouth text branding + welcome page + version bump
- 1.0.5 - (staged) live autologin + sudo nopasswd + ALI Installer label, mousepad dropped, fastfetch logo, debs 1.0.5
- 1.0.6 - release batch: version to 1.0.6 + single push (one workflow run)

## Pre-push checklist (for 1.0.6 single push)
```powershell
git add .
git status --short
git diff --cached --stat
# review, then ONE commit + push = ONE 30-40 min workflow run
git commit -m "ALI Linux 1.0.6 batch: 1.0.1-1.0.5"
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
3. Storage -> mount `ali-linux-1.0.6-amd64.hybrid.iso`
4. Start -> Try Live -> user `ali`, then double-click Debian Installer to install to disk.

QEMU (faster check):
```powershell
qemu-system-x86_64 -m 4096 -cdrom ali-linux-1.0.6-amd64.hybrid.iso -boot d -enable-hvm
# with UEFI:
qemu-system-x86_64 -m 4096 -bios "C:\Program Files\qemu\share\OVMF.fd" -cdrom ali-linux-1.0.6-amd64.hybrid.iso -boot d
```

Verify branding:
```
cat /etc/os-release
cat /etc/issue
fastfetch
```

## Structure
- `auto/config` - live-build base :auto/config:1
- `config/package-lists/ali-xfce.list.chroot` - XFCE pkgs
- `config/includes.chroot/etc/` - os-release, hostname, motd
- `config/hooks/normal/0099-ali-branding.hook.chroot` - enforces branding
- `config/hooks/normal/0990-ali-bootmenu.hook.binary` - boot menu rename
- `.github/workflows/build.yml` - GHA builder :.github/workflows/build.yml:1
