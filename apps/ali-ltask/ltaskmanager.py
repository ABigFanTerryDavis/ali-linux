# ALI Linux port (1.2.6) of LTaskManager by MerixCipher (MIT):
# https://github.com/MerixCipher/LTaskManager @ 5cb21b3
# ALI diff vs upstream: window/sidebar titles + footer rebranded (1.2.6),
# full Turkish display pass via _()/TRMAP + index-keyed sort/filter (1.3.1).
# Everything else is byte-identical upstream - send app bugs there.
import sys
import os
import psutil
import datetime
import subprocess
from collections import deque
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QTableWidget, QTableWidgetItem, QPushButton, QLineEdit, QLabel,
    QHeaderView, QMessageBox, QProgressBar, QComboBox, QMenu, QStackedWidget, QGridLayout, QFrame,
    QFileDialog, QInputDialog, QAbstractItemView
)
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QColor, QBrush, QIcon

try:
    import pyqtgraph as pg
    HAS_PYQTGRAPH = True
except Exception:
    HAS_PYQTGRAPH = False

COLS = ["PID", "Name", "CPU %", "Memory %", "Memory MB", "Status", "User", "Command"]
REFRESH_MS = 2000

# ALI Turkish pass (1.3.1): ALI_LANG=tr translates display strings only.
# Internal keys (sort mapping, user-filter index) stay English on purpose.
import os as _os
TR = _os.environ.get("ALI_LANG", "").lower() == "tr"
TRMAP = {
    "CPU": "CPU",
    "Performance": "Performans",
    "Processes": "İşlemler",
    "Process Manager": "İşlem Yöneticisi",
    "Uptime": "Süre",
    "Processes": "İşlemler",
    "Process Manager": "İşlem Yöneticisi",
    "Memory": "Bellek",
    "Network": "Ağ",
    "Disk": "Disk",
    "Name": "Ad",
    "Memory %": "Bellek %",
    "Memory MB": "Bellek MB",
    "Status": "Durum",
    "User": "Kullanıcı",
    "Command": "Komut",
    "All Users": "Tüm Kullanıcılar",
    "Search PID / Name / User / Command...": "PID / Ad / Kullanıcı / Komut ara...",
    "Sort:": "Sırala:",
    "Sort:": "Sırala:",
    "Refresh": "Yenile",
    "End Task": "Görevi Sonlandır",
    "End Process Tree": "İşlem Ağacını Sonlandır",
    "Restart Service": "Hizmeti Yeniden Başlat",
    "Loading...": "Yükleniyor...",
    "Success": "Başarılı",
    "Timeout": "Zaman Aşımı",
    "Service restart timed out.": "Hizmet yeniden başlatma zaman aşımına uğradı.",
    "Not Found": "Bulunamadı",
    "systemctl not found. Not a systemd system?": "systemctl bulunamadı. systemd sistemi değil mi?",
    "Error": "Hata",
    "Select": "Seç",
    "Select a process to end.": "Sonlandırılacak işlemi seç.",
    "Select a process.": "Bir işlem seç.",
    "Blocked": "Engellendi",
    "Cannot kill PID 0/1.": "PID 0/1 öldürülemez.",
    "Gone": "Gitti",
    "Process already exited.": "İşlem zaten kapanmış.",
    "Access Denied": "Erişim Reddedildi",
    "Suspend": "Askıya Al",
    "Resume": "Sürdür",
    "Copy PID / Command": "PID / Komut Kopyala",
    "Services": "Hizmetler",
    "Service": "Hizmet",
    "Description": "Açıklama",
    "Start": "Başlat",
    "Stop": "Durdur",
    "Restart": "Yeniden Başlat",
    "Load": "Yük",
    "Active": "Etkin",
    "Sub": "Alt",
    "Boot": "Açılışta",
    "Boot On": "Açılışta Aç",
    "Boot Off": "Açılışta Kapat",
    "Startup": "Başlangıç",
    "Application": "Uygulama",
    "Export CSV": "CSV Dışa Aktar",
    "Exported": "Dışa aktarıldı",
    "Kill by Name…": "Ada Göre Öldür…",
    "Exact process name:": "Tam işlem adı:",
    "No such process.": "Böyle bir işlem yok.",
    "Enabled": "Etkin",
    "Source": "Kaynak",
    "Yes": "Evet",
    "No": "Hayır",
    "Enable / Disable": "Etkinleştir / Kapat",
    "Disabled": "Kapatıldı",
    "CPU detail": "CPU ayrıntı",
    "Memory detail": "Bellek ayrıntı",
    "Disk detail": "Disk ayrıntı",
    "Network detail": "Ağ ayrıntı",
    "GPU: No dedicated GPU detected (nvidia-smi not found) — Integrated graphics": "GPU: Adanmış GPU bulunamadı (nvidia-smi yok) — Tümleşik grafik",
    "pyqtgraph not installed — install with: pip install pyqtgraph\n\nShowing bars instead of graphs.": "pyqtgraph kurulu değil — kur: pip install pyqtgraph\n\nGrafik yerine çubuklar.",
}
SORT_KEYS = ["CPU %", "Memory %", "PID", "Name"]

def _(s):
    if TR:
        return TRMAP.get(s, s)
    return s

# ALI house daemons run as shell scripts (Name column says "sh").
# refresh_processes() shows their true names instead - the cmdline proves it.
ALI_NAMES = {
    "/usr/bin/odysseus": "odysseus",
    "/usr/bin/sentinel": "sentinel",
    "/usr/bin/terrydavis": "terrydavis",
    "/usr/bin/templeos": "templeos",
    "/usr/bin/oracle": "oracle",
    "/usr/bin/abigfanterrydavis": "abigfanterrydavis",
    "/usr/bin/linustorvalds": "linustorvalds",
}

def format_mb(bytes_val):
    return f"{bytes_val / 1024 / 1024:.1f}"

def format_bytes(bytes_val):
    for unit in ["B","KB","MB","GB"]:
        if abs(bytes_val) < 1024:
            return f"{bytes_val:.1f} {unit}"
        bytes_val /= 1024
    return f"{bytes_val:.1f} TB"

# ================= PERFORMANCE WIDGET =================
class PerformanceWidget(QWidget):
    def __init__(self):
        super().__init__()
        self.cpu_hist = deque([0]*60, maxlen=60)
        self.mem_hist = deque([0]*60, maxlen=60)
        self.disk_hist = deque([0]*60, maxlen=60)
        self.net_hist = deque([0]*60, maxlen=60)
        # for io rates
        self.last_disk = psutil.disk_io_counters()
        self.last_net = psutil.net_io_counters()
        self.last_time = datetime.datetime.now()

        layout = QVBoxLayout(self)
        title = QLabel(_("Performance"))
        title.setStyleSheet("font-size: 18px; font-weight: bold; padding: 4px;")
        layout.addWidget(title)

        # info bar
        info = QLabel(f"CPU: {psutil.cpu_count(logical=True)} logical / {psutil.cpu_count(logical=False)} physical cores  •  Memory: {format_bytes(psutil.virtual_memory().total)}  •  Disk: {format_bytes(psutil.disk_usage('/').total)}")
        info.setStyleSheet("color: #666; font-size: 11px; padding: 2px;")
        layout.addWidget(info)

        if HAS_PYQTGRAPH:
            grid = QGridLayout()
            grid.setSpacing(12)

            # CPU graph
            self.cpu_label = QLabel(_("CPU") + " — 0%")
            self.cpu_label.setStyleSheet("font-weight: bold;")
            self.cpu_plot = pg.PlotWidget()
            self.setup_plot(self.cpu_plot, "CPU %", "red")
            self.cpu_curve = self.cpu_plot.plot(list(self.cpu_hist), pen=pg.mkPen("#e53935", width=2))

            # Memory graph
            self.mem_label = QLabel(_("Memory") + " — %0")
            self.mem_label.setStyleSheet("font-weight: bold;")
            self.mem_plot = pg.PlotWidget()
            self.setup_plot(self.mem_plot, "Memory %", "#1e88e5")
            self.mem_curve = self.mem_plot.plot(list(self.mem_hist), pen=pg.mkPen("#1e88e5", width=2))

            # Disk graph
            self.disk_label = QLabel(_("Disk") + " G/Ç — 0 MB/s")
            self.disk_label.setStyleSheet("font-weight: bold;")
            self.disk_plot = pg.PlotWidget()
            self.setup_plot(self.disk_plot, "Disk MB/s", "#43a047")
            self.disk_curve = self.disk_plot.plot(list(self.disk_hist), pen=pg.mkPen("#43a047", width=2))

            # Network graph
            self.net_label = QLabel(_("Network") + " G/Ç — 0 MB/s")
            self.net_label.setStyleSheet("font-weight: bold;")
            self.net_plot = pg.PlotWidget()
            self.setup_plot(self.net_plot, "Network MB/s", "#fb8c00")
            self.net_curve = self.net_plot.plot(list(self.net_hist), pen=pg.mkPen("#fb8c00", width=2))

            # layout: 2x2 grid each with label + plot
            def wrap(label, plot):
                w = QWidget()
                v = QVBoxLayout(w)
                v.setContentsMargins(0,0,0,0)
                v.addWidget(label)
                v.addWidget(plot)
                return w

            grid.addWidget(wrap(self.cpu_label, self.cpu_plot), 0, 0)
            grid.addWidget(wrap(self.mem_label, self.mem_plot), 0, 1)
            grid.addWidget(wrap(self.disk_label, self.disk_plot), 1, 0)
            grid.addWidget(wrap(self.net_label, self.net_plot), 1, 1)

            layout.addLayout(grid)

            # bottom stats + gpu
            bottom = QHBoxLayout()
            self.cpu_detail = QLabel(_("CPU detail"))
            self.cpu_detail.setStyleSheet("font-size: 11px; color: #444;")
            self.mem_detail = QLabel(_("Memory detail"))
            self.mem_detail.setStyleSheet("font-size: 11px; color: #444;")
            self.disk_detail = QLabel(_("Disk detail"))
            self.disk_detail.setStyleSheet("font-size: 11px; color: #444;")
            self.net_detail = QLabel(_("Network detail"))
            self.net_detail.setStyleSheet("font-size: 11px; color: #444;")
            for w in [self.cpu_detail, self.mem_detail, self.disk_detail, self.net_detail]:
                bottom.addWidget(w)
            layout.addLayout(bottom)

            # GPU placeholder (no nvidia-smi on this machine)
            self.gpu_label = QLabel(_("GPU: No dedicated GPU detected (nvidia-smi not found) — Integrated graphics"))
            self.gpu_label.setStyleSheet("background: #f5f5f5; border: 1px solid #ddd; padding: 8px; font-size: 11px; color: #666;")
            layout.addWidget(self.gpu_label)
        else:
            # fallback without pyqtgraph
            fallback = QLabel(_("pyqtgraph not installed — install with: pip install pyqtgraph\n\nShowing bars instead of graphs."))
            fallback.setStyleSheet("color: #d32f2f;")
            layout.addWidget(fallback)
            self.cpu_bar = QProgressBar()
            self.mem_bar = QProgressBar()
            self.disk_bar = QProgressBar()
            layout.addWidget(QLabel(_("CPU")))
            layout.addWidget(self.cpu_bar)
            layout.addWidget(QLabel(_("Memory")))
            layout.addWidget(self.mem_bar)
            layout.addWidget(QLabel(_("Disk")))
            layout.addWidget(self.disk_bar)

        layout.addStretch()

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_stats)
        self.timer.start(800)
        self.update_stats()

    def setup_plot(self, plot, title, color):
        plot.setBackground("#fafafa")
        plot.showGrid(x=True, y=True, alpha=0.3)
        plot.setYRange(0, 100 if " %" in title else 50, padding=0)
        plot.setXRange(0, 60, padding=0)
        plot.getAxis("bottom").setTicks([])
        plot.setLabel("left", title)
        plot.setMinimumHeight(180)
        plot.setTitle(title, size="10pt")

    def update_stats(self):
        cpu = psutil.cpu_percent(interval=None)
        mem = psutil.virtual_memory()
        disk = psutil.disk_usage("/")
        # io rates
        now = datetime.datetime.now()
        dt = (now - self.last_time).total_seconds() or 1
        disk_io = psutil.disk_io_counters()
        net_io = psutil.net_io_counters()
        disk_rate = 0
        net_rate = 0
        if self.last_disk and disk_io:
            disk_rate = ((disk_io.read_bytes - self.last_disk.read_bytes) + (disk_io.write_bytes - self.last_disk.write_bytes)) / dt / 1024 / 1024
        if self.last_net and net_io:
            net_rate = ((net_io.bytes_sent - self.last_net.bytes_sent) + (net_io.bytes_recv - self.last_net.bytes_recv)) / dt / 1024 / 1024
        self.last_disk = disk_io
        self.last_net = net_io
        self.last_time = now

        self.cpu_hist.append(cpu)
        self.mem_hist.append(mem.percent)
        # keep disk/net in 0-100 scale by clamping MB/s *10
        self.disk_hist.append(min(disk_rate*4, 100))
        self.net_hist.append(min(net_rate*4, 100))

        if HAS_PYQTGRAPH:
            self.cpu_curve.setData(list(self.cpu_hist))
            self.mem_curve.setData(list(self.mem_hist))
            self.disk_curve.setData(list(self.disk_hist))
            self.net_curve.setData(list(self.net_hist))

            self.cpu_label.setText(f"CPU — {cpu:.1f}%" if not TR else f"CPU — %{cpu:.1f}")
            self.mem_label.setText(f"Memory — {mem.percent:.1f}% ({format_mb(mem.used)} / {format_mb(mem.total)} MB)" if not TR else f"Bellek — %{mem.percent:.1f} ({format_mb(mem.used)} / {format_mb(mem.total)} MB)")
            self.disk_label.setText(f"Disk I/O — {disk_rate:.1f} MB/s" if not TR else f"Disk G/Ç — {disk_rate:.1f} MB/s")
            self.net_label.setText(f"Network I/O — {net_rate:.1f} MB/s" if not TR else f"Ağ G/Ç — {net_rate:.1f} MB/s")

            freq = psutil.cpu_freq()
            freq_str = f"{freq.current:.0f} MHz" if freq else "N/A"
            self.cpu_detail.setText(f"Cores: {psutil.cpu_count()} • Freq: {freq_str} • Uptime: {str(datetime.datetime.now() - datetime.datetime.fromtimestamp(psutil.boot_time())).split('.')[0]}" if not TR else f"Çekirdek: {psutil.cpu_count()} • Frekans: {freq_str} • Süre: {str(datetime.datetime.now() - datetime.datetime.fromtimestamp(psutil.boot_time())).split('.')[0]}")
            self.mem_detail.setText(f"Avail: {format_bytes(mem.available)} • Used: {format_bytes(mem.used)}" if not TR else f"Boş: {format_bytes(mem.available)} • Dolu: {format_bytes(mem.used)}")
            self.disk_detail.setText(f"Used: {format_bytes(disk.used)} / {format_bytes(disk.total)} ({disk.percent}%)" if not TR else f"Dolu: {format_bytes(disk.used)} / {format_bytes(disk.total)} (%{disk.percent})")
            self.net_detail.setText(f"Sent: {format_bytes(net_io.bytes_sent)} • Recv: {format_bytes(net_io.bytes_recv)}" if not TR else f"Gönderilen: {format_bytes(net_io.bytes_sent)} • Alınan: {format_bytes(net_io.bytes_recv)}")
        else:
            self.cpu_bar.setValue(int(cpu))
            self.mem_bar.setValue(int(mem.percent))
            self.disk_bar.setValue(int(disk.percent))


# ================= SERVICES WIDGET (1.3.3) =================
ALI_SERVICES = [
    ("odysseus.service", "First up, last down: enforcer, watchdog, sea"),
    ("sentinel.service", "IDS watch + IPS exterminate"),
    ("terrydavis.service", "QoL daemon: janitor, crier, game, wifi, battery"),
    ("templeos.service", "Tribute daemon: oracle lots + verse"),
    ("oracle.service", "Security voice: verdicts + lots"),
    ("abigfanterrydavis.service", "The fan: identity guardian"),
    ("linustorvalds.service", "Blunt voice: kernel translator"),
]

class ServicesWidget(QWidget):
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        title = QLabel(_("Services"))
        title.setStyleSheet("font-size: 18px; font-weight: bold; padding: 4px;")
        layout.addWidget(title)

        self.table = QTableWidget(0, 6)
        self.table.setHorizontalHeaderLabels([_("Service"), _("Load"), _("Active"), _("Sub"), _("Boot"), _("Description")])
        self.table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeMode.Interactive)
        self.table.horizontalHeader().setSectionResizeMode(5, QHeaderView.ResizeMode.Stretch)
        self.table.setColumnWidth(0, 200)
        self.table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
        self.table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        self.table.setAlternatingRowColors(True)
        layout.addWidget(self.table)

        row = QHBoxLayout()
        self.refresh_btn = QPushButton(_("Refresh"))
        self.refresh_btn.clicked.connect(self.refresh)
        row.addWidget(self.refresh_btn)
        self.start_btn = QPushButton(_("Start"))
        self.start_btn.clicked.connect(lambda: self.act("start"))
        row.addWidget(self.start_btn)
        self.stop_btn = QPushButton(_("Stop"))
        self.stop_btn.clicked.connect(lambda: self.act("stop"))
        row.addWidget(self.stop_btn)
        self.restart_btn = QPushButton(_("Restart"))
        self.restart_btn.clicked.connect(lambda: self.act("restart"))
        row.addWidget(self.restart_btn)
        self.enable_btn = QPushButton(_("Boot On"))
        self.enable_btn.clicked.connect(lambda: self.act("enable"))
        row.addWidget(self.enable_btn)
        self.disable_btn = QPushButton(_("Boot Off"))
        self.disable_btn.clicked.connect(lambda: self.act("disable"))
        row.addWidget(self.disable_btn)
        layout.addLayout(row)

        self.status = QLabel("")
        self.status.setStyleSheet("color: #666; font-size: 11px;")
        layout.addWidget(self.status)

    def _unit_state(self, unit):
        try:
            a = subprocess.run(["systemctl", "is-active", unit],
                               capture_output=True, text=True, timeout=5)
            active = a.stdout.strip() or "unknown"
        except Exception:
            active = "unknown"
        return ("loaded", active, active, "")

    def refresh(self):
        self.table.setRowCount(0)
        try:
            ef = subprocess.run(["systemctl", "list-unit-files", "--type=service",
                                 "--no-legend", "--plain"],
                                capture_output=True, text=True, timeout=10)
            enabled = {}
            for line in ef.stdout.splitlines():
                p = line.split()
                if len(p) >= 2 and p[0].endswith(".service"):
                    enabled[p[0]] = p[1]
        except Exception:
            enabled = {}
        rows = []
        for unit, desc in ALI_SERVICES:
            load, active, sub, _d = self._unit_state(unit)
            rows.append((unit, load, active, sub, enabled.get(unit, "?"), desc))
        try:
            out = subprocess.run(["systemctl", "list-units", "--type=service",
                                  "--all", "--no-legend", "--plain"],
                                 capture_output=True, text=True, timeout=10)
            for line in out.stdout.splitlines():
                parts = line.split(None, 4)
                if len(parts) >= 4 and parts[0].endswith(".service"):
                    if parts[0] in [u for u, _d in ALI_SERVICES]:
                        continue
                    desc = parts[4] if len(parts) > 4 else ""
                    rows.append((parts[0], parts[1], parts[2], parts[3], enabled.get(parts[0], "?"), desc))
        except Exception as e:
            self.status.setText(f"{_('Error')}: {e}")
            return
        self.table.setRowCount(len(rows))
        for i, (u, lo, ac, su, bo, de) in enumerate(rows):
            for j, val in enumerate([u, lo, ac, su, bo, de]):
                it = QTableWidgetItem(val)
                it.setFlags(it.flags() & ~Qt.ItemFlag.ItemIsEditable)
                if j == 2:
                    if ac == "active":
                        it.setForeground(QBrush(QColor("darkgreen")))
                    elif ac in ("failed", "inactive"):
                        it.setForeground(QBrush(QColor("red")))
                if j == 4 and bo == "enabled":
                    it.setForeground(QBrush(QColor("darkgreen")))
                self.table.setItem(i, j, it)
        self.status.setText(f"{len(rows)} {_('Services').lower()}" if not TR else f"{len(rows)} hizmet")

    def selected_unit(self):
        row = self.table.currentRow()
        if row < 0:
            return None
        item = self.table.item(row, 0)
        return item.text() if item else None

    def act(self, action):
        unit = self.selected_unit()
        if not unit:
            return
        if action == "stop":
            if QMessageBox.question(self, _("Stop"), f"Stop service '{unit}'?" if not TR else f"'{unit}' hizmeti durdurulsun mu?") != QMessageBox.StandardButton.Yes:
                return
        try:
            r = subprocess.run(["pkexec", "systemctl", action, unit],
                               capture_output=True, text=True, timeout=30)
            if r.returncode != 0:
                r = subprocess.run(["sudo", "systemctl", action, unit],
                                   capture_output=True, text=True, timeout=30)
            if r.returncode == 0:
                self.status.setText(f"{action}ed {unit}" if not TR else f"{unit}: {action} tamam")
            else:
                raise Exception(r.stderr.strip() or "failed")
        except Exception as e:
            QMessageBox.warning(self, _("Error"), str(e))
        self.refresh()


# ================= STARTUP WIDGET (1.3.5) =================
# Autostart entries: user (~/.config/autostart) + system (/etc/xdg/autostart).
# Toggle writes Hidden=true/false into the USER copy (never touches system).
class StartupWidget(QWidget):
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout(self)
        layout.setContentsMargins(12, 12, 12, 12)
        title = QLabel(_("Startup"))
        title.setStyleSheet("font-size: 18px; font-weight: bold; padding: 4px;")
        layout.addWidget(title)

        self.table = QTableWidget(0, 3)
        self.table.setHorizontalHeaderLabels([_("Application"), _("Enabled"), _("Source")])
        self.table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeMode.Stretch)
        self.table.setColumnWidth(1, 100)
        self.table.setColumnWidth(2, 220)
        self.table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
        self.table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        self.table.setAlternatingRowColors(True)
        layout.addWidget(self.table)

        row = QHBoxLayout()
        self.refresh_btn = QPushButton(_("Refresh"))
        self.refresh_btn.clicked.connect(self.refresh)
        row.addWidget(self.refresh_btn)
        self.toggle_btn = QPushButton(_("Enable / Disable"))
        self.toggle_btn.clicked.connect(self.toggle)
        row.addWidget(self.toggle_btn)
        layout.addLayout(row)

        self.status = QLabel("")
        self.status.setStyleSheet("color: #666; font-size: 11px;")
        layout.addWidget(self.status)

    def _entries(self):
        import configparser
        found = {}
        for src, base in (("system", "/etc/xdg/autostart"), ("user", os.path.expanduser("~/.config/autostart"))):
            try:
                names = os.listdir(base)
            except Exception:
                continue
            for fn in sorted(names):
                if not fn.endswith(".desktop"):
                    continue
                cp = configparser.ConfigParser(interpolation=None)
                try:
                    cp.read(os.path.join(base, fn), encoding="utf-8")
                    name = cp.get("Desktop Entry", "Name", fallback=fn)
                    hidden = cp.get("Desktop Entry", "Hidden", fallback="false").lower() == "true"
                except Exception:
                    name, hidden = fn, False
                found[fn] = {"name": name, "file": fn, "src": src,
                             "user_path": os.path.join(os.path.expanduser("~/.config/autostart"), fn),
                             "sys_path": os.path.join("/etc/xdg/autostart", fn),
                             "hidden": hidden}
        return [found[k] for k in sorted(found)]

    def refresh(self):
        self.rows = self._entries()
        self.table.setRowCount(len(self.rows))
        for i, e in enumerate(self.rows):
            for j, val in enumerate([e["name"], _("Yes") if not e["hidden"] else _("No"), e["src"]]):
                it = QTableWidgetItem(val)
                it.setFlags(it.flags() & ~Qt.ItemFlag.ItemIsEditable)
                if j == 1:
                    it.setForeground(QBrush(QColor("darkgreen" if not e["hidden"] else "red")))
                self.table.setItem(i, j, it)
        self.status.setText(f"{len(self.rows)} {_('Startup').lower()}" if not TR else f"{len(self.rows)} başlangıç öğesi")

    def toggle(self):
        row = self.table.currentRow()
        if row < 0 or row >= len(getattr(self, "rows", [])):
            return
        e = self.rows[row]
        try:
            os.makedirs(os.path.dirname(e["user_path"]), exist_ok=True)
            if not e["hidden"]:
                base = e["sys_path"] if os.path.exists(e["sys_path"]) else e["user_path"]
                with open(base, encoding="utf-8") as f:
                    content = f.read()
                if "[Desktop Entry]" not in content:
                    content = "[Desktop Entry]\n" + content
                content += "\nHidden=true\n"
                with open(e["user_path"], "w", encoding="utf-8") as f:
                    f.write(content)
                self.status.setText(f"{_('Disabled')}: {e['name']}")
            else:
                if e["src"] == "system" and os.path.exists(e["user_path"]):
                    os.remove(e["user_path"])
                    self.status.setText(f"{_('Enabled')}: {e['name']}")
                else:
                    with open(e["user_path"], encoding="utf-8") as f:
                        content = f.read()
                    content = content.replace("Hidden=true", "Hidden=false").replace("Hidden=1", "Hidden=false")
                    with open(e["user_path"], "w", encoding="utf-8") as f:
                        f.write(content)
                    self.status.setText(f"{_('Enabled')}: {e['name']}")
        except Exception as ex:
            QMessageBox.warning(self, _("Error"), str(ex))
        self.refresh()


class ProcessManager(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("ALI Task Manager - Process Manager")
        self.setWindowIcon(QIcon.fromTheme("ali-ltask"))
        self.resize(1200, 650)
        self.sort_col = 2
        self.sort_desc = True
        self.all_procs = []

        # Central with leftbar + stacked
        central = QWidget()
        self.setCentralWidget(central)
        main_h = QHBoxLayout(central)
        main_h.setContentsMargins(0,0,0,0)
        main_h.setSpacing(0)

        # LEFT SIDEBAR
        sidebar = QWidget()
        sidebar.setFixedWidth(170)
        sidebar.setStyleSheet("background: #1a1d29;")
        sb_layout = QVBoxLayout(sidebar)
        sb_layout.setContentsMargins(8,12,8,12)
        sb_layout.setSpacing(6)

        sb_title = QLabel("ALI Task Manager")
        sb_title.setStyleSheet("color: white; font-size: 15px; font-weight: bold; padding: 8px 4px;")
        sb_layout.addWidget(sb_title)

        self.btn_proc = QPushButton("  ◉  " + _("Processes"))
        self.btn_perf = QPushButton("  ◈  " + _("Performance"))
        self.btn_svc = QPushButton("  ⚙  " + _("Services"))
        self.btn_startup = QPushButton("  🚀  " + _("Startup"))
        for b in [self.btn_proc, self.btn_perf, self.btn_svc, self.btn_startup]:
            b.setFixedHeight(42)
            b.setCursor(Qt.CursorShape.PointingHandCursor)
            b.setStyleSheet("""
                QPushButton { text-align: left; padding-left: 12px; color: #e8eaed; background: transparent; border: none; font-size: 13px; border-radius: 6px;}
                QPushButton:hover { background: #2a2f45; }
            """)
        self.btn_proc.setStyleSheet("""
            QPushButton { text-align: left; padding-left: 12px; color: white; background: #2a2f45; border: none; font-size: 13px; border-radius: 6px; font-weight: bold;}
        """)
        sb_layout.addWidget(self.btn_proc)
        sb_layout.addWidget(self.btn_perf)
        sb_layout.addWidget(self.btn_svc)
        sb_layout.addWidget(self.btn_startup)
        sb_layout.addStretch()
        sb_footer = QLabel("ALI Linux • PyQt6")
        sb_footer.setStyleSheet("color: #9aa0a6; font-size: 10px; padding: 6px;")
        sb_layout.addWidget(sb_footer)

        main_h.addWidget(sidebar)

        # STACKED PAGES
        self.stack = QStackedWidget()
        main_h.addWidget(self.stack, stretch=1)

        # PAGE 0: PROCESSES — ENTIRELY SAME AS BEFORE
        self.page_proc = QWidget()
        layout = QVBoxLayout(self.page_proc)
        layout.setContentsMargins(12,12,12,12)

        title = QLabel(_("Process Manager"))
        title.setStyleSheet("font-size: 18px; font-weight: bold; padding: 4px;")
        layout.addWidget(title)

        stats_layout = QHBoxLayout()
        self.cpu_label = QLabel("CPU: --")
        self.mem_label = QLabel(_("Memory") + ": --")
        self.disk_label = QLabel(_("Disk") + ": --")
        self.proc_count_label = QLabel(_("Processes") + ": --")
        self.uptime_label = QLabel(_("Uptime") + ": --")

        self.cpu_bar = QProgressBar()
        self.cpu_bar.setMaximum(100)
        self.cpu_bar.setMaximumWidth(120)
        self.cpu_bar.setTextVisible(True)

        self.mem_bar = QProgressBar()
        self.mem_bar.setMaximum(100)
        self.mem_bar.setMaximumWidth(120)
        self.mem_bar.setTextVisible(True)

        for w in [self.cpu_label, self.cpu_bar, self.mem_label, self.mem_bar, self.disk_label, self.proc_count_label, self.uptime_label]:
            stats_layout.addWidget(w)
        stats_layout.addStretch()
        layout.addLayout(stats_layout)

        top_layout = QHBoxLayout()
        self.search = QLineEdit()
        self.search.setPlaceholderText(_("Search PID / Name / User / Command..."))
        self.search.textChanged.connect(self.apply_filter)
        top_layout.addWidget(self.search, stretch=3)

        self.user_filter = QComboBox()
        self.user_filter.addItem(_("All Users"))
        self.user_filter.currentTextChanged.connect(self.apply_filter)
        top_layout.addWidget(self.user_filter)

        self.sort_combo = QComboBox()
        self.sort_combo.addItems([_(k) for k in SORT_KEYS])
        self.sort_combo.setCurrentIndex(0)
        self.sort_combo.currentTextChanged.connect(self.on_sort_changed)
        top_layout.addWidget(QLabel(_("Sort:")))
        top_layout.addWidget(self.sort_combo)

        self.refresh_btn = QPushButton(_("Refresh"))
        self.refresh_btn.clicked.connect(self.refresh_processes)
        top_layout.addWidget(self.refresh_btn)

        self.kill_btn = QPushButton(_("End Task"))
        self.kill_btn.setStyleSheet("background-color: #d32f2f; color: white; font-weight: bold; padding: 6px 14px;")
        self.kill_btn.clicked.connect(self.kill_process)
        top_layout.addWidget(self.kill_btn)

        self.kill_tree_btn = QPushButton(_("End Process Tree"))
        self.kill_tree_btn.clicked.connect(self.kill_tree)
        top_layout.addWidget(self.kill_tree_btn)
        self.export_btn = QPushButton(_("Export CSV"))
        self.export_btn.clicked.connect(self.export_csv)
        top_layout.addWidget(self.export_btn)
        self.byname_btn = QPushButton(_("Kill by Name…"))
        self.byname_btn.clicked.connect(self.kill_by_name)
        top_layout.addWidget(self.byname_btn)

        layout.addLayout(top_layout)

        self.table = QTableWidget(0, len(COLS))
        self.table.setHorizontalHeaderLabels([_(c) for c in COLS])
        header = self.table.horizontalHeader()
        header.setSectionResizeMode(0, QHeaderView.ResizeMode.Fixed)
        header.setSectionResizeMode(1, QHeaderView.ResizeMode.Interactive)
        header.setSectionResizeMode(2, QHeaderView.ResizeMode.Fixed)
        header.setSectionResizeMode(3, QHeaderView.ResizeMode.Fixed)
        header.setSectionResizeMode(4, QHeaderView.ResizeMode.Fixed)
        header.setSectionResizeMode(5, QHeaderView.ResizeMode.Fixed)
        header.setSectionResizeMode(6, QHeaderView.ResizeMode.Fixed)
        header.setSectionResizeMode(7, QHeaderView.ResizeMode.Stretch)
        self.table.setColumnWidth(0, 80)
        self.table.setColumnWidth(1, 180)
        self.table.setColumnWidth(2, 80)
        self.table.setColumnWidth(3, 90)
        self.table.setColumnWidth(4, 90)
        self.table.setColumnWidth(5, 90)
        self.table.setColumnWidth(6, 110)
        self.table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
        self.table.setSelectionMode(QAbstractItemView.SelectionMode.ExtendedSelection)
        self.table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        self.table.setAlternatingRowColors(True)
        self.table.setSortingEnabled(False)
        self.table.doubleClicked.connect(self.show_details)
        self.table.horizontalHeader().sectionClicked.connect(self.on_header_clicked)
        self.table.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.table.customContextMenuRequested.connect(self.show_context_menu)
        layout.addWidget(self.table)

        self.status = QLabel(_("Loading..."))
        self.status.setStyleSheet("color: #666; font-size: 11px;")
        layout.addWidget(self.status)

        # PAGE 1: PERFORMANCE
        self.page_perf = PerformanceWidget()

        # PAGE 2: SERVICES
        self.page_svc = ServicesWidget()

        # PAGE 3: STARTUP
        self.page_startup = StartupWidget()

        self.stack.addWidget(self.page_proc)
        self.stack.addWidget(self.page_perf)
        self.stack.addWidget(self.page_svc)
        self.stack.addWidget(self.page_startup)
        self.stack.setCurrentIndex(0)

        # sidebar switching
        self.btn_proc.clicked.connect(lambda: self.switch_tab(0))
        self.btn_perf.clicked.connect(lambda: self.switch_tab(1))
        self.btn_svc.clicked.connect(lambda: self.switch_tab(2))
        self.btn_startup.clicked.connect(lambda: self.switch_tab(3))

        # Timer for processes
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.refresh_processes)
        self.timer.start(REFRESH_MS)

        psutil.cpu_percent(interval=None)
        for p in psutil.process_iter():
            try:
                p.cpu_percent(interval=None)
            except Exception:
                pass

        self.refresh_processes()

    def switch_tab(self, idx):
        self.stack.setCurrentIndex(idx)
        # style update
        active = "QPushButton { text-align: left; padding-left: 12px; color: white; background: #2a2f45; border: none; font-size: 13px; border-radius: 6px; font-weight: bold;}"
        inactive = "QPushButton { text-align: left; padding-left: 12px; color: #e8eaed; background: transparent; border: none; font-size: 13px; border-radius: 6px;}"
        self.btn_proc.setStyleSheet(active if idx==0 else inactive)
        self.btn_perf.setStyleSheet(active if idx==1 else inactive)
        self.btn_svc.setStyleSheet(active if idx==2 else inactive)
        self.btn_startup.setStyleSheet(active if idx==3 else inactive)
        if idx == 2:
            self.page_svc.refresh()
        elif idx == 3:
            self.page_startup.refresh()

    def on_sort_changed(self, text):
        idx = self.sort_combo.currentIndex()
        key = SORT_KEYS[idx] if 0 <= idx < len(SORT_KEYS) else "CPU %"
        mapping = {"PID": 0, "Name": 1, "CPU %": 2, "Memory %": 3}
        self.sort_col = mapping.get(key, 2)
        self.apply_filter()

    def on_header_clicked(self, col):
        if self.sort_col == col:
            self.sort_desc = not self.sort_desc
        else:
            self.sort_col = col
            self.sort_desc = True
        rev = {0:"PID",1:"Name",2:"CPU %",3:"Memory %"}
        if col in rev:
            self.sort_combo.blockSignals(True)
            self.sort_combo.setCurrentText(_(rev[col]))
            self.sort_combo.blockSignals(False)
        self.apply_filter()

    def refresh_processes(self):
        try:
            cpu_total = psutil.cpu_percent(interval=None)
            mem = psutil.virtual_memory()
            disk = psutil.disk_usage("/")
            boot = datetime.datetime.fromtimestamp(psutil.boot_time())
            uptime = datetime.datetime.now() - boot
            hours, rem = divmod(int(uptime.total_seconds()), 3600)
            mins, secs = divmod(rem, 60)

            self.cpu_label.setText(f"CPU: {cpu_total:.1f}%" if not TR else f"CPU: %{cpu_total:.1f}")
            self.cpu_bar.setValue(int(cpu_total))
            self.mem_label.setText(f"Memory: {mem.percent:.1f}% ({format_mb(mem.used)} / {format_mb(mem.total)} MB)" if not TR else f"Bellek: %{mem.percent:.1f} ({format_mb(mem.used)} / {format_mb(mem.total)} MB)")
            self.mem_bar.setValue(int(mem.percent))
            self.disk_label.setText(f"Disk: {disk.percent:.1f}%" if not TR else f"Disk: %{disk.percent:.1f}")
            self.uptime_label.setText(f"Uptime: {hours}h {mins}m" if not TR else f"Süre: {hours}s {mins}d")

            procs = []
            users = set()
            for p in psutil.process_iter(['pid','name','cpu_percent','memory_percent','memory_info','status','username','cmdline']):
                try:
                    info = p.info
                    pid = info['pid']
                    name = info['name'] or ""
                    cpu = info['cpu_percent']
                    if cpu is None:
                        cpu = 0.0
                    mem_p = info['memory_percent']
                    if mem_p is None:
                        mem_p = 0.0
                    mem_mb = format_mb(info['memory_info'].rss) if info['memory_info'] else "0"
                    status = info['status'] or ""
                    user = info['username'] or ""
                    cmd = " ".join(info['cmdline']) if info['cmdline'] else name
                    for _ap, _an in ALI_NAMES.items():
                        if _ap in (info['cmdline'] or []):
                            name = _an
                            break
                    users.add(user)
                    procs.append((pid, name, cpu, mem_p, mem_mb, status, user, cmd))
                except (psutil.NoSuchProcess, psutil.AccessDenied):
                    continue

            self.all_procs = procs
            self.proc_count_label.setText(f"Processes: {len(procs)}" if not TR else f"İşlemler: {len(procs)}")

            current_user = self.user_filter.currentText()
            self.user_filter.blockSignals(True)
            self.user_filter.clear()
            self.user_filter.addItem(_("All Users"))
            for u in sorted(users):
                if u:
                    self.user_filter.addItem(u)
            if current_user in [self.user_filter.itemText(i) for i in range(self.user_filter.count())]:
                self.user_filter.setCurrentText(current_user)
            self.user_filter.blockSignals(False)

            self.apply_filter()
        except Exception as e:
            self.status.setText(f"Error: {e}" if not TR else f"Hata: {e}")

    def apply_filter(self):
        if not self.all_procs:
            return
        search = self.search.text().lower().strip()
        user_f = self.user_filter.currentText()

        filtered = []
        for p in self.all_procs:
            pid, name, cpu, mem_p, mem_mb, status, user, cmd = p
            if self.user_filter.currentIndex() != 0 and user != user_f:
                continue
            if search:
                if search not in str(pid).lower() and search not in name.lower() and search not in user.lower() and search not in cmd.lower():
                    continue
            filtered.append(p)

        col = self.sort_col
        reverse = self.sort_desc
        if col == 0:
            filtered.sort(key=lambda x: x[0], reverse=reverse)
        elif col == 1:
            filtered.sort(key=lambda x: x[1].lower(), reverse=reverse)
        elif col == 2:
            filtered.sort(key=lambda x: x[2], reverse=reverse)
        elif col == 3:
            filtered.sort(key=lambda x: x[3], reverse=reverse)
        elif col == 4:
            filtered.sort(key=lambda x: float(x[4]), reverse=reverse)
        elif col == 6:
            filtered.sort(key=lambda x: x[6].lower(), reverse=reverse)
        else:
            filtered.sort(key=lambda x: x[2], reverse=True)

        self.table.setRowCount(len(filtered))
        for row, (pid, name, cpu, mem_p, mem_mb, status, user, cmd) in enumerate(filtered):
            items = [
                QTableWidgetItem(str(pid)),
                QTableWidgetItem(name),
                QTableWidgetItem(f"{cpu:.1f}"),
                QTableWidgetItem(f"{mem_p:.1f}"),
                QTableWidgetItem(mem_mb),
                QTableWidgetItem(status),
                QTableWidgetItem(user),
                QTableWidgetItem(cmd),
            ]
            if cpu > 50:
                items[2].setForeground(QBrush(QColor("red")))
            elif cpu > 20:
                items[2].setForeground(QBrush(QColor("darkorange")))
            if mem_p > 5:
                items[3].setForeground(QBrush(QColor("red")))

            for col_idx, it in enumerate(items):
                it.setFlags(it.flags() & ~Qt.ItemFlag.ItemIsEditable)
                if col_idx == 0:
                    it.setData(Qt.ItemDataRole.UserRole, pid)
                self.table.setItem(row, col_idx, it)

        self.status.setText(f"Showing {len(filtered)} / {len(self.all_procs)} processes | Sort: {COLS[self.sort_col]} {'↓' if self.sort_desc else '↑'} | Auto-refresh {REFRESH_MS//1000}s" if not TR else f"{len(filtered)} / {len(self.all_procs)} işlem | Sırala: {_(COLS[self.sort_col])} {'↓' if self.sort_desc else '↑'} | Otomatik yenileme {REFRESH_MS//1000}sn")

    def get_selected_pid(self):
        row = self.table.currentRow()
        if row < 0:
            return None
        item = self.table.item(row, 0)
        if not item:
            return None
        return int(item.data(Qt.ItemDataRole.UserRole))

    def get_selected_pids(self):
        pids = []
        for it in self.table.selectedItems():
            if it.column() == 0 and it.data(Qt.ItemDataRole.UserRole):
                pids.append(int(it.data(Qt.ItemDataRole.UserRole)))
        return sorted(set(pids))

    def is_system_process(self, pid, name, user):
        if pid <= 1:
            return True
        if user == "root":
            return True
        system_names = {
            "systemd", "init", "kernel", "kthreadd", "ksoftirqd", "kworker",
            "migration", "rcu_", "watchdog", "kcompactd", "ksmd", "khugepaged",
            "crypto", "kintegrityd", "kblockd", "ata_", "scsi_", "usb_",
            "xorg", "Xorg", "wayland", "gnome-shell", "plasmashell", "kwin",
            "lightdm", "gdm", "sddm", "login", "sshd", "systemd-", "dbus-",
            "networkmanager", "polkitd", "accounts-daemon", "upowerd",
            "udisksd", "colord", "rtkit-daemon", "whoopsie", "snapd"
        }
        name_lower = name.lower()
        for sys_name in system_names:
            if sys_name in name_lower:
                return True
        return False

    def get_service_name(self, pid, name):
        try:
            p = psutil.Process(pid)
            cmdline = p.cmdline()
            if cmdline:
                for arg in cmdline:
                    if ".service" in arg or arg.startswith("--"):
                        continue
                    if "/" in arg and not arg.startswith("/usr/lib") and not arg.startswith("/lib"):
                        base = arg.split("/")[-1]
                        if base.endswith(".service"):
                            return base
            if name.endswith(".service"):
                return name
            common_services = {
                "systemd": "systemd",
                "sshd": "ssh",
                "networkmanager": "NetworkManager",
                "polkitd": "polkit",
                "accounts-daemon": "accounts-daemon",
                "upowerd": "upower",
                "udisksd": "udisks2",
                "colord": "colord",
                "rtkit-daemon": "rtkit-daemon",
                "whoopsie": "whoopsie",
                "snapd": "snapd",
                "gdm": "gdm",
                "lightdm": "lightdm",
                "sddm": "sddm",
                "dbus-daemon": "dbus",
                "avahi-daemon": "avahi-daemon",
                "cupsd": "cups",
                "cron": "cron",
                "rsyslogd": "rsyslog",
                "systemd-journald": "systemd-journald",
                "systemd-logind": "systemd-logind",
                "systemd-udevd": "systemd-udevd",
                "systemd-resolved": "systemd-resolved",
                "systemd-timesyncd": "systemd-timesyncd",
            }
            return common_services.get(name, name)
        except Exception:
            return name

    def restart_service(self, pid, name, user):
        service = self.get_service_name(pid, name)
        if QMessageBox.question(self, _("Restart Service"),
            f"Restart system service '{service}'?\n\n"
            f"PID: {pid} | User: {user} | Name: {name}\n\n"
            f"This will run: sudo systemctl restart {service}" if not TR else
            f"'{service}' sistem hizmeti yeniden başlatılsın mı?\n\n"
            f"PID: {pid} | Kullanıcı: {user} | Ad: {name}\n\n"
            f"Çalışacak: sudo systemctl restart {service}") != QMessageBox.StandardButton.Yes:
            return False
        try:
            result = subprocess.run(["pkexec", "systemctl", "restart", service],
                                  capture_output=True, text=True, timeout=30)
            if result.returncode == 0:
                self.status.setText(f"Restarted service: {service}" if not TR else f"Hizmet yeniden başlatıldı: {service}")
                QMessageBox.information(self, _("Success"), f"Service '{service}' restarted successfully." if not TR else f"'{service}' hizmeti başarıyla yeniden başlatıldı.")
            else:
                result2 = subprocess.run(["sudo", "systemctl", "restart", service],
                                       capture_output=True, text=True, timeout=30)
                if result2.returncode == 0:
                    self.status.setText(f"Restarted service: {service}" if not TR else f"Hizmet yeniden başlatıldı: {service}")
                    QMessageBox.information(self, _("Success"), f"Service '{service}' restarted successfully." if not TR else f"'{service}' hizmeti başarıyla yeniden başlatıldı.")
                else:
                    raise Exception(f"systemctl restart failed: {result2.stderr}")
            return True
        except subprocess.TimeoutExpired:
            QMessageBox.warning(self, _("Timeout"), _("Service restart timed out."))
        except FileNotFoundError:
            QMessageBox.warning(self, _("Not Found"), _("systemctl not found. Not a systemd system?"))
        except Exception as e:
            QMessageBox.warning(self, _("Error"), f"Failed to restart service:\n{e}" if not TR else f"Hizmet yeniden başlatılamadı:\n{e}")
        return False

    def kill_process(self):
        pids = self.get_selected_pids()
        if not pids:
            QMessageBox.information(self, _("Select"), _("Select a process to end."))
            return
        pids = [p for p in pids if p not in (0, 1)]
        if not pids:
            QMessageBox.warning(self, _("Blocked"), _("Cannot kill PID 0/1."))
            return
        if len(pids) > 1:
            if QMessageBox.question(self, _("End Task"),
                    f"End {len(pids)} processes?\nThis may cause data loss." if not TR else f"{len(pids)} işlem sonlandırılsın mı?\nVeri kaybına yol açabilir.") != QMessageBox.StandardButton.Yes:
                return
            done, denied = 0, 0
            for pid in pids:
                try:
                    psutil.Process(pid).terminate()
                    done += 1
                except psutil.NoSuchProcess:
                    done += 1
                except psutil.AccessDenied:
                    denied += 1
                except Exception:
                    denied += 1
            self.status.setText(f"Terminated {done}, denied {denied}" if not TR else f"Sonlandırıldı: {done}, red: {denied}")
            self.refresh_processes()
            return
        pid = pids[0]
        name_item = self.table.item(self.table.currentRow(), 1)
        user_item = self.table.item(self.table.currentRow(), 6)
        name = name_item.text() if name_item else str(pid)
        user = user_item.text() if user_item else ""
        if self.is_system_process(pid, name, user):
            self.restart_service(pid, name, user)
            self.refresh_processes()
            return
        if QMessageBox.question(self, _("End Task"), f"End process {name} (PID {pid})?\nThis may cause data loss." if not TR else f"{name} işlemi (PID {pid}) sonlandırılsın mı?\nVeri kaybına yol açabilir.") != QMessageBox.StandardButton.Yes:
            return
        try:
            p = psutil.Process(pid)
            p.terminate()
            p.wait(timeout=3)
            self.status.setText(f"Terminated PID {pid}" if not TR else f"PID {pid} sonlandırıldı")
        except psutil.NoSuchProcess:
            QMessageBox.information(self, _("Gone"), _("Process already exited."))
        except psutil.AccessDenied:
            QMessageBox.warning(self, _("Access Denied"), f"Cannot terminate PID {pid}. Try running with sudo:\n\nsudo ali-ltask" if not TR else f"PID {pid} sonlandırılamadı. sudo ile dene:\n\nsudo ali-ltask")
        except psutil.TimeoutExpired:
            try:
                psutil.Process(pid).kill()
                self.status.setText(f"Killed PID {pid}" if not TR else f"PID {pid} öldürüldü")
            except Exception as e:
                QMessageBox.warning(self, _("Error"), str(e))
        except Exception as e:
            QMessageBox.warning(self, _("Error"), str(e))
        self.refresh_processes()

    def kill_tree(self):
        pid = self.get_selected_pid()
        if pid is None:
            QMessageBox.information(self, _("Select"), _("Select a process."))
            return
        if QMessageBox.question(self, _("End Process Tree"), f"Kill process tree for PID {pid} and all children?" if not TR else f"PID {pid} ve tüm alt işlemleri öldürülsün mü?") != QMessageBox.StandardButton.Yes:
            return
        try:
            parent = psutil.Process(pid)
            children = parent.children(recursive=True)
            for c in children:
                try:
                    c.terminate()
                except Exception:
                    pass
            parent.terminate()
            gone, alive = psutil.wait_procs(children + [parent], timeout=3)
            for p in alive:
                try:
                    p.kill()
                except Exception:
                    pass
            self.status.setText(f"Killed tree PID {pid}" if not TR else f"PID {pid} ağacı öldürüldü")
        except Exception as e:
            QMessageBox.warning(self, _("Error"), str(e))
        self.refresh_processes()

    def export_csv(self):
        import csv
        path, _f = QFileDialog.getSaveFileName(self, _("Export CSV"),
            os.path.expanduser("~/ltask-processes.csv"), "CSV (*.csv)")
        if not path:
            return
        try:
            with open(path, "w", newline="", encoding="utf-8") as f:
                w = csv.writer(f)
                w.writerow(COLS)
                for p in self.all_procs:
                    w.writerow(list(p))
            self.status.setText(f"{_('Exported')}: {path}")
        except Exception as e:
            QMessageBox.warning(self, _("Error"), str(e))

    def kill_by_name(self):
        name, ok = QInputDialog.getText(self, _("Kill by Name…"), _("Exact process name:"))
        if not ok or not name.strip():
            return
        name = name.strip()
        if name in ("init", "systemd"):
            QMessageBox.warning(self, _("Blocked"), _("Cannot kill PID 0/1."))
            return
        hits = [(p[0], p[1]) for p in self.all_procs if p[1] == name and p[0] not in (0, 1)]
        if not hits:
            QMessageBox.information(self, _("Gone"), _("No such process.") if not TR else "Böyle bir işlem yok.")
            return
        if QMessageBox.question(self, _("End Task"),
                f"End {len(hits)}x '{name}' (PIDs {', '.join(str(h[0]) for h in hits[:8])})?" if not TR else f"{len(hits)} adet '{name}' sonlandırılsın mı?") != QMessageBox.StandardButton.Yes:
            return
        done, denied = 0, 0
        for pid, _n in hits:
            try:
                psutil.Process(pid).terminate()
                done += 1
            except psutil.NoSuchProcess:
                done += 1
            except Exception:
                denied += 1
        self.status.setText(f"Terminated {done}, denied {denied}" if not TR else f"Sonlandırıldı: {done}, red: {denied}")
        self.refresh_processes()

    def show_context_menu(self, pos):
        row = self.table.rowAt(pos.y())
        if row >= 0:
            self.table.selectRow(row)
        pid = self.get_selected_pid()
        if pid is None:
            return
        name_item = self.table.item(self.table.currentRow(), 1)
        user_item = self.table.item(self.table.currentRow(), 6)
        name = name_item.text() if name_item else str(pid)
        user = user_item.text() if user_item else ""
        is_system = self.is_system_process(pid, name, user)

        menu = QMenu(self)
        act_details = menu.addAction(f"Details - {name} ({pid})" if not TR else f"Ayrıntılar - {name} ({pid})")
        act_copy = menu.addAction(_("Copy PID / Command"))
        menu.addSeparator()
        if is_system:
            act_restart = menu.addAction(_("Restart Service"))
            act_end = None
        else:
            act_end = menu.addAction(_("End Task"))
            act_restart = None
        act_tree = menu.addAction(_("End Process Tree"))
        menu.addSeparator()
        act_refresh = menu.addAction(_("Refresh"))
        act_suspend = menu.addAction(_("Suspend"))
        act_resume = menu.addAction(_("Resume"))

        action = menu.exec(self.table.viewport().mapToGlobal(pos))
        if action == act_details:
            self.show_details()
        elif action == act_copy:
            cmd_item = self.table.item(self.table.currentRow(), 7)
            cmd = cmd_item.text() if cmd_item else name
            QApplication.clipboard().setText(f"{pid} - {cmd}")
            self.status.setText(f"Copied PID {pid} to clipboard" if not TR else f"PID {pid} panoya kopyalandı")
        elif action == act_end:
            self.kill_process()
        elif action == act_restart:
            self.restart_service(pid, name, user)
            self.refresh_processes()
        elif action == act_tree:
            self.kill_tree()
        elif action == act_refresh:
            self.refresh_processes()
        elif action == act_suspend:
            self.suspend_process()
        elif action == act_resume:
            self.resume_process()

    def suspend_process(self):
        pid = self.get_selected_pid()
        if pid is None:
            return
        try:
            psutil.Process(pid).suspend()
            self.status.setText(f"Suspended PID {pid}" if not TR else f"PID {pid} askıya alındı")
        except psutil.AccessDenied:
            QMessageBox.warning(self, _("Access Denied"), f"Cannot suspend PID {pid}. Try sudo." if not TR else f"PID {pid} askıya alınamadı. sudo dene.")
        except Exception as e:
            QMessageBox.warning(self, _("Error"), str(e))
        self.refresh_processes()

    def resume_process(self):
        pid = self.get_selected_pid()
        if pid is None:
            return
        try:
            psutil.Process(pid).resume()
            self.status.setText(f"Resumed PID {pid}" if not TR else f"PID {pid} sürdürüldü")
        except psutil.AccessDenied:
            QMessageBox.warning(self, _("Access Denied"), f"Cannot resume PID {pid}. Try sudo." if not TR else f"PID {pid} sürdürülemedi. sudo dene.")
        except Exception as e:
            QMessageBox.warning(self, _("Error"), str(e))
        self.refresh_processes()

    def show_details(self):
        row = self.table.currentRow()
        if row < 0:
            return
        pid = self.get_selected_pid()
        if pid is None:
            return
        try:
            p = psutil.Process(pid)
            with p.oneshot():
                info = f"PID: {p.pid}\nName: {p.name()}\nStatus: {p.status()}\nUser: {p.username()}\nCPU: {p.cpu_percent():.1f}%\nMemory: {p.memory_percent():.1f}% ({format_mb(p.memory_info().rss)} MB)\nThreads: {p.num_threads()}\nCreate time: {datetime.datetime.fromtimestamp(p.create_time())}\n\nCmdline:\n{' '.join(p.cmdline()) or p.name()}\n\nExe:\n{p.exe()}\n\nCWD:\n{p.cwd()}" if not TR else f"PID: {p.pid}\nAd: {p.name()}\nDurum: {p.status()}\nKullanıcı: {p.username()}\nCPU: %{p.cpu_percent():.1f}\nBellek: %{p.memory_percent():.1f} ({format_mb(p.memory_info().rss)} MB)\nİş parçacığı: {p.num_threads()}\nOluşturulma: {datetime.datetime.fromtimestamp(p.create_time())}\n\nKomut satırı:\n{' '.join(p.cmdline()) or p.name()}\n\nExe:\n{p.exe()}\n\nCWD:\n{p.cwd()}"
            QMessageBox.information(self, f"Details - PID {pid}" if not TR else f"Ayrıntılar - PID {pid}", info)
        except Exception as e:
            QMessageBox.warning(self, _("Error"), str(e))

def main():
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    w = ProcessManager()
    w.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
