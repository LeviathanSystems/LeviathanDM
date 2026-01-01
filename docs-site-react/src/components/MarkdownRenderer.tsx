import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
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

// Fetch content using the optimized manifest for efficient lookup
async function fetchMarkdownWithManifest(path: string, currentVersion: string): Promise<string> {
  const baseUrl = import.meta.env.BASE_URL || '/';
  
  console.log(`Fetching ${path} for version ${currentVersion}`);
  
  try {
    // Load the manifest (optimized format: { v: [versions], f: { path: [indices] } })
    const manifestResponse = await fetch(`${baseUrl}content-manifest.json`);
    if (!manifestResponse.ok) {
      throw new Error('Failed to load content manifest');
    }
    
    const manifest = await manifestResponse.json();
    const versions = manifest.v; // Array of version names
    const files = manifest.f; // Files map
    
    // Find current version index
    const versionIndex = versions.indexOf(currentVersion);
    if (versionIndex === -1) {
      throw new Error(`Version ${currentVersion} not found in manifest`);
    }
    
    // Look up the file in the manifest
    const versionIndices = files[path];
    
    if (!versionIndices) {
      throw new Error(`File ${path} not found in manifest`);
    }
    
    // Get the source version index for this target version
    const sourceVersionIndex = versionIndices[versionIndex];
    
    if (sourceVersionIndex === -1) {
      throw new Error(`File ${path} not available in version ${currentVersion}`);
    }
    
    // Get the actual source version name
    const sourceVersion = versions[sourceVersionIndex];
    
    // Fetch from the specific version indicated by the manifest
    const filePath = `${baseUrl}content/${sourceVersion}${path}.md`;
    console.log(`✓ Manifest says: fetch from ${sourceVersion} at ${filePath}`);
    
    const response = await fetch(filePath);
    if (!response.ok) {
      throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    }
    
    const text = await response.text();
    
    // Verify it's markdown, not HTML
    if (text.includes('<!DOCTYPE html>') || text.includes('<html')) {
      throw new Error('Received HTML instead of markdown');
    }
    
    console.log(`✓ Successfully loaded from ${sourceVersion}`);
    return text;
    
  } catch (err) {
    console.error(`✗ Failed to load ${path}:`, err);
    throw err;
  }
}

export default function MarkdownRenderer({ path }: MarkdownRendererProps) {
  const [content, setContent] = useState<string>('');
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string>('');
  const { setTOCItems } = useTOC();
  const { currentVersion } = useVersion();
  const navigate = useNavigate();

  useEffect(() => {
    setLoading(true);
    setError('');
    
    fetchMarkdownWithManifest(path, currentVersion)
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
      .catch((err: Error) => {
        console.error('Content not found, redirecting to default page:', err);
        setError(err.message);
        setLoading(false);
        
        // Redirect to the default "building" page if content not found
        setTimeout(() => {
          navigate('/docs/getting-started/building', { replace: true });
        }, 100);
      });
  }, [path, currentVersion, setTOCItems, navigate]);

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
