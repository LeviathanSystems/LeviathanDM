import { Box, List, ListItem, ListItemButton, ListItemText, Typography } from '@mui/material';

interface TOCItem {
  id: string;
  title: string;
  level: number;
}

interface SidebarProps {
  tocItems?: TOCItem[];
}

export default function Sidebar({ tocItems = [] }: SidebarProps) {
  const scrollToHeading = (id: string) => {
    const element = document.getElementById(id);
    if (element) {
      element.scrollIntoView({ behavior: 'smooth', block: 'start' });
    }
  };

  return (
    <Box sx={{ 
      width: '100%', 
      p: 3, 
      height: '100%', 
      overflowY: 'auto',
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
    }}>
      <Typography 
        variant="overline" 
        sx={{ 
          mb: 2, 
          display: 'block',
          fontWeight: 700, 
          fontSize: '0.75rem',
          letterSpacing: 1.2,
          color: 'text.secondary'
        }}
      >
        ON THIS PAGE
      </Typography>
      
      {tocItems.length > 0 ? (
        <List component="nav" sx={{ py: 0 }}>
          {tocItems.map((item) => (
            <ListItem 
              key={item.id} 
              disablePadding 
              sx={{ 
                pl: (item.level - 1) * 2,
                mb: 0.5
              }}
            >
              <ListItemButton
                onClick={() => scrollToHeading(item.id)}
                sx={{
                  borderRadius: 1,
                  py: 0.75,
                  px: 1.5,
                  minHeight: 36,
                  borderLeft: 2,
                  borderColor: 'transparent',
                  transition: 'all 0.2s',
                  '&:hover': {
                    bgcolor: 'action.hover',
                    borderColor: 'primary.main',
                    transform: 'translateX(4px)'
                  }
                }}
              >
                <ListItemText 
                  primary={item.title}
                  primaryTypographyProps={{
                    variant: 'body2',
                    fontSize: item.level === 1 ? '0.9rem' : '0.85rem',
                    fontWeight: item.level === 1 ? 600 : 400,
                    color: 'text.secondary',
                    sx: {
                      transition: 'color 0.2s',
                      '&:hover': {
                        color: 'text.primary'
                      }
                    }
                  }}
                />
              </ListItemButton>
            </ListItem>
          ))}
        </List>
      ) : (
        <Typography 
          variant="body2" 
          sx={{ 
            color: 'text.disabled', 
            fontStyle: 'italic',
            px: 1.5,
            py: 1
          }}
        >
          No headings found
        </Typography>
      )}
    </Box>
  );
}
