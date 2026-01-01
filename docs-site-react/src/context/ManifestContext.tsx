import { createContext, useContext, useState, useEffect } from 'react';
import type { ReactNode } from 'react';

interface ManifestData {
  v: string[]; // versions array
  f: Record<string, number[]>; // files mapping
  nav: Record<string, {
    title: string;
    path: string;
    mdPath: string;
    section: string;
    subsection?: string;
    isIndex: boolean;
    since: string;
  }>;
}

interface ManifestContextType {
  manifest: ManifestData | null;
  loading: boolean;
  error: string | null;
}

const ManifestContext = createContext<ManifestContextType | undefined>(undefined);

export function ManifestProvider({ children }: { children: ReactNode }) {
  const [manifest, setManifest] = useState<ManifestData | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    const baseUrl = import.meta.env.BASE_URL || '/';
    
    fetch(`${baseUrl}content-manifest.json`)
      .then(response => {
        if (!response.ok) {
          throw new Error(`Failed to fetch manifest: ${response.status}`);
        }
        return response.json();
      })
      .then(data => {
        console.log('✓ Manifest loaded:', {
          versions: data.v.length,
          files: Object.keys(data.f).length,
          pages: Object.keys(data.nav || {}).length
        });
        setManifest(data);
        setLoading(false);
      })
      .catch(err => {
        console.error('✗ Failed to load manifest:', err);
        setError(err.message);
        setLoading(false);
      });
  }, []);

  return (
    <ManifestContext.Provider value={{ manifest, loading, error }}>
      {children}
    </ManifestContext.Provider>
  );
}

export function useManifest() {
  const context = useContext(ManifestContext);
  if (context === undefined) {
    throw new Error('useManifest must be used within a ManifestProvider');
  }
  return context;
}
