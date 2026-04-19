<template>
  <el-row justify="center" style="width: 100%; margin: 0; padding: 0;">
    <el-col :span="18" style="margin: 0; padding: 0;">
      <div class="header-card">
        <div class="header-left">
          <router-link to="/Home" custom v-slot="{ navigate }">
            <el-button @click="handleHomeClick(navigate)" style="color: #8bad42" text>
              <el-icon size="30">
                <grid />
              </el-icon>
              <p style="margin-left: 5px">Lark</p>
            </el-button>
          </router-link>
        </div>
        <div class="header-center">
          <router-link to="/Home" custom v-slot="{ navigate }">
            <el-button 
              @click="handleHomeClick(navigate)" 
              :style="{ color: isActive('/Home') ? '#8bad42' : '#000' }" 
              :type="isActive('/Home') ? 'primary' : 'default'"
              text
            > 
              总览 
            </el-button>
          </router-link>
          <router-link to="/Data" custom v-slot="{ navigate }">
            <el-button 
              @click="navigate" 
              :style="{ color: isActive('/Data') ? '#8bad42' : '#000' }" 
              :type="isActive('/Data') ? 'primary' : 'default'"
              text
            > 
              数据 
            </el-button>
          </router-link>
          <router-link to="/Device" custom v-slot="{ navigate }">
            <el-button 
              @click="navigate" 
              :style="{ color: isActive('/Device') ? '#8bad42' : '#000' }" 
              :type="isActive('/Device') ? 'primary' : 'default'"
              text
            > 
              设备 
            </el-button>
          </router-link>
          <router-link to="/birds" custom v-slot="{ navigate }">
            <el-button 
              @click="navigate" 
              :style="{ color: isActive('/birds') ? '#8bad42' : '#000' }" 
              :type="isActive('/birds') ? 'primary' : 'default'"
              text
            > 
              雏鸟管理 
            </el-button>
          </router-link>
          <router-link v-if="userRole === 'root'" to="/UserManage" custom v-slot="{ navigate }">
            <el-button 
              @click="navigate" 
              :style="{ color: isActive('/UserManage') ? '#8bad42' : '#000' }" 
              :type="isActive('/UserManage') ? 'primary' : 'default'"
              text
            > 
              用户管理 
            </el-button>
          </router-link>
        </div>
        <div class="header-right">
          <template v-if="!isLoggedIn">
            <router-link to="/Login" custom v-slot="{ navigate }">
              <el-button @click="navigate" color="#8bad42" plain>登录</el-button>
            </router-link>
            <router-link to="/Register" custom v-slot="{ navigate }">
              <el-button @click="navigate" text>注册</el-button>
            </router-link>
          </template>
          <div v-else class="avatar-header" @click="navigateToProfile">
            <div class="avatar-circle-header">
              <img v-if="avatar" :src="avatar" :alt="username" />
              <div v-else class="default-avatar-header">
                {{ username ? username.charAt(0) : '?' }}
              </div>
            </div>
          </div>
        </div>
      </div>
    </el-col>
  </el-row>
</template>

<script lang="ts" setup>
import { ref, onMounted, onUnmounted } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import { useDeviceStore } from '../stores/deviceStore';

const route = useRoute();
const router = useRouter();
const { isFullscreen, setFullscreen } = useDeviceStore();

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

// 处理登录状态变化事件
const handleLoginStatusChanged = () => {
  checkLoginStatus();
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
  
  // 保存回调函数引用以便移除
  (window as any).__headerStorageHandler = handleStorageChange;
});

// 组件卸载时移除事件监听
onUnmounted(() => {
  if ((window as any).__headerStorageHandler) {
    window.removeEventListener('storage', (window as any).__headerStorageHandler);
    delete (window as any).__headerStorageHandler;
  }
  window.removeEventListener('loginStatusChanged', handleLoginStatusChanged);
});

const isActive = (path: string) => {
  return route.path === path;
};

const handleHomeClick = (navigate: () => void) => {
  // 检查当前是否处于全屏状态
  if (isFullscreen.value) {
    // 退出全屏状态
    setFullscreen(false);
    // 清除URL中的fullscreen参数
    router.replace({ query: {} });
  } else {
    // 直接导航
    navigate();
  }
};

// 清除cookie的函数
const clearCookies = () => {
  // 清除所有可能的登录相关cookie
  const cookies = document.cookie.split(';');
  for (let cookie of cookies) {
    const eqPos = cookie.indexOf('=');
    const name = eqPos > -1 ? cookie.substring(0, eqPos).trim() : cookie.trim();
    if (name) {
      // 尝试多种方式清除cookie
      document.cookie = `${name}=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/`;
      document.cookie = `${name}=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=${window.location.hostname}`;
      document.cookie = `${name}=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=.${window.location.hostname.split('.').slice(-2).join('.')}`;
    }
  }
  
  // 确保localStorage和sessionStorage也被清除
  localStorage.removeItem('isAuthenticated');
  localStorage.removeItem('username');
  localStorage.removeItem('accessToken');
  localStorage.removeItem('tokenType');
  localStorage.removeItem('avatar');
  localStorage.removeItem('role');
  localStorage.removeItem('userId');
  sessionStorage.removeItem('isFromLogout');
};

// 导航到个人信息页面
const navigateToProfile = () => {
  router.push('/Profile');
};

// 退出登录功能
const handleLogout = () => {
  // 显示退出成功提示
  ElMessage.success('已退出登录');
  
  // 清除所有登录相关信息
  clearCookies();
  
  // 立即更新登录状态
  checkLoginStatus();
  
  // 触发自定义事件，通知其他组件登录状态已变化
  window.dispatchEvent(new CustomEvent('loginStatusChanged'));
  
  // 重定向到登录页面
  router.push('/Login');
};
</script>

<style lang="scss" scoped>
.header-card {
  height: 70px;
  z-index: 1000;
  display: flex;
  align-items: center;
  justify-content: space-between;
  border-bottom-left-radius: 15px;
  border-bottom-right-radius: 15px;
  background-color: #fff;
  box-shadow: 5px 5px 10px rgba(0, 0, 0, 0.1);
}

.header-left {
  flex: 0 0 auto;
  padding-left: 20px;
}

.header-center {
  flex: 1;
  display: flex;
  justify-content: center;
  gap: 20px;
}

.header-right {
  flex: 0 0 auto;
  display: flex;
  align-items: center;
  gap: 10px;
  padding-right: 20px;
}
.el-button {
  height: 40px;
  font-size: 16px;
  text-align: center;
  align-items: center;
}

.avatar-header {
  display: flex;
  justify-content: center;
  align-items: center;
  cursor: pointer;
}

.avatar-circle-header {
  width: 36px;
  height: 36px;
  border-radius: 50%;
  overflow: hidden;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  
  img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }
}

.default-avatar-header {
  width: 100%;
  height: 100%;
  background-color: #8bad42;
  display: flex;
  justify-content: center;
  align-items: center;
  color: white;
  font-size: 16px;
  font-weight: bold;
}

.avatar-header:hover .avatar-circle-header {
  box-shadow: 0 2px 8px rgba(139, 173, 66, 0.3);
}
</style>
