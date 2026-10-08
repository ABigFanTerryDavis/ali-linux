# terrydavis 1.2.2 - ALI QoL daemon (duties 1 + 3 + 6, no Battery brain)

Named for Terry Davis: software with soul. Quiet background comfort:

1. **Janitor** (hourly) - `apt-get autoclean`, `/tmp` files untouched 10+
   days, trash deleted 30+ days ago, app caches unopened 14+ days.
   Reports to `/run/terrydavis-status`; notifies only if it freed 100MB+.
2. **Town crier** (hourly) - Odysseus `last_warn` changes and pending
   update counts become one desktop notification each. Nags once, then
   shuts up until something new happens. (Sentinel already pings its own
   alerts at event time, so the crier leaves those alone - no doubles.)
3. **USB announcer** (every 15s) - new removable block devices get a
   "plugged in" ping with mount location, or a hint to open Files.

Files: `terrydavis` (daemon, `--once` mode), `terrydavis.service`
(systemd, enabled by branding hook). Feed: `/run/terrydavis-status`.
