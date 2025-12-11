#!/usr/bin/env python3
import sys
import struct
from pathlib import Path

if len(sys.argv) < 2:
    print("Usage: analyze_profile_dump.py <path-to-dump>")
    sys.exit(2)

p = Path(sys.argv[1])
if not p.exists():
    print(f"File not found: {p}")
    sys.exit(2)

data = p.read_bytes()
size = len(data)
print(f"Analyzing: {p} ({size} bytes)")

# Helpers
def le16(v):
    return struct.pack('<H', v)

def le32(v):
    return struct.pack('<I', v)

# Values to find
vals = [6096, 8052]

matches = []
for v in vals:
    b16 = le16(v)
    b32 = le32(v)
    off16 = [i for i in range(size-1) if data[i:i+2] == b16]
    off32 = [i for i in range(size-3) if data[i:i+4] == b32]
    matches.append((v, off16, off32))

# Print matches with hex context
for v, off16, off32 in matches:
    print(f"\nValue {v} (le16 found {len(off16)} times, le32 found {len(off32)} times)")
    for o in off16[:20]:
        start = max(0, o-16)
        end = min(size, o+16)
        ctx = data[start:end]
        print(f"  le16 @ {o}: ..{start:06x}:{end:06x} len={len(ctx)} -> {ctx.hex()}\n    context_offset={start}")
    for o in off32[:20]:
        start = max(0, o-24)
        end = min(size, o+24)
        ctx = data[start:end]
        print(f"  le32 @ {o}: ..{start:06x}:{end:06x} len={len(ctx)} -> {ctx.hex()}\n    context_offset={start}")

# Also search for suspicious 0xFFFF0000 pattern that appeared in logs for mana_tot
pattern = bytes.fromhex('FFFF0000')
occ = [i for i in range(size-3) if data[i:i+4] == pattern]
print(f"\nPattern 0xFFFF0000 occurrences: {len(occ)}")
for i in occ[:10]:
    start = max(0, i-16)
    end = min(size, i+16)
    print(f"  @ {i}: {data[start:end].hex()}")

# Dump a small hexdump around the stat cluster earlier-found (approx offset 912..1000)
approx = 912
if size > approx:
    start = max(0, approx-32)
    end = min(size, approx+200)
    block = data[start:end]
    print(f"\nHex dump around approx offset {approx} (start={start} end={end}):\n{block.hex()}")

print('\nDone.')
