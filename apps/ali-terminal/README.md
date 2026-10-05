# ALI Terminal - visible rebrand (no fork)

Underneath: `xfce4-terminal`. Visible: `ALI Terminal` everywhere.

Files:
- `ali-terminal` - wrapper, forces `--title="ALI Terminal"`
- `ali-terminal.desktop` - menu entry `Name=ALI Terminal`

## Try on Debian trixie
```bash
sudo apt install xfce4-terminal
chmod +x ali-terminal
./ali-terminal
```

## Add to ISO (1.0.2 batch, NOT yet)
1. copy `ali-terminal` -> `config/includes.chroot/usr/bin/ali-terminal` (chmod +x)
2. copy `ali-terminal.desktop` -> `config/includes.chroot/usr/share/applications/ali-terminal.desktop`
3. hook: set `NoDisplay=true` on stock `xfce4-terminal.desktop` so only ALI Terminal shows
4. ALI Center Apps tab already launches `xfce4-terminal --title='ALI Terminal'` - switch it to `ali-terminal` after this lands
