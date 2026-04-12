import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import vueDevTools from 'vite-plugin-vue-devtools'

// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    vueDevTools(),
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    },
  },
  server: {
    proxy: {
      '/fastapi': {
        target: 'http://localhost:8000',
        //target: 'http://192.168.216.109:8080',
        changeOrigin: true,
        rewrite: path => path.replace(/^\/fastapi/, '')
      },
      '/api': {
        target: 'http://localhost:8000',
        //target: 'https://lark.mintlab.top',
        changeOrigin: true,
        secure: false,
        rewrite: path => path,
        ws: true // 支持WebSocket代理
      },
      '/stream': {
        target: 'http://localhost:8000',
        //target: 'https://lark.mintlab.top',
        changeOrigin: true,
        secure: false,
        rewrite: path => path,
        ws: true // 支持WebSocket代理
      }
    }
  }
})
