<template>
  <nav class="bottom-nav" :class="{ 'is-hidden': isHidden }">
    <div class="nav-blur-bg"></div>
    <div class="nav-content">
      <router-link
        v-for="item in navItems"
        :key="item.path"
        :to="item.path"
        class="nav-item"
        :class="{ 'is-active': isActive(item.path) }"
        @click="handleNavClick(item)"
      >
        <div class="nav-icon-wrapper">
          <svg class="nav-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
            <path v-if="item.icon === 'home'" d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
            <path v-if="item.icon === 'home'" d="M9 22V12h6v10" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
            
            <path v-if="item.icon === 'birds'" d="M12 2C8 2 5 5 5 9c0 3 2 5 4 6v2c0 1 1 2 2 2h2c1 0 2-1 2-2v-2c2-1 4-3 4-6 0-4-3-7-7-7z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
            <path v-if="item.icon === 'birds'" d="M9 9h6M9 12h6" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
            
            <path v-if="item.icon === 'my'" d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
            <circle v-if="item.icon === 'my'" cx="12" cy="7" r="4" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
          </svg>
        </div>
        <span class="nav-label">{{ item.label }}</span>
        <div class="nav-indicator" v-if="isActive(item.path)"></div>
      </router-link>
    </div>
    <div class="safe-area-bottom"></div>
  </nav>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import { useRoute, useRouter } from 'vue-router';

const route = useRoute();
const router = useRouter();

interface NavItem {
  path: string;
  label: string;
  icon: 'home' | 'birds' | 'my';
}

const navItems: NavItem[] = [
  { path: '/cloud/home', label: '首页', icon: 'home' },
  { path: '/cloud/birds', label: '雏鸟', icon: 'birds' },
  { path: '/cloud/my', label: '我的', icon: 'my' }
];

const isActive = (path: string): boolean => {
  return route.path === path || route.path.startsWith(path + '/');
};

const isHidden = computed(() => {
  const hiddenPaths = ['/cloud/login', '/cloud/register'];
  return hiddenPaths.includes(route.path);
});

const handleNavClick = (item: NavItem) => {
  if (route.path !== item.path) {
    router.push(item.path);
  }
};
</script>

<style scoped>
.bottom-nav {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  z-index: 1000;
  transition: transform 0.3s ease;
}

.bottom-nav.is-hidden {
  transform: translateY(100%);
}

.nav-blur-bg {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(255, 255, 255, 0.65);
  backdrop-filter: blur(24px) saturate(180%);
  -webkit-backdrop-filter: blur(24px) saturate(180%);
  border-top: 1px solid rgba(255, 255, 255, 0.3);
  box-shadow: 0 -4px 20px rgba(0, 0, 0, 0.05);
}

.nav-content {
  position: relative;
  display: flex;
  justify-content: space-around;
  align-items: center;
  padding: 8px 0 4px;
}

.nav-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 4px 16px;
  text-decoration: none;
  color: #9CA3AF;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  position: relative;
  min-width: 64px;
}

.nav-item:active {
  transform: scale(0.95);
}

.nav-item.is-active {
  color: #8BAD42;
}

.nav-icon-wrapper {
  position: relative;
  width: 28px;
  height: 28px;
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 4px;
  transition: all 0.3s ease;
}

.nav-item.is-active .nav-icon-wrapper {
  transform: translateY(-2px);
}

.nav-icon {
  width: 24px;
  height: 24px;
  transition: all 0.3s ease;
}

.nav-item.is-active .nav-icon {
  filter: drop-shadow(0 2px 4px rgba(139, 173, 66, 0.3));
}

.nav-label {
  font-size: 11px;
  font-weight: 500;
  transition: all 0.3s ease;
  white-space: nowrap;
}

.nav-item.is-active .nav-label {
  font-weight: 600;
}

.nav-indicator {
  position: absolute;
  top: 0;
  left: 50%;
  transform: translateX(-50%);
  width: 20px;
  height: 3px;
  background: linear-gradient(90deg, #8BAD42, #A4C65F);
  border-radius: 0 0 2px 2px;
  opacity: 0;
  animation: slideDown 0.3s ease forwards;
}

@keyframes slideDown {
  from {
    opacity: 0;
    transform: translateX(-50%) translateY(-4px);
  }
  to {
    opacity: 1;
    transform: translateX(-50%) translateY(0);
  }
}

.safe-area-bottom {
  height: env(safe-area-inset-bottom, 0px);
  min-height: 8px;
}

/* 触摸滑动支持 */
@media (hover: none) and (pointer: coarse) {
  .nav-item {
    -webkit-tap-highlight-color: transparent;
  }
  
  .nav-item:active .nav-icon-wrapper {
    transform: scale(0.9);
  }
}

/* 适配不同屏幕尺寸 */
@media (min-width: 390px) {
  .nav-icon-wrapper {
    width: 32px;
    height: 32px;
  }
  
  .nav-icon {
    width: 26px;
    height: 26px;
  }
  
  .nav-label {
    font-size: 12px;
  }
}

@media (min-width: 428px) {
  .nav-item {
    padding: 6px 20px;
  }
  
  .nav-icon-wrapper {
    width: 34px;
    height: 34px;
  }
  
  .nav-icon {
    width: 28px;
    height: 28px;
  }
}

/* 强制浅色模式 - 移除深色模式支持 */
</style>
