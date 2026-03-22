<template>
  <div class="mobile-login">
    <div class="login-header">
      <h1>欢迎回来</h1>
      <p>请登录您的账号</p>
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
    </div>
  </div>
</template>

<script setup lang="ts">
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
    
    const response = await fetch(`${API_BASE_URL}/api/refresh`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
      },
      credentials: 'include',
    });

    if (response.ok) {
      const data = await response.json();
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
      
      localStorage.setItem('isAuthenticated', 'true');
      localStorage.setItem('username', form.value.username);
      localStorage.setItem('accessToken', data.access_token);
      localStorage.setItem('tokenType', data.token_type || 'bearer');
      localStorage.setItem('avatar', data.avatar || '');
      localStorage.setItem('role', data.role || '');
      
      if (!data.role) {
        try {
          const userResponse = await fetch(`${API_BASE_URL}/api/users/me`, {
            method: 'GET',
            headers: {
              'Accept': 'application/json',
              'Authorization': `Bearer ${data.access_token}`
            },
            credentials: 'include'
          });
          
          if (userResponse.ok) {
            const userData = await userResponse.json();
            if (userData.code === 200 && userData.data) {
              localStorage.setItem('role', userData.data.role || '');
              localStorage.setItem('avatar', userData.data.avatar || '');
              localStorage.setItem('userId', userData.data.id || '');
            }
          }
        } catch (err) {
          console.error('获取用户信息失败:', err);
        }
      }
      
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
      router.push('/Home');
    } else {
      if (response.status === 422 && data.detail && Array.isArray(data.detail) && data.detail.length > 0) {
        const errorMessages = data.detail
          .map((err: any) => {
            const msg = err.msg || '';
            return msg.replace(/^Value error,\s*/, '');
          })
          .filter((msg: string) => msg.length > 0)
          .join('\n');
        ElMessage.error(errorMessages || '登录失败');
      } else {
        const errorMsg = data.detail || data.msg || '登录失败，请检查用户名和密码';
        ElMessage.error(errorMsg);
      }
    }
  } catch (error) {
    console.error('登录错误:', error);
    const errorMessage = (error as Error).message;
    
    if (errorMessage === '表单验证失败') {
      // 表单验证失败已经在validate回调中处理过
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

<style scoped>
.mobile-login {
  min-height: 100vh;
  background: linear-gradient(135deg, #8bad42 0%, #6a9c3d 100%);
  display: flex;
  flex-direction: column;
  padding: 0;
}

.login-header {
  padding: 60px 24px 40px;
  text-align: center;
  color: white;
}

.login-header h1 {
  font-size: 28px;
  font-weight: 600;
  margin: 0 0 12px 0;
}

.login-header p {
  font-size: 16px;
  margin: 0;
  opacity: 0.9;
}

.login-form-container {
  flex: 1;
  background: white;
  border-radius: 24px 24px 0 0;
  padding: 32px 24px;
}

.login-form {
  margin-bottom: 24px;
}

.login-form :deep(.el-form-item) {
  margin-bottom: 20px;
}

.login-form :deep(.el-input__wrapper) {
  border-radius: 12px;
  padding: 4px 12px;
}

.login-form :deep(.el-form-item__error) {
  padding-top: 4px;
}

.login-btn {
  width: 100%;
  height: 48px;
  font-size: 16px;
  border-radius: 12px;
  margin-top: 8px;
}

.register-link {
  text-align: center;
  font-size: 14px;
  color: #909399;
}

.register-link a {
  color: #8bad42;
  text-decoration: none;
  margin-left: 5px;
  font-weight: 500;
}

.register-link a:active {
  opacity: 0.8;
}
</style>