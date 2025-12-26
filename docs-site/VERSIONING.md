# Documentation Versioning System

This documentation site supports multiple versions, allowing users to view docs for different releases of LeviathanDM.

## Features

- **Version Selector**: Dropdown in the sidebar to switch between versions
- **Version-Specific Content**: Show/hide content based on version
- **Feature Badges**: Indicate when features were introduced
- **Deprecation Warnings**: Alert users about deprecated features

## Directory Structure

```
docs-site/
├── data/
│   └── versions.toml          # Version configuration
├── layouts/
│   ├── partials/
│   │   └── docs/
│   │       ├── version-selector.html   # Version dropdown component
│   │       └── inject/
│   │           └── menu-before.html    # Injects selector into sidebar
│   └── shortcodes/
│       ├── version-feature.html        # Version-specific content
│       └── version-warning.html        # Version warnings/badges
└── content/
    └── docs/                  # Latest/development docs
```

## Configuration

Edit `data/versions.toml` to manage versions:

```toml
[versions]
  [[versions.entries]]
    name = "latest"
    title = "Latest (Development)"
    path = "/docs"
    default = true
    badge = "Dev"
    badge_color = "#ff6b6b"
```

### Fields

- **name**: Internal version identifier
- **title**: Display name in selector
- **path**: URL path for this version
- **default**: Whether this is the default version
- **badge**: Optional badge text (e.g., "Stable", "Dev")
- **badge_color**: Badge background color

## Shortcodes

### Version Feature

Show content only for specific versions:

```markdown
{{</* version-feature since="v0.2.0" */>}}
This feature is only available in v0.2.0 and later.
{{</* /version-feature */>}}
```

### Version Warning

Display version-related warnings:

```markdown
{{</* version-warning type="new" version="v0.2.0" */>}}
The wallpaper system was introduced in v0.2.0.
{{</* /version-warning */>}}
```

**Types:**
- `new`: New feature
- `deprecated`: Deprecated feature
- `changed`: Changed behavior
- `info`: General information

## Creating Version-Specific Docs

### Option 1: Multiple Directories (Recommended)

Create separate content directories for each version:

```
content/
├── docs/              # Latest/development
├── v0.2.0/           # Stable release
└── v0.1.0/           # Previous version
```

Update `versions.toml` paths accordingly.

### Option 2: Conditional Content

Use shortcodes to show/hide content within the same files:

```markdown
# Feature X

{{</* version-feature since="v0.2.0" */>}}
## New in v0.2.0
This feature is enhanced in v0.2.0 with...
{{</* /version-feature */>}}

{{</* version-warning type="deprecated" version="v0.3.0" */>}}
**Deprecated:** This API will be removed in v0.3.0.
Use the new API instead.
{{</* /version-warning */>}}
```

## Workflow

### Adding a New Version

1. **Create content directory**:
   ```bash
   cp -r content/docs content/v0.2.0
   ```

2. **Update `versions.toml`**:
   ```toml
   [[versions.entries]]
     name = "v0.2.0"
     title = "v0.2.0 (Stable)"
     path = "/v0.2.0"
     badge = "Stable"
     badge_color = "#51cf66"
   ```

3. **Build and test**:
   ```bash
   hugo server
   ```

### Marking Features

When documenting a new feature:

```markdown
{{</* version-warning type="new" version="v0.2.0" */>}}
Wallpaper support added in v0.2.0.
{{</* /version-warning */>}}
```

When deprecating:

```markdown
{{</* version-warning type="deprecated" version="v0.3.0" */>}}
The old config format is deprecated and will be removed in v0.3.0.
{{</* /version-warning */>}}
```

## Styling

The version selector automatically adapts to light/dark themes. Customize colors in:

- `layouts/partials/docs/version-selector.html` (selector styling)
- `layouts/shortcodes/version-warning.html` (warning colors)
- `data/versions.toml` (badge colors)

## Examples

See `content/docs/features/wallpapers.md` for a complete example showing:

- Version warnings for new features
- Version-specific content blocks
- Proper documentation structure

## Future Enhancements

Potential improvements:

- [ ] Automatic version detection from git tags
- [ ] Version comparison/changelog view
- [ ] Search scoped to current version
- [ ] Version-specific navigation menus
- [ ] API for programmatic version switching

## Maintenance

- Keep `versions.toml` up to date with each release
- Archive old version docs when they're no longer supported
- Update "latest" docs for upcoming features
- Add version badges to changelog entries
