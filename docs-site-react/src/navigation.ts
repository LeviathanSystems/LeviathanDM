// Navigation is dynamically generated from the content manifest at runtime
// This ensures navigation always reflects available documentation

export interface NavItem {
  title: string;
  path: string;
  mdPath: string;
  since?: string;
  children?: NavItem[];
}

// Section ordering for consistent navigation
const SECTION_ORDER = ['getting-started', 'features', 'development', 'tools', 'about'];

// Title case helper
function titleCase(str: string): string {
  return str
    .split('-')
    .map(word => word.charAt(0).toUpperCase() + word.slice(1))
    .join(' ');
}

// Build navigation structure from manifest metadata
export function buildNavigationFromManifest(manifest: any): NavItem[] {
  try {
    const { nav, v: versions } = manifest;
    
    if (!nav) {
      console.error('Manifest missing navigation metadata');
      return [];
    }
    
    // Group pages by section and subsection
    const sections: Record<string, {
      title: string;
      path: string;
      mdPath?: string;
      since?: string;
      pages: Array<{
        title: string;
        path: string;
        mdPath: string;
        since: string;
        subsection?: string;
      }>;
    }> = {};
    
    Object.values(nav).forEach((page: any) => {
      const { section, subsection, isIndex, title, path, mdPath, since } = page;
      
      // Initialize section if needed
      if (!sections[section]) {
        sections[section] = {
          title: titleCase(section),
          path: `/docs/${section}`,
          pages: []
        };
      }
      
      // Handle index files
      if (isIndex) {
        sections[section].mdPath = mdPath;
        sections[section].since = since;
      } else {
        // Regular page
        sections[section].pages.push({
          title,
          path,
          mdPath,
          since,
          subsection
        });
      }
    });
    
    // Convert to NavItem array with ordering
    const navigation: NavItem[] = [];
    
    SECTION_ORDER.forEach(sectionKey => {
      const section = sections[sectionKey];
      if (!section) return;
      
      // Sort pages by path
      const children = section.pages
        .sort((a, b) => a.path.localeCompare(b.path))
        .map(page => ({
          title: page.title,
          path: page.path,
          mdPath: page.mdPath,
          since: page.since
        }));
      
      navigation.push({
        title: section.title,
        path: section.path,
        mdPath: section.mdPath || `${section.path}/_index`,
        since: section.since || children[0]?.since || versions[versions.length - 1],
        children: children.length > 0 ? children : undefined
      });
    });
    
    // Add any sections not in the predefined order
    Object.keys(sections).forEach(sectionKey => {
      if (!SECTION_ORDER.includes(sectionKey)) {
        const section = sections[sectionKey];
        const children = section.pages
          .sort((a, b) => a.path.localeCompare(b.path))
          .map(page => ({
            title: page.title,
            path: page.path,
            mdPath: page.mdPath,
            since: page.since
          }));
        
        navigation.push({
          title: section.title,
          path: section.path,
          mdPath: section.mdPath || `${section.path}/_index`,
          since: section.since || children[0]?.since || versions[versions.length - 1],
          children: children.length > 0 ? children : undefined
        });
      }
    });
    
    return navigation;
  } catch (error) {
    console.error('Error building navigation:', error);
    return [];
  }
}

// Compare versions (simple comparison for x.y.z format)
function compareVersions(v1: string, v2: string): number {
  const parts1 = v1.replace('v', '').split('.').map(Number);
  const parts2 = v2.replace('v', '').split('.').map(Number);
  
  for (let i = 0; i < Math.max(parts1.length, parts2.length); i++) {
    const p1 = parts1[i] || 0;
    const p2 = parts2[i] || 0;
    if (p1 !== p2) return p1 - p2;
  }
  return 0;
}

// Filter navigation based on version
export function getNavigationForVersion(navigation: NavItem[], version: string): NavItem[] {
  function filterItems(items: NavItem[]): NavItem[] {
    return items
      .filter(item => {
        const shouldInclude = !item.since || compareVersions(version, item.since) >= 0;
        return shouldInclude;
      })
      .map(item => {
        const filteredChildren = item.children ? filterItems(item.children) : undefined;
        return {
          ...item,
          children: filteredChildren
        };
      })
      .filter(item => {
        // Keep items that either have no children or have at least one child after filtering
        const hasNoChildren = !item.children;
        const hasChildren = item.children && item.children.length > 0;
        return hasNoChildren || hasChildren;
      });
  }
  
  return filterItems(navigation);
}

// Flatten navigation to get all routes
export function getAllRoutes(navigation: NavItem[]): string[] {
  const routes: string[] = [];
  
  function addRoutes(items: NavItem[]) {
    items.forEach(item => {
      routes.push(item.path);
      if (item.children) {
        addRoutes(item.children);
      }
    });
  }
  
  addRoutes(navigation);
  return routes;
}
