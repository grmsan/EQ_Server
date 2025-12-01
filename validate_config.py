import json
import os
import subprocess
import sys
import time

def validate_json():
    config_file = "eqemu_config.json"
    if not os.path.exists(config_file):
        print(f"[FAIL] {config_file} not found.")
        return False

    try:
        with open(config_file, 'r') as f:
            data = json.load(f)
        print(f"[PASS] {config_file} is valid JSON.")

        # Check for critical keys
        if "server" not in data:
            print("[FAIL] 'server' key missing in config.")
            return False
        if "database" not in data["server"]:
            print("[FAIL] 'database' section missing in 'server'.")
            return False

        db_config = data["server"]["database"]
        print(f"[INFO] Configured Database: {db_config.get('username')}@{db_config.get('host')}:{db_config.get('port')}/{db_config.get('db')}")
        return True
    except json.JSONDecodeError as e:
        print(f"[FAIL] JSON Syntax Error: {e}")
        return False

def test_db_connection():
    # We will use shared_memory.exe to test the connection since it's the simplest binary
    # that connects to the DB.

    build_dir = os.path.join(os.getcwd(), "build")
    # Try to find the binary
    possible_paths = [
        os.path.join(build_dir, "bin", "RelWithDebInfo", "shared_memory.exe"),
        os.path.join(build_dir, "bin", "Debug", "shared_memory.exe"),
        os.path.join(build_dir, "bin", "Release", "shared_memory.exe"),
        os.path.join(build_dir, "bin", "shared_memory.exe"),
    ]

    exe_path = None
    for path in possible_paths:
        if os.path.exists(path):
            exe_path = path
            break

    if not exe_path:
        print("[FAIL] Could not find shared_memory.exe to test connection.")
        return

    print(f"[INFO] Using {os.path.basename(exe_path)} to test connection...")

    # Setup Environment (PATH)
    vcpkg_bin = os.path.join(os.getcwd(), "vcpkg", "vcpkg-export-x64", "installed", "x64-windows", "bin")
    env = os.environ.copy()
    if os.path.exists(vcpkg_bin):
        env["PATH"] = vcpkg_bin + os.pathsep + env["PATH"]

    # Run process
    try:
        # shared_memory usually runs and exits, or fails and exits.
        # We'll capture output.
        process = subprocess.Popen(
            [exe_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            cwd=os.getcwd(),
            env=env
        )

        # Read output for a few seconds or until exit
        start_time = time.time()
        output_lines = []

        while True:
            if process.poll() is not None:
                break

            line = process.stdout.readline()
            if line:
                output_lines.append(line.strip())
                # Check for success/failure markers in output
                if "Connected to database" in line:
                    print("[PASS] Successfully connected to the database!")
                    process.terminate()
                    return
                if "Access denied" in line or "Can't connect" in line:
                    print(f"[FAIL] Database connection failed: {line.strip()}")
                    process.terminate()
                    return

            if time.time() - start_time > 10:
                print("[WARN] Timeout waiting for shared_memory response.")
                process.terminate()
                break

        # If we exited, check return code
        if process.returncode != 0:
            print(f"[FAIL] Process exited with code {process.returncode}")
            # Print last few lines of output
            for line in output_lines[-5:]:
                print(f"  > {line}")
        else:
            # If it exited with 0, it probably worked (shared_memory often exits after loading)
            print("[PASS] Process finished successfully. Assuming connection worked.")

    except Exception as e:
        print(f"[FAIL] Error running test: {e}")

if __name__ == "__main__":
    print("--- Validating Configuration ---")
    if validate_json():
        print("\n--- Testing Database Connection ---")
        test_db_connection()
    print("\n--- Done ---")
