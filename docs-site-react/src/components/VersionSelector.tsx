import { Box, Select, MenuItem, Chip, Typography, Alert } from '@mui/material';
import SettingsIcon from '@mui/icons-material/Settings';
import { useVersion } from '../context/VersionContext';

interface VersionSelectorProps {
  onVersionChange?: (version: { name: string; title: string }) => void;
}

export default function VersionSelector({ onVersionChange }: VersionSelectorProps) {
  const { currentVersion, setCurrentVersion, versions } = useVersion();

  const handleChange = (versionName: string) => {
    setCurrentVersion(versionName);
    const version = versions.find(v => v.name === versionName);
    if (version && onVersionChange) {
      onVersionChange(version);
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
