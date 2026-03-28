<template>
  <div class="login-container" :class="{ 'mobile': isMobile }">
    <!-- 桌面端：使用 el-card 包装 -->
    <el-card v-if="!isMobile" class="login-card">
      <h3 class="title">云雀管理系统 - 登录</h3>

      <el-form :model="form" ref="formRef" :rules="rules">
        <el-form-item label="用户名" prop="username">
          <el-input v-model="form.username" placeholder="请输入用户名"></el-input>
        </el-form-item>

        <el-form-item label="密码" prop="password">
          <el-input v-model="form.password" type="password" placeholder="请输入密码" show-password @keyup.enter="handleLogin"></el-input>
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

    <!-- 移动端：全屏设计 -->
    <div v-else class="mobile-login">
      <div class="login-header">
        <h1>鸟厂后台管理</h1>
        <p>欢迎回来，请登录您的账号</p>
      </div>

      <div class="login-form-container">
        <el-form :model="form" ref="formRef" :rules="rules" class="login-form">
          <el-form-item prop="username">
            <el-input 
              v-model="form.username" 
              placeholder="请输入用户名"
              prefix-icon="User"
              size="large"
            />
          </el-form-item>

          <el-form-item prop="password">
            <el-input 
              v-model="form.password" 
              type="password" 
              placeholder="请输入密码"
              prefix-icon="Lock"
              size="large"
              show-password
              @keyup.enter="handleLogin"
            />
          </el-form-item>

          <el-button 
            type="primary" 
            size="large" 
            class="login-btn"
            @click="handleLogin" 
            :loading="loading"
          >
            登录
          </el-button>
        </el-form>

        <div class="register-link">
          <span>还没有账号？</span>
          <router-link to="/Register">去注册</router-link>
        </div>

        <!-- 底部备案信息 -->
        <div class="mobile-footer">
          <div class="copyright">© 2026</div>
          <div class="beian">
            <img src="https://www.beian.gov.cn/img/ghs.png" style="width: 15px; height: 15px;display: inline-block;margin-bottom: 4px;"> 
            <a class="beian-link" href="https://beian.mps.gov.cn/#/query/webSearch?code=41010502007493" rel="noreferrer" target="_blank">豫公网安备41010502007493号</a><br> 
            <a class="beian-link" href="https://beian.miit.gov.cn/" target="_blank">豫ICP备2026008104号</a><br>
            <img src="https://icp.gov.moe/images/ico64.png" style="width: 16px; height: 16px;display: inline-block;margin-bottom: 2px;">
            <a class="beian-link" target="_blank" href="https://icp.gov.moe/?keyword=20261324">萌ICP备20261324号</a>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from "vue";
import { useRouter } from "vue-router";
import { ElMessage } from "element-plus";
import { api } from '../utils/api';

const router = useRouter();
const formRef = ref();
const loading = ref(false);

// 检测是否为移动设备
const isMobile = ref(window.innerWidth < 768);

// 监听窗口大小变化
const handleWindowResize = () => {
  isMobile.value = window.innerWidth < 768;
};

const form = ref({
  username: "",
  password: "",
});

const rules = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, max: 20, message: '长度在 3 到 20 个字符', trigger: 'blur' },
    { pattern: /^[a-zA-Z0-9][a-zA-Z0-9_]*[a-zA-Z0-9]$/, message: '用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 8, max: 32, message: '长度在 8 到 32 个字符', trigger: 'blur' },
    { pattern: /^(?=.*[a-zA-Z0-9])[a-zA-Z0-9]+$/, message: '密码必须包含至少一个数字或字母', trigger: 'blur' }
  ]
};

const API_BASE_URL = '';

const tryAutoLogin = async (): Promise<boolean> => {
  try {
    const isAuthenticated = localStorage.getItem('isAuthenticated') === 'true';
    if (!isAuthenticated) {
      return false;
    }
    
    const data = await api.post('/api/refresh', {}, {
      headers: {
        'Accept': 'application/json',
      }
    });
    
    if (data.access_token) {
      localStorage.setItem('isAuthenticated', 'true');
      localStorage.setItem('accessToken', data.access_token);
      localStorage.setItem('tokenType', data.token_type || 'bearer');
      if (!localStorage.getItem('username')) {
        localStorage.setItem('username', '');
      }
      if (!localStorage.getItem('avatar')) {
        localStorage.setItem('avatar', '');
      }
      return true;
    }
    
    localStorage.removeItem('isAuthenticated');
    localStorage.removeItem('username');
    localStorage.removeItem('accessToken');
    localStorage.removeItem('tokenType');
    localStorage.removeItem('avatar');
    localStorage.removeItem('role');
    localStorage.removeItem('userId');
    return false;
  } catch (error) {
    console.error('自动登录失败:', error);
    localStorage.removeItem('isAuthenticated');
    localStorage.removeItem('username');
    localStorage.removeItem('accessToken');
    localStorage.removeItem('tokenType');
    localStorage.removeItem('avatar');
    localStorage.removeItem('role');
    localStorage.removeItem('userId');
    return false;
  }
};

onMounted(async () => {
  window.addEventListener('resize', handleWindowResize);
  
  const isFromLogout = sessionStorage.getItem('isFromLogout') === 'true';
  
  if (!isFromLogout) {
    const autoLoginSuccess = await tryAutoLogin();
    if (autoLoginSuccess) {
      ElMessage.success('自动登录成功');
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
      router.push('/Home');
    }
  } else {
    sessionStorage.removeItem('isFromLogout');
  }
});

onUnmounted(() => {
  window.removeEventListener('resize', handleWindowResize);
});

const handleLogin = async () => {
  try {
    await formRef.value.validate((valid: boolean, invalidFields: any) => {
      if (!valid) {
        const firstField = Object.keys(invalidFields)[0];
        if (firstField && invalidFields[firstField][0]) {
          ElMessage.error(invalidFields[firstField][0].message);
        } else {
          ElMessage.error('请检查输入信息是否符合要求');
        }
        throw new Error('表单验证失败');
      }
    });
    
    loading.value = true;
    
    const params = new URLSearchParams();
    params.append('grant_type', 'password');
    params.append('username', form.value.username);
    params.append('password', form.value.password);
    params.append('scope', '');
    params.append('client_id', '');
    params.append('client_secret', '');
    
    const data = await api.post('/api/login', params.toString(), {
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
        'Accept': 'application/json',
      }
    });
    
    ElMessage.success("登录成功");
    
    localStorage.setItem('isAuthenticated', 'true');
    localStorage.setItem('username', form.value.username);
    localStorage.setItem('accessToken', data.access_token);
    localStorage.setItem('tokenType', data.token_type || 'bearer');
    localStorage.setItem('avatar', data.avatar || '');
    localStorage.setItem('role', data.role || '');
    
    if (!data.role) {
      try {
        const userData = await api.get('/api/users/me', {
          headers: {
            'Accept': 'application/json',
            'Authorization': `Bearer ${data.access_token}`
          }
        });
        
        if (userData.code === 200 && userData.data) {
          localStorage.setItem('role', userData.data.role || '');
          localStorage.setItem('avatar', userData.data.avatar || '');
          localStorage.setItem('userId', userData.data.id || '');
        }
      } catch (err) {
        console.error('获取用户信息失败:', err);
      }
    }
    
    window.dispatchEvent(new CustomEvent('loginStatusChanged'));
    router.push('/Home');
  } catch (error) {
    console.error('登录错误:', error);
    const errorMessage = (error as Error).message;
    
    if (errorMessage === '表单验证失败') {
    } else if (errorMessage.includes('Failed to fetch') || errorMessage.includes('NetworkError')) {
      ElMessage.error('网络连接失败，请检查网络设置');
    } else {
      ElMessage.error(errorMessage || '登录失败，请稍后重试');
    }
  } finally {
    loading.value = false;
  }
};
</script>

<style lang="scss" scoped>
.login-container {
  min-height: 100vh;
  display: flex;
  justify-content: center;
  position: relative;
  z-index: 1;
}

/* 桌面端样式 */
.login-card {
  z-index: 1;
  padding-bottom: 30px;
  width: 400px;
  margin-top: 40%;
  margin-bottom: 60px;
  border-radius: 15px;
  height: fit-content;
}

.login-card .title {
  text-align: center;
  margin-bottom: 20px;
}

.login-card :deep(.el-form-item) {
  margin-bottom: 36px;
}

.login-card :deep(.el-form-item__error) {
  white-space: normal;
  word-break: break-word;
  max-width: 100%;
  margin-top: 8px;
  line-height: 1.4;
  padding: 0 0 5px 0;
  min-height: 30px;
  display: block;
  overflow: visible !important;
  font-size: 11px;
}

.login-card .register-link {
  text-align: center;
  margin-top: 20px;
  font-size: 14px;
  color: #606266;
}

.login-card .register-link a {
  color: #8bad42;
  text-decoration: none;
  margin-left: 5px;
}

.login-card .register-link a:hover {
  text-decoration: underline;
}

/* 移动端样式 */
.mobile-login {
  width: 100%;
  min-height: 100vh;
  background: linear-gradient(135deg, #8bad42 0%, #6a9c3d 100%);
  display: flex;
  flex-direction: column;
  padding: 0;
  overflow-x: hidden;
  box-sizing: border-box;
}

.mobile-login .login-header {
  display: flex;
  flex-direction: column;
  justify-content: flex-end;
  padding: 30px 24px 20px;
  text-align: center;
  color: white;
  min-height: 120px;
}

.mobile-login .login-header h1 {
  font-size: 28px;
  font-weight: 600;
  margin: 0 0 12px 0;
}

.mobile-login .login-header p {
  font-size: 16px;
  margin: 0;
  opacity: 0.9;
}

.mobile-login .login-form-container {
  flex: 1;
  background: white;
  border-radius: 24px 24px 0 0;
  padding: 32px 24px;
}

.mobile-login .login-form {
  margin-bottom: 24px;
}

.mobile-login .login-form :deep(.el-form-item) {
  margin-bottom: 20px;
}

.mobile-login .login-form :deep(.el-input__wrapper) {
  border-radius: 12px;
  padding: 4px 12px;
}

.mobile-login .login-form :deep(.el-form-item__error) {
  padding-top: 4px;
}

.mobile-login .login-btn {
  width: 100%;
  height: 48px;
  font-size: 16px;
  border-radius: 12px;
  margin-top: 8px;
}

.mobile-login .register-link {
  text-align: center;
  font-size: 14px;
  color: #909399;
}

.mobile-login .register-link a {
  color: #8bad42;
  text-decoration: none;
  margin-left: 5px;
  font-weight: 500;
}

.mobile-login .register-link a:active {
  opacity: 0.8;
}

.mobile-login .mobile-footer {
  margin-top: 40px;
  text-align: center;
}

.mobile-login .mobile-footer .copyright {
  font-size: 12px;
  color: #909399;
  margin-bottom: 8px;
}

.mobile-login .mobile-footer .beian {
  font-size: 11px;
  color: #c0c4cc;
  line-height: 1.8;
}

.mobile-login .mobile-footer .beian-link {
  color: #c0c4cc;
  text-decoration: none;
}

.mobile-login .mobile-footer .beian-link:active {
  color: #8bad42;
}
</style>