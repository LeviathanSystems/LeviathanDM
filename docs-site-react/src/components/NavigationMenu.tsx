import { 
  List, 
  ListItem, 
  ListItemButton, 
  ListItemText,
  Box,
  Typography,
  Collapse,
  IconButton
} from '@mui/material';
import { useState, useEffect } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';
import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
import ExpandLessIcon from '@mui/icons-material/ExpandLess';
import CloseIcon from '@mui/icons-material/Close';
import { getNavigationForVersion } from '../navigation';
import { useVersion } from '../context/VersionContext';
import type { NavItem } from '../navigation';

interface NavigationMenuProps {
  open?: boolean;
  onClose?: () => void;
}

export default function NavigationMenu({ onClose }: NavigationMenuProps) {
  const navigate = useNavigate();
  const location = useLocation();
  const { currentVersion } = useVersion();
  const [expandedSections, setExpandedSections] = useState<string[]>([]);
  const [navigation, setNavigation] = useState<NavItem[]>([]);

  useEffect(() => {
    // Update navigation based on current version
    const filteredNav = getNavigationForVersion(currentVersion);
    setNavigation(filteredNav);
  }, [currentVersion]);

  const toggleSection = (path: string) => {
    setExpandedSections(prev =>
      prev.includes(path)
        ? prev.filter(p => p !== path)
        : [...prev, path]
    );
  };

  const handleNavigation = (item: NavItem) => {
    navigate(item.path);
    if (onClose) onClose();
  };

  const isActive = (path: string) => location.pathname === path;
  const isExpanded = (path: string) => expandedSections.includes(path);

  return (
    <Box
      sx={{
        width: '100%',
        height: '100%',
        overflowY: 'auto',
        bgcolor: 'background.paper',
        '&::-webkit-scrollbar': {
          width: '8px',
        },
        '&::-webkit-scrollbar-track': {
          bgcolor: 'transparent',
        },
        '&::-webkit-scrollbar-thumb': {
          bgcolor: 'action.hover',
          borderRadius: 1,
          '&:hover': {
            bgcolor: 'action.selected',
          }
        }
      }}
    >
      <Box sx={{ 
        display: 'flex', 
        alignItems: 'center', 
        justifyContent: 'space-between', 
        px: 3, 
        py: 2,
        position: 'sticky',
        top: 0,
        bgcolor: 'background.paper',
        zIndex: 1,
        borderBottom: 1,
        borderColor: 'divider'
      }}>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1.5 }}>
          <Box 
            component="img" 
            src="/logo.png" 
            alt="LeviathanDM Logo" 
            sx={{ height: 32, width: 32 }} 
          />
          <Typography variant="h6" sx={{ fontWeight: 700, color: 'primary.main' }}>
            Documentation
          </Typography>
        </Box>
        {onClose && (
          <IconButton 
            onClick={onClose} 
            size="small"
            sx={{
              '&:hover': {
                bgcolor: 'action.hover'
              }
            }}
          >
            <CloseIcon />
          </IconButton>
        )}
      </Box>

      <List sx={{ pt: 2, px: 2, pb: 4 }}>
        {navigation.map((section) => (
          <Box key={section.path} sx={{ mb: 1 }}>
            <ListItem disablePadding>
              <ListItemButton
                onClick={() => {
                  if (section.children) {
                    toggleSection(section.path);
                  } else {
                    handleNavigation(section);
                  }
                }}
                selected={isActive(section.path)}
                sx={{
                  borderRadius: 2,
                  mb: 0.5,
                  transition: 'all 0.2s',
                  '&.Mui-selected': {
                    bgcolor: 'primary.main',
                    color: 'white',
                    boxShadow: 1,
                    '&:hover': {
                      bgcolor: 'primary.dark',
                    }
                  },
                  '&:hover': {
                    bgcolor: 'action.hover',
                    transform: 'translateX(4px)'
                  }
                }}
              >
                <ListItemText 
                  primary={section.title}
                  primaryTypographyProps={{
                    fontWeight: 600,
                    fontSize: '0.95rem'
                  }}
                />
                {section.children && (
                  isExpanded(section.path) ? <ExpandLessIcon /> : <ExpandMoreIcon />
                )}
              </ListItemButton>
            </ListItem>

            {section.children && (
              <Collapse in={isExpanded(section.path)} timeout="auto" unmountOnExit>
                <List component="div" disablePadding sx={{ mt: 0.5 }}>
                  {section.children.map((child) => (
                    <ListItem key={child.path} disablePadding sx={{ pl: 2 }}>
                      <ListItemButton
                        onClick={() => handleNavigation(child)}
                        selected={isActive(child.path)}
                        sx={{
                          borderRadius: 1.5,
                          mx: 1,
                          mb: 0.5,
                          py: 1,
                          transition: 'all 0.2s',
                          borderLeft: 2,
                          borderColor: 'transparent',
                          '&.Mui-selected': {
                            bgcolor: 'primary.main',
                            color: 'white',
                            borderColor: 'primary.dark',
                            boxShadow: 1,
                            '&:hover': {
                              bgcolor: 'primary.dark',
                            }
                          },
                          '&:hover': {
                            bgcolor: 'action.hover',
                            borderColor: 'primary.light',
                            transform: 'translateX(4px)'
                          }
                        }}
                      >
                        <ListItemText 
                          primary={child.title}
                          primaryTypographyProps={{
                            fontSize: '0.875rem',
                            fontWeight: isActive(child.path) ? 600 : 400
                          }}
                        />
                      </ListItemButton>
                    </ListItem>
                  ))}
                </List>
              </Collapse>
            )}
          </Box>
        ))}
      </List>
    </Box>
  );
}
