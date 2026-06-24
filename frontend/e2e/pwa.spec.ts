/**
 * E2E tests for the Lolooper PWA.
 * Uses Playwright to load the real app and verify rendering/interaction.
 */
import { test, expect } from '@playwright/test'

test.describe('PWA - Page Load & Navigation', () => {
  test('home page loads and redirects to editor', async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('h1')).toContainText('Lolooper')
    await expect(page.locator('nav')).toBeVisible()
  })

  test('navigation links exist and are clickable', async ({ page }) => {
    await page.goto('/')
    const nav = page.locator('nav')
    await expect(nav.getByRole('link', { name: /Editor/ })).toBeVisible()
    await expect(nav.getByRole('link', { name: /Músicas/ })).toBeVisible()
    await expect(nav.getByRole('link', { name: /Setlists/ })).toBeVisible()
    await expect(nav.getByRole('link', { name: /Config/ })).toBeVisible()
  })

  test('MIDI status indicator is visible', async ({ page }) => {
    await page.goto('/')
    await expect(page.locator('header .rounded-full').first()).toBeVisible()
  })
})

test.describe('PWA - Pattern Editor', () => {
  test('editor page loads', async ({ page }) => {
    await page.goto('/editor')
    await expect(page.getByRole('heading', { name: 'Pattern Editor' })).toBeVisible()
  })

  test('BPM display is visible', async ({ page }) => {
    await page.goto('/editor')
    await expect(page.getByText('BPM')).toBeVisible()
  })

  test('style selector exists with default samba', async ({ page }) => {
    await page.goto('/editor')
    // The style dropdown button shows the active style
    await expect(page.getByText('samba').first()).toBeVisible()
  })

  test('can change active style via dropdown', async ({ page }) => {
    await page.goto('/editor')
    // The style dropdown button is next to the heading
    const heading = page.getByRole('heading', { name: 'Pattern Editor' })
    const styleBtn = heading.locator('..').locator('..').locator('button').first()
    // Click it to open dropdown
    await styleBtn.click()
    await page.waitForTimeout(300)
    // Select "pagode" from the dropdown
    await page.getByText('pagode').click()
    await page.waitForTimeout(300)
    // Button should now show "pagode"
    await expect(styleBtn).toContainText('pagode')
  })

  test('pattern grid is rendered', async ({ page }) => {
    await page.goto('/editor')
    // Grid header shows step numbers
    await expect(page.getByText('1').first()).toBeVisible()
    await expect(page.getByText('16').first()).toBeVisible()
  })

  test('track names appear in the grid', async ({ page }) => {
    await page.goto('/editor')
    // Use first() to avoid strict mode (track names appear in grid and tracklist)
    await expect(page.getByText('Surdo 1').first()).toBeVisible()
    await expect(page.getByText('Caixa').first()).toBeVisible()
    await expect(page.getByText('Banjo').first()).toBeVisible()
  })

  test('export and import buttons exist', async ({ page }) => {
    await page.goto('/editor')
    await expect(page.getByText('Exportar')).toBeVisible()
    await expect(page.getByText('Importar')).toBeVisible()
  })
})

test.describe('PWA - Songs Page', () => {
  test('songs page loads', async ({ page }) => {
    await page.goto('/songs')
    await expect(page.getByRole('heading', { name: 'Músicas' })).toBeVisible()
  })

  test('can create a new song', async ({ page }) => {
    await page.goto('/songs')
    await page.getByText('+ Nova Música').click()
    await page.waitForTimeout(200)

    await page.getByPlaceholder('Nome da música').fill('E2E Test Song')
    await page.locator('input[type="number"]').first().fill('140')
    await page.getByText('Criar').click()
    await page.waitForTimeout(300)

    await expect(page.getByText('E2E Test Song')).toBeVisible()
  })
})

test.describe('PWA - Setlists Page', () => {
  test('setlists page loads', async ({ page }) => {
    await page.goto('/setlists')
    await expect(page.getByRole('heading', { name: 'Setlists' })).toBeVisible()
  })

  test('can create a setlist', async ({ page }) => {
    await page.goto('/setlists')
    await page.getByText('+ Nova Setlist').click()
    await page.waitForTimeout(200)

    await page.getByPlaceholder(/Nome da setlist/).fill('E2E Show')
    await page.getByText('Criar').click()
    await page.waitForTimeout(300)

    await expect(page.getByText('E2E Show')).toBeVisible()
  })
})

test.describe('PWA - Settings Page', () => {
  test('settings page loads', async ({ page }) => {
    await page.goto('/settings')
    await expect(page.getByRole('heading', { name: 'Configurações' })).toBeVisible()
    await expect(page.getByRole('heading', { name: 'MIDI' })).toBeVisible()
    await expect(page.getByRole('heading', { name: 'IA' })).toBeVisible()
  })

  test('MIDI connection status shows', async ({ page }) => {
    await page.goto('/settings')
    // Either "Conectado" or "Desconectado" should be visible
    const status = page.locator('text=/Conectado|Desconectado/').first()
    await expect(status).toBeVisible()
  })

  test('IA mode buttons exist', async ({ page }) => {
    await page.goto('/settings')
    await expect(page.getByText('Desligada')).toBeVisible()
    await expect(page.getByText('Observar')).toBeVisible()
    await expect(page.getByText('Auxiliar')).toBeVisible()
    await expect(page.getByText('Tomar Conta')).toBeVisible()
  })
})
