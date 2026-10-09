# ali-fail 1.5.0 - the honest blue screen (C + GTK3)

`ali-fail <unit>`: fullscreen #0000AA, names the fallen daemon, shows
its last log lines, offers Restart / Full Log / Continue Without
(acknowledged flag in /run, stops the restart loop for this boot).
Odysseus gets the special truth: without it, this is just Debian 13.

Wired via `OnFailure=ali-fail@%n.service` on all seven daemons.
Headless fallback: wall + logger. Triggered only on repeated death
(first: systemd restarts quietly, no nagging).
