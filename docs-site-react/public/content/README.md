# Version-Based Documentation Structure

This directory contains documentation organized by version using a cascading system.

## How It Works

When requesting a documentation file for a specific version, the system searches through versions from **newest to oldest** until it finds the file:

1. Look in the current version folder (e.g., `v0.0.4/`)
2. If not found, look in previous version (e.g., `v0.0.3/`)
3. Continue cascading back to `v0.0.1/`
4. Finally, fall back to base `/content/` folder

## Version Organization

### v0.0.1 (Base Version)
Contains all initial documentation:
- Getting Started: building, configuration, keybindings
- Features: wallpapers, notifications, status-bar
- Development: architecture, plugins, contributing
- About: releases, versioning

### v0.0.2 (Added Layouts)
**New files:**
- `features/layouts.md` - Tiling layout documentation
- `features/_index.md` - Updated to include Layouts section

**Cascades from v0.0.1:** All other files

### v0.0.3 (Added Widget System)
**New files:**
- `development/widget-system.md` - Widget-based popover system
- `features/_index.md` - Updated to mention widget popovers

**Cascades from v0.0.2 → v0.0.1:** All other files

### v0.0.4 (Added Modal & Scroll)
**New files:**
- `features/_index.md` - Updated to include Modal system and ScrollView
- `about/releases.md` - Updated with v0.0.4 release notes

**Cascades from v0.0.3 → v0.0.2 → v0.0.1:** All other files

## Adding New Content

When adding content for a new version:

1. Create version folder: `mkdir -p v0.0.X/en/docs/...`
2. Add **ONLY** files that are new or changed in that version
3. Update navigation.ts to add `since: "v0.0.X"` to new menu items
4. The cascade system automatically handles the rest!

## Benefits

- **Space Efficient**: Only store diffs between versions
- **Easy Maintenance**: Update a file in one place for all future versions
- **Clear History**: See exactly what changed in each version
- **Automatic Fallback**: No need to duplicate unchanged files
