# Odysseus v2 - first task to start, last to end

Duties:
1. **Boot-mode enforcer** - reads `/proc/cmdline`; `mitigations=off`
   (Performance boot entry) -> governor `performance`, else default.
   Re-enforced every 5 min in performance mode.
2. **Health watchdog** - every 5 min: disk / over 90%, RAM over 90%,
   failed systemd units -> warnings in `/var/log/odysseus.log`.
3. **safe-rm guard** (`ali-safe-rm.sh`) - interactive-shell accident guard
   refusing `rm -rf /`-style wipes. NOT a security boundary
   (`command rm` bypasses by design).

A systemd service ordered before `sysinit.target` (so it starts first)
and before `shutdown.target` with no default deps (so it stops last).
v1 idles and logs boot/shutdown markers to `/var/log/odysseus.log`.

Check on a live system:
```bash
systemctl status odysseus
cat /var/log/odysseus.log
systemd-analyze blame | tail   # odysseus near the top (started first)
```

## In the ISO (1.1.0 batch, NOT yet pushed)
- `odysseus` -> `config/includes.chroot/usr/bin/odysseus` (+x via `0099` hook)
- `odysseus.service` -> `config/includes.chroot/usr/lib/systemd/system/`
- enabled by symlink in `sysinit.target.wants` (created by `0099` hook,
  git on Windows can't store the symlink itself)
