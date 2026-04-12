<template>
  <div class="cloud-app">
    <!-- 桌面端提示 -->
    <div class="desktop-notice">
      <div class="notice-content">
        <svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" class="notice-icon">
          <rect x="5" y="2" width="14" height="20" rx="2" ry="2"></rect>
          <line x1="12" y1="18" x2="12.01" y2="18"></line>
        </svg>
        <p class="notice-text">请使用移动端访问此网页</p>
      </div>
    </div>

    <transition name="page-transition" mode="out-in">
      <RouterView :key="$route.fullPath" />
    </transition>
    <BottomNav />
  </div>
</template>

<script setup lang="ts">
import { RouterView } from 'vue-router';
import BottomNav from './components/BottomNav.vue';
</script>

<style scoped>
.cloud-app {
  min-height: 100vh;
  width: 100%;
}

/* 页面过渡动画 */
.page-transition-enter-active,
.page-transition-leave-active {
  transition: all 0.1s ease;
}

.page-transition-enter-from {
  opacity: 0;
  transform: translateY(20px);
}

.page-transition-leave-to {
  opacity: 0;
  transform: translateY(-20px);
}

/* 确保页面内容在动画期间不被遮挡 */
.page-transition-enter-active,
.page-transition-leave-active {
  position: relative;
  z-index: 1;
}

.page-transition-leave-active {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
}

/* 桌面端提示 */
.desktop-notice {
  display: none;
}

@media (min-width: 769px) {
  .desktop-notice {
    display: flex;
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    z-index: 9999;
    background: #ffffff;
    align-items: center;
    justify-content: center;
  }

  .notice-content {
    text-align: center;
  }

  .notice-icon {
    color: #9ca3af;
    margin-bottom: 24px;
  }

  .notice-text {
    font-size: 18px;
    color: #6b7280;
    margin: 0;
    font-weight: 500;
  }
}
</style>