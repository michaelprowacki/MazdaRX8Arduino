# Repository Structure Reorganization Plan

## Overview

This document describes the reorganized folder structure that reflects the **module consolidations** completed in Phases 1-3.

**Last Updated**: 2025-11-16
**Status**: Proposed structure to implement

---

## Problem Statement

The current folder structure doesn't reflect the consolidations documented in:
- `CONSOLIDATION_SUMMARY.md` (Wipers merged into ECU)
- `PHASE2_CODE_QUALITY.md` (Shared CAN library)
- `PHASE3_ARCHITECTURAL_UPGRADE.md` (EV_ECU refactoring, safety architecture)

**Issues**:
- ❌ `Wipers_Module/` still exists as top-level folder (functionality merged into ECU)
- ❌ No clear separation between **active** vs **archived** modules
- ❌ Modules scattered at root level (hard to navigate)
- ❌ No clear hierarchy (core vs optional vs deprecated)

---

## New Structure (Proposed)

```
MazdaRX8Arduino/
│
├── core/                           # ✅ ACTIVE CORE MODULES (Primary ECU functionality)
│   ├── ECU_Module/                 # Main ICE ECU (includes wipers via #define)
│   │   ├── RX8_CANBUS.ino
│   │   └── README.md
│   │
│   └── EV_ECU_Module/              # Electric vehicle ECU
│       ├── rx8can_v1.4__8khz_pwm_adjusted_micros_.ino
│       └── README.md
│
├── displays/                       # ✅ ACTIVE DISPLAY MODULES
│   ├── AC_Display_Module/          # Factory AC display controller
│   │   ├── src/
│   │   ├── include/
│   │   ├── lib/
│   │   ├── platformio.ini
│   │   ├── ESP32_MIGRATION.md      # Migration plan
│   │   └── README.md
│   │
│   ├── Aftermarket_Display_Module/ # OBD2 OLED displays
│   │   ├── additional_display/
│   │   └── README.md
│   │
│   └── Coolant_Monitor_Module/     # Dedicated temp/pressure monitor
│       ├── EMBEDDED/
│       ├── ELEC/
│       ├── MECHA/
│       └── README.md
│
├── specialized/                    # ✅ ACTIVE SPECIALIZED MODULES
│   ├── Sim_Racing_Module/          # Sim racing cluster driver
│   │   ├── src/
│   │   └── README.md
│   │
│   └── Dash_Controller_Module/     # Alternative dashboard (reference impl)
│       ├── main.cpp
│       ├── messages/
│       └── README.md
│
├── archived/                       # 🗄️ DEPRECATED MODULES (kept for reference)
│   ├── Wipers_Module/              # ⚠️ DEPRECATED - Now in ECU_Module
│   │   ├── firmware/
│   │   ├── hardware/
│   │   ├── DEPRECATED.md           # Why deprecated, where moved
│   │   └── README.md               # Original documentation
│   │
│   └── ESP8266_Companion/          # ⚠️ WILL BE DEPRECATED after ESP32 migration
│       ├── src/
│       ├── lib/
│       └── README.md
│
├── lib/                            # 📚 SHARED LIBRARIES (used by multiple modules)
│   └── RX8_CAN_Messages/
│       ├── RX8_CAN_Messages.h      # Shared CAN encoder/decoder
│       └── README.md               # Library documentation
│
├── examples/                       # 📋 EXAMPLE CODE
│   ├── CAN_Decoder_Example/
│   └── OLED_Display_Example/
│
├── tools/                          # 🔧 DEVELOPMENT TOOLS
│   ├── PCM_Analysis/               # ECU reverse engineering
│   ├── ECU_Definitions/            # Tuning definitions
│   └── README.md
│
├── docs/                           # 📖 DOCUMENTATION
│   ├── architecture/               # Architecture diagrams
│   ├── hardware/                   # Hardware guides
│   ├── integration/                # Multi-module integration
│   ├── related_projects.md         # RX8 ecosystem projects
│   └── PDFs/                       # Technical PDFs
│       ├── 08_Steering.pdf
│       ├── 13Electrical.pdf
│       └── RX8 CanBus and Throttle Explained.pdf
│
├── CLAUDE.md                       # 🤖 AI assistant guide (primary reference)
├── README.md                       # 👋 Project introduction
├── STRUCTURE.md                    # 📂 This file - structure documentation
├── CONSOLIDATION_SUMMARY.md        # 📊 Phase 1 consolidation details
├── PHASE2_CODE_QUALITY.md          # 📊 Phase 2 code quality improvements
├── PHASE3_ARCHITECTURAL_UPGRADE.md # 📊 Phase 3 safety architecture
├── AUTOMOTIVE_MCU_MIGRATION.md     # 🚗 Future automotive MCU migration
├── RX8_ECOSYSTEM.md                # 🌐 Related projects integration
└── CREDITS.md                      # 👏 Attribution and credits
```

---

## Module Categories Explained

### 🎯 Core Modules (`core/`)
**Purpose**: Critical ECU replacement functionality
**Criteria**:
- Replaces factory ECU
- Controls engine/motor operation
- Required for vehicle to function
- Safety-critical code

**Modules**:
- `ECU_Module/` - ICE engine ECU (includes wipers)
- `EV_ECU_Module/` - Electric motor ECU

### 📺 Display Modules (`displays/`)
**Purpose**: Visual displays and monitoring
**Criteria**:
- Provides driver interface
- Non-safety-critical
- Can be added/removed independently
- Display-focused functionality

**Modules**:
- `AC_Display_Module/` - Factory AC display
- `Aftermarket_Display_Module/` - Aftermarket OLED gauges
- `Coolant_Monitor_Module/` - Coolant temp/pressure display

### 🎮 Specialized Modules (`specialized/`)
**Purpose**: Non-core, specialized use cases
**Criteria**:
- Not part of normal vehicle operation
- Specific use case (sim racing, alternative implementations)
- Reference implementations
- Experimental or alternative approaches

**Modules**:
- `Sim_Racing_Module/` - PC sim racing integration
- `Dash_Controller_Module/` - Alternative dashboard protocol

### 🗄️ Archived Modules (`archived/`)
**Purpose**: Deprecated modules kept for reference
**Criteria**:
- Functionality moved to another module
- Superseded by better implementation
- Kept for historical reference or rollback
- Each contains `DEPRECATED.md` explaining status

**Modules**:
- `Wipers_Module/` - Merged into ECU_Module (Phase 1)
- `ESP8266_Companion/` - Will be deprecated after ESP32 migration

### 📚 Shared Libraries (`lib/`)
**Purpose**: Code used by multiple modules
**Criteria**:
- Used by 2+ modules
- Provides common functionality
- Single source of truth
- Reduces code duplication

**Libraries**:
- `RX8_CAN_Messages/` - CAN encoding/decoding (Phase 2)

---

## Migration Steps

### Phase 1: Create New Structure (This Session)
```bash
# 1. Create new directories
mkdir -p core displays specialized archived

# 2. Move active core modules
mv ECU_Module core/
mv EV_ECU_Module core/

# 3. Move display modules
mv AC_Display_Module displays/
mv Aftermarket_Display_Module displays/
mv Coolant_Monitor_Module displays/

# 4. Move specialized modules
mv Sim_Racing_Module specialized/
mv Dash_Controller_Module specialized/

# 5. Archive deprecated modules
mv Wipers_Module archived/

# 6. Move ESP8266 to archive when ESP32 is ready
# mv AC_Display_Module/ESP8266_Companion archived/

# 7. Rename Tools to tools (lowercase consistency)
mv Tools tools

# 8. Consolidate docs
mv Documentation/* docs/
rmdir Documentation
```

### Phase 2: Update Documentation References
```bash
# Update all relative paths in:
- README.md
- CLAUDE.md
- All module README files
- CONSOLIDATION_SUMMARY.md
- PHASE2_CODE_QUALITY.md
- PHASE3_ARCHITECTURAL_UPGRADE.md
```

### Phase 3: Create Deprecation Notices
```bash
# Create archived/Wipers_Module/DEPRECATED.md
# Explaining:
# - Why deprecated
# - Where functionality moved
# - How to use new implementation
# - When deprecated
```

### Phase 4: Update Build Paths
```bash
# Check and update:
- PlatformIO configurations (platformio.ini)
- Include paths (#include directives)
- Library paths
- Example references
```

---

## Benefits of New Structure

### 🎯 Clarity
- ✅ Clear separation of active vs archived
- ✅ Modules grouped by function (core, displays, specialized)
- ✅ Easy to find what you need
- ✅ New users understand organization immediately

### 📦 Modularity
- ✅ Core modules separate from optional features
- ✅ Displays grouped together (similar use cases)
- ✅ Specialized/experimental code isolated
- ✅ Archived modules don't clutter main view

### 🔧 Maintainability
- ✅ Deprecated modules clearly marked
- ✅ Shared libraries in one place
- ✅ Documentation consolidated
- ✅ Easier to navigate for contributors

### 📊 Scalability
- ✅ Easy to add new modules (clear categories)
- ✅ Easy to deprecate old modules (archive folder)
- ✅ Easy to extract shared code (lib folder)
- ✅ Room for future growth

---

## Backward Compatibility

### Git History
✅ **Preserved**: `git mv` preserves file history
✅ **Trackable**: All moves tracked in git log
✅ **Reversible**: Can checkout old structure from previous commits

### External Links
⚠️ **Breaking**: GitHub URLs will change
📝 **Solution**: Update README with migration notice
📝 **Solution**: Add redirects in deprecation notices

### User Workflows
⚠️ **Impact**: Users with cloned repos need to update paths
📝 **Solution**: Document migration in README
📝 **Solution**: Provide migration script

---

## Implementation Checklist

### Pre-Migration
- [x] Document current structure
- [x] Plan new structure
- [x] Identify all modules to move
- [x] Create this STRUCTURE.md document

### Migration
- [ ] Create new directories (`core/`, `displays/`, `specialized/`, `archived/`)
- [ ] Move core modules (ECU_Module, EV_ECU_Module)
- [ ] Move display modules (AC_Display, Aftermarket, Coolant)
- [ ] Move specialized modules (Sim_Racing, Dash_Controller)
- [ ] Move deprecated modules (Wipers_Module)
- [ ] Reorganize documentation (docs/)
- [ ] Rename Tools → tools

### Post-Migration
- [ ] Create DEPRECATED.md for archived modules
- [ ] Update README.md (new paths)
- [ ] Update CLAUDE.md (new structure section)
- [ ] Update all module READMEs (relative paths)
- [ ] Update CONSOLIDATION_SUMMARY.md
- [ ] Update PHASE2_CODE_QUALITY.md
- [ ] Update PHASE3_ARCHITECTURAL_UPGRADE.md
- [ ] Verify all PlatformIO configs
- [ ] Test build for at least one module per category
- [ ] Commit with clear message
- [ ] Push to branch

---

## Future Additions

### When ESP32 Migration Completes
```bash
# Move ESP8266 to archive
mv displays/AC_Display_Module/ESP8266_Companion archived/

# Create deprecation notice
cat > archived/ESP8266_Companion/DEPRECATED.md <<EOF
# ESP8266 Companion - DEPRECATED

**Status**: Deprecated as of [DATE]
**Reason**: Functionality merged into ESP32-based AC_Display_Module
**Replacement**: See displays/AC_Display_Module/ESP32_MIGRATION.md
EOF
```

### When Automotive MCU Migration Completes
```bash
# Create new automotive ECU module
mkdir -p core/Automotive_ECU_Module

# Archive Arduino-based ECU
mv core/ECU_Module archived/Arduino_ECU_Module_Legacy

# Update deprecation notice
```

---

## Questions & Answers

### Q: Why not delete deprecated modules?
**A**: Keep for reference, rollback, and historical context. Disk space is cheap, knowledge is expensive.

### Q: Why separate `core/` from `displays/`?
**A**: Safety-critical code should be clearly separated from UI code. Core modules control the engine, displays are non-critical.

### Q: What about the shared CAN library?
**A**: Lives in `lib/RX8_CAN_Messages/` at root level (accessible to all modules).

### Q: Can I still use the old paths?
**A**: Not after migration. Update your local clones to match new structure.

### Q: Will this break my build?
**A**: PlatformIO projects use relative paths. Update `platformio.ini` to match new structure.

---

## Related Documentation

- `CONSOLIDATION_SUMMARY.md` - Phase 1 hardware consolidation
- `PHASE2_CODE_QUALITY.md` - Phase 2 code quality (shared library)
- `PHASE3_ARCHITECTURAL_UPGRADE.md` - Phase 3 safety architecture
- `AUTOMOTIVE_MCU_MIGRATION.md` - Future automotive MCU migration
- `CLAUDE.md` - Main AI assistant guide (will be updated)
- `README.md` - Project introduction (will be updated)

---

*Last Updated: 2025-11-16*
*Status: Proposed - Awaiting Implementation*
*Created By: Claude (AI Assistant)*
