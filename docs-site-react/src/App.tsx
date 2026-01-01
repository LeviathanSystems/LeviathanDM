import { useState, useMemo } from 'react';
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import { ThemeProvider, createTheme, CssBaseline } from '@mui/material';
import { TOCProvider } from './context/TOCContext';
import { VersionProvider } from './context/VersionContext';
import Layout from './components/Layout';
import MarkdownRenderer from './components/MarkdownRenderer';
import { getAllRoutes } from './navigation';

function App() {
  console.log('App rendering...');
  
  const [darkMode, setDarkMode] = useState(() => {
    const saved = localStorage.getItem('darkMode');
    return saved ? JSON.parse(saved) : true;
  });

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

  console.log('Theme mode:', darkMode ? 'dark' : 'light');

  const allRoutes = getAllRoutes();

  return (
    <ThemeProvider theme={theme}>
      <CssBaseline />
      <VersionProvider>
        <TOCProvider>
          <BrowserRouter>
            <Routes>
              <Route path="/" element={<Layout darkMode={darkMode} toggleDarkMode={toggleDarkMode} />}>
                <Route index element={<Navigate to="/docs/getting-started/building" replace />} />
                {allRoutes.map(route => (
                  <Route 
                    key={route.path} 
                    path={route.path} 
                    element={<MarkdownRenderer path={route.mdPath} />} 
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

export default App;
