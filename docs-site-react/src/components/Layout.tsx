import { useState } from 'react';
import { 
  AppBar, 
  Toolbar, 
  Typography, 
  IconButton, 
  Drawer, 
  Box, 
  useMediaQuery,
  useTheme as useMuiTheme
} from '@mui/material';
import Brightness4Icon from '@mui/icons-material/Brightness4';
import Brightness7Icon from '@mui/icons-material/Brightness7';
import GitHubIcon from '@mui/icons-material/GitHub';
import ListIcon from '@mui/icons-material/List';
import VersionSelector from './VersionSelector';
import Sidebar from './Sidebar';
import NavigationMenu from './NavigationMenu';
import { Outlet } from 'react-router-dom';
import { useTOC } from '../context/TOCContext';
import { useVersion } from '../context/VersionContext';

interface LayoutProps {
  darkMode: boolean;
  toggleDarkMode: () => void;
}

const tocWidth = 240;
const navMenuWidth = 280;

export default function Layout({ darkMode, toggleDarkMode }: LayoutProps) {
  const [mobileOpen, setMobileOpen] = useState(false);
  const theme = useMuiTheme();
  const isMobile = useMediaQuery(theme.breakpoints.down('md'));
  const isLargeScreen = useMediaQuery(theme.breakpoints.up('lg'));
  const { tocItems } = useTOC();
  const { currentVersion } = useVersion();

  const handleDrawerToggle = () => {
    setMobileOpen(!mobileOpen);
  };

  return (
    <Box sx={{ display: 'flex', minHeight: '100vh', bgcolor: 'background.default' }}>
      {/* Fixed Navigation Menu - Always visible on desktop */}
      {!isMobile && (
        <Drawer
          variant="permanent"
          sx={{
            width: navMenuWidth,
            flexShrink: 0,
            '& .MuiDrawer-paper': {
              width: navMenuWidth,
              boxSizing: 'border-box',
              mt: 8,
              borderRight: 1,
              borderColor: 'divider',
              bgcolor: 'background.paper'
            },
          }}
        >
          <NavigationMenu open={true} onClose={() => {}} />
        </Drawer>
      )}

      {/* Mobile Navigation Drawer */}
      {isMobile && (
        <Drawer
          variant="temporary"
          open={mobileOpen}
          onClose={handleDrawerToggle}
          ModalProps={{ keepMounted: true }}
          sx={{
            '& .MuiDrawer-paper': {
              width: navMenuWidth,
              boxSizing: 'border-box'
            },
          }}
        >
          <NavigationMenu open={mobileOpen} onClose={handleDrawerToggle} />
        </Drawer>
      )}
      
      <AppBar
        position="fixed"
        elevation={0}
        sx={{
          zIndex: (theme) => theme.zIndex.drawer + 1,
          bgcolor: darkMode ? 'grey.900' : 'primary.main',
          borderBottom: 1,
          borderColor: 'divider'
        }}
      >
        <Toolbar>
          {isMobile && (
            <IconButton
              color="inherit"
              edge="start"
              onClick={handleDrawerToggle}
              sx={{ mr: 2 }}
            >
              <ListIcon />
            </IconButton>
          )}
          
          {/* Logo */}
          <Box 
            component="img" 
            src="/logo.png" 
            alt="LeviathanDM Logo" 
            sx={{ 
              height: 40, 
              width: 40, 
              mr: 2,
              display: { xs: 'none', sm: 'block' }
            }} 
          />
          
          <Typography variant="h6" noWrap component="div" sx={{ flexGrow: 1, fontWeight: 700 }}>
            LeviathanDM Documentation
          </Typography>
          
          {/* Version Selector in AppBar */}
          <Box sx={{ mr: 2, minWidth: 200 }}>
            <VersionSelector />
          </Box>
          
          <IconButton color="inherit" onClick={toggleDarkMode} sx={{ mr: 1 }}>
            {darkMode ? <Brightness7Icon /> : <Brightness4Icon />}
          </IconButton>
          <IconButton 
            color="inherit" 
            component="a"
            href="https://github.com/LeviathanSystems/LeviathanDM"
            target="_blank"
          >
            <GitHubIcon />
          </IconButton>
        </Toolbar>
      </AppBar>

      <Box
        component="main"
        sx={{
          flexGrow: 1,
          width: { 
            xs: '100%',
            md: `calc(100% - ${navMenuWidth}px)`,
            lg: `calc(100% - ${navMenuWidth + tocWidth}px)`
          },
          ml: { md: `${navMenuWidth}px`, xs: 0 },
          mt: 8,
          minHeight: 'calc(100vh - 64px)',
          bgcolor: 'background.default'
        }}
      >
        {currentVersion !== 'v0.0.4' && (
          <Box 
            sx={{ 
              mx: 3,
              mt: 3,
              mb: 2,
              p: 2, 
              bgcolor: 'info.main', 
              color: 'white', 
              borderRadius: 2,
              boxShadow: 1
            }}
          >
            <Typography variant="body2" sx={{ fontWeight: 500 }}>
              📖 You're viewing documentation for {currentVersion}. 
              <Typography component="span" sx={{ ml: 1, textDecoration: 'underline', cursor: 'pointer' }}>
                Switch to latest version
              </Typography>
            </Typography>
          </Box>
        )}
        
        <Box sx={{ 
          px: { xs: 2, md: 4, lg: 6 }, 
          py: 3,
          maxWidth: '900px',
          mx: 'auto'
        }}>
          <Outlet />
        </Box>
      </Box>

      {/* Right TOC Sidebar - Only on large screens */}
      {isLargeScreen && tocItems.length > 0 && (
        <Drawer
          variant="permanent"
          anchor="right"
          sx={{
            width: tocWidth,
            flexShrink: 0,
            '& .MuiDrawer-paper': {
              width: tocWidth,
              boxSizing: 'border-box',
              mt: 8,
              borderLeft: 1,
              borderColor: 'divider',
              bgcolor: 'background.paper'
            },
          }}
        >
          <Sidebar tocItems={tocItems} />
        </Drawer>
      )}
    </Box>
  );
}
