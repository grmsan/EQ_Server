# QoL Test Tracker

**Status**: Active Testing
**Tracker Area**: QoL
**Tracker State**: Active
**Last Updated**: 2026-02-27

## Purpose

Tracks quality-of-life systems that are not tightly scoped to one feature domain.

## Session Header

**Date**: __________
**Tester**: __________
**Build/Branch**: __________

---

## 1) Travel QoL

### [QOL-01] Bazaar and Back AA (Instance-Aware Return)

**Goal**: Verify THJ-style `Bazaar and Back` AA is auto-granted, ports to Bazaar, and returns to saved location including instances across relog.
**Legacy ID**: `B-09`
**Steps**:

1. Log in on a fresh character and open AA window; confirm `Bazaar and Back` is present.
2. In a normal zone, activate `Bazaar and Back` and verify you port to Bazaar.
3. Activate it again in Bazaar and verify you return to your original coordinates.
4. Repeat from an instance (save location in instance, port to Bazaar, camp/relog in Bazaar, then use AA again).
5. Trigger AA twice quickly and verify reuse lockout message/cooldown behavior (~60s).
**Expected**: AA is auto-granted to all characters, uses 60s cooldown, and return location persists across relog with instance-aware restore.
**Status**: [ ] Pass  [ ] Fail
**Notes**: ______________________________
