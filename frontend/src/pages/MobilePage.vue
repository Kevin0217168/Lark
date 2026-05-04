<template>
  <div class="mobile-page">
    <!-- 移动端头部 -->
    <div class="mobile-header">
      <div class="header-left">
        <router-link to="/Home" custom v-slot="{ navigate }">
          <div class="logo" @click="navigate">
            <el-icon size="24">
              <grid />
            </el-icon>
            <span>Lark</span>
          </div>
        </router-link>
      </div>
      <div class="header-right">
        <div v-if="isLoggedIn" class="avatar" @click="navigateToProfile">
          <img v-if="avatar" :src="avatar" :alt="username" />
          <div v-else class="default-avatar">
            {{ username ? username.charAt(0) : '?' }}
          </div>
        </div>
        <div v-else class="auth-buttons">
          <el-button type="primary" size="small" @click="goToLogin">登录</el-button>
          <el-button size="small" @click="goToRegister">注册</el-button>
        </div>
      </div>
    </div>

    <!-- 移动端顶部菜单（对应Sider功能） -->
    <div v-if="isDataPage || isDevicePage" class="mobile-top-menu">
      <div class="menu-container">
        <!-- 数据页面菜单 -->
        <div v-if="isDataPage" class="menu-tabs">
          <div 
            class="menu-tab" 
            :class="{ active: activeTab === 'realtime' }"
            @click="handleTabSelect('realtime')"
          >
            实时
          </div>
          <div 
            class="menu-tab" 
            :class="{ active: activeTab === 'analysis' }"
            @click="handleTabSelect('analysis')"
          >
            分析
          </div>
          <div 
            class="menu-tab" 
            :class="{ active: activeTab === 'history' }"
            @click="handleTabSelect('history')"
          >
            历史数据
          </div>
        </div>
        
        <!-- 设备页面菜单 -->
        <div v-else-if="isDevicePage" class="menu-tabs">
          <div 
            class="menu-tab" 
            :class="{ active: activeTab === 'overview' }"
            @click="handleTabSelect('overview')"
          >
            总览
          </div>
          <div 
            class="menu-tab" 
            :class="{ active: activeTab === 'management' }"
            @click="handleTabSelect('management')"
          >
            管理
          </div>
          <div 
            class="menu-tab" 
            :class="{ active: activeTab === 'logs' }"
            @click="handleTabSelect('logs')"
          >
            日志
          </div>
        </div>
      </div>
    </div>

    <!-- 移动端内容 -->
    <div class="mobile-content">
      <!-- 根据路由显示不同的移动端组件 -->
      <HomePage v-if="route.path === '/Home'" />
      <Data v-else-if="route.path === '/Data' || route.path === '/Stream'" :activeTab="activeTab" />
      <DevicePage v-else-if="route.path === '/Device'" :activeTab="activeTab" />
      <BirdsPage v-else-if="route.path === '/birds'" />
      <ProfilePage v-else-if="route.path === '/Profile'" />
      <UserManage v-else-if="route.path === '/UserManage' && isAdmin" />
      <AirQualityDetail v-else-if="route.path === '/air-quality'" />
      <div v-else class="mobile-placeholder">
        <h2>移动端页面</h2>
        <p>内容区域</p>
      </div>
      
      <!-- 备案信息 -->
      <div class="mobile-beian">
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
    </div>

    <!-- 移动端底部导航 -->
    <div class="mobile-bottom-nav">
      <router-link to="/Home" custom v-slot="{ navigate }">
        <div 
          class="nav-item" 
          :class="{ active: isActive('/Home') }"
          @click="navigate"
        >
          <el-icon><HomeFilled /></el-icon>
          <span>总览</span>
        </div>
      </router-link>
      <router-link to="/Data" custom v-slot="{ navigate }">
        <div 
          class="nav-item" 
          :class="{ active: isActive('/Data') }"
          @click="navigate"
        >
          <el-icon><DataAnalysis /></el-icon>
          <span>数据</span>
        </div>
      </router-link>
      <router-link to="/Device" custom v-slot="{ navigate }">
        <div 
          class="nav-item" 
          :class="{ active: isActive('/Device') }"
          @click="navigate"
        >
          <el-icon><Monitor /></el-icon>
          <span>设备</span>
        </div>
      </router-link>
      <router-link to="/birds" custom v-slot="{ navigate }">
        <div 
          class="nav-item" 
          :class="{ active: isActive('/birds') }"
          @click="navigate"
        >
          <el-icon><Avatar /></el-icon>
          <span>雏鸟管理</span>
        </div>
      </router-link>
      <div 
        class="nav-item" 
        :class="{ active: isActive('/Profile') }"
        @click="navigateToProfile"
      >
        <el-icon><User /></el-icon>
        <span>{{ isLoggedIn ? '我的' : '登录' }}</span>
      </div>
    </div>

  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { HomeFilled, DataAnalysis, Monitor, User, Grid, Avatar } from '@element-plus/icons-vue';
import HomePage from '../components/HomePage.vue';
import Data from '../components/Data.vue';
import DevicePage from '../components/DevicePage.vue';
import ProfilePage from '../components/ProfilePage.vue';
import UserManage from '../components/UserManage.vue';
import BirdsPage from '../components/BirdsPage.vue';
import AirQualityDetail from '../components/AirQualityDetail.vue';

const route = useRoute();
const router = useRouter();

// 接收activeTab属性
const props = defineProps<{
  activeTab?: string;
}>();

// 菜单相关
const activeTab = ref<string>(props.activeTab || 'realtime');

// 暴露给父组件的事件
const emit = defineEmits(['tabChange']);

const isDataPage = computed(() => route.path === '/Stream' || route.path === '/Data');
const isDevicePage = computed(() => route.path === '/Device');
const isAdmin = computed(() => userRole.value === 'root');

// 处理菜单选择
const handleTabSelect = (key: string) => {
  activeTab.value = key;
  emit('tabChange', key);
};

// 响应式登录状态
const isLoggedIn = ref(false);
const username = ref('');
const avatar = ref('');
const userRole = ref('');

// 检查是否已登录
const checkLoginStatus = () => {
  isLoggedIn.value = localStorage.getItem('isAuthenticated') === 'true';
  username.value = localStorage.getItem('username') || '';
  avatar.value = localStorage.getItem('avatar') || '';
  userRole.value = localStorage.getItem('role') || '';
};

// 跳转到登录页
const goToLogin = () => {
  router.push('/Login');
};

// 跳转到注册页
const goToRegister = () => {
  router.push('/Register');
};

// 处理登录状态变化事件
const handleLoginStatusChanged = () => {
  checkLoginStatus();
};

// 监听路由变化，重置默认激活项
const handleRouteChange = (newPath: string) => {
  if (newPath === '/Stream' || newPath === '/Data') {
    // 检查是否有 activeTab 查询参数
    const activeTabParam = route.query.activeTab as string;
    if (activeTabParam && ['realtime', 'analysis', 'history'].includes(activeTabParam)) {
      activeTab.value = activeTabParam;
    } else {
      activeTab.value = 'realtime';
    }
  } else if (newPath === '/Device') {
    // 检查是否有 tab 查询参数
    const tabParam = route.query.tab as string;
    if (tabParam && ['overview', 'management', 'logs'].includes(tabParam)) {
      activeTab.value = tabParam;
    } else {
      activeTab.value = 'overview';
    }
  }
  emit('tabChange', activeTab.value);
};

// 监听查询参数变化
const handleQueryChange = (newTab: any) => {
  if (route.path === '/Device' && newTab) {
    const tabParam = newTab as string;
    if (['overview', 'management', 'logs'].includes(tabParam)) {
      activeTab.value = tabParam;
      emit('tabChange', activeTab.value);
    }
  }
};

// 监听 activeTab 查询参数变化
const handleActiveTabQueryChange = (newActiveTab: any) => {
  if ((route.path === '/Stream' || route.path === '/Data') && newActiveTab) {
    const activeTabParam = newActiveTab as string;
    if (['realtime', 'analysis', 'history'].includes(activeTabParam)) {
      activeTab.value = activeTabParam;
      emit('tabChange', activeTab.value);
    }
  }
};

// 组件挂载时检查登录状态并监听事件
onMounted(() => {
  checkLoginStatus();
  
  // 监听localStorage变化
  const handleStorageChange = (event: StorageEvent) => {
    if (event.key === 'isAuthenticated' || event.key === 'role') {
      checkLoginStatus();
    }
  };
  window.addEventListener('storage', handleStorageChange);
  
  // 监听自定义事件，立即响应登录状态变化
  window.addEventListener('loginStatusChanged', handleLoginStatusChanged);
  
  // 监听路由变化
  watch(() => route.path, handleRouteChange, { immediate: true });
  
  // 监听查询参数变化
  watch(() => route.query.tab, handleQueryChange);
  
  // 监听 activeTab 查询参数变化
  watch(() => route.query.activeTab, handleActiveTabQueryChange);
  
  // 保存回调函数引用以便移除
  (window as any).__mobileStorageHandler = handleStorageChange;
});

// 组件卸载时移除事件监听
onUnmounted(() => {
  if ((window as any).__mobileStorageHandler) {
    window.removeEventListener('storage', (window as any).__mobileStorageHandler);
    delete (window as any).__mobileStorageHandler;
  }
  window.removeEventListener('loginStatusChanged', handleLoginStatusChanged);
});

const isActive = (path: string) => {
  return route.path === path;
};

// 导航到个人信息页面
const navigateToProfile = () => {
  if (isLoggedIn.value) {
    router.push('/Profile');
  } else {
    router.push('/Login');
  }
};
</script>

<style scoped>
.mobile-page {
  width: 100%;
  min-height: 100vh;
  background: #f5f7fa;
  display: flex;
  flex-direction: column;
}

/* 移动端头部 */
.mobile-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: white;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  position: sticky;
  top: 0;
  z-index: 100;
}

.logo {
  display: flex;
  align-items: center;
  color: #8bad42;
  font-weight: bold;
  font-size: 18px;
  cursor: pointer;
}

.logo .el-icon {
  margin-right: 8px;
}

.avatar {
  width: 32px;
  height: 32px;
  border-radius: 50%;
  overflow: hidden;
  cursor: pointer;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.avatar img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.default-avatar {
  width: 100%;
  height: 100%;
  background-color: #8bad42;
  display: flex;
  justify-content: center;
  align-items: center;
  color: white;
  font-size: 14px;
  font-weight: bold;
}

.auth-buttons {
  display: flex;
  gap: 8px;
}

/* 移动端顶部菜单 */
.mobile-top-menu {
  background: white;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  position: sticky;
  top: 56px;
  z-index: 90;
}

.menu-container {
  padding: 0 16px;
}

.menu-tabs {
  display: flex;
  justify-content: center;
  overflow-x: auto;
  white-space: nowrap;
  padding: 12px 0;
  gap: 16px;
}

.menu-tab {
  padding: 8px 16px;
  font-size: 14px;
  font-weight: 500;
  color: #606266;
  border-radius: 20px;
  cursor: pointer;
  transition: all 0.3s;
  flex-shrink: 0;
}

.menu-tab.active {
  background: #8bad42;
  color: white;
}

.menu-tab:active {
  opacity: 0.8;
}

/* 隐藏滚动条 */
.menu-tabs::-webkit-scrollbar {
  display: none;
}

.menu-tabs {
  -ms-overflow-style: none;
  scrollbar-width: none;
}

/* 移动端内容 */
.mobile-content {
  flex: 1;
  padding: 16px;
  overflow-y: auto;
}

.mobile-placeholder {
  text-align: center;
  padding: 40px 0;
  color: #999;
}

/* 移动端底部导航 */
.mobile-bottom-nav {
  display: flex;
  justify-content: space-around;
  align-items: center;
  height: 56px;
  background: white;
  box-shadow: 0 -2px 10px rgba(0, 0, 0, 0.1);
  position: sticky;
  bottom: 0;
  z-index: 100;
}

.nav-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  flex: 1;
  height: 100%;
  color: #999;
  font-size: 12px;
  cursor: pointer;
  transition: all 0.3s;
}

.nav-item .el-icon {
  font-size: 20px;
  margin-bottom: 2px;
}

.nav-item.active {
  color: #8bad42;
}

.nav-item:active {
  background: #f5f5f5;
}

/* 移动端备案信息 */
.mobile-beian {
  background: transparent;
  padding: 16px;
  text-align: center;
  margin-top: 20px;
}

.mobile-beian .copyright {
  font-size: 12px;
  color: #909399;
  margin-bottom: 8px;
}

.mobile-beian .beian {
  font-size: 11px;
  color: #909399;
}

.mobile-beian .beian-item {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
  line-height: 1.6;
}

.mobile-beian .beian-icon {
  width: 15px;
  height: 15px;
  display: inline-block;
  vertical-align: middle;
}

.mobile-beian .beian-icon.moe-icon {
  width: 16px;
  height: 16px;
}

.mobile-beian .beian-placeholder {
  width: 15px;
  height: 15px;
  display: inline-block;
}

.mobile-beian .beian-link {
  color: #909399;
  text-decoration: none;
  vertical-align: middle;
}

.mobile-beian .beian-link:active {
  color: #8bad42;
}
</style>
