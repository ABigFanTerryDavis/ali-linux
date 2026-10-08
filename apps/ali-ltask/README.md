# ali-ltask 1.2.6 - ALI Task Manager (ported LTaskManager)

Your PyQt6 task manager from the MerixCipher account, baked into ALI:
upstream `main.py` @ `5cb21b3` vendored as `ltaskmanager.py`, with a
3-line ALI diff (window title, sidebar title, footer). App bugs go
upstream; ALI owns the packaging (`.deb`, launcher, deps, menu entry).

- Processes tab: live table, search, user filter, sort, end task/tree,
  suspend/resume, details dialog, Restart Service for system units.
- Performance tab: CPU/Mem/Disk/Net live graphs (pyqtgraph, bars fallback).
- Yes, odysseus/sentinel/terrydavis/templeos all show up in it - search
  the name, end them if you dare (Sentinel's watchdog will just restart
  Sentinel, and Odysseus will note your violence in its log).

Turkish since 1.3.1 (`ALI_LANG=tr` / second launcher).
