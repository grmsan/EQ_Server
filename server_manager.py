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
        self.geometry("800x600")

        self.processes = {}
        self.process_info = [
            {"name": "shared_memory", "display": "Shared Memory", "args": ""},
            {"name": "loginserver", "display": "Login Server", "args": ""},
            {"name": "world", "display": "World Server", "args": ""},
            {"name": "ucs", "display": "UCS (Chat)", "args": ""},
            {"name": "queryserv", "display": "Query Server", "args": ""},
            {"name": "eqlaunch", "display": "EQ Launch", "args": "zone"},
            {"name": "zone", "display": "Zone Server", "args": ""}, # Zone often needs args, '.' might be a placeholder or it might just boot a dynamic zone
        ]

        self.build_dir = os.path.join(os.getcwd(), "build")
        self.bin_dir = self.find_bin_dir()
        self.vcpkg_bin_dir = os.path.join(os.getcwd(), "vcpkg", "vcpkg-export-x64", "installed", "x64-windows", "bin")

        self.create_widgets()
        self.update_status_loop()

    def find_bin_dir(self):
        # Try to find where the binaries are
        possible_paths = [
            os.path.join(self.build_dir, "bin", "RelWithDebInfo"),
            os.path.join(self.build_dir, "bin", "Debug"),
            os.path.join(self.build_dir, "bin", "Release"),
            os.path.join(self.build_dir, "bin"),
        ]
        for path in possible_paths:
            if os.path.exists(path):
                return path
        return os.path.join(self.build_dir, "bin") # Default fallback

    def create_widgets(self):
        # Build Section
        build_frame = ttk.LabelFrame(self, text="Build Server")
        build_frame.pack(fill="x", padx=10, pady=5)

        self.build_btn = ttk.Button(build_frame, text="Build (CMake)", command=self.run_build_thread)
        self.build_btn.pack(side="left", padx=5, pady=5)

        self.clean_btn = ttk.Button(build_frame, text="Clean Build", command=self.clean_build)
        self.clean_btn.pack(side="left", padx=5, pady=5)

        self.build_status_lbl = ttk.Label(build_frame, text="Ready")
        self.build_status_lbl.pack(side="left", padx=5, pady=5)

        # Global Controls Section
        global_frame = ttk.LabelFrame(self, text="Global Controls")
        global_frame.pack(fill="x", padx=10, pady=5)

        self.start_all_btn = ttk.Button(global_frame, text="Start All", command=self.start_all_processes)
        self.start_all_btn.pack(side="left", padx=5, pady=5)

        self.stop_all_btn = ttk.Button(global_frame, text="Stop All", command=self.stop_all_processes)
        self.stop_all_btn.pack(side="left", padx=5, pady=5)

        # Processes Section
        proc_frame = ttk.LabelFrame(self, text="Server Processes")
        proc_frame.pack(fill="both", expand=True, padx=10, pady=5)

        # Header
        ttk.Label(proc_frame, text="Process").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        ttk.Label(proc_frame, text="Status").grid(row=0, column=1, padx=5, pady=5, sticky="w")
        ttk.Label(proc_frame, text="Arguments").grid(row=0, column=2, padx=5, pady=5, sticky="w")
        ttk.Label(proc_frame, text="Actions").grid(row=0, column=3, padx=5, pady=5, sticky="w")

        self.proc_widgets = {}

        for i, info in enumerate(self.process_info):
            row = i + 1
            name = info["name"]

            # Name
            ttk.Label(proc_frame, text=info["display"]).grid(row=row, column=0, padx=5, pady=5, sticky="w")

            # Status
            status_lbl = ttk.Label(proc_frame, text="Stopped", foreground="red")
            status_lbl.grid(row=row, column=1, padx=5, pady=5, sticky="w")

            # Args
            args_var = tk.StringVar(value=info["args"])
            args_entry = ttk.Entry(proc_frame, textvariable=args_var, width=20)
            args_entry.grid(row=row, column=2, padx=5, pady=5, sticky="w")

            # Buttons
            btn_frame = ttk.Frame(proc_frame)
            btn_frame.grid(row=row, column=3, padx=5, pady=5, sticky="w")

            start_btn = ttk.Button(btn_frame, text="Start", command=lambda n=name: self.start_process(n))
            start_btn.pack(side="left", padx=2)

            stop_btn = ttk.Button(btn_frame, text="Stop", state="disabled", command=lambda n=name: self.stop_process(n))
            stop_btn.pack(side="left", padx=2)

            # Console Checkbox
            console_var = tk.BooleanVar(value=True)
            console_chk = ttk.Checkbutton(btn_frame, text="New Console", variable=console_var)
            console_chk.pack(side="left", padx=5)

            self.proc_widgets[name] = {
                "status_lbl": status_lbl,
                "start_btn": start_btn,
                "stop_btn": stop_btn,
                "args_var": args_var,
                "console_var": console_var
            }

        # Log/Output Section
        log_frame = ttk.LabelFrame(self, text="Manager Log")
        log_frame.pack(fill="both", expand=True, padx=10, pady=5)

        self.log_text = scrolledtext.ScrolledText(log_frame, height=10, state="disabled")
        self.log_text.pack(fill="both", expand=True, padx=5, pady=5)

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

            # Configure
            self.log("Running CMake Configure...")
            cmd_config = ["cmake", "-S", ".", "-B", "build"]
            if sys.platform == "win32":
                # You might want to specify generator if needed, but default is usually fine (VS)
                pass

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

            # Build
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
                self.bin_dir = self.find_bin_dir() # Refresh bin dir

        except Exception as e:
            self.log(f"Build Error: {e}")
            self.build_status_lbl.config(text="Error")
        finally:
            self.build_btn.config(state="normal")
            self.clean_btn.config(state="normal")

    def start_process(self, name):
        if name in self.processes and self.processes[name].poll() is None:
            self.log(f"{name} is already running.")
            return

        exe_name = f"{name}.exe" if sys.platform == "win32" else name
        exe_path = os.path.join(self.bin_dir, exe_name)

        if not os.path.exists(exe_path):
            self.log(f"Executable not found: {exe_path}")
            # Try to find it in other folders if not found
            self.bin_dir = self.find_bin_dir()
            exe_path = os.path.join(self.bin_dir, exe_name)
            if not os.path.exists(exe_path):
                 messagebox.showerror("Error", f"Could not find {exe_name} in {self.bin_dir}. Please build the server first.")
                 return

        args = self.proc_widgets[name]["args_var"].get().split()
        new_console = self.proc_widgets[name]["console_var"].get()

        creationflags = 0
        if sys.platform == "win32" and new_console:
            creationflags = subprocess.CREATE_NEW_CONSOLE

        try:
            # Set working directory to bin_dir or current dir?
            # Usually servers need to run from the folder containing config files or have paths set correctly.
            # EQEmu servers often expect to run from the root or bin folder.
            # If config is in root, running from root is safer, but exe is in bin.
            # Let's try running from current working directory (root of repo) and calling the exe with full path.

            cwd = os.getcwd()

            # Add vcpkg bin dir to PATH for this process
            env = os.environ.copy()
            if os.path.exists(self.vcpkg_bin_dir):
                env["PATH"] = self.vcpkg_bin_dir + os.pathsep + env["PATH"]

            self.log(f"Starting {name}...")

            # Use cmd /c ... & pause to keep window open on error if new_console is used
            if sys.platform == "win32" and new_console:
                # We need to construct the command string carefully
                # cmd /c "path/to/exe arg1 arg2 & pause"
                # We use a string for Popen to avoid list2cmdline escaping quotes with backslashes which cmd doesn't like
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
                # Give it a moment
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    proc.kill()
                self.log(f"{name} stopped.")
            else:
                self.log(f"{name} was already stopped.")

            del self.processes[name]
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
        # Check status of processes
        for name, proc in list(self.processes.items()):
            if proc.poll() is not None:
                # Process has exited
                self.log(f"{name} exited with code {proc.returncode}")
                del self.processes[name]
                self.update_ui_state(name, False)

        self.after(1000, self.update_status_loop)

    def start_all_processes(self, index=0):
        if index == 0:
            self.log("Starting all processes sequence...")

        if index >= len(self.process_info):
            self.log("All start commands issued.")
            return

        info = self.process_info[index]
        name = info["name"]

        self.start_process(name)

        # Delay 1.5 seconds between starts
        self.after(1500, lambda: self.start_all_processes(index + 1))

    def stop_all_processes(self):
        self.log("Stopping all processes...")
        for info in reversed(self.process_info):
            name = info["name"]
            if name in self.processes:
                self.stop_process(name)

    def on_closing(self):
        if self.processes:
            if messagebox.askokcancel("Quit", "Running processes will be stopped. Do you want to quit?"):
                for name in list(self.processes.keys()):
                    self.stop_process(name)
                self.destroy()
        else:
            self.destroy()

if __name__ == "__main__":
    app = ServerManagerApp()
    app.protocol("WM_DELETE_WINDOW", app.on_closing)
    app.mainloop()
