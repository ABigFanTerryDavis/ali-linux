# ALI Linux 1.0 - Debian trixie based, XFCE, amd64

Learning project. Visible rebrand only - underneath 100% Debian.

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
3. Storage -> mount `ali-linux-1.0-amd64.hybrid.iso`
4. Start -> Try Live -> user `ali`, then double-click Debian Installer to install to disk.

QEMU (faster check):
```powershell
qemu-system-x86_64 -m 4096 -cdrom ali-linux-1.0-amd64.hybrid.iso -boot d -enable-hvm
# with UEFI:
qemu-system-x86_64 -m 4096 -bios "C:\Program Files\qemu\share\OVMF.fd" -cdrom ali-linux-1.0-amd64.hybrid.iso -boot d
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
