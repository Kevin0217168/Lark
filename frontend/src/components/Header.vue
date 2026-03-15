<template>
  <el-row justify="center" style="width: 100%; margin: 0; padding: 0;">
    <el-col :span="18" style="margin: 0; padding: 0;">
      <el-row class="header-card">
        <el-col :span="3">
          <router-link to="/Home" custom v-slot="{ navigate }">
            <el-button @click="handleHomeClick(navigate)" style="color: #8bad42" text>
              <el-icon size="30">
                <grid />
              </el-icon>
              <p style="margin-left: 5px">Lark</p>
            </el-button>
          </router-link>
        </el-col>
        <el-col :span="2" :offset="6">
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
        </el-col>
        <el-col :span="2">
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
        </el-col>
        <el-col :span="2">
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
        </el-col>
        <el-col :span="4" :offset="1" v-if="isLoggedIn">
          <span class="username-display">用户：{{ username }}</span>
        </el-col>
        <el-col :span="2" :offset="isLoggedIn ? 1 : 5">
          <router-link v-if="!isLoggedIn" to="/Login" custom v-slot="{ navigate }">
            <el-button @click="navigate" color="#8bad42" plain>登录</el-button>
          </router-link>
          <el-button v-else @click="handleLogout" color="#f56c6c" plain>退出登录</el-button>
        </el-col>
        <el-col :span="2" v-if="!isLoggedIn">
          <router-link to="/Register" custom v-slot="{ navigate }">
            <el-button @click="navigate" text>注册</el-button>
          </router-link>
        </el-col>
      </el-row>
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

// 检查是否已登录
const checkLoginStatus = () => {
  isLoggedIn.value = localStorage.getItem('isAuthenticated') === 'true';
  username.value = localStorage.getItem('username') || '';
};

// 处理登录状态变化事件
const handleLoginStatusChanged = () => {
  checkLoginStatus();
};

// 组件挂载时检查登录状态并监听事件
onMounted(() => {
  checkLoginStatus();
  
  // 监听localStorage变化
  window.addEventListener('storage', (event) => {
    if (event.key === 'isAuthenticated') {
      checkLoginStatus();
    }
  });
  
  // 监听自定义事件，立即响应登录状态变化
  window.addEventListener('loginStatusChanged', handleLoginStatusChanged);
});

// 组件卸载时移除事件监听
onUnmounted(() => {
  window.removeEventListener('storage', handleLoginStatusChanged);
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

// 退出登录功能
const handleLogout = () => {
  // 清除登录状态
  localStorage.removeItem('isAuthenticated');
  localStorage.removeItem('username');
  
  // 立即更新登录状态
  checkLoginStatus();
  
  // 触发自定义事件，通知其他组件登录状态已变化
  window.dispatchEvent(new CustomEvent('loginStatusChanged'));
  
  // 显示退出成功提示
  ElMessage.success('已退出登录');
  
  // 重定向到登录页面
  router.push('/Login');
};
</script>

<style lang="scss" scoped>
.header-card {
  height: 70px;
  z-index: 1000;
  text-align: center;
  align-items: center;
  border-bottom-left-radius: 15px;
  border-bottom-right-radius: 15px;
  background-color: #fff;
  box-shadow: 5px 5px 10px rgba(0, 0, 0, 0.1);
}
.el-button {
  height: 40px;
  font-size: 16px;
  text-align: center;
  align-items: center;
}

.username-display {
  font-size: 14px;
  color: #606266;
  font-weight: 500;
  white-space: nowrap;
  display: inline-block;
  line-height: 40px;
  margin-right: 30px;
}
</style>
