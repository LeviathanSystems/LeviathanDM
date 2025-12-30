# Documentation Site Version Management Guide

This guide explains how to update the Hugo documentation site when releasing new versions of LeviathanDM.

## Table of Contents
1. [Quick Checklist](#quick-checklist)
2. [Version Release Process](#version-release-process)
3. [Adding Version-Specific Features](#adding-version-specific-features)
4. [Using Version Shortcodes](#using-version-shortcodes)
5. [File Locations Reference](#file-locations-reference)
6. [Common Patterns](#common-patterns)
7. [Testing](#testing)

---

## Quick Checklist

When releasing a new version (e.g., v0.0.4), update these files in order:

- [ ] `/VERSION` - Update version number
- [ ] `/docs-site/hugo.toml` - Update `version` parameter
- [ ] `/docs-site/data/versions.toml` - Add new version entry and new features
- [ ] Create `/CHANGELOG-X.X.X.md` - Technical changelog
- [ ] Create `/docs-site/content/en/docs/about/releases.md` - Add release notes section
- [ ] Update `/docs-site/content/en/_index.md` - Add links to new features (if applicable)
- [ ] Update feature index pages - Add version warnings for new features
- [ ] Create feature documentation pages - For any new major features
- [ ] Test with `hugo server`
- [ ] Commit and push

---

## Version Release Process

### Step 1: Update Core Version Files

#### 1.1 Update `/VERSION`
```bash
echo "0.0.4" > VERSION
```

#### 1.2 Update `/docs-site/hugo.toml`
```toml
[params]
  # Version information
  version_menu = "Versions"
  version = "0.0.4"  # ← Change this
  url_latest_version = "https://leviathansystems.github.io/LeviathanDM/docs/"
```

### Step 2: Update Version Registry

#### 2.1 Update `/docs-site/data/versions.toml`

Add new version at the top and move previous "Latest" to "Previous":

```toml
[versions]
  # Current stable version
  [[versions.entries]]
    name = "v0.0.4"                    # New version
    title = "v0.0.4 (Latest)"
    path = "/docs"
    default = true
    badge = "Latest"
    badge_color = "#51cf66"
    
  # Previous versions
  [[versions.entries]]
    name = "v0.0.3"                    # Was latest, now previous
    title = "v0.0.3"
    path = "/v0.0.3"                   # Changed path
    badge = "Previous"
    badge_color = "#868e96"
    
  [[versions.entries]]
    name = "v0.0.2"
    title = "v0.0.2"
    path = "/v0.0.2"
    badge = "Previous"
    badge_color = "#868e96"
```

#### 2.2 Add New Features to `versions.toml`

Add any new features at the top of the `[features]` section:

```toml
[features]
  # New features in v0.0.4
  [features.your-new-feature]
    since = "v0.0.4"
    description = "Brief description of the feature"
    
  [features.another-new-feature]
    since = "v0.0.4"
    description = "Another feature description"
    
  # Existing features below...
  [features.widget-based-popovers]
    since = "v0.0.3"
    description = "Composable widget system for building popovers"
```

**Feature Naming Convention:**
- Use kebab-case (lowercase with hyphens)
- Be descriptive but concise
- Examples: `widget-based-popovers`, `window-decorations`, `ipopover-provider`

### Step 3: Create Documentation

#### 3.1 Create Repository Changelog `/CHANGELOG-X.X.X.md`

Technical changelog for developers:

```markdown
# Changelog - Version 0.0.4

**Release Date:** January 15, 2026

## Major Features
- Feature 1 description
- Feature 2 description

## Bug Fixes
- Bug fix 1
- Bug fix 2

## Breaking Changes
- Breaking change description

## Files Modified
- `path/to/file1.cpp`
- `path/to/file2.hpp`
```

#### 3.2 Update `/docs-site/content/en/docs/about/releases.md`

Add a new section at the top:

```markdown
## Version 0.0.4 - Feature Name

{{< version-banner version="v0.0.4" >}}

**Release Date:** January 15, 2026

### 🎉 Major Features

#### Your New Feature

Description of the feature...

**Example:**
\`\`\`cpp
// Code example
\`\`\`

### 🐛 Bug Fixes
- Fixed issue X
- Fixed issue Y

---

## Version 0.0.3 - Widget-Based Popovers
...previous content...
```

#### 3.3 Create Feature Documentation Page (if needed)

For major features, create a new page in `/docs-site/content/en/docs/`:

**For User Features:** `/docs-site/content/en/docs/features/your-feature.md`
**For Developer Features:** `/docs-site/content/en/docs/development/your-feature.md`

```markdown
---
title: "Your Feature Name"
weight: 4
version: "v0.0.4"
description: "Brief description"
---

# Your Feature Name

{{< version-warning type="new" version="v0.0.4" >}}
This feature was introduced in v0.0.4.
{{< /version-warning >}}

## Overview
...

## Usage
...

## Examples
...
```

### Step 4: Update Navigation and Index Pages

#### 4.1 Update Feature Index

If adding a user-facing feature, update `/docs-site/content/en/docs/features/_index.md`:

```markdown
### [Your Feature](your-feature)
{{< version-warning type="new" version="v0.0.4" >}}
Brief description of the feature.
{{< /version-warning >}}
```

#### 4.2 Update Main Index (Optional)

If the feature is significant, add to `/docs-site/content/en/_index.md`:

```markdown
### Features
- [Your New Feature]({{< relref "/docs/features/your-feature" >}}) 🆕
```

Or in the development section:

```markdown
### Development
- [Your New API]({{< relref "/docs/development/your-api" >}}) 🆕
```

---

## Adding Version-Specific Features

### When to Add a Feature Entry

Add to `versions.toml` when:
- ✅ New public API is introduced
- ✅ New user-facing feature is added
- ✅ New configuration option is available
- ✅ New plugin capability is added
- ✅ Major architecture change affects developers

Don't add when:
- ❌ Bug fixes (mention in release notes only)
- ❌ Internal refactoring (unless it affects API)
- ❌ Minor improvements to existing features

### Feature Entry Format

```toml
[features.feature-name]
  since = "vX.X.X"
  description = "One-line description (under 80 chars)"
```

**Description Guidelines:**
- Start with action verb when possible
- Be specific about what it does
- Keep under 80 characters
- Don't include implementation details

**Examples:**
```toml
# Good
[features.widget-based-popovers]
  since = "v0.0.3"
  description = "Composable widget system for building popovers with VBox, HBox, and Label"

# Bad (too vague)
[features.widget-based-popovers]
  since = "v0.0.3"
  description = "New widget system"
  
# Bad (too technical)
[features.widget-based-popovers]
  since = "v0.0.3"
  description = "IPopoverProvider interface with recursive RecalculateContainerTree implementation"
```

---

## Using Version Shortcodes

Hugo provides several shortcodes for version-specific content:

### 1. Version Banner

Shows at the top of version-specific pages:

```markdown
{{< version-banner version="v0.0.4" >}}
```

### 2. Version Warnings

#### New Feature
```markdown
{{< version-warning type="new" version="v0.0.4" >}}
This feature was introduced in v0.0.4.
{{< /version-warning >}}
```

#### Changed Feature
```markdown
{{< version-warning type="changed" version="v0.0.4" >}}
This behavior changed in v0.0.4. See migration guide below.
{{< /version-warning >}}
```

#### Deprecated Feature
```markdown
{{< version-warning type="deprecated" version="v0.0.4" >}}
This feature is deprecated and will be removed in v0.1.0. Use XYZ instead.
{{< /version-warning >}}
```

### 3. Version-Conditional Content

Show content only in specific versions:

```markdown
{{< version-feature since="v0.0.4" >}}
**New in v0.0.4**: This content only appears in v0.0.4+
{{< /version-feature >}}
```

Show content for a version range:

```markdown
{{< version-feature since="v0.0.2" until="v0.0.4" >}}
This content only appears in v0.0.2 through v0.0.3
{{< /version-feature >}}
```

### 4. Hints and Alerts

```markdown
{{< hint info >}}
**Info**: Important information
{{< /hint >}}

{{< hint warning >}}
**Warning**: Caution needed
{{< /hint >}}

{{< hint danger >}}
**Danger**: Breaking change or critical issue
{{< /hint >}}
```

---

## File Locations Reference

### Core Version Files
```
/VERSION                                    # Single version number
/CHANGELOG-X.X.X.md                        # Technical changelog
/docs-site/hugo.toml                       # Hugo config with version
/docs-site/data/versions.toml              # Version registry
```

### Documentation Content
```
/docs-site/content/en/
├── _index.md                              # Home page
├── docs/
│   ├── about/
│   │   ├── releases.md                    # Release notes (user-friendly)
│   │   └── versioning.md                  # Versioning guide
│   ├── features/                          # User-facing features
│   │   ├── _index.md                      # Features index
│   │   ├── window-decorations.md
│   │   ├── wallpapers.md
│   │   └── your-feature.md                # New feature page
│   ├── development/                       # Developer documentation
│   │   ├── architecture.md
│   │   ├── widget-system.md
│   │   ├── plugins.md
│   │   └── your-api.md                    # New API documentation
│   └── getting-started/
│       ├── building.md
│       └── configuration.md
```

---

## Common Patterns

### Pattern 1: New User Feature with Documentation

**Scenario:** Adding wallpaper animations in v0.0.5

1. **Update versions.toml:**
```toml
[features.wallpaper-animations]
  since = "v0.0.5"
  description = "Animated wallpaper support with GIF and video formats"
```

2. **Create feature page:** `/docs-site/content/en/docs/features/wallpaper-animations.md`

3. **Update features index:**
```markdown
### [Wallpaper Animations](wallpaper-animations)
{{< version-warning type="new" version="v0.0.5" >}}
Animate your desktop with GIF and video wallpapers.
{{< /version-warning >}}
```

### Pattern 2: Breaking API Change

**Scenario:** Config format changed in v0.0.6

1. **Update versions.toml:**
```toml
[features.yaml-config-v2]
  since = "v0.0.6"
  description = "New YAML configuration format with schema validation"
```

2. **Mark old feature as deprecated in docs:**
```markdown
## Configuration (v0.0.5 and earlier)

{{< version-warning type="deprecated" version="v0.0.6" >}}
This configuration format is deprecated. See the [migration guide](#migration) below.
{{< /version-warning >}}
```

3. **Add migration guide:**
```markdown
## Migration from v0.0.5 to v0.0.6

{{< version-warning type="changed" version="v0.0.6" >}}
Configuration format changed in v0.0.6.
{{< /version-warning >}}

**Old format (v0.0.5):**
\`\`\`yaml
old: syntax
\`\`\`

**New format (v0.0.6+):**
\`\`\`yaml
new: syntax
\`\`\`
```

### Pattern 3: Developer API Addition

**Scenario:** New plugin API in v0.0.7

1. **Update versions.toml:**
```toml
[features.plugin-lifecycle-hooks]
  since = "v0.0.7"
  description = "OnInit, OnShutdown, and OnConfigReload lifecycle hooks for plugins"
```

2. **Create API documentation:** `/docs-site/content/en/docs/development/plugin-lifecycle.md`

3. **Update plugin guide:**
```markdown
## Lifecycle Hooks

{{< version-warning type="new" version="v0.0.7" >}}
Lifecycle hooks were introduced in v0.0.7.
{{< /version-warning >}}

Your plugin can implement lifecycle hooks:
\`\`\`cpp
class MyPlugin : public Plugin {
  void OnInit() override;      // v0.0.7+
  void OnShutdown() override;  // v0.0.7+
};
\`\`\`
```

---

## Testing

### Before Committing

1. **Test Hugo build:**
```bash
cd docs-site
hugo server --buildDrafts --buildFuture
```

2. **Check for errors:**
   - No build errors
   - No missing shortcodes
   - All internal links work (click through navigation)

3. **Verify version display:**
   - Version dropdown shows new version as "Latest"
   - Old versions show as "Previous"
   - Version badges display correctly

4. **Test version-specific content:**
   - New feature warnings appear
   - Version-conditional content shows/hides correctly
   - Release notes formatted properly

5. **Check navigation:**
   - New pages appear in sidebar
   - Feature index updated
   - Main page links work

### Manual Testing Checklist

```markdown
- [ ] Hugo builds without errors
- [ ] Version dropdown shows vX.X.X as latest
- [ ] Release notes page has new version section
- [ ] New feature pages render correctly
- [ ] Version warnings display properly
- [ ] All internal links work
- [ ] Code examples are properly formatted
- [ ] Navigation sidebar includes new pages
- [ ] Search finds new content
- [ ] No broken shortcodes or missing templates
```

---

## Troubleshooting

### "template for shortcode X not found"

**Cause:** Using a non-existent Hugo shortcode.

**Solution:** Check available shortcodes in the theme. Common working shortcodes:
- `{{< hint >}}`
- `{{< columns >}}`
- `{{< relref >}}`

Don't use (doesn't exist in Docsy/Book theme):
- `{{< badge >}}` ❌ Use emoji instead: 🆕
- `{{< label >}}` ❌

### Version dropdown not updating

**Cause:** Cached Hugo data or incorrect TOML syntax.

**Solution:**
1. Stop hugo server (Ctrl+C)
2. Clear Hugo cache: `hugo --cleanDestinationDir`
3. Check TOML syntax: `toml-test versions.toml` or validate online
4. Restart: `hugo server`

### Internal links broken

**Cause:** Incorrect `relref` path or file moved.

**Solution:**
```markdown
# Wrong
{{< relref "feature.md" >}}

# Correct (absolute from content/)
{{< relref "/docs/features/feature.md" >}}
```

### Version comparison not working

**Cause:** Version string format mismatch.

**Solution:** Always use format `vX.X.X` (with leading 'v'):
- ✅ `v0.0.3`
- ❌ `0.0.3`
- ❌ `v0.0.3-alpha`

---

## Quick Reference

### Emoji for "New" badges
Use `🆕` instead of `{{< badge >}}` shortcode.

### Version String Format
Always: `vX.X.X` (e.g., `v0.0.4`)

### Feature Name Format
Always: `kebab-case` (e.g., `widget-based-popovers`)

### File Naming
- Markdown files: `kebab-case.md`
- Page titles in frontmatter: "Title Case"

### Shortcode Reference
| Shortcode | Usage |
|-----------|-------|
| `version-banner` | Top of version-specific pages |
| `version-warning` | Mark new/changed/deprecated features |
| `version-feature` | Conditional content by version |
| `hint` | Info/warning/danger boxes |
| `relref` | Internal links |
| `columns` | Multi-column layout |

---

## Examples from Version 0.0.3 Release

See actual implementation:
- `/CHANGELOG-0.0.3.md` - Technical changelog
- `/docs-site/content/en/docs/about/releases.md` - Release notes section
- `/docs-site/content/en/docs/development/widget-system.md` - New feature documentation
- `/docs-site/data/versions.toml` - Version and feature entries

These serve as templates for future releases.

---

**Last Updated:** December 30, 2025 (v0.0.3)
**Maintainer:** LeviathanDM Documentation Team
