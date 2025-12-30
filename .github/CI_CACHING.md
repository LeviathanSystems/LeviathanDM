# GitHub Actions CI Caching Strategy

## Overview

The CI workflow now uses multiple caching layers to significantly speed up builds and reduce GitHub Actions minutes usage.

## Cache Layers

### 1. Dependencies Cache (Wayland + Pixman)
**Purpose:** Cache compiled wayland 1.23.1 and pixman 0.43.4 libraries

**Cache Key:** `deps-${{ runner.os }}-wayland-1.23.1-pixman-0.43.4-v1`

**Cached Paths:**
- `/usr/lib/x86_64-linux-gnu/libwayland*.so*`
- `/usr/lib/x86_64-linux-gnu/libpixman*.so*`
- `/usr/lib/x86_64-linux-gnu/pkgconfig/*.pc` files
- `/usr/include/wayland`
- `/usr/include/pixman-1`
- `/usr/share/wayland`
- `/usr/bin/wayland-scanner`

**Build Time:**
- First run (cache miss): ~2-3 minutes to build wayland + pixman
- Subsequent runs (cache hit): ~5 seconds to restore

**When to Invalidate:**
Increment the version suffix (`-v1` → `-v2`) when:
- Upgrading wayland version
- Upgrading pixman version
- Changing build configuration

### 2. CMake Build Cache
**Purpose:** Cache compiled object files and build artifacts

**Cache Key:** `cmake-${{ runner.os }}-${{ hashFiles('**/CMakeLists.txt', 'subprojects/**') }}-${{ github.sha }}`

**Cached Paths:**
- `build/` directory (excluding `compile_commands.json`)

**Build Time:**
- First run: Full build (~5-10 minutes)
- Subsequent runs: Only changed files rebuild

**When Cache Invalidates:**
- Any `CMakeLists.txt` file changes
- Any file in `subprojects/` changes
- Different commit SHA (but uses restore-keys for partial matches)

### 3. ccache (Compiler Cache)
**Purpose:** Cache compiled object files across builds

**Action:** `hendrikmuhs/ccache-action@v1.2`

**Cache Key:** `${{ runner.os }}-ccache-${{ github.sha }}`

**How it Works:**
- Caches compiled `.o` files based on source file hash
- Works across different commits if source hasn't changed
- CMake configured with `-DCMAKE_C_COMPILER_LAUNCHER=ccache`

**Build Time Impact:**
- First run: Normal compilation speed
- Subsequent runs: 50-80% faster compilation

## Combined Impact

### First Build (All Cache Misses)
```
Install apt packages:     ~1 min
Build wayland:            ~2 min
Build pixman:             ~1 min
Setup dependencies:       ~1 min
CMake configure:          ~30 sec
Compile LeviathanDM:      ~5 min
-----------------------------------
Total:                    ~10-11 min
```

### Subsequent Build (All Cache Hits, No Code Changes)
```
Install apt packages:     ~1 min
Restore deps cache:       ~5 sec
Restore ccache:           ~10 sec
Setup dependencies:       ~1 min
CMake configure:          ~30 sec
Compile (cached):         ~30 sec
-----------------------------------
Total:                    ~3-4 min
```

### Typical Build (Cache Hit, Small Code Changes)
```
Install apt packages:     ~1 min
Restore deps cache:       ~5 sec
Restore ccache:           ~10 sec
Setup dependencies:       ~1 min
CMake configure:          ~30 sec
Compile (partial):        ~2 min
-----------------------------------
Total:                    ~5-6 min
```

## Cache Size Limits

GitHub Actions cache limits:
- **Per repository:** 10 GB total
- **Per cache entry:** 10 GB max
- **Eviction:** Caches not accessed in 7 days are deleted
- **Retention:** Oldest caches deleted if repo limit exceeded

### Estimated Cache Sizes
- Dependencies cache: ~50-100 MB
- CMake build cache: ~500 MB - 1 GB
- ccache: ~500 MB - 2 GB

**Total:** ~1-3 GB (well under 10 GB limit)

## Cache Invalidation Strategy

### Manual Cache Invalidation
To force rebuild of dependencies, increment version in cache key:

```yaml
key: deps-${{ runner.os }}-wayland-1.23.1-pixman-0.43.4-v2  # Changed v1 → v2
```

### Automatic Cache Invalidation
Caches automatically invalidate when:
- `CMakeLists.txt` changes
- Files in `subprojects/` change
- Dependencies versions change

## Monitoring Cache Usage

### Check Cache Hit/Miss in Workflow Logs
Look for these lines in GitHub Actions logs:

```
✅ Cache hit: "Cache restored from key: deps-..."
❌ Cache miss: "Cache not found for input keys: deps-..."
```

### View Repository Cache
1. Go to repository Settings
2. Click "Actions" → "Caches"
3. See all active caches, sizes, and last accessed time

## Best Practices

### 1. Don't Cache Everything
❌ **Don't cache:**
- `/tmp` directories (cleaned between runs anyway)
- Log files
- `compile_commands.json` (regenerated every build)

✅ **Do cache:**
- Built libraries
- Compiled object files
- Downloaded dependencies

### 2. Use Specific Cache Keys
```yaml
# ✅ Good: Specific key with hash
key: cmake-${{ runner.os }}-${{ hashFiles('**/CMakeLists.txt') }}

# ❌ Bad: Too generic, rarely invalidates
key: cmake-${{ runner.os }}
```

### 3. Use Restore Keys for Fallback
```yaml
restore-keys: |
  cmake-${{ runner.os }}-${{ hashFiles('**/CMakeLists.txt') }}-
  cmake-${{ runner.os }}-
```

This allows partial cache hits even if exact key doesn't match.

### 4. Separate Slow-Changing Dependencies
Wayland/Pixman change rarely, so they have their own cache separate from the frequently-changing source code build cache.

## Troubleshooting

### Problem: Build fails with "library not found"
**Cause:** Cache restored but files in wrong location

**Solution:**
1. Check cache paths match install locations
2. Run `sudo ldconfig` after restoring cache
3. Verify with: `ldconfig -p | grep wayland`

### Problem: Cache keeps missing
**Cause:** Cache key changes on every run

**Solution:**
1. Check `${{ hashFiles() }}` patterns match actual files
2. Verify files aren't changing unexpectedly (e.g., timestamps)
3. Use `restore-keys` for fallback

### Problem: Out of cache space
**Cause:** Repository exceeds 10 GB cache limit

**Solution:**
1. Delete old caches in Settings → Actions → Caches
2. Reduce cached paths (exclude large unnecessary files)
3. Adjust cache retention

### Problem: Slow cache restore
**Cause:** Cache too large

**Solution:**
1. Compress large directories before caching
2. Exclude unnecessary files from cache path
3. Split into smaller, more specific caches

## Future Improvements

### Possible Optimizations
1. **Docker layer caching:** Use pre-built Docker image with all dependencies
2. **Self-hosted runner:** Keep persistent cache on runner itself
3. **Artifact reuse:** Share build artifacts between jobs
4. **Dependency vendoring:** Include prebuilt dependencies in repo

### When to Consider Docker
If build times still too long:

```dockerfile
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y ...
RUN cd /tmp && build wayland ...
RUN cd /tmp && build pixman ...
# Push to ghcr.io/LeviathanSystems/leviathan-builder:latest
```

Then in workflow:
```yaml
jobs:
  build:
    container: ghcr.io/LeviathanSystems/leviathan-builder:latest
```

## Maintenance

### Weekly Check
- Review cache hit rates in Actions logs
- Check total cache size in Settings
- Delete unused old caches

### When Upgrading Dependencies
1. Update version in cache key
2. Test build works with new version
3. Document in CHANGELOG

### Performance Baseline
Track build times in each release:
- Full clean build time
- Incremental build time
- Cache hit/miss ratios

---

**Last Updated:** December 30, 2025
**CI Workflow:** `.github/workflows/ci.yml`
