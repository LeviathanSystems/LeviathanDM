import { createContext, useContext, useState } from 'react';
import type { ReactNode } from 'react';

export interface TOCItem {
  id: string;
  title: string;
  level: number;
}

interface TOCContextType {
  tocItems: TOCItem[];
  setTOCItems: (items: TOCItem[]) => void;
}

const TOCContext = createContext<TOCContextType | undefined>(undefined);

export function TOCProvider({ children }: { children: ReactNode }) {
  const [tocItems, setTOCItems] = useState<TOCItem[]>([]);

  return (
    <TOCContext.Provider value={{ tocItems, setTOCItems }}>
      {children}
    </TOCContext.Provider>
  );
}

export function useTOC() {
  const context = useContext(TOCContext);
  if (!context) {
    throw new Error('useTOC must be used within TOCProvider');
  }
  return context;
}
