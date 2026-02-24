import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, filedialog
import subprocess
import os
import sys
import time
import threading
import glob
from datetime import datetime
import shutil
import json
import shlex
import mysql.connector

class ServerManagerApp(tk.Tk):
    def __init__(self):
        super().__init__()

        self.title("EQEmu Server Manager")
        self.geometry("1100x850")
        self.portrait_breakpoint = 1300

        self.processes = {}
        self.settings_path = os.path.join(os.getcwd(), ".server_manager_settings.json")
        self._settings = self._load_manager_settings()

        # Define the standard server processes
        self.process_info = [
            {"name": "loginserver", "display": "Login Server", "args": ""},
            {"name": "world", "display": "World Server", "args": ""},
            {"name": "ucs", "display": "UCS (Chat)", "args": ""},
            {"name": "queryserv", "display": "Query Server", "args": ""},
            {"name": "eqlaunch", "display": "EQ Launch", "args": "node_1"},
            {"name": "zone", "display": "Zone Server", "args": "."},
        ]

        self.build_dir = os.path.join(os.getcwd(), "build")
        self.bin_dir = self.find_bin_dir()
        # DLL search path (only care about this one build output)
        self.extra_dll_dir = os.path.join("extras", "eq-core-dll-main", "bin")
        self.vcpkg_bin_dir = os.path.join(os.getcwd(), "vcpkg", "vcpkg-export-x64", "installed", "x64-windows", "bin")
        self.perl_bin_dir = os.path.join(os.getcwd(), "perl", "x64", "perl", "bin")
        # Default EQ client directory for exports/copies
        self.eq_dir_var = tk.StringVar(value=self._settings.get("eq_dir", r"D:\Rof2"))
        self.build_target_var = tk.StringVar(value=self._settings.get("build_target", "all"))
        self.layout_mode_var = tk.StringVar(value=self._settings.get("layout_mode", "auto"))
        self.log_follow_var = tk.BooleanVar(value=True)
        self.selected_log_path_var = tk.StringVar(value="")
        self.log_source_var = tk.StringVar(value="Source: none")
        self.process_summary_var = tk.StringVar(value="Running: 0/0  |  External: 0")
        self.log_file_entries = {}
        self._log_tail_pos = 0
        self._active_layout_mode = None
        self._active_proc_cols = None

        self.create_widgets()
        self._apply_saved_process_settings()
        self.eq_dir_var.trace_add("write", self._on_setting_changed)
        self.build_target_var.trace_add("write", self._on_setting_changed)
        self.layout_mode_var.trace_add("write", self._on_layout_mode_changed)
        self.bind("<Configure>", self._on_window_configure)
        self.update_status_loop()

    def find_bin_dir(self):
        possible_paths = [
            os.path.join(self.build_dir, "bin", "RelWithDebInfo"),
            os.path.join(self.build_dir, "bin", "Release"),
            os.path.join(self.build_dir, "bin", "Debug"),
            os.path.join(self.build_dir, "bin"),
        ]
        for path in possible_paths:
            if os.path.exists(path):
                return path
        return os.path.join(self.build_dir, "bin")

    def ui_call(self, fn, *args, **kwargs):
        if threading.current_thread() is threading.main_thread():
            fn(*args, **kwargs)
        else:
            self.after(0, lambda: fn(*args, **kwargs))

    def _set_build_controls_enabled(self, enabled):
        state = "normal" if enabled else "disabled"
        if hasattr(self, "build_btn"):
            self.build_btn.config(state=state)
        if hasattr(self, "clean_btn"):
            self.clean_btn.config(state=state)

    def _set_build_status(self, text):
        if hasattr(self, "build_status_lbl"):
            self.build_status_lbl.config(text=text)

    def _set_text_widget(self, widget, text):
        widget.delete(1.0, "end")
        widget.insert("end", text)
        widget.see("end")

    def _append_text_widget(self, widget, text):
        widget.insert("end", text)
        widget.see("end")

    def _load_manager_settings(self):
        try:
            if os.path.exists(self.settings_path):
                with open(self.settings_path, "r", encoding="utf-8") as f:
                    data = json.load(f)
                if isinstance(data, dict):
                    return data
        except Exception:
            pass
        return {}

    def _save_manager_settings(self):
        try:
            data = {
                "eq_dir": self.eq_dir_var.get(),
                "build_target": self.build_target_var.get(),
                "layout_mode": self.layout_mode_var.get(),
                "process_args": {},
                "process_console": {},
            }
            if hasattr(self, "proc_widgets"):
                for name, widgets in self.proc_widgets.items():
                    data["process_args"][name] = widgets["args_var"].get()
                    data["process_console"][name] = bool(widgets["console_var"].get())
            with open(self.settings_path, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=2)
        except Exception as e:
            self.log(f"Warning: failed to save manager settings: {e}")

    def _on_setting_changed(self, *_):
        self._save_manager_settings()

    def _on_layout_mode_changed(self, *_):
        self._save_manager_settings()
        self._apply_main_layout(force=True)

    def _apply_saved_process_settings(self):
        saved_args = self._settings.get("process_args", {})
        saved_console = self._settings.get("process_console", {})
        for name, widgets in self.proc_widgets.items():
            if name in saved_args:
                widgets["args_var"].set(saved_args[name])
            if name in saved_console:
                widgets["console_var"].set(bool(saved_console[name]))
            widgets["args_var"].trace_add("write", self._on_setting_changed)
            widgets["console_var"].trace_add("write", self._on_setting_changed)
            widgets["args_var"].trace_add("write", lambda *_args, n=name: self._on_process_option_changed(n))
            widgets["console_var"].trace_add("write", lambda *_args, n=name: self._on_process_option_changed(n))
            self._update_process_hint(name)

    def _on_process_option_changed(self, name):
        self._update_process_hint(name)

    def _build_process_hint(self, name):
        widgets = self.proc_widgets.get(name)
        if not widgets:
            return ""
        args = (widgets["args_var"].get() or "").strip()
        console_mode = bool(widgets["console_var"].get())
        parts = []
        if args:
            parts.append(f"Args: {args}")
        else:
            parts.append("Args: default")
        if console_mode:
            parts.append("Console: separate window")
        return "  |  ".join(parts)

    def _update_process_hint(self, name):
        widgets = self.proc_widgets.get(name)
        if not widgets:
            return
        hint_text = self._build_process_hint(name)
        if "hint_var" in widgets:
            widgets["hint_var"].set(hint_text)

    def create_widgets(self):
        self._configure_styles()

        # Create Tabs
        tab_control = ttk.Notebook(self)

        self.main_tab = ttk.Frame(tab_control)
        self.database_tab = ttk.Frame(tab_control)
        self.tools_tab = ttk.Frame(tab_control)

        tab_control.add(self.main_tab, text="Server Control")
        tab_control.add(self.database_tab, text="Database Tools")
        tab_control.add(self.tools_tab, text="Quick Actions & Scripts")
        tab_control.pack(expand=1, fill="both")

        # --- Main Tab ---
        self.create_main_tab()

        # --- Database Tab ---
        self.create_database_tab()

        # --- Tools Tab ---
        self.create_tools_tab()

        # Kick off initial status checks
        self.after(100, self.refresh_status_indicators)

    def _configure_styles(self):
        style = ttk.Style()
        try:
            style.theme_use("clam")
        except Exception:
            pass

        style.configure("Shell.TFrame", background="#f4f6f8")
        style.configure("OpsRail.TFrame", background="#eef2f6")
        style.configure("Workspace.TFrame", background="#f4f6f8")
        style.configure("Overview.TFrame", background="#dfe7ef")
        style.configure("OverviewTitle.TLabel", font=("Segoe UI Semibold", 11), background="#dfe7ef")
        style.configure("OverviewValue.TLabel", font=("Segoe UI", 10), background="#dfe7ef")
        style.configure("ProcessCard.TFrame", background="#ffffff", relief="solid", borderwidth=1)
        style.configure("ProcessTitle.TLabel", font=("Segoe UI Semibold", 10), background="#ffffff")
        style.configure("ProcessHint.TLabel", font=("Consolas", 8), foreground="#596777", background="#ffffff")
        style.configure("ProcessRunning.TFrame", background="#e6f4ea", relief="solid", borderwidth=1)
        style.configure("ProcessExternal.TFrame", background="#fff4e5", relief="solid", borderwidth=1)
        style.configure("ProcessStopped.TFrame", background="#ffffff", relief="solid", borderwidth=1)

    def create_main_tab(self):
        shell = ttk.Frame(self.main_tab, style="Shell.TFrame", padding=(10, 8, 10, 8))
        shell.pack(fill="both", expand=True)
        self.main_shell = shell

        ops_rail = ttk.Frame(shell, style="OpsRail.TFrame", padding=(8, 8, 8, 8))
        ops_rail.grid_columnconfigure(0, weight=1)
        self.ops_rail = ops_rail

        workspace = ttk.Frame(shell, style="Workspace.TFrame")
        workspace.grid_columnconfigure(0, weight=1)
        workspace.grid_rowconfigure(2, weight=1)
        self.workspace = workspace

        build_card = ttk.LabelFrame(ops_rail, text="Build & Deploy", padding=8)
        build_card.grid(row=0, column=0, sticky="ew", pady=(0, 8))
        build_card.grid_columnconfigure(1, weight=1)

        self.build_btn = ttk.Button(build_card, text="Build", command=self.run_build_thread)
        self.build_btn.grid(row=0, column=0, padx=(0, 6), pady=3, sticky="w")
        self.clean_btn = ttk.Button(build_card, text="Clean", command=self.clean_build)
        self.clean_btn.grid(row=0, column=1, padx=(0, 6), pady=3, sticky="w")
        self.build_status_lbl = ttk.Label(build_card, text="Ready")
        self.build_status_lbl.grid(row=0, column=2, padx=2, pady=3, sticky="e")
        ttk.Label(build_card, text="Target").grid(row=1, column=0, padx=(0, 6), pady=3, sticky="w")
        self.build_target_combo = ttk.Combobox(
            build_card,
            textvariable=self.build_target_var,
            width=18,
            state="readonly",
            values=["all", "zone", "world", "loginserver", "ucs", "queryserv", "shared_memory", "eqlaunch"],
        )
        self.build_target_combo.grid(row=1, column=1, columnspan=2, padx=0, pady=3, sticky="ew")

        server_card = ttk.LabelFrame(ops_rail, text="Server Actions", padding=8)
        server_card.grid(row=1, column=0, sticky="ew", pady=(0, 8))
        server_card.grid_columnconfigure(0, weight=1)
        server_card.grid_columnconfigure(1, weight=1)

        self.start_all_btn = ttk.Button(server_card, text="Start All", command=self.start_all_sequence_thread)
        self.start_all_btn.grid(row=0, column=0, padx=3, pady=3, sticky="ew")
        self.stop_all_btn = ttk.Button(server_card, text="Stop All", command=self.stop_all_processes)
        self.stop_all_btn.grid(row=0, column=1, padx=3, pady=3, sticky="ew")
        self.force_stop_btn = ttk.Button(server_card, text="Force Kill", command=self.force_kill_all)
        self.force_stop_btn.grid(row=1, column=0, padx=3, pady=3, sticky="ew")
        self.shared_mem_btn = ttk.Button(server_card, text="Shared Memory", command=self.run_shared_memory_thread)
        self.shared_mem_btn.grid(row=1, column=1, padx=3, pady=3, sticky="ew")
        self.restart_zones_btn = ttk.Button(server_card, text="Restart Zones", command=self.restart_zones)
        self.restart_zones_btn.grid(row=2, column=0, columnspan=2, padx=3, pady=3, sticky="ew")

        client_card = ttk.LabelFrame(ops_rail, text="Client Sync", padding=8)
        client_card.grid(row=2, column=0, sticky="ew", pady=(0, 8))
        client_card.grid_columnconfigure(0, weight=1)

        ttk.Label(client_card, text="EQ Folder").grid(row=0, column=0, sticky="w", pady=(0, 3))
        ttk.Entry(client_card, textvariable=self.eq_dir_var).grid(row=1, column=0, sticky="ew", pady=(0, 4))
        browse_row = ttk.Frame(client_card)
        browse_row.grid(row=2, column=0, sticky="ew", pady=(0, 4))
        browse_row.grid_columnconfigure(0, weight=1)
        browse_row.grid_columnconfigure(1, weight=1)
        ttk.Button(browse_row, text="Browse", command=self.browse_eq_dir).grid(row=0, column=0, padx=(0, 3), sticky="ew")
        ttk.Button(browse_row, text="Refresh Status", command=self.refresh_status_indicators).grid(row=0, column=1, padx=(3, 0), sticky="ew")

        action_row_1 = ttk.Frame(client_card)
        action_row_1.grid(row=3, column=0, sticky="ew", pady=2)
        action_row_1.grid_columnconfigure(0, weight=1)
        action_row_1.grid_columnconfigure(1, weight=1)
        ttk.Button(action_row_1, text="Build+Copy DLL", command=self.run_build_eqcore_thread).grid(row=0, column=0, padx=(0, 3), sticky="ew")
        ttk.Button(action_row_1, text="Copy DLL", command=self.copy_eqcore_dll).grid(row=0, column=1, padx=(3, 0), sticky="ew")

        action_row_2 = ttk.Frame(client_card)
        action_row_2.grid(row=4, column=0, sticky="ew", pady=2)
        action_row_2.grid_columnconfigure(0, weight=1)
        action_row_2.grid_columnconfigure(1, weight=1)
        ttk.Button(action_row_2, text="Export spells_us", command=self.run_export_spells_thread).grid(row=0, column=0, padx=(0, 3), sticky="ew")
        ttk.Button(action_row_2, text="Export dbstr_us", command=self.run_export_dbstr_thread).grid(row=0, column=1, padx=(3, 0), sticky="ew")
        ttk.Button(client_card, text="Launch eqgame", command=self.launch_eqgame).grid(row=5, column=0, sticky="ew", pady=(4, 0))

        status_card = ttk.LabelFrame(ops_rail, text="Client Asset Status", padding=8)
        status_card.grid(row=3, column=0, sticky="ew")
        self.status_labels = {
            "dll": ttk.Label(status_card, text="DLL: checking...", foreground="blue"),
            "spells": ttk.Label(status_card, text="spells_us: checking...", foreground="blue"),
            "dbstr": ttk.Label(status_card, text="dbstr_us: checking...", foreground="blue"),
        }
        self.status_labels["dll"].pack(anchor="w", pady=1)
        self.status_labels["spells"].pack(anchor="w", pady=1)
        self.status_labels["dbstr"].pack(anchor="w", pady=1)

        overview = ttk.Frame(workspace, style="Overview.TFrame", padding=(10, 8, 10, 8))
        overview.grid(row=0, column=0, sticky="ew", pady=(0, 8))
        overview.grid_columnconfigure(0, weight=1)
        overview.grid_columnconfigure(1, weight=0)
        overview.grid_columnconfigure(2, weight=0)

        ttk.Label(overview, text="Live Server Workspace", style="OverviewTitle.TLabel").grid(row=0, column=0, sticky="w")
        ttk.Label(overview, textvariable=self.process_summary_var, style="OverviewValue.TLabel").grid(row=1, column=0, sticky="w", pady=(2, 0))
        ttk.Button(overview, text="Refresh Logs", command=self.refresh_runtime_log_files).grid(row=0, column=1, rowspan=2, padx=(10, 0), sticky="e")
        layout_selector = ttk.Combobox(
            overview,
            textvariable=self.layout_mode_var,
            width=10,
            state="readonly",
            values=["auto", "landscape", "portrait"],
        )
        layout_selector.grid(row=0, column=2, rowspan=2, padx=(8, 0), sticky="e")

        proc_frame = ttk.LabelFrame(workspace, text="Service Grid", padding=8)
        proc_frame.grid(row=1, column=0, sticky="ew", pady=(0, 8))
        self.proc_frame = proc_frame

        self.proc_widgets = {}
        for i, info in enumerate(self.process_info):
            name = info["name"]

            card = ttk.Frame(proc_frame, style="ProcessCard.TFrame", padding=(10, 8, 10, 8))
            card.grid_columnconfigure(0, weight=1)

            ttk.Label(card, text=info["display"], style="ProcessTitle.TLabel").grid(row=0, column=0, sticky="w")
            status_lbl = ttk.Label(card, text="Stopped", foreground="red")
            status_lbl.grid(row=1, column=0, sticky="w", pady=(2, 4))

            args_var = tk.StringVar(value=info["args"])
            console_var = tk.BooleanVar(value=False)
            hint_var = tk.StringVar(value="")

            ttk.Label(card, textvariable=hint_var, style="ProcessHint.TLabel", wraplength=260).grid(row=2, column=0, sticky="w", pady=(0, 6))

            btn_row = ttk.Frame(card)
            btn_row.grid(row=3, column=0, sticky="ew")
            btn_row.grid_columnconfigure(0, weight=1)
            btn_row.grid_columnconfigure(1, weight=1)
            btn_row.grid_columnconfigure(2, weight=1)

            start_btn = ttk.Button(btn_row, text="Start", command=lambda n=name: self.start_process(n))
            start_btn.grid(row=0, column=0, padx=(0, 3), sticky="ew")
            stop_btn = ttk.Button(btn_row, text="Stop", state="disabled", command=lambda n=name: self.stop_process(n))
            stop_btn.grid(row=0, column=1, padx=3, sticky="ew")
            opts_btn = ttk.Button(btn_row, text="Options", command=lambda n=name: self.open_process_options(n))
            opts_btn.grid(row=0, column=2, padx=(3, 0), sticky="ew")

            self.proc_widgets[name] = {
                "index": i,
                "card": card,
                "status_lbl": status_lbl,
                "start_btn": start_btn,
                "stop_btn": stop_btn,
                "opts_btn": opts_btn,
                "args_var": args_var,
                "console_var": console_var,
                "hint_var": hint_var,
            }

        log_tabs = ttk.Notebook(workspace)
        log_tabs.grid(row=2, column=0, sticky="nsew")

        manager_log_tab = ttk.Frame(log_tabs)
        runtime_log_tab = ttk.Frame(log_tabs)
        log_tabs.add(manager_log_tab, text="Manager Log")
        log_tabs.add(runtime_log_tab, text="Runtime Log Viewer")

        self.log_text = scrolledtext.ScrolledText(manager_log_tab, height=10, state="disabled")
        self.log_text.pack(fill="both", expand=True, padx=5, pady=5)

        controls = ttk.Frame(runtime_log_tab)
        controls.pack(fill="x", padx=5, pady=5)

        ttk.Button(controls, text="Active Zone Log", command=self.select_active_zone_log).pack(side="left", padx=3)
        ttk.Button(controls, text="Active World Log", command=self.select_active_world_log).pack(side="left", padx=3)
        ttk.Button(controls, text="Most Recent Log", command=self.select_most_recent_runtime_log).pack(side="left", padx=3)
        ttk.Button(controls, text="Refresh Logs", command=self.refresh_runtime_log_files).pack(side="left", padx=3)
        ttk.Checkbutton(controls, text="Follow", variable=self.log_follow_var).pack(side="left", padx=8)
        ttk.Label(controls, textvariable=self.log_source_var).pack(side="left", padx=10)

        selector_row = ttk.Frame(runtime_log_tab)
        selector_row.pack(fill="x", padx=5, pady=2)
        ttk.Label(selector_row, text="Log File:").pack(side="left", padx=(0, 5))
        self.runtime_log_combo = ttk.Combobox(
            selector_row,
            textvariable=self.selected_log_path_var,
            state="readonly",
            width=115,
            values=[],
        )
        self.runtime_log_combo.pack(side="left", fill="x", expand=True)
        self.runtime_log_combo.bind("<<ComboboxSelected>>", self.on_runtime_log_selected)

        self.runtime_log_text = scrolledtext.ScrolledText(runtime_log_tab, height=14, state="disabled", font=("Consolas", 9))
        self.runtime_log_text.pack(fill="both", expand=True, padx=5, pady=5)

        self._refresh_process_overview()
        self._apply_main_layout(force=True)
        self.after(200, self.refresh_runtime_log_files)
        self.after(1000, self.runtime_log_follow_loop)

    def _on_window_configure(self, event=None):
        if event is not None and event.widget is not self:
            return
        if self.layout_mode_var.get() == "auto":
            self._apply_main_layout()

    def _effective_layout_mode(self):
        selected_mode = self.layout_mode_var.get()
        if selected_mode in ("landscape", "portrait"):
            return selected_mode
        width = self.winfo_width()
        if width <= 1:
            width = self.winfo_reqwidth()
        return "portrait" if width < self.portrait_breakpoint else "landscape"

    def _reflow_process_cards(self, columns):
        columns = max(1, int(columns))
        if self._active_proc_cols == columns:
            return

        for col in range(6):
            self.proc_frame.grid_columnconfigure(col, weight=0)
        for col in range(columns):
            self.proc_frame.grid_columnconfigure(col, weight=1)

        sorted_items = sorted(self.proc_widgets.items(), key=lambda item: item[1]["index"])
        for idx, (_, widgets) in enumerate(sorted_items):
            row = idx // columns
            col = idx % columns
            widgets["card"].grid(row=row, column=col, padx=5, pady=5, sticky="nsew")

        self._active_proc_cols = columns

    def _apply_main_layout(self, force=False):
        if not hasattr(self, "main_shell"):
            return

        mode = self._effective_layout_mode()
        if not force and self._active_layout_mode == mode:
            return

        shell = self.main_shell
        for col in range(2):
            shell.grid_columnconfigure(col, weight=0, minsize=0)
        for row in range(2):
            shell.grid_rowconfigure(row, weight=0)

        if mode == "portrait":
            shell.grid_columnconfigure(0, weight=1)
            shell.grid_rowconfigure(0, weight=0)
            shell.grid_rowconfigure(1, weight=1)
            self.ops_rail.grid(row=0, column=0, sticky="ew", padx=0, pady=(0, 10))
            self.workspace.grid(row=1, column=0, sticky="nsew")
            self._reflow_process_cards(columns=2)
        else:
            shell.grid_columnconfigure(0, weight=0, minsize=360)
            shell.grid_columnconfigure(1, weight=1)
            shell.grid_rowconfigure(0, weight=1)
            self.ops_rail.grid(row=0, column=0, sticky="nsew", padx=(0, 10), pady=0)
            self.workspace.grid(row=0, column=1, sticky="nsew")
            self._reflow_process_cards(columns=3)

        self._active_layout_mode = mode

    def create_database_tab(self):
        """Create database management tools"""
        db_frame = ttk.Frame(self.database_tab)
        db_frame.pack(fill="both", expand=True, padx=10, pady=10)

        # Character Management Section
        char_frame = ttk.LabelFrame(db_frame, text="Character Management")
        char_frame.pack(fill="x", pady=5)

        # Reset Password
        pwd_frame = ttk.Frame(char_frame)
        pwd_frame.pack(fill="x", padx=5, pady=5)
        ttk.Label(pwd_frame, text="Account:").pack(side="left")
        self.reset_pwd_account = ttk.Entry(pwd_frame, width=20)
        self.reset_pwd_account.pack(side="left", padx=5)
        ttk.Label(pwd_frame, text="New Password:").pack(side="left")
        self.reset_pwd_password = ttk.Entry(pwd_frame, width=20, show="*")
        self.reset_pwd_password.pack(side="left", padx=5)
        ttk.Button(pwd_frame, text="Reset Password",
                  command=self.reset_password).pack(side="left", padx=5)

        # Set GM Level
        gm_frame = ttk.Frame(char_frame)
        gm_frame.pack(fill="x", padx=5, pady=5)
        ttk.Label(gm_frame, text="Account:").pack(side="left")
        self.gm_account = ttk.Entry(gm_frame, width=20)
        self.gm_account.pack(side="left", padx=5)
        ttk.Label(gm_frame, text="GM Level:").pack(side="left")
        self.gm_level = ttk.Spinbox(gm_frame, from_=0, to=255, width=10)
        self.gm_level.pack(side="left", padx=5)
        ttk.Button(gm_frame, text="Set GM Level",
                  command=self.set_gm_level).pack(side="left", padx=5)

        # Launcher Zones Section (controls how many zones are pre-booted by launchers)
        launcher_frame = ttk.LabelFrame(db_frame, text="Launcher Zones (Pre-Booted)")
        launcher_frame.pack(fill="x", pady=5)

        launcher_row = ttk.Frame(launcher_frame)
        launcher_row.pack(fill="x", padx=5, pady=5)

        ttk.Label(launcher_row, text="Keep per launcher:").pack(side="left")
        self.launcher_keep_count = ttk.Spinbox(launcher_row, from_=0, to=100, width=10)
        self.launcher_keep_count.set("1")
        self.launcher_keep_count.pack(side="left", padx=5)

        ttk.Button(launcher_row, text="List launcher_zones", command=self.list_launcher_zones).pack(side="left", padx=5)
        ttk.Button(launcher_row, text="Trim launcher_zones", command=self.trim_launcher_zones).pack(side="left", padx=5)

        # Database Scripts Section
        scripts_frame = ttk.LabelFrame(db_frame, text="Database Diagnostic Scripts")
        scripts_frame.pack(fill="both", expand=True, pady=5)

        # Find all check_*.py and fix_*.py scripts
        script_btn_frame = ttk.Frame(scripts_frame)
        script_btn_frame.pack(fill="both", expand=True, padx=5, pady=5)

        check_scripts = sorted(glob.glob("check_*.py"))
        fix_scripts = sorted(glob.glob("fix_*.py"))

        # Left column - check scripts
        left_frame = ttk.LabelFrame(script_btn_frame, text="Check Scripts")
        left_frame.pack(side="left", fill="both", expand=True, padx=2)

        for script in check_scripts:
            btn = ttk.Button(left_frame, text=script.replace(".py", ""),
                           command=lambda s=script: self.run_db_script(s))
            btn.pack(fill="x", padx=5, pady=2)

        # Right column - fix scripts
        right_frame = ttk.LabelFrame(script_btn_frame, text="Fix Scripts")
        right_frame.pack(side="left", fill="both", expand=True, padx=2)

        for script in fix_scripts:
            btn = ttk.Button(right_frame, text=script.replace(".py", ""),
                           command=lambda s=script: self.run_db_script(s))
            btn.pack(fill="x", padx=5, pady=2)

        # Backup Section
        backup_frame = ttk.LabelFrame(db_frame, text="Database Backup")
        backup_frame.pack(fill="x", pady=5)

        backup_btn_frame = ttk.Frame(backup_frame)
        backup_btn_frame.pack(fill="x", padx=5, pady=5)

        ttk.Button(backup_btn_frame, text="Backup Database",
                  command=self.backup_database).pack(side="left", padx=5)
        ttk.Button(backup_btn_frame, text="Restore Database",
                  command=self.restore_database).pack(side="left", padx=5)
        ttk.Button(backup_btn_frame, text="Open Backup Folder",
                  command=self.open_backup_folder).pack(side="left", padx=5)

        # Client export section
        export_frame = ttk.LabelFrame(db_frame, text="Export Client Files")
        export_frame.pack(fill="x", pady=5)
        ttk.Label(export_frame, text="EQ Client Folder:").grid(row=0, column=0, padx=5, pady=5, sticky="e")
        ttk.Entry(export_frame, textvariable=self.eq_dir_var, width=50).grid(row=0, column=1, padx=5, pady=5, sticky="w")
        ttk.Button(export_frame, text="Browse", command=self.browse_eq_dir).grid(row=0, column=2, padx=5, pady=5)
        ttk.Button(export_frame, text="Export spells_us.txt", command=self.run_export_spells_thread).grid(row=1, column=0, padx=5, pady=5, sticky="w")
        ttk.Button(export_frame, text="Export dbstr_us.txt", command=self.run_export_dbstr_thread).grid(row=1, column=1, padx=5, pady=5, sticky="w")
        ttk.Button(export_frame, text="Show Export Status", command=self.show_export_status).grid(row=1, column=2, padx=5, pady=5, sticky="w")

        # Database output
        self.db_output = scrolledtext.ScrolledText(db_frame, height=10, bg="white",
                                                   font=("Consolas", 9))
        self.db_output.pack(fill="both", expand=True, pady=5)

    def create_tools_tab(self):
        """Create tools and script runner tab"""
        tools_frame = ttk.Frame(self.tools_tab)
        tools_frame.pack(fill="both", expand=True, padx=10, pady=10)

        # Quick Actions for testing workflow
        quick_frame = ttk.LabelFrame(tools_frame, text="Common Workflows (Quick Actions)")
        quick_frame.pack(fill="x", pady=5, padx=2)
        ttk.Button(quick_frame, text="Build Zone Only",
                  command=lambda: self.quick_build("zone")).grid(row=0, column=0, padx=5, pady=5, sticky="ew")
        ttk.Button(quick_frame, text="Restart Zones",
                  command=self.restart_zones).grid(row=0, column=1, padx=5, pady=5, sticky="ew")
        ttk.Button(quick_frame, text="Launch Client",
                  command=self.launch_eqgame).grid(row=0, column=2, padx=5, pady=5, sticky="ew")
        ttk.Button(quick_frame, text="Build + Copy DLL",
                  command=self.run_build_eqcore_thread).grid(row=1, column=0, padx=5, pady=5, sticky="ew")
        ttk.Button(quick_frame, text="Export Client Files",
                  command=self.export_all_client_files).grid(row=1, column=1, padx=5, pady=5, sticky="ew")
        ttk.Button(quick_frame, text="Full Rebuild & Restart",
                  command=self.full_rebuild_and_restart).grid(row=1, column=2, padx=5, pady=5, sticky="ew")

        # Python Script Runner
        script_frame = ttk.LabelFrame(tools_frame, text="Python Script Runner")
        script_frame.pack(fill="both", expand=True, pady=10)

        ttk.Label(script_frame, text="Select a script to launch in a new console window:").pack(anchor="w", padx=5, pady=5)

        self.scripts_listbox = tk.Listbox(script_frame, selectmode="single", font=("Consolas", 10))
        self.scripts_listbox.pack(fill="both", expand=True, pady=5, padx=5)

        scrollbar = ttk.Scrollbar(self.scripts_listbox, orient="vertical", command=self.scripts_listbox.yview)
        scrollbar.pack(side="right", fill="y")
        self.scripts_listbox.config(yscrollcommand=scrollbar.set)

        # Populate scripts
        self.refresh_scripts()

        btn_row = ttk.Frame(script_frame)
        btn_row.pack(fill="x", pady=5)

        ttk.Button(btn_row, text="Refresh List", command=self.refresh_scripts).pack(side="left", padx=5)
        ttk.Button(btn_row, text="Run Selected Script", command=self.run_selected_script).pack(side="left", padx=5)

    # ==================== Database Methods ====================

    def reset_password(self):
        """Reset account password"""
        account = self.reset_pwd_account.get().strip()
        password = self.reset_pwd_password.get().strip()

        if not account or not password:
            messagebox.showerror("Error", "Please enter account and password")
            return

        script = "reset_password.py"
        if not os.path.exists(script):
            messagebox.showerror("Error", f"{script} not found")
            return

        self.run_python_script_with_output(script, [account, password], self.db_output)

    def set_gm_level(self):
        """Set GM level for account"""
        account = self.gm_account.get().strip()
        level = self.gm_level.get()

        if not account:
            messagebox.showerror("Error", "Please enter account name")
            return

        script = "set_gm.py"
        if not os.path.exists(script):
            messagebox.showerror("Error", f"{script} not found")
            return

        self.run_python_script_with_output(script, [account, level], self.db_output)

    def list_launcher_zones(self):
        def run():
            try:
                self.ui_call(self._set_text_widget, self.db_output, "")

                db_cfg = self._load_db_config()
                conn = mysql.connector.connect(**db_cfg)
                cur = conn.cursor()

                cur.execute("SHOW TABLES LIKE 'launcher_zones'")
                if not cur.fetchone():
                    self.ui_call(self._append_text_widget, self.db_output, "Table launcher_zones not found in this database.\n")
                    cur.close()
                    conn.close()
                    return

                cur.execute("SHOW COLUMNS FROM launcher_zones")
                columns = [r[0] for r in cur.fetchall()]
                self.ui_call(self._append_text_widget, self.db_output, f"launcher_zones columns: {', '.join(columns)}\n\n")

                order_cols = [c for c in ["launcher", "zone", "port", "number", "startzone"] if c in columns]
                if not order_cols:
                    order_cols = [columns[0]]

                q = f"SELECT * FROM launcher_zones ORDER BY {', '.join(order_cols)} LIMIT 200"
                cur.execute(q)
                rows = cur.fetchall()

                self.ui_call(self._append_text_widget, self.db_output, f"Showing up to {len(rows)} rows:\n")
                for row in rows:
                    self.ui_call(self._append_text_widget, self.db_output, f"{row}\n")

                cur.close()
                conn.close()
            except Exception as e:
                self.ui_call(self._append_text_widget, self.db_output, f"Error listing launcher_zones: {e}\n")

        threading.Thread(target=run, daemon=True).start()

    def trim_launcher_zones(self):
        keep = int(self.launcher_keep_count.get())
        if keep < 0:
            keep = 0

        if not messagebox.askyesno(
            "Trim launcher_zones",
            f"This will keep only {keep} launcher_zones rows per launcher (based on sorted order) and delete the rest.\n\nContinue?",
        ):
            return

        def run():
            try:
                self.ui_call(self._set_text_widget, self.db_output, f"Trimming launcher_zones to {keep} per launcher...\n")

                db_cfg = self._load_db_config()
                conn = mysql.connector.connect(**db_cfg)
                cur = conn.cursor()

                cur.execute("SHOW TABLES LIKE 'launcher_zones'")
                if not cur.fetchone():
                    self.ui_call(self._append_text_widget, self.db_output, "Table launcher_zones not found in this database.\n")
                    cur.close()
                    conn.close()
                    return

                cur.execute("SHOW COLUMNS FROM launcher_zones")
                columns = [r[0] for r in cur.fetchall()]

                id_col = "id" if "id" in columns else None
                launcher_col = "launcher" if "launcher" in columns else None

                order_cols = [c for c in ["launcher", "zone", "port", "number", "startzone"] if c in columns]
                if not order_cols:
                    order_cols = [columns[0]]

                select_cols = []
                if id_col:
                    select_cols.append(id_col)
                if launcher_col and launcher_col not in select_cols:
                    select_cols.append(launcher_col)
                for c in ["zone", "port", "number", "startzone"]:
                    if c in columns and c not in select_cols:
                        select_cols.append(c)

                # Fallback to all columns if we couldn't find a stable key.
                if not select_cols:
                    select_cols = columns[:]

                q = f"SELECT {', '.join(select_cols)} FROM launcher_zones ORDER BY {', '.join(order_cols)}"
                cur.execute(q)
                rows = cur.fetchall()

                def get_launcher_key(row):
                    if launcher_col and launcher_col in select_cols:
                        return row[select_cols.index(launcher_col)]
                    return "__all__"

                to_delete = []
                seen = {}
                for row in rows:
                    lk = get_launcher_key(row)
                    seen.setdefault(lk, 0)
                    seen[lk] += 1
                    if keep == 0 or seen[lk] > keep:
                        to_delete.append(row)

                if not to_delete:
                    self.ui_call(self._append_text_widget, self.db_output, "Nothing to delete.\n")
                    cur.close()
                    conn.close()
                    return

                deleted = 0
                if id_col:
                    id_idx = select_cols.index(id_col)
                    cur.executemany("DELETE FROM launcher_zones WHERE id = %s", [(r[id_idx],) for r in to_delete])
                    deleted = cur.rowcount
                else:
                    where_cols = [c for c in ["launcher", "zone", "port", "number", "startzone"] if c in select_cols]
                    if not where_cols:
                        where_cols = select_cols[:]

                    for row in to_delete:
                        parts = []
                        params = []
                        for c in where_cols:
                            v = row[select_cols.index(c)]
                            if v is None:
                                parts.append(f"{c} IS NULL")
                            else:
                                parts.append(f"{c} = %s")
                                params.append(v)
                        cur.execute(f"DELETE FROM launcher_zones WHERE {' AND '.join(parts)}", params)
                        deleted += cur.rowcount

                conn.commit()
                self.ui_call(self._append_text_widget, self.db_output, f"Deleted {deleted} rows.\n")
                self.ui_call(self._append_text_widget, self.db_output, "Done.\n")

                cur.close()
                conn.close()
            except Exception as e:
                self.ui_call(self._append_text_widget, self.db_output, f"Error trimming launcher_zones: {e}\n")

        threading.Thread(target=run, daemon=True).start()

    def run_db_script(self, script):
        """Run a database diagnostic script"""
        self.log(f"Running database script: {script}")
        self.run_python_script_with_output(script, [], self.db_output)

    def run_python_script_with_output(self, script, args, output_widget):
        """Run Python script and display output in widget"""
        def run():
            try:
                self.ui_call(self._set_text_widget, output_widget, "")
                self.ui_call(self._append_text_widget, output_widget, f"Running: python {script} {' '.join(str(a) for a in args)}\n")
                self.ui_call(self._append_text_widget, output_widget, "=" * 60 + "\n")

                result = subprocess.run(
                    [sys.executable, script] + [str(a) for a in args],
                    capture_output=True,
                    text=True,
                    cwd=os.getcwd()
                )

                if result.stdout:
                    self.ui_call(self._append_text_widget, output_widget, result.stdout)
                if result.stderr:
                    self.ui_call(self._append_text_widget, output_widget, "\n=== ERRORS ===\n")
                    self.ui_call(self._append_text_widget, output_widget, result.stderr)

                self.ui_call(self._append_text_widget, output_widget, "\n" + "=" * 60 + "\n")
                self.ui_call(self._append_text_widget, output_widget, f"Exit code: {result.returncode}\n")

            except Exception as e:
                self.ui_call(self._append_text_widget, output_widget, f"\nError: {e}\n")

        threading.Thread(target=run, daemon=True).start()

    def backup_database(self):
        """Backup database using mysqldump"""
        self.log("Starting database backup...")

        def backup():
            try:
                db_cfg = self._load_db_config()
                db_host = db_cfg["host"]
                db_port = db_cfg["port"]
                db_user = db_cfg["user"]
                db_pass = db_cfg["password"]
                db_name = db_cfg["database"]

                # Create backup directory
                backup_dir = "backups"
                if not os.path.exists(backup_dir):
                    os.makedirs(backup_dir)

                # Generate filename with timestamp
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                backup_file = os.path.join(backup_dir, f"eqemu_backup_{timestamp}.sql")

                self.ui_call(self._set_text_widget, self.db_output, f"Backing up database to: {backup_file}\n")

                # Run mysqldump
                cmd = [
                    "mysqldump",
                    f"-h{db_host}",
                    f"-P{db_port}",
                    f"-u{db_user}",
                    f"-p{db_pass}",
                    db_name
                ]

                with open(backup_file, 'w') as f:
                    result = subprocess.run(cmd, stdout=f, stderr=subprocess.PIPE, text=True)

                if result.returncode == 0:
                    file_size = os.path.getsize(backup_file) / (1024 * 1024)  # MB
                    self.ui_call(self._append_text_widget, self.db_output, "Backup completed successfully!\n")
                    self.ui_call(self._append_text_widget, self.db_output, f"File size: {file_size:.2f} MB\n")
                    self.log(f"Database backed up to {backup_file}")
                else:
                    self.ui_call(self._append_text_widget, self.db_output, f"Backup failed:\n{result.stderr}\n")

            except Exception as e:
                self.ui_call(self._append_text_widget, self.db_output, f"Error: {e}\n")

        threading.Thread(target=backup, daemon=True).start()

    def restore_database(self):
        """Restore database from backup"""
        filename = filedialog.askopenfilename(
            initialdir="backups",
            title="Select Backup File",
            filetypes=[("SQL files", "*.sql"), ("All files", "*.*")]
        )

        if not filename:
            return

        if not messagebox.askyesno("Confirm Restore",
                                   f"This will restore the database from:\n{filename}\n\n"
                                   "ALL CURRENT DATA WILL BE REPLACED!\n\n"
                                   "Are you sure you want to continue?"):
            return

        self.log(f"Restoring database from {filename}...")

        def restore():
            try:
                db_cfg = self._load_db_config()
                db_host = db_cfg["host"]
                db_port = db_cfg["port"]
                db_user = db_cfg["user"]
                db_pass = db_cfg["password"]
                db_name = db_cfg["database"]

                self.ui_call(self._set_text_widget, self.db_output, f"Restoring database from: {filename}\n")

                cmd = [
                    "mysql",
                    f"-h{db_host}",
                    f"-P{db_port}",
                    f"-u{db_user}",
                    f"-p{db_pass}",
                    db_name
                ]

                with open(filename, 'r') as f:
                    result = subprocess.run(cmd, stdin=f, stderr=subprocess.PIPE, text=True)

                if result.returncode == 0:
                    self.ui_call(self._append_text_widget, self.db_output, "Restore completed successfully!\n")
                    self.log("Database restored successfully")
                else:
                    self.ui_call(self._append_text_widget, self.db_output, f"Restore failed:\n{result.stderr}\n")

            except Exception as e:
                self.ui_call(self._append_text_widget, self.db_output, f"Error: {e}\n")

        threading.Thread(target=restore, daemon=True).start()

    def open_backup_folder(self):
        """Open backup folder in file explorer"""
        backup_dir = "backups"
        if not os.path.exists(backup_dir):
            os.makedirs(backup_dir)

        if sys.platform == "win32":
            os.startfile(backup_dir)
        else:
            subprocess.run(["xdg-open", backup_dir])

    # ==================== Client Export / EQ Utilities ====================

    def browse_eq_dir(self):
        path = filedialog.askdirectory(initialdir=self.eq_dir_var.get(), title="Select EQ Client Folder")
        if path:
            self.eq_dir_var.set(path)

    def _backup_existing(self, path):
        if os.path.exists(path):
            ts = datetime.now().strftime("%Y%m%d_%H%M%S")
            new_name = f"{path}.{ts}.old"
            os.rename(path, new_name)
            self.log(f"Backed up existing file to {new_name}")

    def _load_db_config(self):
        with open("eqemu_config.json", "r") as f:
            cfg = json.load(f)["server"]["database"]
        return {
            "host": cfg["host"],
            "port": int(cfg["port"]),
            "user": cfg["username"],
            "password": cfg["password"],
            "database": cfg["db"],
        }

    def run_export_spells_thread(self):
        threading.Thread(target=self.export_spells, daemon=True).start()

    def run_export_dbstr_thread(self):
        threading.Thread(target=self.export_dbstr, daemon=True).start()

    def export_spells(self):
        dest = os.path.join(self.eq_dir_var.get(), "spells_us.txt")
        try:
            db_cfg = self._load_db_config()
            conn = mysql.connector.connect(**db_cfg)
            cur = conn.cursor()
            cur.execute("SELECT * FROM spells_new ORDER BY id")
            columns = cur.column_names

            os.makedirs(os.path.dirname(dest), exist_ok=True)
            self._backup_existing(dest)
            with open(dest, "w", encoding="utf-8", newline="") as f:
                for row in cur:
                    parts = []
                    for val in row:
                        if val is None:
                            parts.append("")
                        else:
                            parts.append(str(val))
                    f.write("^".join(parts) + "\n")
            cur.close()
            conn.close()
            self.log(f"Exported spells to {dest}")
            if hasattr(self, "db_output"):
                self.ui_call(self._append_text_widget, self.db_output, f"Exported spells to {dest}\n")
        except Exception as e:
            self.log(f"Spell export failed: {e}")
            if hasattr(self, "db_output"):
                self.ui_call(self._append_text_widget, self.db_output, f"Spell export failed: {e}\n")

    def export_dbstr(self):
        dest = os.path.join(self.eq_dir_var.get(), "dbstr_us.txt")
        try:
            db_cfg = self._load_db_config()
            conn = mysql.connector.connect(**db_cfg)
            cur = conn.cursor()
            cur.execute("SELECT id, type, value FROM db_str ORDER BY id, type")

            os.makedirs(os.path.dirname(dest), exist_ok=True)
            self._backup_existing(dest)
            with open(dest, "w", encoding="utf-8", newline="") as f:
                for row in cur:
                    parts = []
                    for val in row:
                        if val is None:
                            parts.append("")
                        else:
                            parts.append(str(val))
                    f.write("^".join(parts) + "\n")
            cur.close()
            conn.close()
            self.log(f"Exported dbstr to {dest}")
            if hasattr(self, "db_output"):
                self.ui_call(self._append_text_widget, self.db_output, f"Exported dbstr to {dest}\n")
        except Exception as e:
            self.log(f"dbstr export failed: {e}")
            if hasattr(self, "db_output"):
                self.ui_call(self._append_text_widget, self.db_output, f"dbstr export failed: {e}\n")

    def _dll_candidates(self):
        path = os.path.join(self.extra_dll_dir, "dinput8.dll")
        return [path] if os.path.exists(path) else []

    def _compute_dll_status(self):
        candidates = self._dll_candidates()
        src_path = os.path.join(self.extra_dll_dir, "dinput8.dll")
        if not candidates:
            return False, f"DLL: missing {src_path}"
        src = candidates[0]
        src_size = os.path.getsize(src)
        src_mtime = datetime.fromtimestamp(os.path.getmtime(src)).strftime("%Y-%m-%d %H:%M:%S")
        client_path = os.path.join(self.eq_dir_var.get(), "dinput8.dll")
        if not os.path.exists(client_path):
            return False, f"DLL: client dinput8 missing (built {os.path.basename(src)} {src_size/1024:.1f} KB @ {src_mtime})"
        dst_size = os.path.getsize(client_path)
        dst_mtime = datetime.fromtimestamp(os.path.getmtime(client_path)).strftime("%Y-%m-%d %H:%M:%S")
        ok = src_size == dst_size
        status = "DLL: up to date" if ok else "DLL: OUT OF DATE"
        msg = f"{status} (built {src_size/1024:.1f} KB {src_mtime} vs client {dst_size/1024:.1f} KB {dst_mtime})"
        return ok, msg

    def _compute_export_status(self):
        db_cfg = self._load_db_config()
        conn = mysql.connector.connect(**db_cfg)
        cur = conn.cursor()
        cur.execute("SELECT COUNT(*) FROM spells_new")
        spells_rows = cur.fetchone()[0]
        cur.execute("SELECT COUNT(*) FROM db_str")
        dbstr_rows = cur.fetchone()[0]
        cur.close()
        conn.close()

        def check_file(path, label, expected):
            if not os.path.exists(path):
                return False, f"{label}: missing ({path})"
            with open(path, "r", encoding="utf-8", errors="ignore") as f:
                line_count = sum(1 for _ in f)
            mtime = datetime.fromtimestamp(os.path.getmtime(path)).strftime("%Y-%m-%d %H:%M:%S")
            ok = (line_count == expected)
            status = "up to date" if ok else "OUT OF DATE"
            return ok, f"{label}: {status} ({line_count} lines, db {expected}, modified {mtime})"

        spells_path = os.path.join(self.eq_dir_var.get(), "spells_us.txt")
        dbstr_path = os.path.join(self.eq_dir_var.get(), "dbstr_us.txt")
        spells_ok, spells_msg = check_file(spells_path, "spells_us", spells_rows)
        dbstr_ok, dbstr_msg = check_file(dbstr_path, "dbstr_us", dbstr_rows)
        return spells_ok, spells_msg, dbstr_ok, dbstr_msg

    def _find_msbuild(self):
        # 1) PATH
        found = shutil.which("msbuild")
        if found and os.path.exists(found):
            return found

        # 2) vswhere (most reliable on modern VS installs)
        vswhere = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
        if os.path.exists(vswhere):
            try:
                result = subprocess.run(
                    [
                        vswhere,
                        "-latest",
                        "-products",
                        "*",
                        "-requires",
                        "Microsoft.Component.MSBuild",
                        "-find",
                        r"MSBuild\**\Bin\MSBuild.exe",
                    ],
                    capture_output=True,
                    text=True,
                    check=False,
                )
                candidate = (result.stdout or "").strip().splitlines()
                if candidate:
                    msbuild = candidate[0].strip()
                    if os.path.exists(msbuild):
                        return msbuild
            except Exception:
                pass

        # 3) common fixed paths
        fixed = [
            r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
            r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
            r"C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
            r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
        ]
        for path in fixed:
            if os.path.exists(path):
                return path
        return None

    def run_build_eqcore_thread(self):
        threading.Thread(target=self.build_eqcore_and_copy, daemon=True).start()

    def build_eqcore_and_copy(self):
        """Build the eq-core DLL using MSBuild and copy to EQ client folder"""
        try:
            self.log("Building eq-core DLL (MSBuild)...")

            msbuild = self._find_msbuild()
            sln_path = os.path.join("extras", "eq-core-dll-main", "eq-core-dll-visualstudio2022.sln")

            if not msbuild:
                self.log("MSBuild not found. Install VS Build Tools or add msbuild to PATH.")
                return

            if not os.path.exists(sln_path):
                self.log(f"Solution not found at {sln_path}")
                return

            cmd = [
                msbuild,
                sln_path,
                "/p:Configuration=Release",
                "/p:Platform=Win32",
                "/p:PlatformToolset=v143"
            ]

            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            while True:
                line = proc.stdout.readline()
                if not line:
                    break
                line = line.strip()
                if line:  # Skip empty lines
                    self.log(line)
            proc.wait()

            if proc.returncode == 0:
                self.log("DLL build complete, copying...")
                self.copy_eqcore_dll()
            else:
                self.log(f"DLL build failed with code {proc.returncode}")
        except Exception as e:
            self.log(f"DLL build/copy failed: {e}")

    def copy_eqcore_dll(self):
        try:
            # search for dll in extras/eq-core-dll-main/bin only
            candidates = self._dll_candidates()
            if not candidates:
                self.ui_call(messagebox.showerror, "Error", f"No dinput8.dll found in {self.extra_dll_dir}")
                return
            src = max(candidates, key=os.path.getmtime)
            dest_dir = self.eq_dir_var.get()
            os.makedirs(dest_dir, exist_ok=True)
            dest_name = "dinput8.dll" if "dinput8" in os.path.basename(src).lower() else os.path.basename(src)
            dest = os.path.join(dest_dir, dest_name)
            self._backup_existing(dest)
            shutil.copy2(src, dest)
            self.log(f"Copied {src} -> {dest}")
        except Exception as e:
            self.log(f"Copy eqcore DLL failed: {e}")

    def launch_eqgame(self):
        eq_dir = self.eq_dir_var.get()
        exe_path = os.path.join(eq_dir, "eqgame.exe")
        if not os.path.exists(exe_path):
            messagebox.showerror("Error", f"eqgame.exe not found in {eq_dir}")
            return
        try:
            subprocess.Popen([exe_path, "patchme"], cwd=eq_dir)
            self.log(f"Launched eqgame.exe from {eq_dir}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to launch eqgame.exe: {e}")
            self.log(f"Failed to launch eqgame.exe: {e}")

    def show_dll_info(self):
        def info_for(path, label):
            if not os.path.exists(path):
                return f"{label}: missing ({path})"
            size_kb = os.path.getsize(path) / 1024
            mtime = datetime.fromtimestamp(os.path.getmtime(path)).strftime("%Y-%m-%d %H:%M:%S")
            return f"{label}: {os.path.basename(path)} | {size_kb:.1f} KB | modified {mtime}"

        try:
            # Only care about dinput8.dll in extras/eq-core-dll-main/bin
            candidates = self._dll_candidates()
            built_msg = f"Built dll: none found at {os.path.join(self.extra_dll_dir, 'dinput8.dll')}"
            if candidates:
                src = candidates[0]
                built_msg = info_for(src, "Built dll")

            # Client dinput8.dll in EQ folder
            client_dinput = os.path.join(self.eq_dir_var.get(), "dinput8.dll")
            client_msg = info_for(client_dinput, "Client dinput8")

            msg = built_msg + "\n" + client_msg
            self.log(msg)
        except Exception as e:
            self.log(f"DLL info failed: {e}")
            if hasattr(self, "db_output"):
                self.db_output.insert("end", f"DLL info failed: {e}\n")
                self.db_output.see("end")

    def show_export_status(self):
        def file_info(path, label, expected_rows):
            if not os.path.exists(path):
                return f"{label}: missing ({path})"
            try:
                with open(path, "r", encoding="utf-8", errors="ignore") as f:
                    line_count = sum(1 for _ in f)
                mtime = datetime.fromtimestamp(os.path.getmtime(path)).strftime("%Y-%m-%d %H:%M:%S")
                status = "up to date" if expected_rows is not None and line_count == expected_rows else "out of date"
                return f"{label}: {os.path.basename(path)} | {line_count} lines | modified {mtime} | {status} (db rows {expected_rows})"
            except Exception as e:
                return f"{label}: error reading file ({e})"

        try:
            db_cfg = self._load_db_config()
            conn = mysql.connector.connect(**db_cfg)
            cur = conn.cursor()
            cur.execute("SELECT COUNT(*) FROM spells_new")
            spells_rows = cur.fetchone()[0]
            cur.execute("SELECT COUNT(*) FROM db_str")
            dbstr_rows = cur.fetchone()[0]
            cur.close()
            conn.close()

            eq_dir = self.eq_dir_var.get()
            spells_path = os.path.join(eq_dir, "spells_us.txt")
            dbstr_path = os.path.join(eq_dir, "dbstr_us.txt")

            spells_msg = file_info(spells_path, "spells_us.txt", spells_rows)
            dbstr_msg = file_info(dbstr_path, "dbstr_us.txt", dbstr_rows)

            msg = spells_msg + "\n" + dbstr_msg
            self.log(msg)
            if hasattr(self, "db_output"):
                self.ui_call(self._append_text_widget, self.db_output, msg + "\n")
        except Exception as e:
            self.log(f"Export status failed: {e}")
            if hasattr(self, "db_output"):
                self.ui_call(self._append_text_widget, self.db_output, f"Export status failed: {e}\n")

    # ==================== Quick Action Methods ====================

    def quick_build(self, target):
        """Quick build a specific target without reconfiguring"""
        def do_build():
            self.ui_call(self._set_build_controls_enabled, False)
            self.ui_call(self._set_build_status, f"Building {target}...")

            cmd = ["cmake", "--build", "build", "--target", target, "--config", "RelWithDebInfo", "--parallel"]
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

            while True:
                line = proc.stdout.readline()
                if not line:
                    break
                line = line.strip()
                if line:
                    self.log(line)
            proc.wait()

            if proc.returncode == 0:
                self.log(f"Build {target} complete.")
                self.ui_call(self._set_build_status, "Build Complete")
                self.bin_dir = self.find_bin_dir()
            else:
                self.log(f"Build {target} failed.")
                self.ui_call(self._set_build_status, "Build Failed")

            self.ui_call(self._set_build_controls_enabled, True)

        threading.Thread(target=do_build, daemon=True).start()

    def export_all_client_files(self):
        """Export both spells_us and dbstr_us"""
        def do_export():
            self.log("Exporting client files...")
            self.export_spells()
            self.export_dbstr()
            self.log("Client file export complete.")
            self.refresh_status_indicators()

        threading.Thread(target=do_export, daemon=True).start()

    def full_rebuild_and_restart(self):
        """Build zone, restart zones - common testing workflow"""
        def do_full():
            self.log("=== Full Rebuild and Restart ===")

            # Build zone
            self.ui_call(self._set_build_controls_enabled, False)
            self.ui_call(self._set_build_status, "Building zone...")

            cmd = ["cmake", "--build", "build", "--target", "zone", "--config", "RelWithDebInfo", "--parallel"]
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

            while True:
                line = proc.stdout.readline()
                if not line:
                    break
                line = line.strip()
                if line:
                    self.log(line)
            proc.wait()

            if proc.returncode != 0:
                self.log("Build failed, aborting restart.")
                self.ui_call(self._set_build_status, "Build Failed")
                self.ui_call(self._set_build_controls_enabled, True)
                return

            self.ui_call(self._set_build_status, "Restarting...")
            self.bin_dir = self.find_bin_dir()

            # Stop managed zone stack only (safer than image-wide taskkill).
            self._stop_managed_process("zone")
            self._stop_managed_process("eqlaunch")

            time.sleep(1)

            # Restart eqlaunch
            self.after(0, lambda: self.start_process("eqlaunch"))

            self.ui_call(self._set_build_status, "Ready")
            self.ui_call(self._set_build_controls_enabled, True)
            self.log("=== Rebuild and Restart Complete ===")

        threading.Thread(target=do_full, daemon=True).start()

    # ==================== Original Methods ====================

    def refresh_scripts(self):
        self.scripts_listbox.delete(0, "end")
        scripts = glob.glob("*.py")
        for script in scripts:
            if script not in ["server_manager.py", "start_server.py", "stop_server.py"]:
                self.scripts_listbox.insert("end", script)

    def run_selected_script(self):
        selection = self.scripts_listbox.curselection()
        if not selection:
            return
        script_name = self.scripts_listbox.get(selection[0])
        self.log(f"Running script: {script_name}")

        # Run in new console
        if sys.platform == "win32":
            subprocess.Popen(["cmd", "/k", sys.executable, script_name], creationflags=subprocess.CREATE_NEW_CONSOLE)
        else:
            subprocess.Popen([sys.executable, script_name])

    def log(self, message):
        def _do_log():
            self.log_text.config(state="normal")
            self.log_text.insert("end", f"{message}\n")
            self.log_text.see("end")
            self.log_text.config(state="disabled")
        # Thread-safe: always dispatch to main thread
        try:
            self.after(0, _do_log)
        except Exception:
            pass

    def _get_logs_dir(self):
        return os.path.join(os.getcwd(), "logs")

    def _friendly_log_label(self, path):
        base = os.path.basename(path).lower()
        if base.startswith("zone"):
            return f"ZONE | {os.path.basename(path)}"
        if base.startswith("world"):
            return f"WORLD | {os.path.basename(path)}"
        if base.startswith("loginserver"):
            return f"LOGIN | {os.path.basename(path)}"
        if base.startswith("ucs"):
            return f"UCS | {os.path.basename(path)}"
        if base.startswith("queryserv"):
            return f"QUERYSERV | {os.path.basename(path)}"
        if base.startswith("eqlaunch"):
            return f"EQLAUNCH | {os.path.basename(path)}"
        return os.path.basename(path)

    def _latest_log_for_prefix(self, prefix):
        logs_dir = self._get_logs_dir()
        if not os.path.isdir(logs_dir):
            return None
        matches = glob.glob(os.path.join(logs_dir, f"{prefix}*.log"))
        matches = [p for p in matches if os.path.isfile(p)]
        if not matches:
            return None
        return max(matches, key=os.path.getmtime)

    def refresh_runtime_log_files(self):
        logs_dir = self._get_logs_dir()
        if not os.path.isdir(logs_dir):
            self.log_source_var.set(f"Source: missing {logs_dir}")
            self.log_file_entries = {}
            self.runtime_log_combo["values"] = []
            self.selected_log_path_var.set("")
            return

        patterns = [
            "zone*.log",
            "world*.log",
            "loginserver*.log",
            "ucs*.log",
            "queryserv*.log",
            "eqlaunch*.log",
            "shared_memory*.log",
            "dinput8_debug.log",
            "stats_debug.log",
            "*.log",
        ]

        found = []
        for pattern in patterns:
            found.extend(glob.glob(os.path.join(logs_dir, pattern)))

        # unique + existing
        paths = sorted(
            {p for p in found if os.path.isfile(p)},
            key=lambda p: os.path.getmtime(p),
            reverse=True,
        )

        entries = {}
        labels = []
        for p in paths:
            label = self._friendly_log_label(p)
            # keep labels unique
            unique = label
            n = 2
            while unique in entries:
                unique = f"{label} ({n})"
                n += 1
            entries[unique] = p
            labels.append(unique)

        self.log_file_entries = entries
        self.runtime_log_combo["values"] = labels

        # keep currently selected if still present
        current_path = self.selected_log_path_var.get()
        if current_path and current_path in entries.values():
            for lbl, p in entries.items():
                if p == current_path:
                    self.runtime_log_combo.set(lbl)
                    break
        elif labels:
            self.runtime_log_combo.set(labels[0])
            self.selected_log_path_var.set(entries[labels[0]])
            self._load_selected_log_initial()
        else:
            self.selected_log_path_var.set("")
            self.log_source_var.set("Source: no logs found")

    def _latest_zone_log(self):
        return self._latest_log_for_prefix("zone")

    def _select_runtime_log_path(self, path):
        if not path:
            return
        for lbl, entry_path in self.log_file_entries.items():
            if entry_path == path:
                self.runtime_log_combo.set(lbl)
                break
        self.selected_log_path_var.set(path)
        self._load_selected_log_initial()

    def select_active_zone_log(self):
        self.refresh_runtime_log_files()
        zone_path = self._latest_zone_log()
        if not zone_path:
            self.log("No zone log found in /logs.")
            return
        self._select_runtime_log_path(zone_path)

    def select_active_world_log(self):
        self.refresh_runtime_log_files()
        world_path = self._latest_log_for_prefix("world")
        if not world_path:
            self.log("No world log found in /logs.")
            return
        self._select_runtime_log_path(world_path)

    def select_most_recent_runtime_log(self):
        self.refresh_runtime_log_files()
        if not self.log_file_entries:
            self.log("No runtime logs found in /logs.")
            return
        newest = max(self.log_file_entries.values(), key=os.path.getmtime)
        self._select_runtime_log_path(newest)

    def on_runtime_log_selected(self, _event=None):
        label = self.runtime_log_combo.get()
        path = self.log_file_entries.get(label, "")
        self.selected_log_path_var.set(path)
        self._load_selected_log_initial()

    def _load_selected_log_initial(self):
        path = self.selected_log_path_var.get()
        if not path or not os.path.isfile(path):
            self.log_source_var.set("Source: none")
            self.runtime_log_text.config(state="normal")
            self.runtime_log_text.delete("1.0", "end")
            self.runtime_log_text.insert("end", "No log selected.\n")
            self.runtime_log_text.config(state="disabled")
            self._log_tail_pos = 0
            return

        self.log_source_var.set(f"Source: {os.path.basename(path)}")
        # Show last ~400 lines for immediate context.
        try:
            with open(path, "r", encoding="utf-8", errors="ignore") as f:
                data = f.read()
            lines = data.splitlines()
            tail = "\n".join(lines[-400:]) + ("\n" if lines else "")
            self.runtime_log_text.config(state="normal")
            self.runtime_log_text.delete("1.0", "end")
            self.runtime_log_text.insert("end", tail)
            self.runtime_log_text.see("end")
            self.runtime_log_text.config(state="disabled")
            self._log_tail_pos = os.path.getsize(path)
        except Exception as e:
            self.runtime_log_text.config(state="normal")
            self.runtime_log_text.delete("1.0", "end")
            self.runtime_log_text.insert("end", f"Failed to read log: {e}\n")
            self.runtime_log_text.config(state="disabled")
            self._log_tail_pos = 0

    def runtime_log_follow_loop(self):
        try:
            path = self.selected_log_path_var.get()
            if self.log_follow_var.get() and path and os.path.isfile(path):
                size = os.path.getsize(path)
                # rotated/truncated
                if size < self._log_tail_pos:
                    self._log_tail_pos = 0
                if size > self._log_tail_pos:
                    with open(path, "r", encoding="utf-8", errors="ignore") as f:
                        f.seek(self._log_tail_pos)
                        chunk = f.read()
                    self._log_tail_pos = size
                    if chunk:
                        self.runtime_log_text.config(state="normal")
                        self.runtime_log_text.insert("end", chunk)
                        # keep widget bounded
                        max_lines = 3000
                        current_lines = int(self.runtime_log_text.index("end-1c").split(".")[0])
                        if current_lines > max_lines:
                            self.runtime_log_text.delete("1.0", f"{current_lines - max_lines}.0")
                        self.runtime_log_text.see("end")
                        self.runtime_log_text.config(state="disabled")
        except Exception:
            pass
        finally:
            self.after(1000, self.runtime_log_follow_loop)

    def _refresh_process_overview(self):
        total = len(self.process_info)
        running = 0
        external = 0
        for info in self.process_info:
            name = info["name"]
            managed_running = name in self.processes and self.processes[name].poll() is None
            is_external = hasattr(self, "_external_state") and bool(self._external_state.get(name, False))
            if managed_running or is_external:
                running += 1
            if is_external:
                external += 1
        self.process_summary_var.set(f"Running: {running}/{total}  |  External: {external}")

    def set_status_label(self, key, ok, msg):
        if key not in self.status_labels:
            return
        lbl = self.status_labels[key]
        lbl.config(text=msg, foreground="green" if ok else "red")

    def refresh_status_indicators(self):
        """Run status checks in a thread and update UI"""
        threading.Thread(target=self._refresh_status_worker, daemon=True).start()

    def _refresh_status_worker(self):
        try:
            # DLL status
            dll_ok, dll_msg = self._compute_dll_status()
            self.after(0, lambda ok=dll_ok, msg=dll_msg: self.set_status_label("dll", ok, msg))
        except Exception as e:
            err_msg = f"DLL: error {e}"
            self.after(0, lambda msg=err_msg: self.set_status_label("dll", False, msg))

        try:
            spells_ok, spells_msg, dbstr_ok, dbstr_msg = self._compute_export_status()
            self.after(0, lambda ok=spells_ok, msg=spells_msg: self.set_status_label("spells", ok, msg))
            self.after(0, lambda ok=dbstr_ok, msg=dbstr_msg: self.set_status_label("dbstr", ok, msg))
        except Exception as e:
            spells_err = f"spells_us: error {e}"
            dbstr_err = f"dbstr_us: error {e}"
            self.after(0, lambda msg=spells_err: self.set_status_label("spells", False, msg))
            self.after(0, lambda msg=dbstr_err: self.set_status_label("dbstr", False, msg))

    def clean_build(self):
        if messagebox.askyesno("Clean Build", "Are you sure you want to delete the build directory?"):
            if os.path.exists(self.build_dir):
                try:
                    shutil.rmtree(self.build_dir)
                    self.log("Build directory removed.")
                except Exception as e:
                    self.log(f"Error removing build directory: {e}")
            else:
                self.log("Build directory does not exist.")

    def run_build_thread(self):
        threading.Thread(target=self.run_build, daemon=True).start()

    def _is_lock_related_build_line(self, line):
        if not line:
            return False
        s = line.lower()
        lock_patterns = [
            "fatal error lnk1168",
            "cannot open",
            "for writing",
            "pdb",
            "idb",
            "mspdbsrv",
            "c2471",
            "lnk1318",
            "unexpected pdb error",
            "access is denied",
            "being used by another process",
        ]
        return any(p in s for p in lock_patterns)

    def _kill_process_images(self, image_names):
        if os.name != "nt":
            return
        for image in image_names:
            try:
                subprocess.run(
                    ["taskkill", "/F", "/IM", image],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                    check=False
                )
            except Exception:
                pass

    def _run_streamed_command(self, cmd, cwd=None):
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=cwd or os.getcwd()
        )
        lines = []
        while True:
            line = proc.stdout.readline()
            if not line:
                break
            msg = line.strip()
            lines.append(msg)
            self.log(msg)
        proc.wait()
        return proc.returncode, lines

    def _build_cmake_target_once(self, build_target):
        cmd_build = ["cmake", "--build", "build"]
        if build_target and build_target != "all":
            cmd_build += ["--target", build_target]
        cmd_build += ["--config", "RelWithDebInfo"]

        # Pass MSBuild-specific anti-lock flags via the native tool switch
        if os.name == "nt":
            cmd_build += ["--", "/p:BuildInParallel=false", "/p:TrackFileAccess=false"]

        return self._run_streamed_command(cmd_build)

    def _attempt_lock_recovery(self):
        self.log("Build lock recovery: stopping all server processes (managed + external)...")
        self._kill_all_server_processes(include_external=True)
        # Also clear common Windows lock holders for PDB/EXE writes.
        self.log("Build lock recovery: stopping compiler/linker lock holders...")
        self._kill_process_images([
            "zone.exe",
            "world.exe",
            "eqlaunch.exe",
            "loginserver.exe",
            "ucs.exe",
            "queryserv.exe",
            "shared_memory.exe",
            "mspdbsrv.exe",
        ])
        time.sleep(2)

    def run_build(self):
        self.ui_call(self._set_build_controls_enabled, False)
        build_target = (self.build_target_var.get() or "all").strip()
        self.ui_call(self._set_build_status, f"Building ({build_target})...")

        # Automatically stop all server processes to release file locks
        self.log("Pre-build maintenance: Stopping all server processes...")
        self._kill_all_server_processes(include_external=False)
        time.sleep(2)

        # Automatically clean up corrupted PDBs before buildup to prevent LNK1318/C2471 errors
        if os.name == 'nt':
            self.log("Pre-build maintenance: Checking for corrupted PDB files...")
            pdb_patterns = [
                "build/**/*.pdb",
                "build/**/*.idb"
            ]
            for pattern in pdb_patterns:
                for pdb_file in glob.glob(os.path.join(os.getcwd(), pattern), recursive=True):
                    try:
                        # Attempting to delete PDBs. If they are locked by a running server,
                        # this will fail safely, but it clears the 'Unexpected PDB error' corruption.
                        os.remove(pdb_file)
                    except Exception:
                        pass

        try:
            if not os.path.exists(self.build_dir):
                os.makedirs(self.build_dir)

            self.log("Running CMake Configure...")
            cmd_config = ["cmake", "-S", ".", "-B", "build"]
            config_rc, _ = self._run_streamed_command(cmd_config)

            if config_rc != 0:
                self.log("CMake Configure Failed.")
                self.ui_call(self._set_build_status, "Configure Failed")
                return

            self.log("Running CMake Build...")
            build_rc, build_lines = self._build_cmake_target_once(build_target)

            if build_rc != 0:
                lock_related = any(self._is_lock_related_build_line(line) for line in build_lines)
                if os.name == "nt" and lock_related:
                    self.log("Build failed with lock-related errors (PDB/EXE). Retrying once after recovery...")
                    self._attempt_lock_recovery()
                    self.log("Retrying CMake Build...")
                    build_rc_retry, _ = self._build_cmake_target_once(build_target)
                    if build_rc_retry != 0:
                        self.log("Build Failed after lock recovery retry.")
                        self.ui_call(self._set_build_status, "Build Failed")
                        return
                else:
                    self.log("Build Failed.")
                    self.ui_call(self._set_build_status, "Build Failed")
                    return

            if build_rc == 0:
                self.log("Build Successful.")
                self.ui_call(self._set_build_status, "Build Complete")
                self.bin_dir = self.find_bin_dir()
            else:
                # Success through retry path
                self.log("Build Successful (after lock recovery retry).")
                self.ui_call(self._set_build_status, "Build Complete")
                self.bin_dir = self.find_bin_dir()

        except Exception as e:
            self.log(f"Build Error: {e}")
            self.ui_call(self._set_build_status, "Error")
        finally:
            self.ui_call(self._set_build_controls_enabled, True)

    def run_shared_memory_thread(self):
        threading.Thread(target=self.run_shared_memory, daemon=True).start()

    def run_shared_memory(self):
        self.log("Updating Shared Memory...")
        try:
            exe_path = os.path.join(self.bin_dir, "shared_memory.exe")
            if not os.path.exists(exe_path):
                self.log(f"shared_memory.exe not found at {exe_path}")
                return

            self.log(f"Executing: {exe_path}")

            env = os.environ.copy()
            env["PATH"] = self.bin_dir + os.pathsep + self.vcpkg_bin_dir + os.pathsep + self.perl_bin_dir + os.pathsep + env["PATH"]

            # Run shared memory and wait for it to complete
            proc = subprocess.Popen(
                [exe_path],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                cwd=os.getcwd(),
                env=env
            )

            while True:
                line = proc.stdout.readline()
                if not line: break
                self.log(f"[SharedMemory] {line.strip()}")
            proc.wait()

            if proc.returncode == 0:
                self.log("Shared memory updated successfully.")
            else:
                self.log(f"Shared memory failed with code {proc.returncode}")

        except Exception as e:
            self.log(f"Shared memory error: {e}")

    def start_all_sequence_thread(self):
        threading.Thread(target=self.start_all_sequence, daemon=True).start()

    def start_all_sequence(self):
        self.after(0, lambda: self.start_all_btn.config(state="disabled"))

        # 1. Shared Memory
        self.run_shared_memory()

        # 2. Start processes in order, calling start_process directly
        #    from the bg thread so sleeps actually gate each launch.
        start_order = ["loginserver", "world", "ucs", "queryserv", "eqlaunch"]

        for name in start_order:
            self.log(f"Starting {name}...")
            self._launch_process(name)

            # Wait for process to initialize
            if name == "world":
                time.sleep(5)
            elif name == "loginserver":
                time.sleep(2)
            else:
                time.sleep(1)

        self.log("All start commands issued.")
        self.after(0, lambda: self.start_all_btn.config(state="normal"))

    def open_process_options(self, name):
        if name not in self.proc_widgets:
            return

        widgets = self.proc_widgets[name]
        top = tk.Toplevel(self)
        top.title(f"Process Options - {name}")
        top.resizable(False, False)
        top.transient(self)
        top.grab_set()

        frame = ttk.Frame(top, padding=10)
        frame.pack(fill="both", expand=True)

        ttk.Label(frame, text="Arguments:").grid(row=0, column=0, sticky="w", padx=(0, 6), pady=4)
        args_entry = ttk.Entry(frame, textvariable=widgets["args_var"], width=50)
        args_entry.grid(row=0, column=1, sticky="ew", pady=4)

        ttk.Checkbutton(frame, text="Start in new console window", variable=widgets["console_var"]).grid(
            row=1, column=0, columnspan=2, sticky="w", pady=4
        )

        ttk.Label(
            frame,
            text="Tip: Use quotes for args with spaces. Example: --name \"Zone Worker 1\"",
            foreground="gray",
        ).grid(row=2, column=0, columnspan=2, sticky="w", pady=(0, 8))

        btns = ttk.Frame(frame)
        btns.grid(row=3, column=0, columnspan=2, sticky="e")

        def reset_default():
            default_args = ""
            for info in self.process_info:
                if info["name"] == name:
                    default_args = info.get("args", "")
                    break
            widgets["args_var"].set(default_args)
            widgets["console_var"].set(False)

        ttk.Button(btns, text="Reset Defaults", command=reset_default).pack(side="left", padx=4)
        ttk.Button(btns, text="Close", command=top.destroy).pack(side="left", padx=4)

        frame.grid_columnconfigure(1, weight=1)
        args_entry.focus_set()

    def _get_exe_name(self, name):
        return f"{name}.exe" if sys.platform == "win32" else name

    def _is_process_running_system(self, name):
        exe_name = self._get_exe_name(name)
        try:
            if sys.platform == "win32":
                result = subprocess.run(
                    ["tasklist", "/FI", f"IMAGENAME eq {exe_name}"],
                    capture_output=True,
                    text=True,
                    check=False,
                )
                return exe_name.lower() in (result.stdout or "").lower()
            result = subprocess.run(["pgrep", "-f", exe_name], capture_output=True, text=True, check=False)
            return result.returncode == 0 and bool((result.stdout or "").strip())
        except Exception:
            return False

    def _taskkill_image(self, exe_name, force=False):
        if sys.platform != "win32":
            return
        cmd = ["taskkill", "/IM", exe_name]
        if force:
            cmd.insert(1, "/F")
        subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False)

    def _stop_managed_process(self, name):
        if name not in self.processes:
            return
        proc = self.processes[name]
        try:
            if proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    proc.kill()
        except Exception:
            pass
        self.processes.pop(name, None)
        self._set_external_state(name, False)
        self.ui_call(self.update_ui_state, name, False, False)

    def _launch_process(self, name):
        """Launch a process (thread-safe). Can be called from any thread.
        Performs the actual Popen and dispatches UI updates to the main thread."""
        if name in self.processes and self.processes[name].poll() is None:
            self.log(f"{name} is already running.")
            return

        exe_name = self._get_exe_name(name)
        exe_path = os.path.join(self.bin_dir, exe_name)

        if not os.path.exists(exe_path):
            self.log(f"Executable not found: {exe_path}")
            return

        try:
            args = shlex.split(self.proc_widgets[name]["args_var"].get(), posix=False)
        except ValueError as e:
            self.log(f"Invalid arguments for {name}: {e}")
            return

        try:
            cwd = os.getcwd()
            env = os.environ.copy()
            # Important: Add bin_dir to PATH so DLLs are found
            env["PATH"] = self.bin_dir + os.pathsep + self.vcpkg_bin_dir + os.pathsep + self.perl_bin_dir + os.pathsep + env["PATH"]

            proc = subprocess.Popen(
                [exe_path] + args,
                cwd=cwd,
                env=env,
                creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0
            )

            self.processes[name] = proc
            self._set_external_state(name, False)
            self.after(0, lambda: self.update_ui_state(name, True))
            self.log(f"Started {name} (PID {proc.pid})")
        except Exception as e:
            self.log(f"Failed to start {name}: {e}")

    def start_process(self, name):
        """Start a process from the UI (main thread). Supports console mode."""
        if name in self.processes and self.processes[name].poll() is None:
            self.log(f"{name} is already running.")
            return

        exe_name = self._get_exe_name(name)
        exe_path = os.path.join(self.bin_dir, exe_name)

        if not os.path.exists(exe_path):
            self.log(f"Executable not found: {exe_path}")
            return

        try:
            args = shlex.split(self.proc_widgets[name]["args_var"].get(), posix=False)
        except ValueError as e:
            self.log(f"Invalid arguments for {name}: {e}")
            return
        new_console = self.proc_widgets[name]["console_var"].get()

        try:
            cwd = os.getcwd()
            env = os.environ.copy()
            env["PATH"] = self.bin_dir + os.pathsep + self.vcpkg_bin_dir + os.pathsep + self.perl_bin_dir + os.pathsep + env["PATH"]

            if new_console:
                creationflags = subprocess.CREATE_NEW_CONSOLE if sys.platform == "win32" else 0
                proc = subprocess.Popen([exe_path] + args, cwd=cwd, creationflags=creationflags, env=env)
            else:
                proc = subprocess.Popen(
                    [exe_path] + args,
                    cwd=cwd,
                    env=env,
                    creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0
                )

            self.processes[name] = proc
            self._set_external_state(name, False)
            self.update_ui_state(name, True)
            self.log(f"Started {name} (PID {proc.pid})")
        except Exception as e:
            self.log(f"Failed to start {name}: {e}")

    def stop_process(self, name):
        if name in self.processes:
            proc = self.processes[name]
            if proc.poll() is None:
                self.log(f"Stopping {name}...")
                proc.terminate()
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    proc.kill()
                self.log(f"{name} stopped.")
            del self.processes[name]
            self.update_ui_state(name, False)
            return

        # If not managed but currently running, offer to stop external instance.
        if self._is_process_running_system(name):
            exe_name = self._get_exe_name(name)
            if messagebox.askyesno("Stop External Process", f"{exe_name} appears to be running outside this manager.\nStop it now?"):
                self._taskkill_image(exe_name, force=False)
                time.sleep(0.2)
                if self._is_process_running_system(name):
                    self._taskkill_image(exe_name, force=True)
                self.log(f"Stopped external {name}.")
                self._set_external_state(name, False)
                self.update_ui_state(name, False, external=False)
            else:
                self._set_external_state(name, True)
                self.update_ui_state(name, True, external=True)

    def stop_all_processes(self):
        self.log("Stopping all managed processes...")
        for name in list(self.processes.keys()):
            self.stop_process(name)

    def force_kill_all(self):
        if messagebox.askyesno("Force Kill", "This will forcefully terminate all server processes (taskkill). Continue?"):
            include_external = messagebox.askyesno(
                "Include External Instances?",
                "Also kill matching server processes not started by this manager?\n\n"
                "Yes = kill all by image name.\n"
                "No = kill only managed PIDs."
            )
            self._kill_all_server_processes(include_external=include_external)

    def _kill_all_server_processes(self, include_external=True):
        """Kill all server processes without confirmation"""
        self.log("Force killing all server processes...")

        # 1) Kill managed processes by PID first (safest scope).
        for name, proc in list(self.processes.items()):
            try:
                if proc.poll() is None:
                    proc.kill()
            except Exception:
                pass

        # 2) Optionally kill external instances by image name.
        if include_external and sys.platform == "win32":
            targets = ["loginserver.exe", "world.exe", "ucs.exe", "queryserv.exe", "eqlaunch.exe", "zone.exe", "shared_memory.exe"]
            for target in targets:
                try:
                    subprocess.run(["taskkill", "/F", "/IM", target], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                except Exception:
                    pass

        # Clear internal state
        self.processes.clear()
        for name in self.proc_widgets:
            self.update_ui_state(name, False, external=False)
        self.log("All processes killed.")

    def restart_zones(self):
        """Quick restart: kill zone processes and restart eqlaunch for rapid testing"""
        def do_restart():
            self.log("Restarting zones...")
            # Stop managed zone stack only.
            self._stop_managed_process("zone")
            self._stop_managed_process("eqlaunch")

            time.sleep(2)

            # Restart eqlaunch
            self._launch_process("eqlaunch")
            self.log("Zones restarting via eqlaunch...")

        threading.Thread(target=do_restart, daemon=True).start()

    def update_ui_state(self, name, is_running, external=None):
        widgets = self.proc_widgets[name]
        if external is None:
            external = False
            if hasattr(self, "_external_state") and name in self._external_state:
                external = self._external_state[name]

        if is_running and external:
            widgets["status_lbl"].config(text="Running (External)", foreground="orange")
            widgets["start_btn"].config(state="disabled")
            widgets["stop_btn"].config(state="normal")
            if "card" in widgets:
                widgets["card"].config(style="ProcessExternal.TFrame")
        elif is_running:
            widgets["status_lbl"].config(text="Running", foreground="green")
            widgets["start_btn"].config(state="disabled")
            widgets["stop_btn"].config(state="normal")
            if "card" in widgets:
                widgets["card"].config(style="ProcessRunning.TFrame")
        else:
            widgets["status_lbl"].config(text="Stopped", foreground="red")
            widgets["start_btn"].config(state="normal")
            widgets["stop_btn"].config(state="disabled")
            if "card" in widgets:
                widgets["card"].config(style="ProcessStopped.TFrame")
        self._update_process_hint(name)
        self._refresh_process_overview()

    def _set_external_state(self, name, external):
        if not hasattr(self, "_external_state"):
            self._external_state = {}
        self._external_state[name] = external

    def update_status_loop(self):
        for name, proc in list(self.processes.items()):
            if proc.poll() is not None:
                self.log(f"{name} exited with code {proc.returncode}")
                del self.processes[name]
                self._set_external_state(name, False)
                self.update_ui_state(name, False)

        # Reconcile external/manual processes so status reflects reality.
        for info in self.process_info:
            name = info["name"]
            managed_running = name in self.processes and self.processes[name].poll() is None
            if managed_running:
                self._set_external_state(name, False)
                continue
            external_running = self._is_process_running_system(name)
            self._set_external_state(name, external_running)
            self.update_ui_state(name, external_running)
        self.after(1000, self.update_status_loop)

    def on_closing(self):
        self._save_manager_settings()
        if self.processes:
            if messagebox.askokcancel("Quit", "Running processes will be stopped. Do you want to quit?"):
                self.stop_all_processes()
                self.destroy()
        else:
            self.destroy()

if __name__ == "__main__":
    app = ServerManagerApp()
    app.protocol("WM_DELETE_WINDOW", app.on_closing)
    app.mainloop()
