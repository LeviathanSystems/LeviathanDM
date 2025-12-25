# Release Process

This document describes how to create a new release of LeviathanDM.

## Overview

LeviathanDM uses semantic versioning (MAJOR.MINOR.PATCH) and automated GitHub Actions for building releases and deploying documentation.

## Automated Workflows

### 1. CI Build (`.github/workflows/ci.yml`)
- **Triggers**: Push to master/develop, Pull Requests
- **Purpose**: Continuous integration checks
- **Actions**:
  - Build compositor and plugins
  - Verify binaries
  - Build documentation
  - Code quality checks

### 2. Release Build (`.github/workflows/release.yml`)
- **Triggers**: Version tags (v1.0.0, v1.2.3, etc.)
- **Purpose**: Create GitHub releases with binaries
- **Actions**:
  - Build release binaries
  - Create distributable archive
  - Generate checksums
  - Upload to GitHub Releases
  - Create release notes

### 3. Documentation Deploy (`.github/workflows/hugo.yml`)
- **Triggers**: Version tags, manual dispatch
- **Purpose**: Deploy documentation to GitHub Pages
- **Actions**:
  - Build Hugo site
  - Deploy to https://leviathansystems.github.io/LeviathanDM/

## Creating a Release

### Prerequisites

1. All changes merged to `master` branch
2. CI builds passing
3. Documentation updated
4. CHANGELOG updated (if exists)

### Steps

#### 1. Update VERSION file

```bash
echo "1.2.3" > VERSION
git add VERSION
git commit -m "Bump version to 1.2.3"
```

#### 2. Create and push tag

```bash
# Create annotated tag
git tag -a v1.2.3 -m "Release version 1.2.3"

# Push tag to trigger workflows
git push origin v1.2.3
```

#### 3. Monitor GitHub Actions

Watch the workflows:
- https://github.com/LeviathanSystems/LeviathanDM/actions

The following will happen automatically:
1. **Release workflow** builds binaries and creates GitHub release
2. **Documentation workflow** deploys updated docs to GitHub Pages

#### 4. Edit release notes (optional)

After the release is created:
1. Go to https://github.com/LeviathanSystems/LeviathanDM/releases
2. Click "Edit" on the new release
3. Add detailed changelog, breaking changes, and notable features

### Release Notes Template

```markdown
## What's New

### Features
- Feature 1
- Feature 2

### Bug Fixes
- Fix 1
- Fix 2

### Changes
- Change 1
- Change 2

### Breaking Changes
- Breaking change 1 (if any)

## Installation

Download the pre-built binary for your system below.

### From Binary

\`\`\`bash
# Download and extract
tar xzf leviathan-1.2.3-linux-x86_64.tar.gz
cd leviathan-1.2.3-linux-x86_64

# Install
sudo install -m 755 leviathan /usr/local/bin/
sudo install -m 755 leviathanctl /usr/local/bin/
sudo install -m 755 libleviathan-ui.so /usr/local/lib/

# Copy plugins (optional)
sudo mkdir -p /usr/local/lib/leviathan/plugins
sudo cp plugins/*.so /usr/local/lib/leviathan/plugins/

# Copy config
mkdir -p ~/.config/leviathan
cp -n config/* ~/.config/leviathan/
\`\`\`

### From Source

See [Building from Source](https://leviathansystems.github.io/LeviathanDM/docs/getting-started/building/)

## Requirements

- Linux kernel 5.x or newer
- Wayland-capable GPU drivers
- libwayland, libxkbcommon, pixman, cairo

## Documentation

Full documentation: https://leviathansystems.github.io/LeviathanDM/
```

## Versioning Guidelines

### MAJOR version (X.0.0)
- Incompatible API changes
- Major architectural changes
- Breaking configuration changes

### MINOR version (1.X.0)
- New features (backwards compatible)
- New keybindings
- New plugins
- Configuration additions

### PATCH version (1.2.X)
- Bug fixes
- Performance improvements
- Documentation updates
- Small tweaks

## Pre-release Versions

For alpha/beta/rc releases:

```bash
git tag -a v1.2.0-beta.1 -m "Beta 1 of version 1.2.0"
git push origin v1.2.0-beta.1
```

The release workflow automatically marks these as "pre-release" on GitHub.

## Hotfix Releases

For urgent fixes on a released version:

1. Create a branch from the release tag:
   ```bash
   git checkout -b hotfix-1.2.4 v1.2.3
   ```

2. Make the fix and commit:
   ```bash
   git commit -m "Fix critical bug"
   ```

3. Tag and push:
   ```bash
   git tag -a v1.2.4 -m "Hotfix release 1.2.4"
   git push origin v1.2.4
   ```

4. Merge back to master:
   ```bash
   git checkout master
   git merge hotfix-1.2.4
   git push origin master
   ```

## Manual Release (Emergency)

If GitHub Actions is down or you need to build manually:

```bash
# Build
./setup-deps.sh
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Package
VERSION="1.2.3"
PACKAGE_NAME="leviathan-${VERSION}-linux-x86_64"

mkdir -p ${PACKAGE_NAME}
cp leviathan leviathanctl libleviathan-ui.so ${PACKAGE_NAME}/
cp -r ../config ${PACKAGE_NAME}/
cp ../README.md ${PACKAGE_NAME}/

tar czf ${PACKAGE_NAME}.tar.gz ${PACKAGE_NAME}
sha256sum ${PACKAGE_NAME}.tar.gz > ${PACKAGE_NAME}.tar.gz.sha256

# Create release on GitHub manually and upload archives
```

## Rollback

To rollback a release:

1. Delete the tag:
   ```bash
   git tag -d v1.2.3
   git push origin :refs/tags/v1.2.3
   ```

2. Delete the release on GitHub:
   - Go to Releases page
   - Click "Delete" on the release

3. Fix the issue and create a new release with a patch version

## Troubleshooting

### Release workflow fails
- Check the workflow logs in GitHub Actions
- Verify all dependencies are available
- Test the build locally first

### Documentation not deploying
- Check the Hugo build logs
- Verify `docs-site/hugo.toml` is correct
- Test locally with `hugo server`

### Binary doesn't work
- Check dependencies with `ldd build/leviathan`
- Verify it was built in Release mode
- Test in a clean environment (Docker/VM)

## Checklist

Before creating a release:

- [ ] All tests passing
- [ ] CI build successful
- [ ] Documentation updated
- [ ] VERSION file updated
- [ ] Breaking changes documented
- [ ] Config examples updated
- [ ] Tested in clean environment
- [ ] Tag created and pushed
- [ ] Release notes written
- [ ] Announcement prepared (optional)

## Communication

After release:
1. Announce on project channels
2. Update website (if separate from docs)
3. Post to relevant communities
4. Update package managers (AUR, etc.)

## References

- [Semantic Versioning](https://semver.org/)
- [GitHub Releases](https://docs.github.com/en/repositories/releasing-projects-on-github)
- [GitHub Actions](https://docs.github.com/en/actions)
