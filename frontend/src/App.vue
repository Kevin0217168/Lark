<template>
  <!-- 移动端界面 -->
  <div v-if="isMobile" class="mobile-app">
    <!-- 登录和注册页面使用RouterView -->
    <template v-if="isAuthPage">
      <RouterView v-slot="{ Component }">
        <component :is="Component" />
      </RouterView>
    </template>
    <!-- 其他页面使用MobilePage -->
    <template v-else>
      <MobilePage @tabChange="handleTabChange" :activeTab="activeTab" />
    </template>
  </div>
  
  <!-- 桌面端界面 -->
  <div v-else>
    <el-image
      class="banner"
      src="banner.jpg"
      fit="cover"
    />
    <el-container class="app-container">
      <el-header>
        <Header />
      </el-header>
      <Sider @tabChange="handleTabChange" />
      <el-main>
        <RouterView v-slot="{ Component }">
          <component :is="Component" :activeTab="activeTab" />
        </RouterView>
      </el-main>
      <el-footer> © 2026 </el-footer>
    </el-container>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted, onUnmounted, computed } from "vue";
import { el } from "element-plus/es/locale/index.mjs";
import Header from "./components/Header.vue";
import Login from "./components/Login.vue";
import Sider from "./components/Sider.vue";
import MobilePage from "./pages/MobilePage.vue";
import { shouldUseMobilePage } from "./utils/mobileAdapter";

import { RouterView, RouterLink, useRoute } from "vue-router";

const route = useRoute();
const activeTab = ref<string>('realtime');
const isMobile = ref<boolean>(false);

// 检查是否是登录或注册页面
const isAuthPage = computed(() => {
  return route.path === '/Login' || route.path === '/Register';
});

// 检查是否需要使用移动端页面
const checkMobileStatus = () => {
  isMobile.value = shouldUseMobilePage();
};

const handleTabChange = (tab: string) => {
  activeTab.value = tab;
};

// 监听路由变化，从query参数中读取tab值
watch(() => route.query.tab, (newTab) => {
  if (newTab && typeof newTab === 'string') {
    activeTab.value = newTab;
  }
}, { immediate: true });

// 监听路由变化，滚动到页面顶部
watch(() => route.path, () => {
  window.scrollTo({ top: 0, behavior: 'smooth' });
});

// 监听窗口大小变化
const handleResize = () => {
  checkMobileStatus();
};

onMounted(() => {
  // 初始化时检查
  checkMobileStatus();
  // 添加窗口大小变化监听
  window.addEventListener('resize', handleResize);
});

onUnmounted(() => {
  // 移除监听器
  window.removeEventListener('resize', handleResize);
});
</script>

<style scoped>
/* Main */
main,
footer {
  display: flex;
  justify-content: center;
  align-items: center;
  text-align: center;
}

header {
  position: sticky;
  top: 0;
  overflow: visible;
  z-index: 1000;
}

.banner {
  position: absolute;
  z-index: 0;
  top: 0;
  left: 0;
  width: 100%;
  margin: 0;
  --el-header-padding: 0;
  height: 400px;
}

.content-container {
  display: flex;
  flex-direction: row;
  width: 100%;
}

/* 移动端样式 */
.mobile-app {
  width: 100%;
  min-height: 100vh;
}
</style>
