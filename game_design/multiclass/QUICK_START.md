# Multiclass Quick Start Guide

Get the multiclass system running locally in 15 minutes.

---

## Prerequisites

- [ ] EQEmu server source code (this repository)
- [ ] Visual Studio 2022 (Community edition is fine)
- [ ] CMake 3.20+
- [ ] MariaDB/MySQL database with EQEmu schema
- [ ] RoF2 client installed

---

## Step 1: Build the Server

```powershell
# Configure
cmake -S . -B build

# Build all targets
cmake --build build --config RelWithDebInfo --parallel

# Or build specific targets
cmake --build build --target zone --config RelWithDebInfo --parallel
cmake --build build --target world --config RelWithDebInfo --parallel
```

---

## Step 2: Build the Client DLL

```powershell
# Option A: Use VS Code task
# Run task "Build EQ Core DLL (Release)"

# Option B: Command line
cd extras/eq-core-dll-main
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
    eq-core-dll-visualstudio2022.sln /p:Configuration=Release /p:Platform=Win32
```

**Output:** `extras/eq-core-dll-main/bin/dinput8.dll`

---

## Step 3: Install the DLL

1. Copy `dinput8.dll` to your RoF2 client folder
   ```powershell
   Copy-Item "extras\eq-core-dll-main\bin\dinput8.dll" "C:\path\to\RoF2\dinput8.dll"
   ```

2. Verify it's not blocked:
   - Right-click `dinput8.dll` → Properties
   - If "Unblock" checkbox appears, check it and click OK

---

## Step 4: Enable Multiclass Rules

### Option A: Database (Recommended)
```sql
-- Insert or update rules
INSERT INTO rule_values (ruleset_id, rule_name, rule_value) VALUES
    (1, 'Custom:MulticlassingEnabled', 'true'),
    (1, 'Custom:MulticlassMaxClasses', '3'),
    (1, 'Custom:ServerAuthStats', 'true'),
    (1, 'Custom:UseDynamicAATimers', 'true')
ON DUPLICATE KEY UPDATE rule_value = VALUES(rule_value);
```

### Option B: In-Game (Requires GM)
```
#rules set Custom:MulticlassingEnabled true
#rules set Custom:MulticlassMaxClasses 3
#rules set Custom:ServerAuthStats true
#rules set Custom:UseDynamicAATimers true
```

---

## Step 5: Start the Server

```powershell
# Using server manager
python server_manager.py

# Or manually (order matters)
# 1. shared_memory
# 2. loginserver
# 3. world
# 4. ucs
# 5. queryserv
# 6. eqlaunch (starts zones)
```

---

## Step 6: Test Multiclass

1. **Start RoF2 client** (must restart after DLL install)

2. **Log in to a character**

3. **Check current state:**
   ```
   #multiclassdiag
   ```
   You should see your base class in the bitmask.

4. **Add a second class:**
   ```
   #addclass 4
   ```
   (Adds Ranger - class ID 4)

5. **Verify the change:**
   ```
   #multiclassdiag
   #addclass list
   ```

6. **Check client debug log:**
   - Open `<RoF2 folder>/dinput8_debug.log`
   - Look for: `EDGE_STAT multiclass classes_bitmask=0x0009`

---

## Class ID Reference

| ID | Class | Bit Value |
|----|-------|-----------|
| 1 | Warrior | 1 |
| 2 | Cleric | 2 |
| 3 | Paladin | 4 |
| 4 | Ranger | 8 |
| 5 | Shadow Knight | 16 |
| 6 | Druid | 32 |
| 7 | Monk | 64 |
| 8 | Bard | 128 |
| 9 | Rogue | 256 |
| 10 | Shaman | 512 |
| 11 | Necromancer | 1024 |
| 12 | Wizard | 2048 |
| 13 | Magician | 4096 |
| 14 | Enchanter | 8192 |
| 15 | Beastlord | 16384 |
| 16 | Berserker | 32768 |

---

## Common Test Scenarios

### Warrior + Ranger (Hybrid)
```
#addclass 4
```
- Spellbook should open
- Mana bar should appear
- Ranger spells should be usable

### Warrior + Mage (Caster)
```
#addclass 13
```
- Mage spells at spell vendor (with "Show Usable Items")
- Can scribe and memorize mage spells

### Remove a Class
```
#removeclass 4
```
- Class removed from bitmask
- Spells/skills soft-locked (still learned, can't use)

---

## Troubleshooting

### "Multiclassing is disabled" message
```
#rules set Custom:MulticlassingEnabled true
```

### DLL not working (no debug log)
1. Verify DLL is in correct folder
2. Verify DLL is not blocked (Properties → Unblock)
3. Restart client after installing DLL

### Class added but spells still filtered
1. Check `dinput8_debug.log` for EdgeStatLabel
2. Run `#multiclassdiag refresh` to force sync
3. Try `/camp` and relog

### Skills for new class not showing
1. Server seeds skills in `AddExtraClass()`
2. Check skill values with `#myskills`
3. DLL exposes skills with non-zero server values

---

## Diagnostic Commands

| Command | Purpose |
|---------|---------|
| `#multiclassdiag` | Full diagnostic dump |
| `#multiclassdiag refresh` | Force EdgeStatLabel send |
| `#addclass list` | Show all classes with status |
| `#mystats` | Shows classes_bitmask |
| `#myskills` | Shows all skill values |

---

## Next Steps

After basic setup works:

1. **Read the full implementation plan:**
   - [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md)

2. **Understand the DLL:**
   - [DLL_INTEGRATION.md](DLL_INTEGRATION.md)

3. **Track known issues:**
   - [TEST_TRACKER.md](TEST_TRACKER.md)

4. **Port more THJ features:**
   - [PORT_CHECKLIST.md](PORT_CHECKLIST.md)

---

*See [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for complete technical documentation.*
