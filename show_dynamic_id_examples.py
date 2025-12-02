#!/usr/bin/env python3
"""
Dynamic Item ID Format Examples
Shows how the encoding/decoding works for various items and levels
"""

def generate_dynamic_id(base_id, level):
    """Encode base_id + level into dynamic ID"""
    if level == 0:
        return base_id

    prefix = 5
    encoded_level = min(level, 99999)
    encoded_base = base_id % 1000000

    # Format: 5LLLLLIIIIII
    dynamic_id = (prefix * 100000000) + (encoded_level * 1000) + encoded_base
    return dynamic_id

def decode_dynamic_id(item_id):
    """Decode dynamic ID back to base_id + level"""
    if item_id < 500000000:
        return item_id, 0

    level = (item_id // 1000) % 100000
    base_id = item_id % 1000000

    return base_id, level

# Test various items
print("=" * 80)
print("DYNAMIC ITEM ID ENCODING EXAMPLES")
print("=" * 80)
print()

items = [
    (1001, "Cloth Cap"),
    (1, "Cloth Armor"),
    (12345, "Random Item"),
    (999999, "Max Item ID"),
    (50000, "Mid-Range Item"),
]

levels = [0, 1, 10, 50, 100, 500, 1000, 5000, 10000, 99999]

print("Format: 5LLLLLIIIIII")
print("  5      = Prefix (always 5 for dynamic items)")
print("  LLLLL  = Level (5 digits, supports 0-99,999)")
print("  IIIIII = Base Item ID (6 digits, supports 0-999,999)")
print()

for base_id, name in items:
    print(f"\n{name} (Base ID: {base_id})")
    print("-" * 80)

    for level in [0, 1, 10, 50, 100, 500]:
        dynamic_id = generate_dynamic_id(base_id, level)
        decoded_base, decoded_level = decode_dynamic_id(dynamic_id)

        # Format the dynamic ID to show digit groupings
        if level == 0:
            id_str = f"{dynamic_id:,}"
            breakdown = f"(base item, no encoding)"
        else:
            id_str = f"{dynamic_id:,}"
            # Show the digit breakdown
            prefix = dynamic_id // 100000000
            level_part = (dynamic_id // 1000) % 100000
            base_part = dynamic_id % 1000000
            breakdown = f"({prefix}|{level_part:05d}|{base_part:06d})"

        verify = "✓" if decoded_base == base_id and decoded_level == level else "✗"

        print(f"  +{level:5d} → {id_str:>13} {breakdown:20s} → decode: {decoded_base:6d} +{decoded_level:5d} {verify}")

print("\n" + "=" * 80)
print("EDGE CASES & LIMITS")
print("=" * 80)

print("\nSmallest dynamic item:")
base_id, level = 1, 1
dynamic_id = generate_dynamic_id(base_id, level)
print(f"  Base ID: {base_id}, Level: {level}")
print(f"  Dynamic ID: {dynamic_id:,}")
print(f"  Breakdown: 5|{((dynamic_id // 1000) % 100000):05d}|{(dynamic_id % 1000000):06d}")

print("\nLargest dynamic item:")
base_id, level = 999999, 99999
dynamic_id = generate_dynamic_id(base_id, level)
print(f"  Base ID: {base_id}, Level: {level}")
print(f"  Dynamic ID: {dynamic_id:,}")
print(f"  Breakdown: 5|{((dynamic_id // 1000) % 100000):05d}|{(dynamic_id % 1000000):06d}")

print("\nMySQL INT(11) limit:")
max_int = 2**31 - 1
print(f"  Max signed INT: {max_int:,}")
print(f"  Max dynamic ID: {dynamic_id:,}")
print(f"  Fits in INT(11): {dynamic_id <= max_int} ✓")

print("\n" + "=" * 80)
print("YOUR EXAMPLE FROM THE LOGS")
print("=" * 80)
print("\nUpgrading Cloth Cap (1001) by 1 level:")
base_id, level = 1001, 1
dynamic_id = generate_dynamic_id(base_id, level)
decoded_base, decoded_level = decode_dynamic_id(dynamic_id)

print(f"  Input: base_id={base_id}, level={level}")
print(f"  Encoded: {dynamic_id} ({dynamic_id:,})")
print(f"  Binary breakdown:")
print(f"    Prefix:  {dynamic_id // 100000000}")
print(f"    Level:   {(dynamic_id // 1000) % 100000}")
print(f"    Base ID: {dynamic_id % 1000000}")
print(f"  Decoded: base_id={decoded_base}, level={decoded_level}")
print(f"  Match: {decoded_base == base_id and decoded_level == level} ✓")

print("\nBUT with the OLD buggy decoding (% 1000 instead of % 1000000):")
buggy_base = dynamic_id % 1000
print(f"  Buggy decode: base_id={buggy_base} ✗ (should be {base_id})")
print(f"  This caused: 'Base item [1] not found' instead of looking up item 1001")
