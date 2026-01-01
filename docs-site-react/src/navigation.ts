export interface NavItem {
  title: string;
  path: string;
  mdPath: string;
  since?: string; // Which version this was added
  children?: NavItem[];
}

export const navigation: NavItem[] = [
  {
    title: 'Getting Started',
    path: '/docs/getting-started',
    mdPath: '/en/docs/getting-started/_index',
    since: 'v0.0.1',
    children: [
      {
        title: 'Building',
        path: '/docs/getting-started/building',
        mdPath: '/en/docs/getting-started/building',
        since: 'v0.0.1'
      },
      {
        title: 'Configuration',
        path: '/docs/getting-started/configuration',
        mdPath: '/en/docs/getting-started/configuration',
        since: 'v0.0.1'
      },
      {
        title: 'Keybindings',
        path: '/docs/getting-started/keybindings',
        mdPath: '/en/docs/getting-started/keybindings',
        since: 'v0.0.1'
      }
    ]
  },
  {
    title: 'Features',
    path: '/docs/features',
    mdPath: '/en/docs/features/_index',
    since: 'v0.0.1',
    children: [
      {
        title: 'Wallpapers',
        path: '/docs/features/wallpapers',
        mdPath: '/en/docs/features/wallpapers',
        since: 'v0.0.2'
      },
      {
        title: 'Status Bar',
        path: '/docs/features/status-bar',
        mdPath: '/en/docs/features/status-bar',
        since: 'v0.0.1'
      },
      {
        title: 'Notifications',
        path: '/docs/features/notifications',
        mdPath: '/en/docs/features/notifications',
        since: 'v0.0.2'
      },
      {
        title: 'Layouts',
        path: '/docs/features/layouts',
        mdPath: '/en/docs/features/layouts',
        since: 'v0.0.2'
      }
    ]
  },
  {
    title: 'Development',
    path: '/docs/development',
    mdPath: '/en/docs/development/architecture',
    since: 'v0.0.1',
    children: [
      {
        title: 'Architecture',
        path: '/docs/development/architecture',
        mdPath: '/en/docs/development/architecture',
        since: 'v0.0.1'
      },
      {
        title: 'Widget System',
        path: '/docs/development/widget-system',
        mdPath: '/en/docs/development/widget-system',
        since: 'v0.0.3'
      },
      {
        title: 'Plugins',
        path: '/docs/development/plugins',
        mdPath: '/en/docs/development/plugins',
        since: 'v0.0.1'
      },
      {
        title: 'Contributing',
        path: '/docs/development/contributing',
        mdPath: '/en/docs/development/contributing',
        since: 'v0.0.1'
      }
    ]
  },
  {
    title: 'About',
    path: '/docs/about',
    mdPath: '/en/docs/about/releases',
    since: 'v0.0.1',
    children: [
      {
        title: 'Releases',
        path: '/docs/about/releases',
        mdPath: '/en/docs/about/releases',
        since: 'v0.0.1'
      },
      {
        title: 'Versioning',
        path: '/docs/about/versioning',
        mdPath: '/en/docs/about/versioning',
        since: 'v0.0.1'
      }
    ]
  }
];

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
export function getNavigationForVersion(version: string): NavItem[] {
  function filterItems(items: NavItem[]): NavItem[] {
    return items
      .filter(item => !item.since || compareVersions(version, item.since) >= 0)
      .map(item => ({
        ...item,
        children: item.children ? filterItems(item.children) : undefined
      }))
      .filter(item => !item.children || item.children.length > 0);
  }
  
  return filterItems(navigation);
}

// Flatten navigation to get all routes
export function getAllRoutes(): NavItem[] {
  const routes: NavItem[] = [];
  
  function addRoutes(items: NavItem[]) {
    items.forEach(item => {
      routes.push(item);
      if (item.children) {
        addRoutes(item.children);
      }
    });
  }
  
  addRoutes(navigation);
  return routes;
}
