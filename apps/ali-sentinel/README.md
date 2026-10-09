# ali-sentinel 1.2.0 - ALI Sentinel (IDS + IPS, system style)

In-house watcher, not a repackaged suricata. Two halves:

- IDS (finds + alerts): brute-force login attempts, new listening ports,
  processes running deleted binaries / known-bad names, watched system file
  changes (`/etc/passwd`, `sudoers`, `sshd_config`...). Alerts via desktop
  notification + `/run/ali-security` feed + `/var/log/sentinel.log`.
- IPS (exterminates): >=5 failed logins from one IP inside a scan window
  gets `ufw deny` automatically, and 10+ firewall-blocked knocks names
  a port-scanner for the same treatment. Suspicious processes are alert-only with a
  one-click kill (`ali-sentinel kill <pid>`) - blind auto-kill is how you
  shoot your own foot.

Files: `sentinel` (daemon, `--once` / `learn` modes), `ali-sentinel` (CLI),
`sentinel.service` (systemd, enabled by branding hook).
