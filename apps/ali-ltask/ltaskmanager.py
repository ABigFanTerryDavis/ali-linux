# ALI Linux port (1.2.6) of LTaskManager by MerixCipher (MIT):
# https://github.com/MerixCipher/LTaskManager @ 5cb21b3
# ALI diff vs upstream: window/sidebar titles + footer rebranded.
# Everything else is byte-identical upstream - send app bugs there.
import sys
import psutil
import datetime
import subprocess
from collections import deque
from PyQt6.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QTableWidget, QTableWidgetItem, QPushButton, QLineEdit, QLabel,
    QHeaderView, QMessageBox, QProgressBar, QComboBox, QMenu, QStackedWidget, QGridLayout, QFrame
)
from PyQt6.QtCore import Qt, QTimer
from PyQt6.QtGui import QColor, QBrush

try:
    import pyqtgraph as pg
    HAS_PYQTGRAPH = True
except Exception:
    HAS_PYQTGRAPH = False

COLS = ["PID", "Name", "CPU %", "Memory %", "Memory MB", "Status", "User", "Command"]
REFRESH_MS = 2000

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
        title = QLabel("Performance")
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
            self.cpu_label = QLabel("CPU — 0%")
            self.cpu_label.setStyleSheet("font-weight: bold;")
            self.cpu_plot = pg.PlotWidget()
            self.setup_plot(self.cpu_plot, "CPU %", "red")
            self.cpu_curve = self.cpu_plot.plot(list(self.cpu_hist), pen=pg.mkPen("#e53935", width=2))

            # Memory graph
            self.mem_label = QLabel("Memory — 0%")
            self.mem_label.setStyleSheet("font-weight: bold;")
            self.mem_plot = pg.PlotWidget()
            self.setup_plot(self.mem_plot, "Memory %", "#1e88e5")
            self.mem_curve = self.mem_plot.plot(list(self.mem_hist), pen=pg.mkPen("#1e88e5", width=2))

            # Disk graph
            self.disk_label = QLabel("Disk I/O — 0 MB/s")
            self.disk_label.setStyleSheet("font-weight: bold;")
            self.disk_plot = pg.PlotWidget()
            self.setup_plot(self.disk_plot, "Disk MB/s", "#43a047")
            self.disk_curve = self.disk_plot.plot(list(self.disk_hist), pen=pg.mkPen("#43a047", width=2))

            # Network graph
            self.net_label = QLabel("Network I/O — 0 MB/s")
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
            self.cpu_detail = QLabel("CPU detail")
            self.cpu_detail.setStyleSheet("font-size: 11px; color: #444;")
            self.mem_detail = QLabel("Memory detail")
            self.mem_detail.setStyleSheet("font-size: 11px; color: #444;")
            self.disk_detail = QLabel("Disk detail")
            self.disk_detail.setStyleSheet("font-size: 11px; color: #444;")
            self.net_detail = QLabel("Network detail")
            self.net_detail.setStyleSheet("font-size: 11px; color: #444;")
            for w in [self.cpu_detail, self.mem_detail, self.disk_detail, self.net_detail]:
                bottom.addWidget(w)
            layout.addLayout(bottom)

            # GPU placeholder (no nvidia-smi on this machine)
            self.gpu_label = QLabel("GPU: No dedicated GPU detected (nvidia-smi not found) — Integrated graphics")
            self.gpu_label.setStyleSheet("background: #f5f5f5; border: 1px solid #ddd; padding: 8px; font-size: 11px; color: #666;")
            layout.addWidget(self.gpu_label)
        else:
            # fallback without pyqtgraph
            fallback = QLabel("pyqtgraph not installed — install with: pip install pyqtgraph\n\nShowing bars instead of graphs.")
            fallback.setStyleSheet("color: #d32f2f;")
            layout.addWidget(fallback)
            self.cpu_bar = QProgressBar()
            self.mem_bar = QProgressBar()
            self.disk_bar = QProgressBar()
            layout.addWidget(QLabel("CPU"))
            layout.addWidget(self.cpu_bar)
            layout.addWidget(QLabel("Memory"))
            layout.addWidget(self.mem_bar)
            layout.addWidget(QLabel("Disk"))
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

            self.cpu_label.setText(f"CPU — {cpu:.1f}%")
            self.mem_label.setText(f"Memory — {mem.percent:.1f}% ({format_mb(mem.used)} / {format_mb(mem.total)} MB)")
            self.disk_label.setText(f"Disk I/O — {disk_rate:.1f} MB/s")
            self.net_label.setText(f"Network I/O — {net_rate:.1f} MB/s")

            freq = psutil.cpu_freq()
            freq_str = f"{freq.current:.0f} MHz" if freq else "N/A"
            self.cpu_detail.setText(f"Cores: {psutil.cpu_count()} • Freq: {freq_str} • Uptime: {str(datetime.datetime.now() - datetime.datetime.fromtimestamp(psutil.boot_time())).split('.')[0]}")
            self.mem_detail.setText(f"Avail: {format_bytes(mem.available)} • Used: {format_bytes(mem.used)}")
            self.disk_detail.setText(f"Used: {format_bytes(disk.used)} / {format_bytes(disk.total)} ({disk.percent}%)")
            self.net_detail.setText(f"Sent: {format_bytes(net_io.bytes_sent)} • Recv: {format_bytes(net_io.bytes_recv)}")
        else:
            self.cpu_bar.setValue(int(cpu))
            self.mem_bar.setValue(int(mem.percent))
            self.disk_bar.setValue(int(disk.percent))


class ProcessManager(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("ALI Task Manager - Process Manager")
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
        sidebar.setStyleSheet("background: #202124;")
        sb_layout = QVBoxLayout(sidebar)
        sb_layout.setContentsMargins(8,12,8,12)
        sb_layout.setSpacing(6)

        sb_title = QLabel("ALI Task Manager")
        sb_title.setStyleSheet("color: white; font-size: 15px; font-weight: bold; padding: 8px 4px;")
        sb_layout.addWidget(sb_title)

        self.btn_proc = QPushButton("  ◉  Processes")
        self.btn_perf = QPushButton("  ◈  Performance")
        for b in [self.btn_proc, self.btn_perf]:
            b.setFixedHeight(42)
            b.setCursor(Qt.CursorShape.PointingHandCursor)
            b.setStyleSheet("""
                QPushButton { text-align: left; padding-left: 12px; color: #e8eaed; background: transparent; border: none; font-size: 13px; border-radius: 6px;}
                QPushButton:hover { background: #3c4043; }
            """)
        self.btn_proc.setStyleSheet("""
            QPushButton { text-align: left; padding-left: 12px; color: white; background: #3c4043; border: none; font-size: 13px; border-radius: 6px; font-weight: bold;}
        """)
        sb_layout.addWidget(self.btn_proc)
        sb_layout.addWidget(self.btn_perf)
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

        title = QLabel("Process Manager")
        title.setStyleSheet("font-size: 18px; font-weight: bold; padding: 4px;")
        layout.addWidget(title)

        stats_layout = QHBoxLayout()
        self.cpu_label = QLabel("CPU: --")
        self.mem_label = QLabel("Memory: --")
        self.disk_label = QLabel("Disk: --")
        self.proc_count_label = QLabel("Processes: --")
        self.uptime_label = QLabel("Uptime: --")

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
        self.search.setPlaceholderText("Search PID / Name / User / Command...")
        self.search.textChanged.connect(self.apply_filter)
        top_layout.addWidget(self.search, stretch=3)

        self.user_filter = QComboBox()
        self.user_filter.addItem("All Users")
        self.user_filter.currentTextChanged.connect(self.apply_filter)
        top_layout.addWidget(self.user_filter)

        self.sort_combo = QComboBox()
        self.sort_combo.addItems(["CPU %", "Memory %", "PID", "Name"])
        self.sort_combo.setCurrentText("CPU %")
        self.sort_combo.currentTextChanged.connect(self.on_sort_changed)
        top_layout.addWidget(QLabel("Sort:"))
        top_layout.addWidget(self.sort_combo)

        self.refresh_btn = QPushButton("Refresh")
        self.refresh_btn.clicked.connect(self.refresh_processes)
        top_layout.addWidget(self.refresh_btn)

        self.kill_btn = QPushButton("End Task")
        self.kill_btn.setStyleSheet("background-color: #d32f2f; color: white; font-weight: bold; padding: 6px 14px;")
        self.kill_btn.clicked.connect(self.kill_process)
        top_layout.addWidget(self.kill_btn)

        self.kill_tree_btn = QPushButton("End Process Tree")
        self.kill_tree_btn.clicked.connect(self.kill_tree)
        top_layout.addWidget(self.kill_tree_btn)

        layout.addLayout(top_layout)

        self.table = QTableWidget(0, len(COLS))
        self.table.setHorizontalHeaderLabels(COLS)
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
        self.table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        self.table.setAlternatingRowColors(True)
        self.table.setSortingEnabled(False)
        self.table.doubleClicked.connect(self.show_details)
        self.table.horizontalHeader().sectionClicked.connect(self.on_header_clicked)
        self.table.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        self.table.customContextMenuRequested.connect(self.show_context_menu)
        layout.addWidget(self.table)

        self.status = QLabel("Loading...")
        self.status.setStyleSheet("color: #666; font-size: 11px;")
        layout.addWidget(self.status)

        # PAGE 1: PERFORMANCE
        self.page_perf = PerformanceWidget()

        self.stack.addWidget(self.page_proc)
        self.stack.addWidget(self.page_perf)
        self.stack.setCurrentIndex(0)

        # sidebar switching
        self.btn_proc.clicked.connect(lambda: self.switch_tab(0))
        self.btn_perf.clicked.connect(lambda: self.switch_tab(1))

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
        active = "QPushButton { text-align: left; padding-left: 12px; color: white; background: #3c4043; border: none; font-size: 13px; border-radius: 6px; font-weight: bold;}"
        inactive = "QPushButton { text-align: left; padding-left: 12px; color: #e8eaed; background: transparent; border: none; font-size: 13px; border-radius: 6px;}"
        self.btn_proc.setStyleSheet(active if idx==0 else inactive)
        self.btn_perf.setStyleSheet(active if idx==1 else inactive)

    def on_sort_changed(self, text):
        mapping = {"PID": 0, "Name": 1, "CPU %": 2, "Memory %": 3}
        self.sort_col = mapping.get(text, 2)
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
            self.sort_combo.setCurrentText(rev[col])
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

            self.cpu_label.setText(f"CPU: {cpu_total:.1f}%")
            self.cpu_bar.setValue(int(cpu_total))
            self.mem_label.setText(f"Memory: {mem.percent:.1f}% ({format_mb(mem.used)} / {format_mb(mem.total)} MB)")
            self.mem_bar.setValue(int(mem.percent))
            self.disk_label.setText(f"Disk: {disk.percent:.1f}%")
            self.uptime_label.setText(f"Uptime: {hours}h {mins}m")

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
                    users.add(user)
                    procs.append((pid, name, cpu, mem_p, mem_mb, status, user, cmd))
                except (psutil.NoSuchProcess, psutil.AccessDenied):
                    continue

            self.all_procs = procs
            self.proc_count_label.setText(f"Processes: {len(procs)}")

            current_user = self.user_filter.currentText()
            self.user_filter.blockSignals(True)
            self.user_filter.clear()
            self.user_filter.addItem("All Users")
            for u in sorted(users):
                if u:
                    self.user_filter.addItem(u)
            if current_user in [self.user_filter.itemText(i) for i in range(self.user_filter.count())]:
                self.user_filter.setCurrentText(current_user)
            self.user_filter.blockSignals(False)

            self.apply_filter()
        except Exception as e:
            self.status.setText(f"Error: {e}")

    def apply_filter(self):
        if not self.all_procs:
            return
        search = self.search.text().lower().strip()
        user_f = self.user_filter.currentText()

        filtered = []
        for p in self.all_procs:
            pid, name, cpu, mem_p, mem_mb, status, user, cmd = p
            if user_f != "All Users" and user != user_f:
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

        self.status.setText(f"Showing {len(filtered)} / {len(self.all_procs)} processes | Sort: {COLS[self.sort_col]} {'↓' if self.sort_desc else '↑'} | Auto-refresh {REFRESH_MS//1000}s")

    def get_selected_pid(self):
        row = self.table.currentRow()
        if row < 0:
            return None
        item = self.table.item(row, 0)
        if not item:
            return None
        return int(item.data(Qt.ItemDataRole.UserRole))

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
        if QMessageBox.question(self, "Restart Service",
            f"Restart system service '{service}'?\n\n"
            f"PID: {pid} | User: {user} | Name: {name}\n\n"
            f"This will run: sudo systemctl restart {service}") != QMessageBox.StandardButton.Yes:
            return False
        try:
            result = subprocess.run(["pkexec", "systemctl", "restart", service],
                                  capture_output=True, text=True, timeout=30)
            if result.returncode == 0:
                self.status.setText(f"Restarted service: {service}")
                QMessageBox.information(self, "Success", f"Service '{service}' restarted successfully.")
            else:
                result2 = subprocess.run(["sudo", "systemctl", "restart", service],
                                       capture_output=True, text=True, timeout=30)
                if result2.returncode == 0:
                    self.status.setText(f"Restarted service: {service}")
                    QMessageBox.information(self, "Success", f"Service '{service}' restarted successfully.")
                else:
                    raise Exception(f"systemctl restart failed: {result2.stderr}")
            return True
        except subprocess.TimeoutExpired:
            QMessageBox.warning(self, "Timeout", "Service restart timed out.")
        except FileNotFoundError:
            QMessageBox.warning(self, "Not Found", "systemctl not found. Not a systemd system?")
        except Exception as e:
            QMessageBox.warning(self, "Error", f"Failed to restart service:\n{e}")
        return False

    def kill_process(self):
        pid = self.get_selected_pid()
        if pid is None:
            QMessageBox.information(self, "Select", "Select a process to end.")
            return
        if pid == 1 or pid == 0:
            QMessageBox.warning(self, "Blocked", "Cannot kill PID 0/1.")
            return
        name_item = self.table.item(self.table.currentRow(), 1)
        user_item = self.table.item(self.table.currentRow(), 6)
        name = name_item.text() if name_item else str(pid)
        user = user_item.text() if user_item else ""
        if self.is_system_process(pid, name, user):
            self.restart_service(pid, name, user)
            self.refresh_processes()
            return
        if QMessageBox.question(self, "End Task", f"End process {name} (PID {pid})?\nThis may cause data loss.") != QMessageBox.StandardButton.Yes:
            return
        try:
            p = psutil.Process(pid)
            p.terminate()
            p.wait(timeout=3)
            self.status.setText(f"Terminated PID {pid}")
        except psutil.NoSuchProcess:
            QMessageBox.information(self, "Gone", "Process already exited.")
        except psutil.AccessDenied:
            QMessageBox.warning(self, "Access Denied", f"Cannot terminate PID {pid}. Try running with sudo:\n\nsudo python3 main.py")
        except psutil.TimeoutExpired:
            try:
                psutil.Process(pid).kill()
                self.status.setText(f"Killed PID {pid}")
            except Exception as e:
                QMessageBox.warning(self, "Error", str(e))
        except Exception as e:
            QMessageBox.warning(self, "Error", str(e))
        self.refresh_processes()

    def kill_tree(self):
        pid = self.get_selected_pid()
        if pid is None:
            QMessageBox.information(self, "Select", "Select a process.")
            return
        if QMessageBox.question(self, "End Process Tree", f"Kill process tree for PID {pid} and all children?") != QMessageBox.StandardButton.Yes:
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
            self.status.setText(f"Killed tree PID {pid}")
        except Exception as e:
            QMessageBox.warning(self, "Error", str(e))
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
        act_details = menu.addAction(f"Details - {name} ({pid})")
        act_copy = menu.addAction("Copy PID / Command")
        menu.addSeparator()
        if is_system:
            act_restart = menu.addAction("Restart Service")
            act_end = None
        else:
            act_end = menu.addAction("End Task")
            act_restart = None
        act_tree = menu.addAction("End Process Tree")
        menu.addSeparator()
        act_refresh = menu.addAction("Refresh")
        act_suspend = menu.addAction("Suspend")
        act_resume = menu.addAction("Resume")

        action = menu.exec(self.table.viewport().mapToGlobal(pos))
        if action == act_details:
            self.show_details()
        elif action == act_copy:
            cmd_item = self.table.item(self.table.currentRow(), 7)
            cmd = cmd_item.text() if cmd_item else name
            QApplication.clipboard().setText(f"{pid} - {cmd}")
            self.status.setText(f"Copied PID {pid} to clipboard")
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
            self.status.setText(f"Suspended PID {pid}")
        except psutil.AccessDenied:
            QMessageBox.warning(self, "Access Denied", f"Cannot suspend PID {pid}. Try sudo.")
        except Exception as e:
            QMessageBox.warning(self, "Error", str(e))
        self.refresh_processes()

    def resume_process(self):
        pid = self.get_selected_pid()
        if pid is None:
            return
        try:
            psutil.Process(pid).resume()
            self.status.setText(f"Resumed PID {pid}")
        except psutil.AccessDenied:
            QMessageBox.warning(self, "Access Denied", f"Cannot resume PID {pid}. Try sudo.")
        except Exception as e:
            QMessageBox.warning(self, "Error", str(e))
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
                info = f"PID: {p.pid}\nName: {p.name()}\nStatus: {p.status()}\nUser: {p.username()}\nCPU: {p.cpu_percent():.1f}%\nMemory: {p.memory_percent():.1f}% ({format_mb(p.memory_info().rss)} MB)\nThreads: {p.num_threads()}\nCreate time: {datetime.datetime.fromtimestamp(p.create_time())}\n\nCmdline:\n{' '.join(p.cmdline()) or p.name()}\n\nExe:\n{p.exe()}\n\nCWD:\n{p.cwd()}"
            QMessageBox.information(self, f"Details - PID {pid}", info)
        except Exception as e:
            QMessageBox.warning(self, "Error", str(e))

def main():
    app = QApplication(sys.argv)
    app.setStyle("Fusion")
    w = ProcessManager()
    w.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
