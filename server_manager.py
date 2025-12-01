import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import subprocess
import os
import sys
import time
import threading
import glob

class ServerManagerApp(tk.Tk):
    def __init__(self):
        super().__init__()

        self.title("EQEmu Server Manager")
        self.geometry("900x700")

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
        self.tools_tab = ttk.Frame(tab_control)

        tab_control.add(self.main_tab, text="Server Control")
        tab_control.add(self.tools_tab, text="Tools & Scripts")
        tab_control.pack(expand=1, fill="both")

        # --- Main Tab ---

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

        # --- Tools Tab ---
        self.create_tools_tab()

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

        creationflags = 0
        if sys.platform == "win32" and new_console:
            creationflags = subprocess.CREATE_NEW_CONSOLE

        try:
            cwd = os.getcwd()
            env = os.environ.copy()
            # Important: Add bin_dir to PATH so DLLs are found
            env["PATH"] = self.bin_dir + os.pathsep + self.vcpkg_bin_dir + os.pathsep + env["PATH"]

            if sys.platform == "win32" and new_console:
                cmd_str = f'cmd /c ""{exe_path}" {" ".join(args)} & pause"'
                proc = subprocess.Popen(cmd_str, cwd=cwd, creationflags=creationflags, env=env)
            else:
                proc = subprocess.Popen([exe_path] + args, cwd=cwd, creationflags=creationflags, env=env)

            self.processes[name] = proc
            self.update_ui_state(name, True)
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
