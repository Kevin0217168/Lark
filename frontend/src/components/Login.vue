<template>
  <el-card class="login-card">
    <h3 class="title">用户登录</h3>

    <el-form :model="form" ref="formRef" :rules="rules">
      <el-form-item label="用户名" prop="username">
        <el-input v-model="form.username" placeholder="请输入用户名"></el-input>
      </el-form-item>

      <el-form-item label="密码" prop="password">
        <el-input v-model="form.password" type="password" placeholder="请输入密码" show-password></el-input>
      </el-form-item>

      <el-button style="width: 100%; margin-bottom: 10px;" @click="handleLogin" type="primary" :loading="loading">
        登录
      </el-button>
      
      <div class="register-link">
        <span>还没有账号？</span>
        <router-link to="/Register">去注册</router-link>
      </div>
    </el-form>
  </el-card>
</template>

<script lang="ts" setup>
import { ref, onMounted } from "vue";
import { useRouter } from "vue-router";
import { ElMessage } from "element-plus";

const router = useRouter();
const formRef = ref();
const loading = ref(false);

const form = ref({
  username: "",
  password: "",
});

const rules = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, max: 20, message: '长度在 3 到 20 个字符', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 6, max: 20, message: '长度在 6 到 20 个字符', trigger: 'blur' }
  ]
};

// API基础地址
// 使用相对路径，通过Vite代理解决CORS问题
const API_BASE_URL = '';

// 自动登录 - 尝试使用refresh token获取新的access token
const tryAutoLogin = async (): Promise<boolean> => {
  try {
    // 检查是否有登录状态
    const isAuthenticated = localStorage.getItem('isAuthenticated') === 'true';
    if (!isAuthenticated) {
      console.log('未登录，跳过自动登录');
      return false;
    }
    
    const response = await fetch(`${API_BASE_URL}/api/refresh`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
      },
      credentials: 'include', // 携带cookie
    });

    if (response.ok) {
      const data = await response.json();
      if (data.access_token) {
        // 自动登录成功，保存token
        localStorage.setItem('isAuthenticated', 'true');
        localStorage.setItem('accessToken', data.access_token);
        localStorage.setItem('tokenType', data.token_type || 'bearer');
        
        // 设置全局请求头（实际项目中应在请求拦截器中设置）
        console.log('自动登录成功，token已更新');
        return true;
      }
    }
    
    // 自动登录失败（401或其他状态码）
    // 清除登录状态
    localStorage.removeItem('isAuthenticated');
    localStorage.removeItem('username');
    localStorage.removeItem('accessToken');
    localStorage.removeItem('tokenType');
    return false;
  } catch (error) {
    console.error('自动登录失败:', error);
    // 清除登录状态
    localStorage.removeItem('isAuthenticated');
    localStorage.removeItem('username');
    localStorage.removeItem('accessToken');
    localStorage.removeItem('tokenType');
    return false;
  }
};

// 组件挂载时尝试自动登录
onMounted(async () => {
  // 检查是否是从退出登录跳转而来
  const isFromLogout = sessionStorage.getItem('isFromLogout') === 'true';
  
  if (!isFromLogout) {
    // 尝试自动登录
    const autoLoginSuccess = await tryAutoLogin();
    if (autoLoginSuccess) {
      ElMessage.success('自动登录成功');
      // 触发自定义事件，通知其他组件登录状态已变化
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
      // 跳转到首页
      router.push('/Home');
    }
    // 自动登录失败则留在登录页面，让用户手动登录
  } else {
    // 清除退出登录标记
    sessionStorage.removeItem('isFromLogout');
    console.log('从退出登录跳转而来，跳过自动登录');
  }
});

// 正常登录方法 - 连接后端API
// 后端接口地址: 通过Vite代理访问 /api/login
const handleLogin = async () => {
  try {
    await formRef.value.validate();
    

    
    loading.value = true;
    
    // 构建请求参数，符合OAuth2密码模式
    const params = new URLSearchParams();
    params.append('grant_type', 'password');
    params.append('username', form.value.username);
    params.append('password', form.value.password);
    params.append('scope', '');
    params.append('client_id', '');
    params.append('client_secret', '');
    
    // 给后端发送登录请求
    const response = await fetch(`${API_BASE_URL}/api/login`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
        'Accept': 'application/json',
      },
      body: params.toString(),
    });
    
    const data = await response.json();
    
    if (response.ok && data.access_token) {
      ElMessage.success("登录成功");
      
      // 设置登录状态
      localStorage.setItem('isAuthenticated', 'true');
      localStorage.setItem('username', form.value.username);
      localStorage.setItem('accessToken', data.access_token);
      localStorage.setItem('tokenType', data.token_type || 'bearer');
      
      // 触发自定义事件，通知其他组件登录状态已变化
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
      
      // 跳转到首页
      router.push('/Home');
    } else {
      // 处理错误响应
      const errorMsg = data.detail || data.msg || '登录失败，请检查用户名和密码';
      ElMessage.error(errorMsg);
    }
  } catch (error) {
    console.error('登录错误:', error);
    ElMessage.error('登录失败，请检查网络连接');
  } finally {
    loading.value = false;
  }
};


</script>

<style lang="scss" scoped>
/* Login Card */
.login-card {
  z-index: 1;
  padding-bottom: 30px;
  width: 400px;
  margin-top: 5%;
  border-radius: 5%;
}

.register-link {
  text-align: center;
  margin-top: 20px;
  font-size: 14px;
  color: #606266;
}

.register-link a {
  color: #8bad42;
  text-decoration: none;
  margin-left: 5px;
}

.register-link a:hover {
  text-decoration: underline;
}
</style>
