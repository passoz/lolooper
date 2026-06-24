import { createRouter, createWebHistory } from 'vue-router'

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/',
      redirect: '/editor',
    },
    {
      path: '/editor',
      name: 'editor',
      component: () => import('../views/PatternEditorPage.vue'),
    },
    {
      path: '/songs',
      name: 'songs',
      component: () => import('../views/SongEditorPage.vue'),
    },
    {
      path: '/setlists',
      name: 'setlists',
      component: () => import('../views/SetlistPage.vue'),
    },
    {
      path: '/settings',
      name: 'settings',
      component: () => import('../views/SettingsPage.vue'),
    },
  ],
})

export default router
