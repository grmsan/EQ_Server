import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, filedialog
import subprocess
import os
import sys
import time
import threading
import glob
import queue
from datetime import datetime

class ServerManagerApp(tk.Tk):
    def __init__(self):
        super().__init__()

        self.title("EQEmu Server Manager")
        self.geometry("1200x800")

        self.processes = {}
        self.output_queues = {}  # For capturing process output
        self.reader_threads = {}  # Threads reading process output

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
        self.vcpkg_bin_dir = os.path.join(os.getcwd(), "vcpkg", "vcpkg-export-x64", "installed", "x64-windows", "bin")

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
        self.consoles_tab = ttk.Frame(tab_control)
        self.logs_tab = ttk.Frame(tab_control)
        self.database_tab = ttk.Frame(tab_control)
        self.tools_tab = ttk.Frame(tab_control)

        tab_control.add(self.main_tab, text="Server Control")
        tab_control.add(self.consoles_tab, text="Console Output")
        tab_control.add(self.logs_tab, text="Log Files")
        tab_control.add(self.database_tab, text="Database Tools")
        tab_control.add(self.tools_tab, text="Scripts")
        tab_control.pack(expand=1, fill="both")

        # --- Main Tab ---
        self.create_main_tab()

        # --- Console Outputs Tab ---
        self.create_consoles_tab()

        # --- Logs Tab ---
        self.create_logs_tab()

        # --- Database Tab ---
        self.create_database_tab()

        # --- Tools Tab ---
        self.create_tools_tab()

    def create_main_tab(self):
        # Build Section
        build_frame = ttk.LabelFrame(self.main_tab, text="Build Server")
        build_frame.pack(fill="x", padx=10, pady=5)

        self.build_btn = ttk.Button(build_frame, text="Build (CMake)", command=self.run_build_thread)
        self.build_btn.pack(side="left", padx=5, pady=5)

        self.clean_btn = ttk.Button(build_frame, text="Clean Build", command=self.clean_build)
        self.clean_btn.pack(side="left", padx=5, pady=5)

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

            console_var = tk.BooleanVar(value=True)
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

    def create_consoles_tab(self):
        """Create embedded console outputs for each process"""
        console_notebook = ttk.Notebook(self.consoles_tab)
        console_notebook.pack(fill="both", expand=True, padx=5, pady=5)

        self.console_widgets = {}

        for info in self.process_info:
            name = info["name"]
            frame = ttk.Frame(console_notebook)
            console_notebook.add(frame, text=info["display"])

            # Console output
            console_text = scrolledtext.ScrolledText(frame, height=30, bg="black", fg="lightgreen",
                                                     font=("Consolas", 9))
            console_text.pack(fill="both", expand=True, padx=5, pady=5)

            # Control buttons
            btn_frame = ttk.Frame(frame)
            btn_frame.pack(fill="x", padx=5, pady=5)

            ttk.Button(btn_frame, text="Clear",
                      command=lambda ct=console_text: self.clear_console(ct)).pack(side="left", padx=2)
            ttk.Button(btn_frame, text="Copy All",
                      command=lambda ct=console_text: self.copy_console(ct)).pack(side="left", padx=2)
            ttk.Button(btn_frame, text="Save to File",
                      command=lambda ct=console_text: self.save_console(ct)).pack(side="left", padx=2)

            self.console_widgets[name] = console_text
            self.output_queues[name] = queue.Queue()

        # Start queue processors
        self.process_output_queues()

    def create_logs_tab(self):
        """Create log file viewer with auto-refresh"""
        log_frame = ttk.Frame(self.logs_tab)
        log_frame.pack(fill="both", expand=True, padx=10, pady=10)

        # Log file selector
        selector_frame = ttk.Frame(log_frame)
        selector_frame.pack(fill="x", pady=5)

        ttk.Label(selector_frame, text="Log File:").pack(side="left", padx=5)

        self.log_file_var = tk.StringVar()
        self.log_file_combo = ttk.Combobox(selector_frame, textvariable=self.log_file_var, width=50)
        self.log_file_combo.pack(side="left", padx=5, fill="x", expand=True)

        ttk.Button(selector_frame, text="Refresh List",
                  command=self.refresh_log_files).pack(side="left", padx=2)
        ttk.Button(selector_frame, text="Load",
                  command=self.load_log_file).pack(side="left", padx=2)

        self.auto_refresh_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(selector_frame, text="Auto-refresh (tail)",
                       variable=self.auto_refresh_var,
                       command=self.toggle_auto_refresh).pack(side="left", padx=5)

        # Log viewer
        self.log_viewer = scrolledtext.ScrolledText(log_frame, height=30, bg="white",
                                                     font=("Consolas", 9))
        self.log_viewer.pack(fill="both", expand=True, pady=5)

        # Populate log files
        self.refresh_log_files()

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

        # Database output
        self.db_output = scrolledtext.ScrolledText(db_frame, height=10, bg="white",
                                                   font=("Consolas", 9))
        self.db_output.pack(fill="both", expand=True, pady=5)

    def create_tools_tab(self):
        tools_frame = ttk.Frame(self.tools_tab)
        tools_frame.pack(fill="both", expand=True, padx=10, pady=10)

        ttk.Label(tools_frame, text="Available Python Scripts:").pack(anchor="w")

        self.scripts_listbox = tk.Listbox(tools_frame, selectmode="single")
        self.scripts_listbox.pack(fill="both", expand=True, pady=5)

        scrollbar = ttk.Scrollbar(self.scripts_listbox, orient="vertical", command=self.scripts_listbox.yview)
        scrollbar.pack(side="right", fill="y")
        self.scripts_listbox.config(yscrollcommand=scrollbar.set)

        # Populate scripts
        self.refresh_scripts()

        btn_frame = ttk.Frame(tools_frame)
        btn_frame.pack(fill="x", pady=5)

        ttk.Button(btn_frame, text="Refresh List", command=self.refresh_scripts).pack(side="left", padx=5)
        ttk.Button(btn_frame, text="Run Selected Script", command=self.run_selected_script).pack(side="left", padx=5)

    # ==================== Console Output Methods ====================

    def clear_console(self, console_text):
        """Clear console output"""
        console_text.delete(1.0, "end")

    def copy_console(self, console_text):
        """Copy console content to clipboard"""
        self.clipboard_clear()
        self.clipboard_append(console_text.get(1.0, "end"))
        self.log("Console content copied to clipboard")

    def save_console(self, console_text):
        """Save console content to file"""
        filename = filedialog.asksaveasfilename(
            defaultextension=".log",
            filetypes=[("Log files", "*.log"), ("Text files", "*.txt"), ("All files", "*.*")]
        )
        if filename:
            with open(filename, 'w') as f:
                f.write(console_text.get(1.0, "end"))
            self.log(f"Console saved to {filename}")

    def append_to_console(self, name, text):
        """Append text to a process console (thread-safe via queue)"""
        if name in self.output_queues:
            self.output_queues[name].put(text)

    def process_output_queues(self):
        """Process all output queues and update console widgets"""
        for name, q in self.output_queues.items():
            if name in self.console_widgets:
                console = self.console_widgets[name]
                try:
                    while True:
                        text = q.get_nowait()
                        console.insert("end", text)
                        console.see("end")
                except queue.Empty:
                    pass

        # Schedule next update
        self.after(100, self.process_output_queues)

    def read_process_output(self, name, process):
        """Read output from process in background thread"""
        try:
            for line in iter(process.stdout.readline, ''):
                if not line:
                    break
                self.append_to_console(name, line)
        except:
            pass

    # ==================== Log File Methods ====================

    def refresh_log_files(self):
        """Refresh the list of available log files"""
        log_files = []

        # Check logs directory
        if os.path.exists("logs"):
            for root, dirs, files in os.walk("logs"):
                for file in files:
                    if file.endswith(".log"):
                        log_files.append(os.path.join(root, file))

        self.log_file_combo['values'] = sorted(log_files)
        if log_files and not self.log_file_var.get():
            self.log_file_var.set(log_files[0])

    def load_log_file(self):
        """Load selected log file"""
        log_file = self.log_file_var.get()
        if not log_file or not os.path.exists(log_file):
            messagebox.showerror("Error", "Log file not found")
            return

        try:
            with open(log_file, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()

            self.log_viewer.delete(1.0, "end")
            self.log_viewer.insert("end", content)
            self.log_viewer.see("end")
            self.log(f"Loaded log: {log_file}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load log: {e}")

    def toggle_auto_refresh(self):
        """Toggle auto-refresh for log file"""
        if self.auto_refresh_var.get():
            self.auto_refresh_log()

    def auto_refresh_log(self):
        """Auto-refresh log file (tail mode)"""
        if not self.auto_refresh_var.get():
            return

        log_file = self.log_file_var.get()
        if log_file and os.path.exists(log_file):
            try:
                with open(log_file, 'r', encoding='utf-8', errors='ignore') as f:
                    # Get last 1000 lines
                    lines = f.readlines()[-1000:]
                    content = ''.join(lines)

                self.log_viewer.delete(1.0, "end")
                self.log_viewer.insert("end", content)
                self.log_viewer.see("end")
            except:
                pass

        # Schedule next refresh
        self.after(2000, self.auto_refresh_log)

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
                import json
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
                import json
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
        self.log_text.config(state="normal")
        self.log_text.insert("end", f"{message}\n")
        self.log_text.see("end")
        self.log_text.config(state="disabled")

    def clean_build(self):
        if messagebox.askyesno("Clean Build", "Are you sure you want to delete the build directory?"):
            import shutil
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
        self.build_status_lbl.config(text="Building...")

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
            cmd_build = ["cmake", "--build", "build", "--config", "RelWithDebInfo", "--parallel"]
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
        exe_path = os.path.join(self.bin_dir, "shared_memory.exe")
        if not os.path.exists(exe_path):
            self.log(f"Error: {exe_path} not found.")
            return

        env = os.environ.copy()
        env["PATH"] = self.bin_dir + os.pathsep + self.vcpkg_bin_dir + os.pathsep + env["PATH"]

        try:
            proc = subprocess.run([exe_path], cwd=os.getcwd(), env=env, capture_output=True, text=True)
            if proc.returncode == 0:
                self.log("Shared Memory updated successfully.")
            else:
                self.log(f"Shared Memory failed: {proc.stderr}")
        except Exception as e:
            self.log(f"Error running shared_memory: {e}")

    def start_all_sequence_thread(self):
        threading.Thread(target=self.start_all_sequence, daemon=True).start()

    def start_all_sequence(self):
        self.start_all_btn.config(state="disabled")

        # 1. Shared Memory
        self.run_shared_memory()

        # 2. Start processes in order
        start_order = ["loginserver", "world", "ucs", "queryserv", "eqlaunch"]

        for name in start_order:
            self.log(f"Starting {name}...")
            # We need to call start_process on the main thread because it updates UI
            self.after(0, lambda n=name: self.start_process(n))

            # Wait a bit
            if name == "world":
                time.sleep(5)
            else:
                time.sleep(1)

        self.log("All start commands issued.")
        self.start_all_btn.config(state="normal")

    def start_process(self, name):
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
            # Important: Add bin_dir to PATH so DLLs are found
            env["PATH"] = self.bin_dir + os.pathsep + self.vcpkg_bin_dir + os.pathsep + env["PATH"]

            if new_console:
                # Start with new console window
                creationflags = subprocess.CREATE_NEW_CONSOLE if sys.platform == "win32" else 0

                if sys.platform == "win32":
                    cmd_str = f'cmd /c ""{exe_path}" {" ".join(args)} & pause"'
                    proc = subprocess.Popen(cmd_str, cwd=cwd, creationflags=creationflags, env=env)
                else:
                    proc = subprocess.Popen([exe_path] + args, cwd=cwd, creationflags=creationflags, env=env)
            else:
                # Capture output for embedded console
                proc = subprocess.Popen(
                    [exe_path] + args,
                    cwd=cwd,
                    env=env,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    bufsize=1
                )

                # Start thread to read output
                reader = threading.Thread(target=self.read_process_output, args=(name, proc), daemon=True)
                reader.start()
                self.reader_threads[name] = reader

            self.processes[name] = proc
            self.update_ui_state(name, True)
            self.log(f"Started {name}")
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
            self.log("Force killing all server processes...")
            targets = ["loginserver.exe", "world.exe", "ucs.exe", "queryserv.exe", "eqlaunch.exe", "zone.exe", "shared_memory.exe"]
            for target in targets:
                try:
                    subprocess.run(["taskkill", "/F", "/IM", target], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    self.log(f"Sent kill signal to {target}")
                except Exception as e:
                    self.log(f"Error killing {target}: {e}")

            # Clear internal state
            self.processes.clear()
            for name in self.proc_widgets:
                self.update_ui_state(name, False)

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
