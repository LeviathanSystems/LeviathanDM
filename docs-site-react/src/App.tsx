import { useState, useMemo, useEffect } from 'react';
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { ThemeProvider, createTheme, CssBaseline } from '@mui/material';
import { TOCProvider } from './context/TOCContext';
import { VersionProvider } from './context/VersionContext';
import { ManifestProvider, useManifest } from './context/ManifestContext';
import Layout from './components/Layout';
import MarkdownRenderer from './components/MarkdownRenderer';
import { buildNavigationFromManifest, getAllRoutes } from './navigation';

function AppContent() {
  const [darkMode, setDarkMode] = useState(() => {
    const saved = localStorage.getItem('darkMode');
    return saved ? JSON.parse(saved) : true;
  });

  const { manifest, loading: manifestLoading } = useManifest();
  const [routes, setRoutes] = useState<string[]>([]);

  // Load routes from manifest when available
  useEffect(() => {
    if (manifestLoading || !manifest) {
      return;
    }

    const nav = buildNavigationFromManifest(manifest);
    const allRoutes = getAllRoutes(nav);
    setRoutes(allRoutes);
  }, [manifest, manifestLoading]);

  const theme = useMemo(
    () =>
      createTheme({
        palette: {
          mode: darkMode ? 'dark' : 'light',
          primary: {
            main: '#1976d2',
          },
          secondary: {
            main: '#dc004e',
          },
        },
        typography: {
          fontFamily: '"Inter", "Roboto", "Helvetica", "Arial", sans-serif',
        },
      }),
    [darkMode]
  );

  const toggleDarkMode = () => {
    setDarkMode((prev: boolean) => {
      const newMode = !prev;
      localStorage.setItem('darkMode', JSON.stringify(newMode));
      return newMode;
    });
  };

  const basename = import.meta.env.BASE_URL || '/';

  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      <VersionProvider>
        <TOCProvider>
          <BrowserRouter basename={basename}>
            <Routes>
              <Route path="/" element={<Layout darkMode={darkMode} toggleDarkMode={toggleDarkMode} />}>
                <Route index element={<Navigate to="/docs/getting-started/building" replace />} />
                {routes.map(route => (
                  <Route 
                    key={route} 
                    path={route} 
                    element={<MarkdownRenderer path={route} />} 
                  />
                ))}
              </Route>
            </Routes>
          </BrowserRouter>
        </TOCProvider>
      </VersionProvider>
    </ThemeProvider>
  );
}

// Wrapper component that provides ManifestContext
function App() {
  return (
    <ManifestProvider>
      <AppContent />
    </ManifestProvider>
  );
}

export default App;
