import { useEffect, useState } from 'react';
import { Box, CircularProgress, Alert, Paper } from '@mui/material';
import ReactMarkdown from 'react-markdown';
import remarkGfm from 'remark-gfm';
import rehypeHighlight from 'rehype-highlight';
import rehypeRaw from 'rehype-raw';
import 'highlight.js/styles/github-dark.css';
import { useTOC } from '../context/TOCContext';
import { useVersion } from '../context/VersionContext';

interface MarkdownRendererProps {
  path: string;
}

// Helper function to compare versions
function compareVersions(v1: string, v2: string): number {
  const parts1 = v1.replace('v', '').split('.').map(Number);
  const parts2 = v2.replace('v', '').split('.').map(Number);
  
  for (let i = 0; i < Math.max(parts1.length, parts2.length); i++) {
    const p1 = parts1[i] || 0;
    const p2 = parts2[i] || 0;
    if (p1 !== p2) return p1 - p2;
  }
  return 0;
}

// Get all versions up to and including the current one, sorted from newest to oldest
function getVersionCascade(currentVersion: string, allVersions: string[]): string[] {
  return allVersions
    .filter(v => compareVersions(v, currentVersion) <= 0)
    .sort((a, b) => compareVersions(b, a)); // Newest first
}

// Try to fetch a file, cascading through versions from newest to oldest
async function fetchMarkdownWithCascade(path: string, currentVersion: string, versions: string[]): Promise<string> {
  const versionCascade = getVersionCascade(currentVersion, versions);
  
  console.log(`Fetching ${path} for version ${currentVersion}`);
  console.log('Version cascade:', versionCascade);
  
  // Try each version in order (newest to oldest)
  for (const version of versionCascade) {
    const versionPath = `/content/${version}${path}.md`;
    console.log(`Trying: ${versionPath}`);
    
    try {
      const response = await fetch(versionPath);
      if (response.ok) {
        const contentType = response.headers.get('content-type');
        const text = await response.text();
        
        // Check if it's actually markdown/text, not HTML
        if (contentType?.includes('text/plain') || contentType?.includes('text/markdown') || 
            (!text.includes('<!DOCTYPE html>') && !text.includes('<html'))) {
          console.log(`✓ Found in ${version}`);
          return text;
        } else {
          console.log(`✗ Got HTML instead of markdown from ${version}`);
        }
      }
    } catch (err) {
      // Continue to next version
      console.log(`✗ Not found in ${version}:`, err);
    }
  }
  
  // If not found in any versioned folder, try the base content folder
  const basePath = `/content${path}.md`;
  console.log(`Trying base path: ${basePath}`);
  try {
    const response = await fetch(basePath);
    if (response.ok) {
      const text = await response.text();
      if (!text.includes('<!DOCTYPE html>') && !text.includes('<html')) {
        console.log(`✓ Found in base content`);
        return text;
      }
    }
  } catch (err) {
    console.log(`✗ Not found in base path:`, err);
  }
  
  throw new Error(`File not found: ${path}`);
}

export default function MarkdownRenderer({ path }: MarkdownRendererProps) {
  const [content, setContent] = useState<string>('');
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string>('');
  const { setTOCItems } = useTOC();
  const { currentVersion, versions } = useVersion();

  useEffect(() => {
    setLoading(true);
    setError('');
    
    const versionNames = versions.map(v => v.name);
    
    fetchMarkdownWithCascade(path, currentVersion, versionNames)
      .then(text => {
        setContent(text);
        
        // Extract headings for TOC
        const headingRegex = /^(#{1,6})\s+(.+)$/gm;
        const tocItems = [];
        let match;
        
        while ((match = headingRegex.exec(text)) !== null) {
          const level = match[1].length;
          const title = match[2].replace(/[#*`]/g, '').trim();
          const id = title.toLowerCase().replace(/[^\w]+/g, '-');
          tocItems.push({ id, title, level });
        }
        
        setTOCItems(tocItems);
        setLoading(false);
      })
      .catch(err => {
        setError(err.message);
        setLoading(false);
      });
  }, [path, currentVersion, versions, setTOCItems]);

  if (loading) {
    return (
      <Box sx={{ display: 'flex', justifyContent: 'center', p: 4 }}>
        <CircularProgress />
      </Box>
    );
  }

  if (error) {
    return (
      <Alert severity="error" sx={{ mt: 2 }}>
        Failed to load documentation: {error}
      </Alert>
    );
  }

  return (
    <Paper 
      elevation={0}
      sx={{ 
        p: { xs: 3, md: 4 },
        borderRadius: 2,
        bgcolor: 'background.paper',
        border: 1,
        borderColor: 'divider',
        '& h1': { 
          fontSize: '2.5rem', 
          fontWeight: 700,
          mb: 3,
          mt: 2,
          pb: 2,
          borderBottom: 2,
          borderColor: 'divider'
        },
        '& h2': { 
          fontSize: '2rem', 
          fontWeight: 600,
          mt: 4,
          mb: 2,
          pb: 1,
          borderBottom: 1,
          borderColor: 'divider'
        },
        '& h3': { 
          fontSize: '1.5rem', 
          fontWeight: 600,
          mt: 3,
          mb: 1.5,
          color: 'text.primary'
        },
        '& h4': { 
          fontSize: '1.25rem', 
          fontWeight: 600,
          mt: 2,
          mb: 1,
          color: 'text.primary'
        },
        '& p': { 
          lineHeight: 1.8,
          mb: 2,
          fontSize: '1rem',
          color: 'text.secondary'
        },
        '& code': {
          bgcolor: 'action.selected',
          color: 'primary.main',
          px: 1,
          py: 0.5,
          borderRadius: 1,
          fontSize: '0.875em',
          fontFamily: 'Consolas, Monaco, "Courier New", monospace',
          border: 1,
          borderColor: 'divider'
        },
        '& pre': {
          bgcolor: 'grey.900',
          p: 2,
          borderRadius: 2,
          overflow: 'auto',
          mb: 3,
          boxShadow: 1,
          '& code': {
            bgcolor: 'transparent',
            p: 0,
            border: 'none',
            color: 'inherit'
          }
        },
        '& ul, & ol': {
          pl: 3,
          mb: 2.5,
          '& li': {
            mb: 1,
            lineHeight: 1.8
          }
        },
        '& a': {
          color: 'primary.main',
          textDecoration: 'none',
          fontWeight: 500,
          '&:hover': {
            textDecoration: 'underline'
          }
        },
        '& blockquote': {
          borderLeft: 4,
          borderColor: 'primary.main',
          bgcolor: 'action.hover',
          pl: 2,
          py: 1,
          ml: 0,
          my: 2,
          fontStyle: 'italic',
          color: 'text.secondary',
          borderRadius: 1
        },
        '& table': {
          width: '100%',
          borderCollapse: 'collapse',
          mb: 3,
          boxShadow: 1,
          borderRadius: 1,
          overflow: 'hidden',
          '& th, & td': {
            border: 1,
            borderColor: 'divider',
            p: 2,
            textAlign: 'left'
          },
          '& th': {
            bgcolor: 'action.hover',
            fontWeight: 600,
            color: 'text.primary'
          },
          '& tr:hover': {
            bgcolor: 'action.hover'
          }
        },
        '& img': {
          maxWidth: '100%',
          height: 'auto',
          borderRadius: 2,
          boxShadow: 2,
          my: 2
        },
        '& hr': {
          my: 4,
          border: 'none',
          borderTop: 2,
          borderColor: 'divider'
        }
      }}
    >
      <ReactMarkdown 
        remarkPlugins={[remarkGfm]}
        rehypePlugins={[rehypeHighlight, rehypeRaw]}
      >
        {content}
      </ReactMarkdown>
    </Paper>
  );
}
