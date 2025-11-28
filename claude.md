# Claude Code Session Context

## Project Overview

This is **The Powder Toy**, an open-source physics sandbox game that simulates various materials, liquids, gases, and their interactions. The simulation runs particle-based physics where each particle has properties like temperature, pressure, velocity, and can undergo phase transitions.

## Work Completed

### 1. Multithreading Support

Added parallel processing capabilities to improve simulation performance:
- Element category batching for parallel updates
- Early-exit optimization for particle processing
- Particle compaction improvements
- Spatial chunking for better CPU utilization
- SIMD vectorization and cache prefetching optimizations

### 2. Max FPS Setting

Added manual FPS limit options in the game settings:
- Presets: 60, 120, 144, 165 FPS
- Custom/Unlimited option

### 3. Conservation of Matter Fixes

Fixed physics bugs where particles would disappear instead of transitioning to appropriate phases. The root cause was elements using `PT_NONE` or missing transitions (`ITH`/`ITL`) for temperature-based phase changes.

#### New Elements Created

| Element | File | Description | Phase Cycle |
|---------|------|-------------|-------------|
| **N2** (NTRG) | `N2.cpp` | Nitrogen gas | LN2 ↔ N2 at 77K |
| **OICE** | `OICE.cpp` | Solid oxygen | OICE ↔ LO2 at 54.36K |
| **LH2** | `LH2.cpp` | Liquid hydrogen | LH2 ↔ H2 at 20.28K |
| **MRCV** | `MRCV.cpp` | Mercury vapor | MERC ↔ MRCV at 629.88K |
| **DICE** | `DICE.cpp` | Deuterium ice | DICE ↔ DEUT at 276.97K |
| **DTRV** | `DTRV.cpp` | Deuterium vapor | DEUT ↔ DTRV at 374.5K |
| **RKVP** | `RKVP.cpp` | Rock vapor | LAVA ↔ RKVP at 3500K |
| **AICD** | `AICD.cpp` | Frozen acid | AICD ↔ ACID at 233K |
| **ACDV** | `ACDV.cpp` | Acid vapor | ACID ↔ ACDV at 610K |
| **FGEL** | `FGEL.cpp` | Frozen gel | FGEL ↔ GEL at 263K |
| **ISVP** | `ISVP.cpp` | Isotope-Z vapor | ISOZ ↔ ISVP at 400K |

#### Elements Modified

| Element | Change |
|---------|--------|
| **LNTG** | High temp transition → N2 (was PT_NONE) |
| **LO2** | Low temp transition → OICE |
| **H2** | Low temp transition → LH2 |
| **MERC** | High temp transition → MRCV |
| **DEUT** | Added DICE and DTRV transitions |
| **FRZW** | High temp transition → WTRV |
| **LAVA** | High temp transition → RKVP at 3500K |
| **ACID** | Added AICD (freeze) and ACDV (boil) transitions |
| **GEL** | Added FGEL (freeze) and WTRV (decompose) transitions |
| **ISOZ** | High temp transition → ISVP |

## Key Technical Concepts

### Phase Transitions in TPT

Elements define temperature transitions using:
```cpp
LowTemperature = 273.0f;           // Temperature threshold (Kelvin)
LowTemperatureTransition = PT_ICE; // Element to transition to

HighTemperature = 373.0f;
HighTemperatureTransition = PT_WTRV;
```

Special values:
- `ITL` / `ITH` - Infinite low/high temperature (no transition)
- `NT` - No transition
- `PT_NONE` - Destroys the particle (causes conservation violations!)
- `ST` - Special transition (handled in code)

### Adding New Elements

1. Create `ELEM.cpp` in `src/simulation/elements/`
2. Add element name to `src/simulation/elements/meson.build`
3. Element ID is auto-generated in `ElementNumbers.h` during build
4. Use `PT_ELEM` to reference in other element transitions

### Build System

```bash
meson compile -C build
```

## File Locations

- Elements: `src/simulation/elements/*.cpp`
- Element registry: `src/simulation/elements/meson.build`
- Generated IDs: `build/generated/ElementNumbers.h`
- Simulation core: `src/simulation/Simulation.cpp`

## Branch

All work is on: `claude/add-multithreading-support-01USeXch8kmh1ywRyuWot2zE`
