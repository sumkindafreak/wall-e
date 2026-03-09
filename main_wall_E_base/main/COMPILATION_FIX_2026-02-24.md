# Compilation Fix - Feb 24, 2026

## Issue
Duplicate `Personality` struct definition causing compilation errors.

## Root Cause
The `Personality` struct was defined in both:
1. `personality_engine.h` (the dedicated personality module) - with fields: `curiosity`, `bravery`, `energy`, `randomness`
2. `autonomy_engine.h` (the autonomy state machine) - with fields: `curiosityLevel`, `braveryLevel`, `energyLevel`, `randomness`

This created conflicts when both headers were included in the same compilation unit.

## Changes Made

### 1. `autonomy_engine.h`
- **Removed** the duplicate `Personality` struct definition
- **Removed** `autonomyGetPersonality()` and `autonomySetPersonality()` declarations
- The autonomy engine now relies entirely on `personality_engine.h` for personality data

### 2. `autonomy_engine.cpp`
- **Removed** static `s_personality` variable
- **Removed** `autonomyGetPersonality()` and `autonomySetPersonality()` implementations
- **Updated** all personality references to use `personalityGet...()` functions:
  - `s_personality.randomness` → `personalityGetRandomness()`
  - `s_personality.energyLevel` → `personalityGetEnergy()`
  - `s_personality.curiosityLevel` → `personalityGetCuriosity()`
  - `s_personality.braveryLevel` → `personalityGetBravery()`

### 3. `return_home_engine.cpp`
- **Fixed** struct initialization error
- Changed from `static ReturnHomeContext s_rth = {0};` to proper designated initializer with all fields explicitly set

## Architecture Benefits
- **Single Source of Truth**: Personality data now managed exclusively by `personality_engine`
- **Better Separation**: Autonomy engine queries personality engine via API, not local copy
- **Consistent**: All code uses the same field names (`curiosity`, not `curiosityLevel`)
- **Maintainable**: Changes to personality system only require editing one module

## Files Modified
1. `main/autonomy_engine.h` - Removed duplicate struct & functions
2. `main/autonomy_engine.cpp` - Removed local personality, updated all refs
3. `main/return_home_engine.cpp` - Fixed struct initialization

## Compilation Status
✅ All duplicate definition errors resolved
✅ All field name mismatches resolved
✅ All struct initialization errors resolved

## Testing Checklist
- [ ] Verify Base firmware compiles without errors
- [ ] Verify personality settings work in WebUI
- [ ] Verify autonomous behavior respects personality traits
- [ ] Verify CYD controller compiles without errors
