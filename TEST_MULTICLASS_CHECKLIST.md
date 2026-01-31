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



## Tests Still Pending

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
