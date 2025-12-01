
import subprocess
import time
import os

def start_process(executable, args=[], cwd='.', env=None, log_name=None):
    cmd = [executable] + args
    print(f"Starting: {' '.join(cmd)}")

    stdout_file = None
    stderr_file = None

    if log_name:
        log_dir = os.path.join(cwd, "logs", "startup")
        os.makedirs(log_dir, exist_ok=True)
        stdout_path = os.path.join(log_dir, f"{log_name}.log")
        stderr_path = os.path.join(log_dir, f"{log_name}.err")

        print(f"  -> Logging to {stdout_path}")
        stdout_file = open(stdout_path, "w")
        stderr_file = open(stderr_path, "w")

    try:
        subprocess.Popen(cmd, cwd=cwd, env=env, stdout=stdout_file, stderr=stderr_file)
    except Exception as e:
        print(f"Error starting {executable}: {e}")

def main():
    # Paths - using absolute paths or relative with ./
    # Assuming we are in the root of the repo
    bin_dir = os.path.join(os.getcwd(), "build", "bin", "RelWithDebInfo")

    # Update PATH to include bin_dir
    env = os.environ.copy()
    env["PATH"] = bin_dir + os.pathsep + env["PATH"]
    print(f"DEBUG: PATH set to: {env['PATH']}")

    # Helper to get full path
    def get_bin(name):
        return os.path.join(bin_dir, name)

    # Run Shared Memory to update items (Blocking, must finish before World starts)
    print("Updating Shared Memory (this may take a moment)...")
    try:
        # We use subprocess.run to wait for it to finish
        subprocess.run([get_bin("shared_memory.exe")], cwd=os.getcwd(), env=env)
    except Exception as e:
        print(f"Error running shared_memory: {e}")

    # Start Login Server
    start_process(get_bin("loginserver.exe"), env=env, log_name="loginserver")
    time.sleep(1)

    # Start World Server
    start_process(get_bin("world.exe"), env=env, log_name="world")
    time.sleep(5) # Give World more time to initialize

    # Start UCS
    start_process(get_bin("ucs.exe"), env=env, log_name="ucs")

    # Start QueryServ
    start_process(get_bin("queryserv.exe"), env=env, log_name="queryserv")

    # Start EQLaunch (Zone Manager)
    print(f"Starting eqlaunch...")
    # Run from root, relying on PATH to find zone.exe
    # Using simple name 'node_1' to avoid path/dot issues
    launcher_name = "node_1"

    # Use PowerShell to ensure environment and execution match manual success
    ps_command = f'$env:PATH = "{bin_dir};$env:PATH"; & "{get_bin("eqlaunch.exe")}" "{launcher_name}"'
    print(f"Executing PowerShell: {ps_command}")

    # Log eqlaunch output as well
    log_dir = os.path.join(os.getcwd(), "logs", "startup")
    os.makedirs(log_dir, exist_ok=True)
    eq_out = open(os.path.join(log_dir, "eqlaunch.log"), "w")
    eq_err = open(os.path.join(log_dir, "eqlaunch.err"), "w")

    subprocess.Popen(["pwsh", "-Command", ps_command], cwd=os.getcwd(), stdout=eq_out, stderr=eq_err)

    print("All servers started. Check logs/startup/ for output.")

if __name__ == "__main__":
    main()
