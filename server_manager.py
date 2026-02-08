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
import mysql.connector

class ServerManagerApp(tk.Tk):
    def __init__(self):
        super().__init__()

        self.title("EQEmu Server Manager")
        self.geometry("1100x850")

        self.processes = {}

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
        self.eq_dir_var = tk.StringVar(value=r"D:\Rof2")
        self.build_target_var = tk.StringVar(value="all")

        self.create_widgets()
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

    def create_widgets(self):
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

    def create_main_tab(self):
        # Build Section
        build_frame = ttk.LabelFrame(self.main_tab, text="Build Server")
        build_frame.pack(fill="x", padx=10, pady=5)

        self.build_btn = ttk.Button(build_frame, text="Build (CMake)", command=self.run_build_thread)
        self.build_btn.pack(side="left", padx=5, pady=5)

        self.clean_btn = ttk.Button(build_frame, text="Clean Build", command=self.clean_build)
        self.clean_btn.pack(side="left", padx=5, pady=5)

        ttk.Label(build_frame, text="Target:").pack(side="left", padx=(10, 2))
        self.build_target_combo = ttk.Combobox(
            build_frame,
            textvariable=self.build_target_var,
            width=18,
            state="readonly",
            values=[
                "all",
                "zone",
                "world",
                "loginserver",
                "ucs",
                "queryserv",
                "shared_memory",
                "eqlaunch",
            ],
        )
        self.build_target_combo.pack(side="left", padx=5, pady=5)

        self.build_status_lbl = ttk.Label(build_frame, text="Ready")
        self.build_status_lbl.pack(side="left", padx=5, pady=5)

        # Global Controls
        global_frame = ttk.LabelFrame(self.main_tab, text="Global Controls")
        global_frame.pack(fill="x", padx=10, pady=5)

        self.start_all_btn = ttk.Button(global_frame, text="Start All (Sequence)", command=self.start_all_sequence_thread)
        self.start_all_btn.pack(side="left", padx=5, pady=5)

        self.stop_all_btn = ttk.Button(global_frame, text="Stop All", command=self.stop_all_processes)
        self.stop_all_btn.pack(side="left", padx=5, pady=5)

        self.force_stop_btn = ttk.Button(global_frame, text="Force Kill All (Taskkill)", command=self.force_kill_all)
        self.force_stop_btn.pack(side="left", padx=5, pady=5)

        self.shared_mem_btn = ttk.Button(global_frame, text="Run Shared Memory", command=self.run_shared_memory_thread)
        self.shared_mem_btn.pack(side="left", padx=5, pady=5)

        # Quick restart for testing - kills zones and restarts eqlaunch
        self.restart_zones_btn = ttk.Button(global_frame, text="Restart Zones", command=self.restart_zones)
        self.restart_zones_btn.pack(side="left", padx=5, pady=5)

        # Client assets/status + quick actions
        client_frame = ttk.LabelFrame(self.main_tab, text="Client Assets (EQ Folder + DLL/Exports)")
        client_frame.pack(fill="x", padx=10, pady=5)

        ttk.Label(client_frame, text="EQ Client Folder:").grid(row=0, column=0, padx=5, pady=5, sticky="e")
        ttk.Entry(client_frame, textvariable=self.eq_dir_var, width=50).grid(row=0, column=1, padx=5, pady=5, sticky="w")
        ttk.Button(client_frame, text="Browse", command=self.browse_eq_dir).grid(row=0, column=2, padx=5, pady=5)
        ttk.Button(client_frame, text="Refresh Status", command=self.refresh_status_indicators).grid(row=0, column=3, padx=5, pady=5)

        # Status labels (updated on load/refresh)
        self.status_labels = {
            "dll": ttk.Label(client_frame, text="DLL: checking...", foreground="blue"),
            "spells": ttk.Label(client_frame, text="spells_us: checking...", foreground="blue"),
            "dbstr": ttk.Label(client_frame, text="dbstr_us: checking...", foreground="blue"),
        }
        self.status_labels["dll"].grid(row=1, column=0, columnspan=2, padx=5, pady=2, sticky="w")
        self.status_labels["spells"].grid(row=2, column=0, columnspan=2, padx=5, pady=2, sticky="w")
        self.status_labels["dbstr"].grid(row=3, column=0, columnspan=2, padx=5, pady=2, sticky="w")

        # Quick action buttons
        ttk.Button(client_frame, text="Build + Copy DLL", command=self.run_build_eqcore_thread).grid(row=1, column=2, padx=5, pady=2, sticky="w")
        ttk.Button(client_frame, text="Copy DLL Only", command=self.copy_eqcore_dll).grid(row=1, column=3, padx=5, pady=2, sticky="w")
        ttk.Button(client_frame, text="Export spells_us", command=self.run_export_spells_thread).grid(row=2, column=2, padx=5, pady=2, sticky="w")
        ttk.Button(client_frame, text="Export dbstr_us", command=self.run_export_dbstr_thread).grid(row=2, column=3, padx=5, pady=2, sticky="w")
        ttk.Button(client_frame, text="Launch eqgame (patchme)", command=self.launch_eqgame).grid(row=3, column=2, padx=5, pady=2, sticky="w")

        # Processes List
        proc_frame = ttk.LabelFrame(self.main_tab, text="Server Processes")
        proc_frame.pack(fill="both", expand=True, padx=10, pady=5)

        # Headers
        ttk.Label(proc_frame, text="Process").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        ttk.Label(proc_frame, text="Status").grid(row=0, column=1, padx=5, pady=5, sticky="w")
        ttk.Label(proc_frame, text="Arguments").grid(row=0, column=2, padx=5, pady=5, sticky="w")
        ttk.Label(proc_frame, text="Actions").grid(row=0, column=3, padx=5, pady=5, sticky="w")

        self.proc_widgets = {}
        for i, info in enumerate(self.process_info):
            row = i + 1
            name = info["name"]

            ttk.Label(proc_frame, text=info["display"]).grid(row=row, column=0, padx=5, pady=5, sticky="w")

            status_lbl = ttk.Label(proc_frame, text="Stopped", foreground="red")
            status_lbl.grid(row=row, column=1, padx=5, pady=5, sticky="w")

            args_var = tk.StringVar(value=info["args"])
            ttk.Entry(proc_frame, textvariable=args_var, width=20).grid(row=row, column=2, padx=5, pady=5, sticky="w")

            btn_frame = ttk.Frame(proc_frame)
            btn_frame.grid(row=row, column=3, padx=5, pady=5, sticky="w")

            start_btn = ttk.Button(btn_frame, text="Start", command=lambda n=name: self.start_process(n))
            start_btn.pack(side="left", padx=2)

            stop_btn = ttk.Button(btn_frame, text="Stop", state="disabled", command=lambda n=name: self.stop_process(n))
            stop_btn.pack(side="left", padx=2)

            console_var = tk.BooleanVar(value=False)
            ttk.Checkbutton(btn_frame, text="Console", variable=console_var).pack(side="left", padx=5)

            self.proc_widgets[name] = {
                "status_lbl": status_lbl,
                "start_btn": start_btn,
                "stop_btn": stop_btn,
                "args_var": args_var,
                "console_var": console_var
            }

        # Log Section
        log_frame = ttk.LabelFrame(self.main_tab, text="Manager Log")
        log_frame.pack(fill="both", expand=True, padx=10, pady=5)
        self.log_text = scrolledtext.ScrolledText(log_frame, height=8, state="disabled")
        self.log_text.pack(fill="both", expand=True, padx=5, pady=5)

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
                self.db_output.delete(1.0, "end")

                db_cfg = self._load_db_config()
                conn = mysql.connector.connect(**db_cfg)
                cur = conn.cursor()

                cur.execute("SHOW TABLES LIKE 'launcher_zones'")
                if not cur.fetchone():
                    self.db_output.insert("end", "Table launcher_zones not found in this database.\n")
                    self.db_output.see("end")
                    cur.close()
                    conn.close()
                    return

                cur.execute("SHOW COLUMNS FROM launcher_zones")
                columns = [r[0] for r in cur.fetchall()]
                self.db_output.insert("end", f"launcher_zones columns: {', '.join(columns)}\n\n")

                order_cols = [c for c in ["launcher", "zone", "port", "number", "startzone"] if c in columns]
                if not order_cols:
                    order_cols = [columns[0]]

                q = f"SELECT * FROM launcher_zones ORDER BY {', '.join(order_cols)} LIMIT 200"
                cur.execute(q)
                rows = cur.fetchall()

                self.db_output.insert("end", f"Showing up to {len(rows)} rows:\n")
                for row in rows:
                    self.db_output.insert("end", f"{row}\n")
                self.db_output.see("end")

                cur.close()
                conn.close()
            except Exception as e:
                self.db_output.insert("end", f"Error listing launcher_zones: {e}\n")
                self.db_output.see("end")

        threading.Thread(target=run, daemon=True).start()

    def trim_launcher_zones(self):
        def run():
            try:
                keep = int(self.launcher_keep_count.get())
                if keep < 0:
                    keep = 0

                if not messagebox.askyesno(
                    "Trim launcher_zones",
                    f"This will keep only {keep} launcher_zones rows per launcher (based on sorted order) and delete the rest.\n\nContinue?",
                ):
                    return

                self.db_output.delete(1.0, "end")
                self.db_output.insert("end", f"Trimming launcher_zones to {keep} per launcher...\n")

                db_cfg = self._load_db_config()
                conn = mysql.connector.connect(**db_cfg)
                cur = conn.cursor()

                cur.execute("SHOW TABLES LIKE 'launcher_zones'")
                if not cur.fetchone():
                    self.db_output.insert("end", "Table launcher_zones not found in this database.\n")
                    self.db_output.see("end")
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
                    self.db_output.insert("end", "Nothing to delete.\n")
                    self.db_output.see("end")
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
                self.db_output.insert("end", f"Deleted {deleted} rows.\n")
                self.db_output.insert("end", "Done.\n")
                self.db_output.see("end")

                cur.close()
                conn.close()
            except Exception as e:
                self.db_output.insert("end", f"Error trimming launcher_zones: {e}\n")
                self.db_output.see("end")

        threading.Thread(target=run, daemon=True).start()

    def run_db_script(self, script):
        """Run a database diagnostic script"""
        self.log(f"Running database script: {script}")
        self.run_python_script_with_output(script, [], self.db_output)

    def run_python_script_with_output(self, script, args, output_widget):
        """Run Python script and display output in widget"""
        def run():
            try:
                output_widget.delete(1.0, "end")
                output_widget.insert("end", f"Running: python {script} {' '.join(str(a) for a in args)}\n")
                output_widget.insert("end", "=" * 60 + "\n")

                result = subprocess.run(
                    [sys.executable, script] + [str(a) for a in args],
                    capture_output=True,
                    text=True,
                    cwd=os.getcwd()
                )

                output_widget.insert("end", result.stdout)
                if result.stderr:
                    output_widget.insert("end", "\n=== ERRORS ===\n")
                    output_widget.insert("end", result.stderr)

                output_widget.insert("end", "\n" + "=" * 60 + "\n")
                output_widget.insert("end", f"Exit code: {result.returncode}\n")
                output_widget.see("end")

            except Exception as e:
                output_widget.insert("end", f"\nError: {e}\n")
                output_widget.see("end")

        threading.Thread(target=run, daemon=True).start()

    def backup_database(self):
        """Backup database using mysqldump"""
        self.log("Starting database backup...")

        def backup():
            try:
                # Read database config
                with open('eqemu_config.json', 'r') as f:
                    config = json.load(f)

                db_host = config['server']['database']['host']
                db_port = config['server']['database']['port']
                db_user = config['server']['database']['username']
                db_pass = config['server']['database']['password']
                db_name = config['server']['database']['db']

                # Create backup directory
                backup_dir = "backups"
                if not os.path.exists(backup_dir):
                    os.makedirs(backup_dir)

                # Generate filename with timestamp
                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                backup_file = os.path.join(backup_dir, f"eqemu_backup_{timestamp}.sql")

                self.db_output.delete(1.0, "end")
                self.db_output.insert("end", f"Backing up database to: {backup_file}\n")

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
                    self.db_output.insert("end", f"Backup completed successfully!\n")
                    self.db_output.insert("end", f"File size: {file_size:.2f} MB\n")
                    self.log(f"Database backed up to {backup_file}")
                else:
                    self.db_output.insert("end", f"Backup failed:\n{result.stderr}\n")

                self.db_output.see("end")

            except Exception as e:
                self.db_output.insert("end", f"Error: {e}\n")
                self.db_output.see("end")

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
                with open('eqemu_config.json', 'r') as f:
                    config = json.load(f)

                db_host = config['server']['database']['host']
                db_port = config['server']['database']['port']
                db_user = config['server']['database']['username']
                db_pass = config['server']['database']['password']
                db_name = config['server']['database']['db']

                self.db_output.delete(1.0, "end")
                self.db_output.insert("end", f"Restoring database from: {filename}\n")

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
                    self.db_output.insert("end", "Restore completed successfully!\n")
                    self.log("Database restored successfully")
                else:
                    self.db_output.insert("end", f"Restore failed:\n{result.stderr}\n")

                self.db_output.see("end")

            except Exception as e:
                self.db_output.insert("end", f"Error: {e}\n")
                self.db_output.see("end")

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
                self.db_output.insert("end", f"Exported spells to {dest}\n")
                self.db_output.see("end")
        except Exception as e:
            self.log(f"Spell export failed: {e}")
            if hasattr(self, "db_output"):
                self.db_output.insert("end", f"Spell export failed: {e}\n")
                self.db_output.see("end")

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
                self.db_output.insert("end", f"Exported dbstr to {dest}\n")
                self.db_output.see("end")
        except Exception as e:
            self.log(f"dbstr export failed: {e}")
            if hasattr(self, "db_output"):
                self.db_output.insert("end", f"dbstr export failed: {e}\n")
                self.db_output.see("end")

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

    def run_build_eqcore_thread(self):
        threading.Thread(target=self.build_eqcore_and_copy, daemon=True).start()

    def build_eqcore_and_copy(self):
        """Build the eq-core DLL using MSBuild and copy to EQ client folder"""
        try:
            self.log("Building eq-core DLL (MSBuild)...")

            # Use MSBuild directly - this matches the VS Code task
            msbuild = r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
            sln_path = os.path.join("extras", "eq-core-dll-main", "eq-core-dll-visualstudio2022.sln")

            if not os.path.exists(msbuild):
                self.log(f"MSBuild not found at {msbuild}")
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
                messagebox.showerror("Error", f"No dinput8.dll found in {self.extra_dll_dir}")
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
                self.db_output.insert("end", msg + "\n")
                self.db_output.see("end")
        except Exception as e:
            self.log(f"Export status failed: {e}")
            if hasattr(self, "db_output"):
                self.db_output.insert("end", f"Export status failed: {e}\n")
                self.db_output.see("end")

    # ==================== Quick Action Methods ====================

    def quick_build(self, target):
        """Quick build a specific target without reconfiguring"""
        def do_build():
            self.build_btn.config(state="disabled")
            self.build_status_lbl.config(text=f"Building {target}...")

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
                self.build_status_lbl.config(text="Build Complete")
                self.bin_dir = self.find_bin_dir()
            else:
                self.log(f"Build {target} failed.")
                self.build_status_lbl.config(text="Build Failed")

            self.build_btn.config(state="normal")

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
            self.build_btn.config(state="disabled")
            self.build_status_lbl.config(text="Building zone...")

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
                self.build_status_lbl.config(text="Build Failed")
                self.build_btn.config(state="normal")
                return

            self.build_status_lbl.config(text="Restarting...")
            self.bin_dir = self.find_bin_dir()

            # Kill zones
            try:
                subprocess.run(["taskkill", "/F", "/IM", "zone.exe"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                subprocess.run(["taskkill", "/F", "/IM", "eqlaunch.exe"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except Exception:
                pass

            if "eqlaunch" in self.processes:
                del self.processes["eqlaunch"]
            if "zone" in self.processes:
                del self.processes["zone"]

            time.sleep(1)

            # Restart eqlaunch
            self.after(0, lambda: self.start_process("eqlaunch"))

            self.build_status_lbl.config(text="Ready")
            self.build_btn.config(state="normal")
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
            subprocess.Popen(f'start cmd /k python "{script_name}"', shell=True)
        else:
            subprocess.Popen(["python", script_name])

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

    def run_build(self):
        self.build_btn.config(state="disabled")
        self.clean_btn.config(state="disabled")
        build_target = (self.build_target_var.get() or "all").strip()
        self.build_status_lbl.config(text=f"Building ({build_target})...")

        # Automatically stop all server processes to release file locks
        self.log("Pre-build maintenance: Stopping all server processes...")
        self._kill_all_server_processes()
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
            proc = subprocess.Popen(cmd_config, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            while True:
                line = proc.stdout.readline()
                if not line: break
                self.log(line.strip())
            proc.wait()

            if proc.returncode != 0:
                self.log("CMake Configure Failed.")
                self.build_status_lbl.config(text="Configure Failed")
                return

            self.log("Running CMake Build...")
            cmd_build = ["cmake", "--build", "build"]
            if build_target and build_target != "all":
                cmd_build += ["--target", build_target]
            cmd_build += ["--config", "RelWithDebInfo"]
            # Optimization: Use sequential build to avoid PDB locking errors (C2471)
            # cmd_build += ["--parallel"]

            # Pass MSBuild-specific anti-lock flags via the native tool switch
            # Check if we are on Windows/MSVC
            if os.name == 'nt':
                cmd_build += ["--", "/p:BuildInParallel=false", "/p:TrackFileAccess=false"]

            proc = subprocess.Popen(cmd_build, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            while True:
                line = proc.stdout.readline()
                if not line: break
                self.log(line.strip())
            proc.wait()

            if proc.returncode != 0:
                self.log("Build Failed.")
                self.build_status_lbl.config(text="Build Failed")
            else:
                self.log("Build Successful.")
                self.build_status_lbl.config(text="Build Complete")
                self.bin_dir = self.find_bin_dir()

        except Exception as e:
            self.log(f"Build Error: {e}")
            self.build_status_lbl.config(text="Error")
        finally:
            self.build_btn.config(state="normal")
            self.clean_btn.config(state="normal")

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

    def _launch_process(self, name):
        """Launch a process (thread-safe). Can be called from any thread.
        Performs the actual Popen and dispatches UI updates to the main thread."""
        if name in self.processes and self.processes[name].poll() is None:
            self.log(f"{name} is already running.")
            return

        exe_name = f"{name}.exe" if sys.platform == "win32" else name
        exe_path = os.path.join(self.bin_dir, exe_name)

        if not os.path.exists(exe_path):
            self.log(f"Executable not found: {exe_path}")
            return

        args = self.proc_widgets[name]["args_var"].get().split()

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
            self.after(0, lambda: self.update_ui_state(name, True))
            self.log(f"Started {name} (PID {proc.pid})")
        except Exception as e:
            self.log(f"Failed to start {name}: {e}")

    def start_process(self, name):
        """Start a process from the UI (main thread). Supports console mode."""
        if name in self.processes and self.processes[name].poll() is None:
            self.log(f"{name} is already running.")
            return

        exe_name = f"{name}.exe" if sys.platform == "win32" else name
        exe_path = os.path.join(self.bin_dir, exe_name)

        if not os.path.exists(exe_path):
            self.log(f"Executable not found: {exe_path}")
            return

        args = self.proc_widgets[name]["args_var"].get().split()
        new_console = self.proc_widgets[name]["console_var"].get()

        try:
            cwd = os.getcwd()
            env = os.environ.copy()
            env["PATH"] = self.bin_dir + os.pathsep + self.vcpkg_bin_dir + os.pathsep + self.perl_bin_dir + os.pathsep + env["PATH"]

            if new_console:
                creationflags = subprocess.CREATE_NEW_CONSOLE if sys.platform == "win32" else 0
                if sys.platform == "win32":
                    cmd_str = f'cmd /c ""{exe_path}" {" ".join(args)} & pause"'
                    proc = subprocess.Popen(cmd_str, cwd=cwd, creationflags=creationflags, env=env)
                else:
                    proc = subprocess.Popen([exe_path] + args, cwd=cwd, env=env)
            else:
                proc = subprocess.Popen(
                    [exe_path] + args,
                    cwd=cwd,
                    env=env,
                    creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0
                )

            self.processes[name] = proc
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

    def stop_all_processes(self):
        self.log("Stopping all managed processes...")
        for name in list(self.processes.keys()):
            self.stop_process(name)

    def force_kill_all(self):
        if messagebox.askyesno("Force Kill", "This will forcefully terminate all server processes (taskkill). Continue?"):
            self._kill_all_server_processes()

    def _kill_all_server_processes(self):
        """Kill all server processes without confirmation"""
        self.log("Force killing all server processes...")
        targets = ["loginserver.exe", "world.exe", "ucs.exe", "queryserv.exe", "eqlaunch.exe", "zone.exe", "shared_memory.exe"]
        for target in targets:
            try:
                subprocess.run(["taskkill", "/F", "/IM", target], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except Exception:
                pass

        # Clear internal state
        self.processes.clear()
        for name in self.proc_widgets:
            self.update_ui_state(name, False)
        self.log("All processes killed.")

    def restart_zones(self):
        """Quick restart: kill zone processes and restart eqlaunch for rapid testing"""
        def do_restart():
            self.log("Restarting zones...")
            # Kill zone and eqlaunch
            try:
                subprocess.run(["taskkill", "/F", "/IM", "zone.exe"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                subprocess.run(["taskkill", "/F", "/IM", "eqlaunch.exe"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except Exception:
                pass

            # Update UI state
            if "eqlaunch" in self.processes:
                del self.processes["eqlaunch"]
            if "zone" in self.processes:
                del self.processes["zone"]
            self.after(0, lambda: self.update_ui_state("zone", False) if "zone" in self.proc_widgets else None)
            self.after(0, lambda: self.update_ui_state("eqlaunch", False) if "eqlaunch" in self.proc_widgets else None)

            time.sleep(2)

            # Restart eqlaunch
            self._launch_process("eqlaunch")
            self.log("Zones restarting via eqlaunch...")

        threading.Thread(target=do_restart, daemon=True).start()

    def update_ui_state(self, name, is_running):
        widgets = self.proc_widgets[name]
        if is_running:
            widgets["status_lbl"].config(text="Running", foreground="green")
            widgets["start_btn"].config(state="disabled")
            widgets["stop_btn"].config(state="normal")
        else:
            widgets["status_lbl"].config(text="Stopped", foreground="red")
            widgets["start_btn"].config(state="normal")
            widgets["stop_btn"].config(state="disabled")

    def update_status_loop(self):
        for name, proc in list(self.processes.items()):
            if proc.poll() is not None:
                self.log(f"{name} exited with code {proc.returncode}")
                del self.processes[name]
                self.update_ui_state(name, False)
        self.after(1000, self.update_status_loop)

    def on_closing(self):
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
