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
      <Sider v-if="!isAuthPage" @tabChange="handleTabChange" />
      <el-main>
        <RouterView v-slot="{ Component }">
          <component :is="Component" :activeTab="activeTab" />
        </RouterView>
      </el-main>
      <el-footer> 
        <div class="footer-content">
          <div class="copyright">© 2026 Lark. All Rights Reserved.</div>
          <div class="copyright">Powered by Vue.js & FastAPI</div>
          <div class="beian">
            <div class="beian-item">
              <img src="https://www.beian.gov.cn/img/ghs.png" class="beian-icon"> 
              <a class="beian-link" href="https://beian.mps.gov.cn/#/query/webSearch?code=41010502007493" rel="noreferrer" target="_blank">豫公网安备41010502007493号</a>
            </div>
            <div class="beian-item">
              <span class="beian-placeholder"></span>
              <a class="beian-link" href="https://beian.miit.gov.cn/" target="_blank">豫ICP备2026008104号</a>
            </div>
            <div class="beian-item">
              <img src="https://icp.gov.moe/images/ico64.png" class="beian-icon moe-icon">
              <a class="beian-link" target="_blank" href="https://icp.gov.moe/?keyword=20261324">萌ICP备20261324号</a>
            </div>
          </div>
        </div>
      </el-footer>
    </el-container>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, onMounted, onUnmounted, computed } from "vue";
import Header from "./components/Header.vue";
import Sider from "./components/Sider.vue";
import MobilePage from "./pages/MobilePage.vue";
import { shouldUseMobilePage } from "./utils/mobileAdapter";

import { RouterView, useRoute, useRouter } from "vue-router";
import { ElMessage } from "element-plus";

const route = useRoute();
const router = useRouter();
const activeTab = ref<string>('realtime');
const isMobile = ref<boolean>(false);

// 全局401错误处理
const handle401Error = () => {
  ElMessage.error('登录已过期，请重新登录');
  // 清除本地存储的token
  localStorage.removeItem('accessToken');
  localStorage.removeItem('refreshToken');
  // 同步清理登录状态相关字段
  localStorage.removeItem('isAuthenticated');
  localStorage.removeItem('username');
  localStorage.removeItem('role');
  // 通知其它组件登录状态已变更
  window.dispatchEvent(new Event('loginStatusChanged'));
  // 跳转到登录页面
  router.push('/Login');
};

// 监听所有fetch请求的401错误
const originalFetch: typeof window.fetch = window.fetch;
const wrappedFetch: typeof window.fetch = async (...args) => {
  const response = await originalFetch(...args as Parameters<typeof originalFetch>);
  
  // 排除登录、注册和刷新接口的401处理
  const url = args[0] as string;
  const isAuthPage = route.path === '/Login' || route.path === '/Register';
  const isAuthApi = url.includes('/api/login') || url.includes('/api/register') || url.includes('/api/refresh');
  
  if (response.status === 401 && !isAuthPage && !isAuthApi) {
    handle401Error();
  }
  
  return response;
};
window.fetch = wrappedFetch;

// 在组件卸载时恢复原fetch实现
onUnmounted(() => {
  // 仅当当前fetch仍为本组件包裹的实现时才恢复
  if (window.fetch === wrappedFetch) {
    window.fetch = originalFetch;
  }
});

// 在HMR时恢复原fetch实现
if ((import.meta as any).hot) {
  (import.meta as any).hot.dispose(() => {
    if (window.fetch === wrappedFetch) {
      window.fetch = originalFetch;
    }
  });
}

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

// 监听路由变化，从query参数中读取activeTab值
watch(() => route.query.activeTab, (newActiveTab) => {
  if (newActiveTab && typeof newActiveTab === 'string') {
    activeTab.value = newActiveTab;
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

footer {
  flex-direction: column;
  padding: 40px 20px 20px;
  background: transparent;
}

.footer-content {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
}

.copyright {
  font-size: 14px;
  color: #909399;
}

.beian {
  font-size: 12px;
  color: #909399;
}

.beian-item {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
  line-height: 1.6;
}

.beian-icon {
  width: 15px;
  height: 15px;
  display: inline-block;
  vertical-align: middle;
}

.beian-icon.moe-icon {
  width: 16px;
  height: 16px;
}

.beian-placeholder {
  width: 15px;
  height: 15px;
  display: inline-block;
}

.beian-link {
  color: #909399;
  text-decoration: none;
  transition: color 0.3s;
  vertical-align: middle;
}

.beian-link:hover {
  color: #8bad42;
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
  display: flex;
  flex-direction: column;
}


</style>
