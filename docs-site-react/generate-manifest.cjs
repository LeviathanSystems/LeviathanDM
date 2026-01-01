#!/usr/bin/env node

/**
 * Generate a content manifest that maps each content file to its available versions.
 * For each version, we track which version the file should be fetched from.
 * This follows the cascade rule: newer content files override older ones.
 */

const fs = require('fs');
const path = require('path');

const PUBLIC_DIR = path.join(__dirname, 'public');
const CONTENT_DIR = path.join(PUBLIC_DIR, 'content');
const MANIFEST_PATH = path.join(PUBLIC_DIR, 'content-manifest.json');

// Read versions.json to get the version order
const versionsFile = fs.readFileSync(path.join(PUBLIC_DIR, 'versions.json'), 'utf-8');
const versionsData = JSON.parse(versionsFile);
const versions = versionsData.versions.map(v => v.name);

console.log('Versions found:', versions);

// Helper to compare versions (returns -1 if v1 < v2, 0 if equal, 1 if v1 > v2)
function compareVersions(v1, v2) {
  const parts1 = v1.replace('v', '').split('.').map(Number);
  const parts2 = v2.replace('v', '').split('.').map(Number);
  
  for (let i = 0; i < Math.max(parts1.length, parts2.length); i++) {
    const p1 = parts1[i] || 0;
    const p2 = parts2[i] || 0;
    if (p1 !== p2) return p1 - p2;
  }
  return 0;
}

// Function to recursively find all .md files
function findMarkdownFiles(dir, baseDir = dir) {
  let results = [];
  const list = fs.readdirSync(dir);
  
  list.forEach(file => {
    const filePath = path.join(dir, file);
    const stat = fs.statSync(filePath);
    
    if (stat.isDirectory()) {
      results = results.concat(findMarkdownFiles(filePath, baseDir));
    } else if (file.endsWith('.md')) {
      // Get relative path from content directory
      const relativePath = path.relative(baseDir, filePath)
        .replace(/\\/g, '/') // Normalize path separators
        .replace(/\.md$/, ''); // Remove .md extension
      results.push(relativePath);
    }
  });
  
  return results;
}

// Build a map of which files exist in which versions
const fileVersionMap = {}; // { "/en/docs/path": ["v0.0.1", "v0.0.3"] }

// Process each version directory
versions.forEach(version => {
  const versionDir = path.join(CONTENT_DIR, version);
  
  if (!fs.existsSync(versionDir)) {
    console.log(`Skipping ${version} - directory not found`);
    return;
  }
  
  console.log(`Scanning ${version}...`);
  const files = findMarkdownFiles(versionDir);
  
  files.forEach(file => {
    // Remove version prefix from path and ensure leading slash
    const contentPath = '/' + file.replace(new RegExp(`^${version}/`), '');
    
    if (!fileVersionMap[contentPath]) {
      fileVersionMap[contentPath] = [];
    }
    fileVersionMap[contentPath].push(version);
  });
});

// Also include base content files (these are in v0.0.1)
const baseFiles = findMarkdownFiles(CONTENT_DIR)
  .filter(file => !versions.some(v => file.startsWith(v + '/')));

baseFiles.forEach(file => {
  const contentPath = '/' + file;
  
  if (!fileVersionMap[contentPath]) {
    fileVersionMap[contentPath] = [];
  }
  // Base files are considered to be in v0.0.1
  if (!fileVersionMap[contentPath].includes('v0.0.1')) {
    fileVersionMap[contentPath].push('v0.0.1');
  }
});

// Now build the OPTIMIZED manifest
// Format: { v: [versions], f: { path: [v1_index, v2_index, ...] } }
// Each file stores which version index it should use for each target version
const optimizedManifest = {
  v: versions, // Array of version names
  f: {} // Files map: path -> array of version indices
};

// Build index lookup
const versionToIndex = {};
versions.forEach((v, i) => {
  versionToIndex[v] = i;
});

// For each file, build an array showing which version to use for each target version
Object.keys(fileVersionMap).forEach(contentPath => {
  const availableVersions = fileVersionMap[contentPath];
  const versionIndices = [];
  
  versions.forEach(targetVersion => {
    // Filter to only versions <= targetVersion
    const eligibleVersions = availableVersions.filter(v => 
      compareVersions(v, targetVersion) <= 0
    );
    
    if (eligibleVersions.length > 0) {
      // Sort to get the latest (newest first)
      eligibleVersions.sort((a, b) => compareVersions(b, a));
      const sourceVersion = eligibleVersions[0];
      versionIndices.push(versionToIndex[sourceVersion]);
    } else {
      versionIndices.push(-1); // Not available in this version
    }
  });
  
  // Only store if file is available in at least one version
  if (versionIndices.some(i => i !== -1)) {
    optimizedManifest.f[contentPath] = versionIndices;
  }
});

// Write optimized manifest
fs.writeFileSync(MANIFEST_PATH, JSON.stringify(optimizedManifest));

// Calculate size savings
const fullManifest = {};
versions.forEach(targetVersion => {
  fullManifest[targetVersion] = {};
  Object.keys(fileVersionMap).forEach(contentPath => {
    const availableVersions = fileVersionMap[contentPath];
    const eligibleVersions = availableVersions.filter(v => 
      compareVersions(v, targetVersion) <= 0
    );
    if (eligibleVersions.length > 0) {
      eligibleVersions.sort((a, b) => compareVersions(b, a));
      const sourceVersion = eligibleVersions[0];
      fullManifest[targetVersion][contentPath] = {
        version: sourceVersion,
        path: `${sourceVersion}${contentPath}`
      };
    }
  });
});

const fullSize = JSON.stringify(fullManifest).length;
const optimizedSize = JSON.stringify(optimizedManifest).length;
const savings = ((1 - optimizedSize / fullSize) * 100).toFixed(1);

console.log(`\n✓ Optimized manifest generated for ${versions.length} versions`);
console.log(`  Files: ${Object.keys(optimizedManifest.f).length}`);
console.log(`  Full size: ${fullSize} bytes`);
console.log(`  Optimized size: ${optimizedSize} bytes`);
console.log(`  Savings: ${savings}% reduction`);
console.log(`  Written to: ${MANIFEST_PATH}`);

// Show format explanation
console.log('\nFormat: { v: [versions], f: { path: [version_indices] } }');
console.log('Example: f["/path"]: [0, 1, 1, 2] means:');
console.log('  - v0.0.4 uses version[0], v0.0.3 uses version[1], etc.');
