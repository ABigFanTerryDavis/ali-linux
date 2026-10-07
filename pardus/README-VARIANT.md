# ALI Linux - Pardus variant (EXPERIMENTAL)

Same ALI layer as `debian13/`, same shared `apps/`, but aimed at a
Pardus 25 (yirmibes - trixie-based) base instead of pure Debian trixie.

Status 1.1.6: building in CI.
- Pardus repos ENABLED (`config/archives/pardus.list.chroot` + keys from
  `pardus-archive-keyring` 2025.1). Suites verified live: yirmibes,
  yirmibes-deb, guvenlik/yirmibes-deb.
- CI job `Build ISO (pardus)` produces `ali-linux-iso-pardus`.

If the pardus job ever goes red while debian13 stays green, the Pardus
mirror or keyring rotated - check `depo.pardus.org.tr` first.
