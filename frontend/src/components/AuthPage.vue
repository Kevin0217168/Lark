<template>
  <div class="auth-container" :class="{ 'mobile': isMobile }">
    <!-- 桌面端：背景图片 -->
    <div v-if="!isMobile" class="background-banner"></div>
    
    <!-- 桌面端：使用 el-card 包装 -->
    <el-card v-if="!isMobile" class="auth-card">
      <h3 class="title">{{ mode === 'login' ? '云雀管理系统 - 登录' : '云雀管理系统 - 注册' }}</h3>

      <el-form :model="form" ref="formRef" :rules="rules">
        <el-form-item label="用户名" prop="username">
          <el-input v-model="form.username" placeholder="请输入用户名"></el-input>
        </el-form-item>

        <el-form-item label="密码" prop="password">
          <el-input 
            v-model="form.password" 
            type="password" 
            placeholder="请输入密码" 
            show-password 
            @keyup.enter="mode === 'login' ? handleLogin() : handleRegister()"
          ></el-input>
        </el-form-item>

        <el-form-item v-if="mode === 'register'" label="确认密码" prop="confirmPassword">
          <el-input v-model="form.confirmPassword" type="password" placeholder="请再次输入密码" show-password></el-input>
        </el-form-item>

        <el-form-item v-if="mode === 'register'" label="昵称" prop="nickname">
          <el-input v-model="form.nickname" placeholder="请输入昵称"></el-input>
        </el-form-item>

        <!-- 角色默认设置为user，用户无需修改 -->
        <el-form-item v-if="mode === 'register'" label="角色" prop="role" style="display: none;">
          <el-input v-model="form.role" readonly></el-input>
        </el-form-item>

        <el-form-item v-if="mode === 'register'" label="邀请码" prop="invitationCode">
          <el-input v-model="form.invitationCode" placeholder="请输入邀请码"></el-input>
        </el-form-item>

        <el-form-item v-if="mode === 'register'" label="头像" prop="avatar">
          <el-input v-model="form.avatar" placeholder="请输入头像URL（可选）"></el-input>
        </el-form-item>

        <el-button style="width: 100%; margin-bottom: 10px;" @click="mode === 'login' ? handleLogin() : handleRegister()" type="primary" :loading="loading">
          {{ mode === 'login' ? '登录' : '注册' }}
        </el-button>
        
        <div class="toggle-link">
          <span>{{ mode === 'login' ? '还没有账号？' : '已有账号？' }}</span>
          <router-link :to="mode === 'login' ? '/Register' : '/Login'">{{ mode === 'login' ? '去注册' : '去登录' }}</router-link>
        </div>
      </el-form>
    </el-card>

    <!-- 移动端：全屏设计 -->
    <div v-else class="mobile-auth">
      <div class="auth-header">
        <h1>鸟厂后台管理</h1>
        <p>{{ mode === 'login' ? '欢迎回来，请登录您的账号' : '创建账号，请填写注册信息' }}</p>
      </div>

      <div class="auth-form-container">
        <el-form :model="form" ref="formRef" :rules="rules" class="auth-form">
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
              @keyup.enter="mode === 'login' ? handleLogin() : handleRegister()"
            />
          </el-form-item>

          <el-form-item v-if="mode === 'register'" prop="confirmPassword">
            <el-input 
              v-model="form.confirmPassword" 
              type="password" 
              placeholder="请再次输入密码"
              prefix-icon="Lock"
              size="large"
              show-password
            />
          </el-form-item>

          <el-form-item v-if="mode === 'register'" prop="nickname">
            <el-input 
              v-model="form.nickname" 
              placeholder="请输入昵称"
              prefix-icon="UserFilled"
              size="large"
            />
          </el-form-item>

          <el-form-item v-if="mode === 'register'" prop="invitationCode">
            <el-input 
              v-model="form.invitationCode" 
              placeholder="请输入邀请码"
              prefix-icon="Key"
              size="large"
            />
          </el-form-item>

          <el-form-item v-if="mode === 'register'" prop="avatar">
            <el-input 
              v-model="form.avatar" 
              placeholder="请输入头像URL（可选）"
              prefix-icon="Picture"
              size="large"
            />
          </el-form-item>

          <el-button 
            type="primary" 
            size="large" 
            class="auth-btn"
            @click="mode === 'login' ? handleLogin() : handleRegister()" 
            :loading="loading"
          >
            {{ mode === 'login' ? '登录' : '注册' }}
          </el-button>
        </el-form>

        <div class="toggle-link">
          <span>{{ mode === 'login' ? '还没有账号？' : '已有账号？' }}</span>
          <router-link :to="mode === 'login' ? '/Register' : '/Login'">{{ mode === 'login' ? '去注册' : '去登录' }}</router-link>
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
import { ref, computed, onMounted, onUnmounted } from "vue";
import { useRouter, useRoute } from "vue-router";
import { ElMessage } from "element-plus";
import { api } from '../utils/api';


const router = useRouter();
const route = useRoute();
const formRef = ref();
const loading = ref(false);



// 检测是否为移动设备
const isMobile = ref(window.innerWidth < 768);

// 根据路由自动确定当前模式
const mode = computed<'login' | 'register'>(() => {
  return route.path === '/Register' ? 'register' : 'login';
});

// 监听窗口大小变化
const handleWindowResize = () => {
  isMobile.value = window.innerWidth < 768;
};

// 登录表单
const loginForm = ref({
  username: "",
  password: "",
  confirmPassword: "",
  nickname: "",
  role: "user",
  invitationCode: "",
  avatar: "",
});

// 注册表单
const registerForm = ref({
  username: "",
  password: "",
  confirmPassword: "",
  nickname: "",
  role: "user",
  invitationCode: "",
  avatar: "",
});

// 统一表单对象，根据模式返回对应数据
const form = computed(() => {
  return mode.value === 'login' ? loginForm.value : registerForm.value;
});

// 验证两次密码是否一致
const validateConfirmPassword = (rule: any, value: string, callback: Function) => {
  if (mode.value === 'register' && value !== registerForm.value.password) {
    callback(new Error('两次输入的密码不一致'));
  } else {
    callback();
  }
};

// 登录规则
const loginRules = {
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

// 注册规则
const registerRules = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, max: 20, message: '用户名长度在 3 到 20 个字符', trigger: 'blur' },
    { pattern: /^[a-zA-Z0-9][a-zA-Z0-9_]*[a-zA-Z0-9]$/, message: '用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 8, max: 32, message: '密码长度在 8 到 32 个字符', trigger: 'blur' },
    { pattern: /^(?=.*[a-zA-Z])(?=.*\d)[a-zA-Z\d]+$/, message: '密码必须同时包含字母和数字', trigger: 'blur' }
  ],
  confirmPassword: [
    { required: true, message: '请再次输入密码', trigger: 'blur' },
    { validator: validateConfirmPassword, trigger: 'blur' }
  ],
  nickname: [
    { required: true, message: '请输入昵称', trigger: 'blur' },
    { min: 1, max: 50, message: '昵称长度在 1 到 50 个字符', trigger: 'blur' }
  ],
  role: [
    { required: true, message: '请选择角色', trigger: 'change' }
  ],
  invitationCode: [
    { required: true, message: '请输入邀请码', trigger: 'blur' },
    { min: 9, max: 11, message: '邀请码长度必须为9-11个字符', trigger: 'blur' }
  ],
  avatar: [
    { max: 255, message: '头像URL长度不能超过255个字符', trigger: 'blur' }
  ]
};

// 统一规则对象
const rules = computed(() => {
  return mode.value === 'login' ? loginRules : registerRules;
});

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
  
  if (mode.value === 'login') {
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
    params.append('username', loginForm.value.username);
    params.append('password', loginForm.value.password);
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
    localStorage.setItem('username', loginForm.value.username);
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

const handleRegister = async () => {
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
    
    const data = await api.post('/api/users', {
      username: registerForm.value.username,
      password: registerForm.value.password,
      nickname: registerForm.value.nickname,
      role: registerForm.value.role,
      avatar: registerForm.value.avatar,
      invitation_code: registerForm.value.invitationCode,
    });
    
    ElMessage.success("注册成功，请登录");
    router.push('/Login');
  } catch (error: any) {
    console.error('注册错误:', error);
    const errorMessage = error.message;
    
    if (errorMessage === '表单验证失败') {
    } else if (error.response) {
      const status = error.response.status;
      const responseData = error.response.data;
      
      if (status === 400) {
        ElMessage.error(responseData?.msg || '邀请码不存在或已失效');
      } else if (status === 500) {
        ElMessage.error('服务器内部错误，请稍后重试');
      } else {
        ElMessage.error(responseData?.msg || errorMessage || '注册失败，请稍后重试');
      }
    } else if (errorMessage.includes('Failed to fetch') || errorMessage.includes('NetworkError')) {
      ElMessage.error('网络连接失败，请检查网络设置');
    } else {
      ElMessage.error(errorMessage || '注册失败，请稍后重试');
    }
  } finally {
    loading.value = false;
  }
};
</script>

<style lang="scss" scoped>
.auth-container {
  min-height: 100vh;
  display: flex;
  justify-content: center;
  position: relative;
  z-index: 1;
}

/* 桌面端背景图片 */
.background-banner {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: 0;
  background: url('/banner.jpg') center/cover no-repeat;
}

/* 桌面端样式 */
.auth-card {
  z-index: 1;
  padding-bottom: 30px;
  width: 400px;
  margin: auto;
  border-radius: 15px;
  height: fit-content;
  background: rgba(255, 255, 255, 0.95);
  backdrop-filter: blur(10px);
  align-self: center;
}

.auth-card .title {
  text-align: center;
  margin-bottom: 20px;
}

.auth-card :deep(.el-form-item) {
  margin-bottom: 36px;
}

.auth-card :deep(.el-form-item__error) {
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

.auth-card .toggle-link {
  text-align: center;
  margin-top: 20px;
  font-size: 14px;
  color: #606266;
}

.auth-card .toggle-link a {
  color: #8bad42;
  text-decoration: none;
  margin-left: 5px;
}

.auth-card .toggle-link a:hover {
  text-decoration: underline;
}

/* 移动端样式 */
.mobile-auth {
  width: 100%;
  min-height: 100vh;
  background: linear-gradient(135deg, #8bad42 0%, #6a9c3d 100%);
  display: flex;
  flex-direction: column;
  padding: 0;
  overflow-x: hidden;
  box-sizing: border-box;
}

.mobile-auth .auth-header {
  display: flex;
  flex-direction: column;
  justify-content: flex-end;
  padding: 30px 24px 20px;
  text-align: center;
  color: white;
  min-height: 120px;
}

.mobile-auth .auth-header h1 {
  font-size: 28px;
  font-weight: 600;
  margin: 0 0 12px 0;
}

.mobile-auth .auth-header p {
  font-size: 16px;
  margin: 0;
  opacity: 0.9;
}

.mobile-auth .auth-form-container {
  flex: 1;
  background: white;
  border-radius: 24px 24px 0 0;
  padding: 32px 24px;
  overflow-y: auto;
}

.mobile-auth .auth-form {
  margin-bottom: 24px;
}

.mobile-auth .auth-form :deep(.el-form-item) {
  margin-bottom: 18px;
}

.mobile-auth .auth-form :deep(.el-input__wrapper) {
  border-radius: 12px;
  padding: 4px 12px;
}

.mobile-auth .auth-form :deep(.el-form-item__error) {
  padding-top: 4px;
}

.mobile-auth .auth-btn {
  width: 100%;
  height: 48px;
  font-size: 16px;
  border-radius: 12px;
  margin-top: 8px;
}

.mobile-auth .toggle-link {
  text-align: center;
  font-size: 14px;
  color: #909399;
}

.mobile-auth .toggle-link a {
  color: #8bad42;
  text-decoration: none;
  margin-left: 5px;
  font-weight: 500;
}

.mobile-auth .toggle-link a:active {
  opacity: 0.8;
}

.mobile-auth .mobile-footer {
  margin-top: 30px;
  text-align: center;
}

.mobile-auth .mobile-footer .copyright {
  font-size: 12px;
  color: #909399;
  margin-bottom: 8px;
}

.mobile-auth .mobile-footer .beian {
  font-size: 11px;
  color: #c0c4cc;
  line-height: 1.8;
}

.mobile-auth .mobile-footer .beian-link {
  color: #c0c4cc;
  text-decoration: none;
}

.mobile-auth .mobile-footer .beian-link:active {
  color: #8bad42;
}
</style>
