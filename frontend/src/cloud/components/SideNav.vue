<template>
  <nav class="side-nav">
    <div class="side-nav-header">
      <div class="nav-logo">
        <svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="nav-logo-icon" aria-hidden="true">
          <path d="M16 7h.01"></path>
          <path d="M3.4 18H12a8 8 0 0 0 8-8V7a4 4 0 0 0-7.28-2.3L2 20"></path>
          <path d="m20 7 2 .5-2 .5"></path>
          <path d="M10 18v3"></path>
          <path d="M14 17.75V21"></path>
          <path d="M7 18a6 6 0 0 0 3.84-10.61"></path>
        </svg>
        <span class="nav-brand">云养鸟</span>
      </div>
    </div>

    <ul class="nav-list">
      <li v-for="item in navItems" :key="item.path">
        <router-link :to="item.path" class="nav-link" :class="{ 'is-active': isActive(item.path) }">
          <svg class="nav-icon" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
            <path v-if="item.icon === 'home'" d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
            <path v-if="item.icon === 'home'" d="M9 22V12h6v10" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
            
            <path v-if="item.icon === 'birds'" d="M12 2C8 2 5 5 5 9c0 3 2 5 4 6v2c0 1 1 2 2 2h2c1 0 2-1 2-2v-2c2-1 4-3 4-6 0-4-3-7-7-7z" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
            <path v-if="item.icon === 'birds'" d="M9 9h6M9 12h6" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
            
            <path v-if="item.icon === 'my'" d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
            <circle v-if="item.icon === 'my'" cx="12" cy="7" r="4" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
          </svg>
          <span class="nav-label">{{ item.label }}</span>
        </router-link>
      </li>
    </ul>
  </nav>
</template>

<script setup lang="ts">
import { useRoute } from 'vue-router';

const route = useRoute();

interface NavItem {
  path: string;
  label: string;
  icon: 'home' | 'birds' | 'my';
}

const navItems: NavItem[] = [
  { path: '/cloud/home', label: '首页', icon: 'home' },
  { path: '/cloud/birds', label: '我的雏鸟', icon: 'birds' },
  { path: '/cloud/my', label: '个人中心', icon: 'my' }
];

const isActive = (path: string): boolean => {
  return route.path === path || route.path.startsWith(path + '/');
};
</script>

<style scoped>
.side-nav {
  width: 220px;
  min-height: 100vh;
  background: linear-gradient(180deg, #f0fdf4 0%, #ecfdf5 40%, #f8fafc 100%);
  border-right: 1px solid rgba(22, 163, 74, 0.08);
  display: flex;
  flex-direction: column;
  flex-shrink: 0;
  padding: 28px 0;
  box-shadow: 2px 0 24px rgba(0, 0, 0, 0.03);
  position: sticky;
  top: 0;
}

.side-nav-header {
  padding: 0 20px 24px;
  border-bottom: 1px solid rgba(22, 163, 74, 0.08);
  margin-bottom: 16px;
}

.nav-logo {
  display: flex;
  align-items: center;
  gap: 12px;
}

.nav-logo-icon {
  color: #16a34a;
  flex-shrink: 0;
}

.nav-brand {
  font-size: 20px;
  font-weight: 800;
  color: #166534;
  letter-spacing: 1px;
  background: linear-gradient(135deg, #16a34a, #15803d);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.nav-list {
  list-style: none;
  margin: 0;
  padding: 0 12px;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.nav-link {
  display: flex;
  align-items: center;
  gap: 14px;
  padding: 12px 16px;
  border-radius: 12px;
  text-decoration: none;
  color: #4b5563;
  font-size: 15px;
  font-weight: 500;
  transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
}

.nav-link:hover {
  background: rgba(22, 163, 74, 0.08);
  color: #166534;
  transform: translateX(2px);
}

.nav-link.is-active {
  background: linear-gradient(135deg, rgba(22, 163, 74, 0.12), rgba(21, 128, 61, 0.08));
  color: #16a34a;
  font-weight: 600;
  box-shadow: 0 2px 8px rgba(22, 163, 74, 0.1);
}

.nav-icon {
  width: 22px;
  height: 22px;
  flex-shrink: 0;
  transition: all 0.25s ease;
}

.nav-link.is-active .nav-icon {
  filter: drop-shadow(0 2px 4px rgba(22, 163, 74, 0.3));
}

.nav-label {
  white-space: nowrap;
}
</style>
