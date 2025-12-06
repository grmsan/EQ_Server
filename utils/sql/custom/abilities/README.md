# Custom Ability SQL

- `heroic_throw.sql`: Installs the Heroic Throw AA (spell + AA rows + cleanup). Run against your database then `#reload aa` to pick it up. Duration, AC debuff, and damage scaling are controlled by the spell entry plus code hooks in `zone/spell_effects.cpp` (special attack scaler).
