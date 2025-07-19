# Blackhole Documentation Structure

## Documentation Organization

All documentation is organized in subdirectories. The project root contains only:
- **CLAUDE.md** - AI assistant reference (what information is where)
- **GDD.md** - Complete game design document

## Folder Structure

### 📁 SessionReports/
Contains all improvement reports by date:
- **IMPROVEMENT_REPORT_2025_07_16.md** - Wall run improvements and agile enemy enhancements
- **IMPROVEMENT_REPORT_2025_07_17_COMPLETE.md** - StatusEffect system, enemy AI, building mechanics, door system

### 📁 TechnicalGuides/
Contains technical documentation for specific systems:
- **STATUSEFFECT_IMPROVEMENTS_2025_07_17.md** - Detailed StatusEffectComponent documentation
- **AUTOMATIC_DOOR_GUIDE.md** - Setup and usage guide for automatic doors
- **ENEMY_AI_BEHAVIORS.md** - Enemy types, behaviors, and state machines

### 📁 Archives/
Contains historical documentation that has been superseded:
- **IMPROVEMENT_REPORT_2025_07_13.md** - Early session improvements
- **IMPROVEMENT_REPORT_2025_07_16_ENEMIES.md** - Enemy-specific changes (merged)
- **ENEMY_AI_IMPROVEMENTS_2025_07_17.md** - AI improvements (merged)
- **AI_STATE_MACHINE_DOCUMENTATION.md** - Enemy state machine architecture
- **ENEMY_DATA_TABLE_GUIDE.md** - Data table configuration for enemies

## Quick Reference

1. **For AI guidance**: CLAUDE.md in root directory
2. **For game design**: GDD.md in root directory
3. **For recent changes**: Check SessionReports folder
4. **For system documentation**: Check TechnicalGuides folder
5. **For historical reference**: Check Archives folder