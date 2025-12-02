# Server Manager Features

## Overview
The `server_manager.py` GUI tool provides comprehensive management of your EQEmulator server with the following tabs:

## 🎮 Tab 1: Server Control
- **Build Section**: CMake configure and build with live output
- **Global Controls**:
  - Start All (Sequence) - Runs shared_memory, then starts servers in correct order
  - Stop All - Gracefully terminate all processes
  - Force Kill All - Taskkill all server processes
  - Run Shared Memory - Update shared memory separately
- **Process Management**: Individual control for each server process:
  - loginserver
  - world
  - ucs (chat)
  - queryserv
  - eqlaunch
  - zone
- **Live Status**: Real-time status indicators (Running/Stopped)
- **Console Checkbox**: Toggle between embedded output or separate console window
- **Manager Log**: Activity log showing build status, process starts/stops

## 📺 Tab 2: Console Output (NEW!)
Embedded terminal output for each server process with individual tabs:
- **Black terminal background** with green text (classic console look)
- **Real-time output** capture when "Console" checkbox is unchecked
- **Controls per console**:
  - Clear - Clear the console
  - Copy All - Copy content to clipboard
  - Save to File - Export console output to .log file
- **Persistent output** - Keeps history even after process stops

## 📄 Tab 3: Log Files (NEW!)
View and monitor server log files:
- **Log file dropdown** - Browse all .log files in logs/ directory
- **Refresh List** - Update available log files
- **Load** - Load selected log file
- **Auto-refresh (tail)** - Live tail mode, updates every 2 seconds
- **Last 1000 lines** shown when tailing
- **Consolas font** for easy reading

## 💾 Tab 4: Database Tools (NEW!)

### Character Management
- **Reset Password**: Change account password
  - Enter: Account name, New password
  - Calls `reset_password.py`

- **Set GM Level**: Change GM/admin level
  - Enter: Account name, GM level (0-255)
  - Calls `set_gm.py`

### Database Diagnostic Scripts
Two columns of quick-access buttons:
- **Check Scripts** (left): All `check_*.py` scripts
  - check_accounts.py
  - check_bazaar.py
  - check_db_content.py
  - check_ids.py
  - check_item.py
  - check_zone.py
  - etc.

- **Fix Scripts** (right): All `fix_*.py` scripts
  - fix_bazaar_spawns.py
  - fix_launcher.py
  - etc.

### Database Backup
- **Backup Database**:
  - Runs mysqldump automatically
  - Saves to `backups/eqemu_backup_YYYYMMDD_HHMMSS.sql`
  - Shows file size after completion
  - Reads credentials from `eqemu_config.json`

- **Restore Database**:
  - File browser to select backup
  - Confirmation dialog (warns about data loss)
  - Runs mysql restore
  - Shows progress and errors

- **Open Backup Folder**: Opens `backups/` in file explorer

### Database Output Window
- Shows results from all database operations
- Live output as scripts run
- Shows exit codes and errors

## 🔧 Tab 5: Scripts
Browse and run any Python script in your project:
- **Script list** - Shows all .py files (except server manager scripts)
- **Refresh List** - Update available scripts
- **Run Selected Script** - Opens new console window with script

## Usage Tips

### Starting the Server
1. Click "Start All (Sequence)" for automatic startup
   - OR -
2. Manual sequence:
   - Click "Run Shared Memory" first
   - Start "Login Server"
   - Start "World Server" (wait 5 seconds)
   - Start "UCS (Chat)"
   - Start "Query Server"
   - Start "EQ Launch"

### Viewing Output
- **Separate Console Window**: Check "Console" checkbox (default)
  - Good for normal operation
  - Each server has its own window

- **Embedded Console**: Uncheck "Console" checkbox
  - Output appears in "Console Output" tab
  - Good for debugging
  - Can save/copy output easily

### Monitoring Logs
1. Go to "Log Files" tab
2. Select log from dropdown (e.g., `logs\zone-tutorialb.log`)
3. Click "Load"
4. Enable "Auto-refresh (tail)" for live updates

### Database Backups
**Before major changes**:
1. Go to "Database Tools" tab
2. Click "Backup Database"
3. Wait for completion (shows file size)

**To restore**:
1. Click "Restore Database"
2. Select backup file from `backups/` folder
3. Confirm (this DELETES current data!)

### Running Diagnostic Scripts
1. Go to "Database Tools" tab
2. Click any check/fix script button
3. View output in the bottom window
4. Check exit code (0 = success)

## Requirements
- Python 3.7+
- tkinter (usually included with Python)
- mysqldump and mysql (for backup/restore)
- All scripts in root directory

## Launch Command
```bash
python server_manager.py
```

## Notes
- Window size: 1200x800 (larger for new features)
- All operations run in background threads (UI stays responsive)
- Process status updates every 1 second
- Log auto-refresh: every 2 seconds
- Console output buffering: 100ms updates
