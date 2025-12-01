
import subprocess
import os

def stop_servers():
    # List of process names to kill
    processes = [
        "loginserver.exe",
        "world.exe",
        "ucs.exe",
        "queryserv.exe",
        "eqlaunch.exe",
        "zone.exe"
    ]

    print("Stopping servers...")
    for proc in processes:
        try:
            # /F forces termination, /IM specifies image name
            subprocess.run(["taskkill", "/F", "/IM", proc],
                         stdout=subprocess.DEVNULL,
                         stderr=subprocess.DEVNULL)
            print(f"Terminated {proc}")
        except Exception as e:
            print(f"Error stopping {proc}: {e}")

    print("All servers stopped.")

if __name__ == "__main__":
    stop_servers()
