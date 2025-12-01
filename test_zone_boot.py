
import subprocess
import time
import os
import signal

def test_zone_boot():
    bin_path = os.path.join("build", "bin", "RelWithDebInfo", "zone.exe")
    cmd = [bin_path, "bazaar"]

    print(f"Starting: {' '.join(cmd)}")

    try:
        # Start the process
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )

        print("Process started. Monitoring for 10 seconds...")

        start_time = time.time()
        while time.time() - start_time < 10:
            # Check if process is still running
            if process.poll() is not None:
                print(f"Process exited early with code {process.returncode}")
                print("Output:")
                print(process.stdout.read())
                return

            # Read a bit of output if available (non-blocking read is hard in cross-platform python without threads,
            # so we'll just sleep and check poll)
            time.sleep(1)

        print("Process is still running after 10 seconds. It seems stable.")

        # Kill it
        print("Killing process...")
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()

        print("Process killed.")

    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    test_zone_boot()
