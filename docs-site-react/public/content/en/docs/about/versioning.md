---
title: "Documentation Versioning"
weight: 1
description: "How to use version-specific documentation features"
---

# Documentation Versioning

This documentation supports multiple versions of LeviathanDM. You can switch between versions using the version dropdown in the navigation bar.

## Available Versions

- **latest (development)**: Current development version with all the latest features
- **v0.1.0**: First stable release

## Version-Specific Content

### Version Banners

Pages that are specific to a particular version display a version banner at the top:

{{< version-banner version="v0.1.0" >}}

This helps you quickly identify which version of LeviathanDM the documentation applies to.

### New Feature Alerts

When a feature is newly introduced in a version, it's marked with a prominent alert:

{{< version-warning type="new" version="v0.1.0" >}}
This feature was introduced in v0.1.0.
{{< /version-warning >}}

### Changed Features

When a feature's behavior changes:

{{< version-warning type="changed" version="v0.2.0" >}}
This feature was modified in v0.2.0. See the migration guide for details.
{{< /version-warning >}}

### Deprecated Features

Features that will be removed in future versions:

{{< version-warning type="deprecated" version="v0.3.0" >}}
This feature is deprecated and will be removed in v0.4.0. Use the new API instead.
{{< /version-warning >}}

## Version-Conditional Content

Some content only applies to specific version ranges. This content is automatically shown or hidden based on the version you're viewing:

{{< version-feature since="v0.2.0" >}}
**New in v0.2.0**: This feature is only available starting from version 0.2.0.
{{< /version-feature >}}

{{< version-feature since="v0.1.0" until="v0.3.0" >}}
This content only applies to versions 0.1.0 through 0.2.x.
{{< /version-feature >}}

## Page Metadata

Each documentation page can specify which version it applies to using front matter:

```yaml
---
title: "My Feature"
version: "v0.1.0"
description: "Feature description"
---
```

This metadata is used by the version-conditional shortcodes to determine what content to show.

## Switching Versions

Use the version dropdown in the top navigation bar to switch between different versions of the documentation. Each version has its own URL:

- Latest: `https://leviathansystems.github.io/LeviathanDM/docs/`
- v0.1.0: `https://leviathansystems.github.io/LeviathanDM/v0.1.0/`

## Contributing

When adding new features to the documentation:

1. **Set the page version** in the front matter
2. **Add a version banner** at the top of the page if it's version-specific
3. **Mark new features** with `{{</* version-warning type="new" */>}}`
4. **Use conditional content** for features that only exist in certain versions
5. **Update deprecated features** with appropriate warnings

This helps users understand which features are available in their version of LeviathanDM.
