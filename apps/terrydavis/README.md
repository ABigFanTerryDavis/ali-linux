# terrydavis 1.3.6 - ALI QoL daemon (12 duties)

Named for Terry Davis: software with soul. Quiet background comfort:

1. **Janitor** (hourly) - `apt-get autoclean`, `/tmp` files untouched 10+
   days, trash deleted 30+ days ago, app caches unopened 14+ days.
   Reports to `/run/terrydavis-status`; notifies only if it freed 100MB+.
2. **Town crier** (hourly) - Odysseus `last_warn` changes and pending
   update counts become one desktop notification each. Nags once, then
   shuts up until something new happens. Stays quiet about updates while
   game mode is on. (Sentinel already pings its own alerts at event time,
   so the crier leaves those alone - no doubles.)
3. **USB announcer** (every 15s) - new removable block devices get a
   "plugged in" ping with mount location, or a hint to open Files.
4. **Game mode** (every 15s) - fullscreen app detected via xprop ->
   performance governor (+ quiet crier); restores the normal governor on
   quit. Performance boot entry always keeps full speed.
5. **WiFi watchdog** (every 1m) - nudges NetworkManager back up on drop,
   pings you once per outage only if it can't recover.
6. **Night light** (hourly) - redshift 5700:3500 between 21:00 and 06:00.
7. **Download sorter** (every 15s) - files `~/Downloads` by extension
   (ISOs/Pictures/Videos/Music/Archives/Documents), skipping files
   younger than 2 minutes so in-flight downloads are never touched.
8. **Coffee mode** (every 1m) - inhibits suspend while SSH sessions or
   media players are active (systemd inhibitor, released after).
9. **Crash catcher** (hourly) - fresh coredumps / /var/crash entries
   become one visible ping each (`coredumpctl list` for details).
10. **Battery brain** (every 1m) - powersave governor while discharging,
    restored on AC, urgent ping at 15% (once per discharge). Game mode
    and Performance boot entry always win.
11. **Wallpaper rotation** (hourly) - cycles `/usr/share/backgrounds/ali/`,
    skipping the moment the user picks their own. The crier also relays
    fresh Oracle `[verdict]` lines as desktop pings.
12. **Shrine keeper** (every 15s) - 10 idle minutes summons `ali-shrine`
    (fullscreen oracle on black, wakes on input). Needs `xprintidle`.

Needs: `x11-utils` (xprop), `redshift`, `network-manager` (nmcli).

Files: `terrydavis` (daemon, `--once` mode), `terrydavis.service`
(systemd, enabled by branding hook). Feed: `/run/terrydavis-status`.
