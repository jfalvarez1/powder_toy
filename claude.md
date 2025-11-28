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

### 4. Initialization Order Bug Fix

Fixed a startup crash caused by incorrect initialization order in `PowderToy.cpp`:

**Problem**: The `Simulation` constructor (used by `SaveRenderer`) now calls `InitElementCategories()` and `InitElementTransitionTemps()` which require `SimulationData` to be initialized. However, `SaveRenderer` was being created before `SimulationData`.

**Solution**: Moved `SimulationData` creation to occur before `SaveRenderer` in the initialization sequence.

```cpp
// SimulationData must be created before SaveRenderer since Simulation constructor needs element data
explicitSingletons->simulationData = std::make_unique<SimulationData>();

explicitSingletons->saveRenderer = std::make_unique<SaveRenderer>();
```

### 5. Fluid Physics Optimizations

Optimized water, lava, and other fluid simulations for better performance:

#### WATR.cpp Optimizations
- Converted if-else chain to switch statement for faster type dispatch
- Pre-compute velocity magnitude once for erosion checks (avoids repeated `fabs` calls)
- Only check erosion condition when water is moving fast enough

#### MovementPhase Liquid Optimizations (`Simulation.cpp`)
- Cache element collision factor to avoid repeated `elements[t].Collision` lookups
- Pre-compute absolute velocity values once per particle
- Cache bmap cell coordinates to reduce division operations in spreading loops
- Cache bmap values to avoid redundant array lookups in horizontal/vertical spreading

These optimizations reduce CPU overhead in the hot path for liquid particle physics, particularly for large bodies of water or lava.

### 6. New Interactive Elements

Added 8 new elements with unique behaviors and interactions:

| Element | Menu | Description |
|---------|------|-------------|
| **CRYO** | Liquid | Cryogenic liquid (-253°C) that flash-freezes liquids and gases on contact |
| **BLTZ** | Elec | Ball lightning - floats erratically, zaps conductors, explodes in water |
| **SLIM** | Liquid | Sticky slime that traps/slows particles, feeds on plants, grows from yeast |
| **XTLG** | Special | Living crystal that grows by consuming minerals, changes color, emits light |
| **STCL** | Gas | Storm cloud that produces rain, snow, and lightning strikes |
| **NANO** | Special | Nanobots that consume metal, self-replicate, controlled by electricity |
| **FRFL** | Liquid | Ferrofluid (magnetic liquid) attracted to iron and electrical fields |
| **PLZM** | Nuclear | Plasma ball - extremely hot, emits radiation, triggers fusion reactions |

#### Key Interactions

**CRYO (Cryogenic Liquid)**
- Freezes WATR/DSTW/SLTW → ICEI instantly
- Freezes WTRV/FOG → SNOW
- Liquefies O2 → LO2, N2 → LNTG
- Solidifies LAVA rapidly
- Extinguishes FIRE/PLSM

**BLTZ (Ball Lightning)**
- Attracted to metals (METL, IRON, BMTL)
- Sparks conductors and creates THDR arcs
- Explodes on contact with water (steam + thunder)
- Emits PHOT and ELEC particles
- Ignites flammable materials

**SLIM (Slime)**
- Slows down any particle it touches
- Dissolves in water, killed by salt
- Grows when touching ACID or YEST
- Feeds on PLNT (grows more slime)
- Slowly damages stickmen

**XTLG (Living Crystal)**
- Consumes SAND/STNE/GLAS/QRTZ/DMND to grow
- Spreads to empty spaces when mature
- Color cycles through spectrum
- Emits colored PHOT when fully grown
- Dissolved by water, destroyed by acid

**STCL (Storm Cloud)**
- Absorbs WTRV and WATR to grow
- Produces rain (WATR) when mature
- Builds electrical charge over time
- Strikes metals with THDR lightning
- Very cold clouds produce SNOW instead

**NANO (Nanobots)**
- Dormant without power, activated by SPRK
- Consumes metals (METL, IRON, TTAN, GOLD, etc.)
- Self-replicates when enough metal consumed
- Shares energy between nearby nanobots
- Damaged by water, destroyed by acid and heat

**FRFL (Ferrofluid)**
- Strongly attracted to IRON
- Attracted to all metals and SPRK
- Electromagnetic effect from electricity
- Freezes → IRON, boils → BRMT
- Can discharge static electricity

**PLZM (Plasma Ball)**
- Extremely hot (8000°C), floats
- Emits PHOT (colored light) and NEUT
- Vaporizes water instantly
- Triggers fusion with DEUT
- Triggers fission in URAN/PLUT
- Melts metals rapidly

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
