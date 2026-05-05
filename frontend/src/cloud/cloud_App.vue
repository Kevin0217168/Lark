<template>
  <div class="cloud-app" :class="{ 'cloud-app-desktop': isDesktop }">

    <SideNav v-if="isDesktop" />
    <div class="main-wrapper">
      <transition name="page-transition" mode="out-in">
        <RouterView :key="$route.fullPath" />
      </transition>
      <BottomNav v-if="!isDesktop" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import { RouterView } from 'vue-router';
import BottomNav from './components/BottomNav.vue';
import SideNav from './components/SideNav.vue';
import { BREAKPOINTS } from '@/utils/mobileAdapter';

const isDesktop = ref(window.innerWidth >= BREAKPOINTS.DESKTOP);

const handleResize = () => {
  isDesktop.value = window.innerWidth >= BREAKPOINTS.DESKTOP;
};

onMounted(() => {
  window.addEventListener('resize', handleResize);
});

onUnmounted(() => {
  window.removeEventListener('resize', handleResize);
});
</script>

<style scoped>
.cloud-app {
  min-height: 100vh;
  width: 100%;
}

.cloud-app-desktop {
  display: flex;
  min-height: 100vh;
}

.main-wrapper {
  flex: 1;
  min-width: 0;
  display: flex;
  flex-direction: column;
}

/* 页面过渡动画 */
.page-transition-enter-active,
.page-transition-leave-active {
  transition: all 0.2s ease;
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