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
    { min: 3, max: 20, message: '长度在 3 到 20 个字符', trigger: 'blur' },
    { pattern: /^[a-zA-Z0-9][a-zA-Z0-9_]*[a-zA-Z0-9]$/, message: '用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 8, max: 32, message: '长度在 8 到 32 个字符', trigger: 'blur' },
    { pattern: /^(?=.*[a-zA-Z0-9])[a-zA-Z0-9]+$/, message: '密码必须包含至少一个数字或字母', trigger: 'blur' }
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
        // 保留现有的用户名和头像信息
        if (!localStorage.getItem('username')) {
          // 如果没有用户名，尝试从某处获取（这里可能需要根据实际情况调整）
          localStorage.setItem('username', '');
        }
        if (!localStorage.getItem('avatar')) {
          localStorage.setItem('avatar', '');
        }
        
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
    // 验证表单，如果失败会抛出错误
    await formRef.value.validate((valid: boolean, invalidFields: any) => {
      if (!valid) {
        // 提取第一个错误信息并显示
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
      localStorage.setItem('avatar', data.avatar || '');
      localStorage.setItem('role', data.role || '');
      
      // 立即获取用户信息以获取角色（如果登录接口没有返回角色）
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
            }
          }
        } catch (err) {
          console.error('获取用户信息失败:', err);
        }
      }
      
      // 触发自定义事件，通知其他组件登录状态已变化
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
      
      // 跳转到首页
      router.push('/Home');
    } else {
      // 处理错误响应
      if (response.status === 422 && data.detail && Array.isArray(data.detail) && data.detail.length > 0) {
        // 处理 [{ loc: [...], msg: "...", type: "..." }] 格式的错误
        const errorMessages = data.detail
          .map((err: any) => {
            const msg = err.msg || '';
            // 移除 "Value error, " 前缀
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
    
    // 根据错误类型显示不同的错误信息
    if (errorMessage === '表单验证失败') {
      // 表单验证失败已经在validate回调中处理过，这里不需要重复显示
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
/* Login Card */
.login-card {
  z-index: 1;
  padding-bottom: 30px;
  width: 400px;
  margin-top: 5%;
  border-radius: 5%;
}

/* 增加表单项目之间的间距，确保错误信息完整显示 */
:deep(.el-form-item) {
  margin-bottom: 36px;
}

/* 确保错误信息完整显示 */
:deep(.el-form-item__error) {
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
