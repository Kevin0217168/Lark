<template>
  <div class="cloud-app">
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
</style>