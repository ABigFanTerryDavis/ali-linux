# ALI commands v1 - tiny shell helpers, no compile

- `ali-update` - apt update + full-upgrade + autoremove + clean
- `ali-info` - os-release + fastfetch + disk/RAM splash
- `ali-perf` - runtime Performance mode (CPU governor)
- `ali-normal` - back to Normal mode (default governor)
- `ali-update-version <target>` - in-place ALI-layer upgrade (e.g. 1.1.6 to 1.2.6), no format; pulls the vX.Y.Z tag tarball, rebuilds apps, stamps version

Full performance = boot entry `Performance Mode` (also drops CPU mitigations,
which can only change at boot).

## Try on Debian trixie
```bash
chmod +x ali-update ali-info ali-perf ali-normal
sudo cp ali-* /usr/bin/
ali-info
```

## In the ISO (1.0.9 batch, NOT yet pushed)
Copies live in `config/includes.chroot/usr/bin/`, made +x by the `0099` hook.
