import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig(({ mode }) => ({
  plugins: [react()],
  base: mode === 'production' ? '/LeviathanDM/' : '/',
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
  }
}))
