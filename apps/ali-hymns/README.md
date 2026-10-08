# ali-hymns 1.3.5 - TempleOS-tribute chiptune player (C + GTK3)

Six original melodies (no covers): Temple Morning, Oracle's Dance,
640x480, Desert Walk, Shepherd's Flute, Amen. Play/Stop/Test-Speaker,
volume slider, note display, Turkish via
`--tr` / `ALI_LANG=tr` (second launcher included in the .deb).

Sound picks its backend: sox `play` (real audio, everywhere) first,
PC-speaker `beep` as fallback. v1 was beep-only, which is why it stood
silent on VMs - fixed in v2.
