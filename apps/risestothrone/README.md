# risestothrone 1.4.7 - farewell guardian (named for Terry's last video)

Second-to-last voice: ordered before Odysseus stops, so the farewell
lands while the system still listens.

- boot: marks the throne (`/var/lib/risestothrone/last-boot`).
- shutdown: waits out critical work (backup, vault, dpkg lock - max
  60s), archives the Oracle feed tail to `/var/log/farewell.log`,
  writes the final kernel line to `/var/log/last-words.log`, syncs.
- It cannot stop physics. It finishes, and it remembers.
