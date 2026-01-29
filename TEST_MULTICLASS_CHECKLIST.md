# Multiclass Testing Checklist

This file tracks multiclass features as they are implemented. Remove items when verified working, or add notes if they fail.

---

## Setup

### Test Character
- Use `#addclass <class_id>` to add a second class (e.g., `#addclass 6` for Druid)
- Use `#addclass list` to see all classes with status (X = enabled)
- Use `#multiclassdiag` for full diagnostic dump
- Class IDs: 1=WAR, 2=CLR, 3=PAL, 4=RNG, 5=SHD, 6=DRU, 7=MNK, 8=BRD, 9=ROG, 10=SHM, 11=NEC, 12=WIZ, 13=MAG, 14=ENC, 15=BST, 16=BER

### Log Locations
| Log Type | Location | How to Enable |
|----------|----------|---------------|
| Server multiclass debug | `logs/zone/Custom_MulticlassDebug.log` | Already enabled via LogSys |
| DLL debug log | `<RoF2 client folder>/dinput8_debug.log` | Automatic when DLL loaded |
| Server zone log | `logs/zone/zone_*.log` | Standard zone logging |

### Debug Commands
- `#addclass list` - Shows all 16 classes with X for enabled ones
- `#multiclassdiag` - Full multiclass state dump (bitmask, EdgeStat, etc.)
- `#multiclassdiag refresh` - Force resend EdgeStatLabel to client DLL
- `#logs set Custom:MulticlassDebug 3` - Enable multiclass debug logging (level 3 = detail)

---

## Active Bugs to Fix

### 1. Spell Vendor "Show Usable Items" Not Filtering (CLIENT-SIDE BUG)

**Status:** FAIL - Needs Investigation

**Symptoms:**
- "Show Usable Items" checkbox doesn't filter anything - ALL spells shown
- When inspecting a spell item, it shows ALL player's classes with their levels
- Example: Warrior/Mage/Druid with a level 3 Druid spell shows:
  - Warrior: level 3 (WRONG - should be 255)
  - Druid: level 3 (correct)
  - Mage: level 255 (correct)
- The base class (Warrior) is incorrectly getting assigned the same level as an added class

**Root Cause Analysis:**
This appears to be in how the server populates the item's class level data when sending to client. The item struct has per-class level requirements that the client uses for filtering.

**Files to Investigate:**
- `zone/inventory.cpp` - Item packet construction
- `zone/tradeskills.cpp` - May have similar item handling
- Search for where `classes` array is populated in item packets

**Debug Steps:**
1. Check `dinput8_debug.log` for GetSpellLevelNeeded calls
2. Run `#multiclassdiag refresh` to ensure EdgeStatLabel was sent
3. Look for item packet construction code

---

### 2. Merchant Class Filtering (SERVER-SIDE - PARTIAL FIX)

**Status:** FAIL - Related to Bug #1

**What We Changed:**
- `zone/client_process.cpp` ~line 929: Changed merchant class filtering to use `GetClassesBits()` instead of single class

**Current Issue:**
The server-side filtering now correctly allows items for all classes, but the item's per-class level data being sent to the client is wrong (see Bug #1). The client shows level 255 for unusable classes but also incorrectly shows non-255 for the base class on spells it can't use.

---

## Tests Still Pending

### Spell Scribe Button (NEEDS TEST)

**Issue:** Scribe button may be greyed out for spells of added classes.

**How to Test:**
1. As Warrior/Mage multiclass
2. Put a Magician spell scroll on cursor
3. Open spellbook
4. **Expected:** Scribe button should be clickable
5. **Possible Bug:** Button greyed out, cannot scribe

---

### Skill Training for Added Classes (NEEDS TEST)

**How to Test:**
1. As Warrior/Mage multiclass
2. Visit a Magician guildmaster
3. Try to train Magician skills (Conjuration, etc.)
4. **Expected:** Should be able to train skills
5. **Possible Bug:** Guildmaster says "I cannot teach you"

---

### AA Eligibility for Added Classes (NEEDS TEST)

**How to Test:**
1. As Warrior/Mage multiclass with AA points
2. Open AA window
3. Look for Magician-only AAs
4. **Expected:** Should see and be able to purchase Magician AAs

---

### Equipment Usability (NEEDS TEST)

**How to Test:**
1. As Warrior/Mage multiclass
2. Try to equip a Magician-only item (INT caster robe, etc.)
3. **Expected:** Should be able to equip

---

## Verified Working (PASSED)

- ✅ **Spell Memorization** - Server correctly allows memorizing spells for any class in bitmask
- ✅ **Spell Fizzle Rate** - `CheckFizzle()` uses best spell level across all classes

---

## Bugs Fixed

### Item Display Spell Level Bug (FIXED)
**Bug:** When viewing a Druid spell as Warrior/Mage/Druid, Warrior showed level 3 instead of 255.
**Cause:** DLL's `GetSpellLevelNeeded` detour was aggregating (returning best multiclass level) for ALL contexts, not just merchant filtering.
**Fix:** Changed `allowAggregate` to ONLY be true when `in_merchant` window context is active.
**File:** `extras/eq-core-dll-main/src/eqgame.cpp` lines ~2161-2180

---

## Quick Test Commands

```
#addclass 13              # Add Magician class
#addclass list            # See all classes (X = enabled)
#multiclassdiag           # Full diagnostic dump
#multiclassdiag refresh   # Force resend EdgeStatLabel to DLL
```

---

## Files Modified So Far

| File | Change | Line(s) |
|------|--------|---------|
| `zone/client_process.cpp` | Merchant class filtering | ~929 |
| `zone/spells.cpp` | GetBestSpellLevelForMulticlass() helper | ~1051-1115 |
| `zone/spells.cpp` | CheckFizzle() multiclass support | ~1120-1170 |
| `extras/eq-core-dll-main/src/eqgame.cpp` | Fixed allowAggregate to require in_merchant context | ~2161-2180 |
