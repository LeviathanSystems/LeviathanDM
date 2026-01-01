import { Box, Select, MenuItem, Chip, Typography, Alert } from '@mui/material';
import SettingsIcon from '@mui/icons-material/Settings';
import { useVersion } from '../context/VersionContext';
import { useNavigate, useLocation } from 'react-router-dom';
import { navigation } from '../navigation';

interface VersionSelectorProps {
  onVersionChange?: (version: { name: string; title: string }) => void;
}

export default function VersionSelector({ onVersionChange }: VersionSelectorProps) {
  const { currentVersion, setCurrentVersion, versions } = useVersion();
  const navigate = useNavigate();
  const location = useLocation();

  const handleChange = (versionName: string) => {
    setCurrentVersion(versionName);
    const version = versions.find(v => v.name === versionName);
    if (version && onVersionChange) {
      onVersionChange(version);
    }

    // Check if current page exists in the target version
    // If not, redirect to the default page
    const currentPath = location.pathname;
    
    // Find the current page in navigation to check its "since" version
    const findPageInNav = (items: any[]): any => {
      for (const item of items) {
        if (item.path === currentPath) {
          return item;
        }
        if (item.children) {
          const found = findPageInNav(item.children);
          if (found) return found;
        }
      }
      return null;
    };

    const currentPage = findPageInNav(navigation);
    
    // Compare versions: if page.since > selected version, redirect
    if (currentPage && currentPage.since) {
      const compareVersions = (v1: string, v2: string): number => {
        const parts1 = v1.replace('v', '').split('.').map(Number);
        const parts2 = v2.replace('v', '').split('.').map(Number);
        
        for (let i = 0; i < Math.max(parts1.length, parts2.length); i++) {
          const p1 = parts1[i] || 0;
          const p2 = parts2[i] || 0;
          if (p1 !== p2) return p1 - p2;
        }
        return 0;
      };

      // If the page was introduced AFTER the selected version, redirect
      if (compareVersions(currentPage.since, versionName) > 0) {
        console.log(`Page ${currentPath} not available in ${versionName}, redirecting to default`);
        setTimeout(() => {
          navigate('/docs/getting-started/building', { replace: true });
        }, 100);
      }
    }
  };

  if (versions.length === 0) {
    return null;
  }

  return (
    <Box sx={{ minWidth: 200 }}>
      {currentVersion !== 'v0.0.4' && (
        <Alert severity="info" sx={{ mb: 1, py: 0 }}>
          Viewing docs for {currentVersion}
        </Alert>
      )}
      <Select
        value={currentVersion}
        onChange={(e) => handleChange(e.target.value)}
        fullWidth
        size="small"
        startAdornment={
          <SettingsIcon sx={{ mr: 1, fontSize: 18 }} />
        }
        sx={{
          bgcolor: 'rgba(255, 255, 255, 0.15)',
          color: 'white',
          '& .MuiSelect-select': {
            display: 'flex',
            alignItems: 'center',
            gap: 1,
            py: 1
          },
          '& .MuiOutlinedInput-notchedOutline': {
            borderColor: 'rgba(255, 255, 255, 0.3)'
          },
          '&:hover .MuiOutlinedInput-notchedOutline': {
            borderColor: 'rgba(255, 255, 255, 0.5)'
          },
          '& .MuiSvgIcon-root': {
            color: 'white'
          }
        }}
      >
        {versions.map((version) => (
          <MenuItem key={version.name} value={version.name}>
            <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', width: '100%' }}>
              <Typography variant="body2" sx={{ fontWeight: 500 }}>
                {version.title}
              </Typography>
              {version.badge && (
                <Chip 
                  label={version.badge}
                  size="small"
                  sx={{ 
                    ml: 1,
                    height: 20,
                    fontSize: '0.65rem',
                    fontWeight: 600,
                    bgcolor: version.badgeColor,
                    color: 'white',
                    '& .MuiChip-label': {
                      px: 1
                    }
                  }}
                />
              )}
            </Box>
          </MenuItem>
        ))}
      </Select>
    </Box>
  );
}
