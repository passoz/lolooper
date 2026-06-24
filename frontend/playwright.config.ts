import { defineConfig } from '@playwright/test'

export default defineConfig({
  testDir: './e2e',
  timeout: 30000,
  expect: { timeout: 10000 },
  use: {
    baseURL: 'http://localhost:5173',
    headless: true,
    viewport: { width: 1280, height: 720 },
    actionTimeout: 5000,
  },
  webServer: {
    command: 'npx vite preview --port 5173',
    url: 'http://localhost:5173',
    cwd: '.',
    reuseExistingServer: true,
  },
})
