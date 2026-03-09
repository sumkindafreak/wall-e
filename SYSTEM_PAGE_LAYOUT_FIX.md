# ✅ FIXED: System Page Layout (CYD 320x240)

## Screen Layout with Correct Coordinates

```
┌────────────────────────────────────────────┐  Y=0
│ System                                     │  Top Bar (30px)
├────────────────────────────────────────────┤  Y=30
│                                            │
│  Battery: 12.4 V          (Y=56)          │
│  Current: 2.3 A           (Y=80)          │
│  Temp: 28 C               (Y=104)         │
│                                            │
│  ┌─────────┐ ┌─────────┐ ┌────┐          │  Y=130 (Row 2)
│  │Profiles │ │ Servos  │ │ WP │          │  Height=32
│  └─────────┘ └─────────┘ └────┘          │  Y=162
│                                            │
│  ┌──────────────────────────────────┐     │  Y=168 (Row 3)
│  │  AUTO MODE: DISABLED (or ENABLED)│     │  Height=26
│  └──────────────────────────────────┘     │  Y=194
│                                            │
├────────────────────────────────────────────┤  Y=200 (Bottom Bar)
│              ┌──────┐                      │
│              │ Back │                      │
│              └──────┘                      │
└────────────────────────────────────────────┘  Y=240
```

## Button Coordinates

### Row 2 (Y=130-162):
- **Profiles**: X=16-116, Y=130-162 (100x32)
- **Servos**: X=130-230, Y=130-162 (100x32)
- **WP** (Waypoint Info): X=244-304, Y=130-162 (60x32)

### Row 3 (Y=168-194):
- **AUTO MODE** (Wide Toggle): X=16-230, Y=168-194 (214x26)
  - GREEN when enabled
  - RED when disabled
  - Full-width button for easy tapping

### Bottom Bar (Y=200-240):
- **Back**: X=110-210, Y=204-236 (centered)

## Changes Made:

1. ✅ **Moved AUTO MODE button UP** to Y=168 (was overlapping bottom bar)
2. ✅ **Made button WIDER** (214px wide instead of two separate buttons)
3. ✅ **Reduced height to 26px** (fits better, more visible)
4. ✅ **Added small WP button** (far right, Row 2) for waypoint info
5. ✅ **Updated touch zones** to match new coordinates

## Why You Couldn't See It Before:

The AUTO MODE buttons were at Y=168-200, which overlapped with the bottom bar area (Y=200-240). The Back button was covering them!

**Screen height**: 240px
**Bottom bar starts**: Y=200
**Previous button position**: Y=168-200 ❌ (partially hidden)
**New button position**: Y=168-194 ✅ (fully visible)

## How It Looks Now:

```
System Page:
┌──────────────────────────────────────┐
│ System                               │
├──────────────────────────────────────┤
│ Battery: 12.4 V                      │
│ Current: 2.3 A                       │
│ Temp: 28 C                           │
│                                      │
│ [Profiles] [Servos] [WP]            │ ← Row 2
│                                      │
│ [AUTO MODE: DISABLED]                │ ← Row 3 (Wide button!)
│                                      │
│          [Back]                      │ ← Bottom
└──────────────────────────────────────┘
```

## Color States:

- **RED** = "AUTO MODE: DISABLED" (default)
- **GREEN** = "AUTO MODE: ENABLED" (when active)

Text changes dynamically based on telemetry from Base.

## Upload This Fix:

1. Save all files
2. Open Arduino IDE
3. Click Upload
4. Wait for completion
5. Reset CYD

You should now see the AUTO MODE button clearly on Row 3!
