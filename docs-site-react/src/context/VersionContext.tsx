import { createContext, useContext, useState, useEffect } from 'react';
import type { ReactNode } from 'react';

interface Version {
  name: string;
  title: string;
  path: string;
  default?: boolean;
  badge?: string;
  badgeColor?: string;
}

interface VersionContextType {
  currentVersion: string;
  setCurrentVersion: (version: string) => void;
  versions: Version[];
}

const VersionContext = createContext<VersionContextType | undefined>(undefined);

export function VersionProvider({ children }: { children: ReactNode }) {
  const [currentVersion, setCurrentVersion] = useState<string>('v0.0.4');
  const [versions, setVersions] = useState<Version[]>([]);

  useEffect(() => {
    // Load versions from JSON
    fetch('/versions.json')
      .then(res => res.json())
      .then(data => {
        setVersions(data.versions);
        const defaultVersion = data.versions.find((v: Version) => v.default);
        if (defaultVersion) {
          setCurrentVersion(defaultVersion.name);
        }
      })
      .catch(err => console.error('Failed to load versions:', err));
  }, []);

  return (
    <VersionContext.Provider value={{ currentVersion, setCurrentVersion, versions }}>
      {children}
    </VersionContext.Provider>
  );
}

export function useVersion() {
  const context = useContext(VersionContext);
  if (!context) {
    throw new Error('useVersion must be used within VersionProvider');
  }
  return context;
}
