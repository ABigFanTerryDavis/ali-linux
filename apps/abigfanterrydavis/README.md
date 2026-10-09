# abigfanterrydavis 1.4.9 - the fan: identity guardian

You. The fan the distro is named after, running as a daemon.

Updates (base-files, grub, lightdm) keep smuggling Debian strings
back into the visible layer; the fan re-stamps ALI hourly: os-release
(version-preserving), lsb, issue, motd (+Fenn), hostname, grub
distributor config, greeter, sudoers, autologin. Never touches apt
compat (ID_LIKE, sources, keyrings). Pings when it repairs something.

Status: `/run/ali-fan`. Log: `/var/log/fan.log`.
