# templeos 1.2.4 - ALI tribute daemon (duties 1 + 3 + 4 of the pitch)

The fun daemon. Soul, not chores:

1. **Oracle** - boot + hourly WORD + verse to `/run/templeos-oracle`
   and a desktop ping. `ali-oracle` asks right now.
2. **Retro tribute mode** - `ali-retro on` drops to 640x480 (or smallest
   mode) with a fullscreen white-on-navy terminal; `ali-retro off`
   restores. One command to visit 1995.
3. **Daily verse** - date-seeded pick, stable all day, shown in the feed,
   the ping, and ALI Center's About tab.

Files: `templeos` (daemon, `--once` / `verse` modes), `ali-oracle`,
`ali-retro`, `templeos.service` (enabled by branding hook).
Needs: `x11-xserver-utils` (xrandr) for retro mode.
