# ali-style 1.3.4 - the shared ALI look

One palette (`#1a1d29` + `#8b93b8`), two artifacts:

- `ali-style.css` - application-priority GTK3 overrides, loaded by Center,
  Hymns, Welcome, Notepad (silent fallback when absent). Palette only,
  layouts untouched.
- `icons/ali-*.svg` - per-app icons (center, hymns, welcome, notepad,
  ltask) into hicolor + each `.deb`. Window icons via set_icon_name,
  ltask via QIcon.fromTheme.
- `logo.jpg` - THE dice logo (1024px, canonical): welcome page, Welcome
  wizard banner. Menu button keeps the crisp SVG; wallpaper/GRUB keep
  photos; Plymouth keeps generated art.
