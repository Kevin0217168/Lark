<template>
  <div class="cloud-login-page">
    <!-- 桌面端提示 -->
    <div class="desktop-notice">
      <div class="notice-content">
        <svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" class="notice-icon">
          <rect x="5" y="2" width="14" height="20" rx="2" ry="2"></rect>
          <line x1="12" y1="18" x2="12.01" y2="18"></line>
        </svg>
        <p class="notice-text">请使用移动端访问此网页</p>
      </div>
    </div>

    <!-- 背景装饰 -->
    <div class="background-decorations">
      <div class="decoration decoration-1"></div>
      <div class="decoration decoration-2"></div>
      <div class="decoration decoration-3"></div>
      <div class="floating-elements">
        <div class="floating-element"></div>
        <div class="floating-element"></div>
        <div class="floating-element"></div>
        <div class="floating-element"></div>
      </div>
    </div>
    
    <!-- 主内容 -->
    <div class="login-container">
      <!-- Logo区域 -->
      <div class="logo-section">
        <div class="logo-wrapper">
          <svg xmlns="http://www.w3.org/2000/svg" width="80" height="80" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lark-logo" aria-hidden="true"> 
            <path d="M16 7h.01"></path> 
            <path d="M3.4 18H12a8 8 0 0 0 8-8V7a4 4 0 0 0-7.28-2.3L2 20"></path> 
            <path d="m20 7 2 .5-2 .5"></path> 
            <path d="M10 18v3"></path> 
            <path d="M14 17.75V21"></path> 
            <path d="M7 18a6 6 0 0 0 3.84-10.61"></path> 
          </svg>
        </div>
        <h1 class="app-name">云养鸟</h1>
        <p class="app-subtitle">与自然为伴，体验云养乐趣</p>
      </div>
      
      <!-- 登录卡片 -->
      <div class="login-card">
        <h2 class="card-title">欢迎回来</h2>
        
        <!-- 登录表单 -->
        <form class="login-form" @submit.prevent="handleLogin">
          <div class="form-group">
            <div class="input-field" :class="{ 'error': errors.username }">
              <div class="input-icon">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <path d="M19 21v-2a4 4 0 0 0-4-4H9a4 4 0 0 0-4 4v2"></path>
                  <circle cx="12" cy="7" r="4"></circle>
                </svg>
              </div>
              <input 
                v-model="form.username"
                type="text" 
                placeholder="请输入用户名"
                class="input"
                @blur="validateField('username')"
                @keyup.enter="handleLogin"
              />
            </div>
            <div v-if="errors.username" class="error-message">{{ errors.username }}</div>
          </div>
          
          <div class="form-group">
            <div class="input-field" :class="{ 'error': errors.password }">
              <div class="input-icon">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect>
                  <path d="M7 11V7a5 5 0 0 1 10 0v4"></path>
                </svg>
              </div>
              <input 
                v-model="form.password"
                :type="showPassword ? 'text' : 'password'"
                placeholder="请输入密码"
                class="input"
                @blur="validateField('password')"
                @keyup.enter="handleLogin"
              />
              <div class="input-suffix" @click="showPassword = !showPassword">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                  <path d="M9.88 9.88a3 3 0 1 0 4.24 4.24"></path>
                  <path d="M10.73 5.08A10.43 10.43 0 0 1 12 5c7 0 10 7 10 7a13.16 13.16 0 0 1-1.67 2.68"></path>
                  <path d="M6.61 6.61A13.526 13.526 0 0 0 2 12s3 7 10 7a9.74 9.74 0 0 0 5.39-1.61"></path>
                </svg>
              </div>
            </div>
            <div v-if="errors.password" class="error-message">{{ errors.password }}</div>
          </div>
          
          <button type="submit" class="login-button" :disabled="loading">
            <span v-if="!loading">登录</span>
            <div v-else class="loading-spinner">
              <div class="spinner"></div>
              <span>登录中...</span>
            </div>
          </button>
        </form>
        
        <!-- 底部链接 -->
        <div class="login-footer">
          <router-link to="/cloud/register" class="register-link">立即注册</router-link>
        </div>
        
        <button type="button" class="switch-system-btn" @click="switchToAdmin">
          您是工作人员？点击切换到后台管理系统
        </button>
      </div>
      
      <!-- 底部备案信息 -->
      <div class="bottom-info">
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
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import { api } from '../../utils/api';

const router = useRouter();
const loading = ref(false);
const showPassword = ref(false);

const form = reactive({
  username: '',
  password: ''
});

const errors = reactive({
  username: '',
  password: ''
});

const validateUsername = (): string => {
  if (!form.username) {
    return '请输入用户名';
  }
  if (form.username.length < 3 || form.username.length > 20) {
    return '长度在 3 到 20 个字符';
  }
  const pattern = /^[a-zA-Z0-9][a-zA-Z0-9_]*[a-zA-Z0-9]$/;
  if (!pattern.test(form.username)) {
    return '用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾';
  }
  return '';
};

const validatePassword = (): string => {
  if (!form.password) {
    return '请输入密码';
  }
  if (form.password.length < 8 || form.password.length > 32) {
    return '长度在 8 到 32 个字符';
  }
  const pattern = /^(?=.*[a-zA-Z0-9])[a-zA-Z0-9]+$/;
  if (!pattern.test(form.password)) {
    return '密码必须包含至少一个数字或字母';
  }
  return '';
};

const validateField = (field: keyof typeof form) => {
  if (field === 'username') {
    errors.username = validateUsername();
  } else if (field === 'password') {
    errors.password = validatePassword();
  }
};

const validateForm = (): boolean => {
  let isValid = true;
  
  errors.username = validateUsername();
  errors.password = validatePassword();
  
  if (errors.username) isValid = false;
  if (errors.password) isValid = false;
  
  return isValid;
};

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
  const isFromLogout = sessionStorage.getItem('isFromLogout') === 'true';
  
  if (!isFromLogout) {
    const autoLoginSuccess = await tryAutoLogin();
    if (autoLoginSuccess) {
      ElMessage.success('自动登录成功');
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
      router.push('/cloud/my');
    }
  } else {
    sessionStorage.removeItem('isFromLogout');
  }
});

const handleLogin = async () => {
  if (!validateForm()) {
    ElMessage.error('请检查输入信息是否符合要求');
    return;
  }

  try {
    loading.value = true;
    
    const params = new URLSearchParams();
    params.append('grant_type', 'password');
    params.append('username', form.username);
    params.append('password', form.password);
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
    localStorage.setItem('username', form.username);
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
    router.push('/cloud/my');
  } catch (error) {
    console.error('登录错误:', error);
    const errorMessage = (error as Error).message;
    
    if (errorMessage.includes('Failed to fetch') || errorMessage.includes('NetworkError')) {
      ElMessage.error('网络连接失败，请检查网络设置');
    } else {
      ElMessage.error(errorMessage || '登录失败，请稍后重试');
    }
  } finally {
    loading.value = false;
  }
};

const switchToAdmin = () => {
  router.push('/Login');
};
</script>

<style scoped>
/* 全局重置 */
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

/* 主页面样式 */
.cloud-login-page {
  position: relative;
  min-height: 100vh;
  background: linear-gradient(135deg, #f0fdf4 0%, #dcfce7 50%, #bbf7d0 100%);
  overflow: hidden;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
}

/* 背景装饰 */
.background-decorations {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 1;
}

.decoration {
  position: absolute;
  border-radius: 50%;
  background: rgba(167, 243, 208, 0.4);
  filter: blur(60px);
  animation: float 8s ease-in-out infinite;
}

.decoration-1 {
  top: 10%;
  left: 10%;
  width: 200px;
  height: 200px;
  background: rgba(110, 231, 183, 0.3);
  animation-delay: 0s;
}

.decoration-2 {
  top: 60%;
  right: 10%;
  width: 150px;
  height: 150px;
  background: rgba(167, 243, 208, 0.2);
  animation-delay: 2s;
}

.decoration-3 {
  bottom: 20%;
  left: 20%;
  width: 180px;
  height: 180px;
  background: rgba(74, 222, 128, 0.25);
  animation-delay: 4s;
}

/* 浮动元素 */
.floating-elements {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  overflow: hidden;
}

.floating-element {
  position: absolute;
  width: 8px;
  height: 8px;
  background: rgba(74, 222, 128, 0.6);
  border-radius: 50%;
  animation: floatUp 6s ease-in-out infinite;
}

.floating-element:nth-child(1) {
  top: 20%;
  left: 20%;
  animation-delay: 0s;
}

.floating-element:nth-child(2) {
  top: 40%;
  right: 30%;
  animation-delay: 1s;
  background: rgba(110, 231, 183, 0.5);
}

.floating-element:nth-child(3) {
  bottom: 30%;
  left: 40%;
  animation-delay: 2s;
  background: rgba(167, 243, 208, 0.4);
}

.floating-element:nth-child(4) {
  top: 60%;
  left: 70%;
  animation-delay: 3s;
  background: rgba(74, 222, 128, 0.5);
}

/* 浮动动画 */
@keyframes float {
  0%, 100% {
    transform: translateY(0px) translateX(0px);
  }
  25% {
    transform: translateY(-20px) translateX(10px);
  }
  50% {
    transform: translateY(10px) translateX(-10px);
  }
  75% {
    transform: translateY(-15px) translateX(5px);
  }
}

/* 上浮动画 */
@keyframes floatUp {
  0% {
    transform: translateY(100vh) scale(0);
    opacity: 0;
  }
  10% {
    opacity: 1;
    transform: translateY(80vh) scale(1);
  }
  90% {
    opacity: 1;
    transform: translateY(20vh) scale(1);
  }
  100% {
    transform: translateY(-20vh) scale(0);
    opacity: 0;
  }
}

/* 登录容器 */
.login-container {
  position: relative;
  z-index: 2;
  padding: 60px 24px 40px;
  display: flex;
  flex-direction: column;
  min-height: 100vh;
}

/* Logo区域 */
.logo-section {
  text-align: center;
  margin-bottom: 48px;
  transform: translateY(20px);
  animation: fadeInUp 0.8s ease-out forwards;
}

@keyframes fadeInUp {
  to {
    opacity: 1;
    transform: translateY(0);
  }
  from {
    opacity: 0;
    transform: translateY(30px);
  }
}

.logo-wrapper {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 100px;
  height: 100px;
  background: rgba(255, 255, 255, 0.8);
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
  border-radius: 24px;
  box-shadow: 
    0 8px 32px rgba(74, 222, 128, 0.2),
    0 2px 8px rgba(74, 222, 128, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
  border: 1px solid rgba(255, 255, 255, 0.6);
  margin-bottom: 16px;
  animation: pulse 2s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% {
    box-shadow: 
      0 8px 32px rgba(74, 222, 128, 0.2),
      0 2px 8px rgba(74, 222, 128, 0.1),
      inset 0 1px 0 rgba(255, 255, 255, 0.9);
  }
  50% {
    box-shadow: 
      0 12px 40px rgba(74, 222, 128, 0.3),
      0 4px 16px rgba(74, 222, 128, 0.2),
      inset 0 1px 0 rgba(255, 255, 255, 0.9);
  }
}

.lark-logo {
  color: #16a34a;
  width: 56px;
  height: 56px;
}

.app-name {
  font-size: 28px;
  font-weight: 700;
  color: #166534;
  margin-bottom: 8px;
  letter-spacing: 1px;
}

.app-subtitle {
  font-size: 14px;
  color: #16a34a;
  opacity: 0.8;
  margin: 0;
}

/* 登录卡片 */
.login-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(20px);
  -webkit-backdrop-filter: blur(20px);
  border-radius: 20px;
  padding: 32px 24px;
  box-shadow: 
    0 12px 40px rgba(74, 222, 128, 0.15),
    0 4px 16px rgba(74, 222, 128, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
  border: 1px solid rgba(255, 255, 255, 0.6);
  margin-bottom: 32px;
  animation: slideIn 0.6s ease-out 0.2s both;
}

@keyframes slideIn {
  from {
    opacity: 0;
    transform: translateY(20px) scale(0.95);
  }
  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}

.card-title {
  font-size: 20px;
  font-weight: 600;
  color: #166534;
  text-align: center;
  margin-bottom: 28px;
}

/* 表单样式 */
.login-form {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.form-group {
  position: relative;
}

.input-field {
  position: relative;
  border-radius: 12px;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.9);
  border: 2px solid rgba(74, 222, 128, 0.2);
  transition: all 0.3s ease;
  display: flex;
  align-items: center;
}

.input-field:focus-within,
.input-field:not(.error):focus-within {
  border-color: #16a34a;
  box-shadow: 
    0 8px 24px rgba(22, 163, 74, 0.2),
    0 0 0 4px rgba(22, 163, 74, 0.1);
  transform: translateY(-2px);
}

.input-field.error {
  border-color: #ef4444;
  box-shadow: 0 0 0 4px rgba(239, 68, 68, 0.1);
}

.input-icon {
  padding: 0 16px;
  color: rgba(22, 163, 74, 0.5);
  transition: all 0.3s ease;
  font-size: 16px;
}

.input-field:focus-within .input-icon {
  color: #16a34a;
  transform: scale(1.1);
}

.input-field.error .input-icon {
  color: #ef4444;
}

.input {
  flex: 1;
  height: 52px;
  padding: 0 16px 0 0;
  font-size: 16px;
  color: #166534;
  background: transparent;
  border: none;
  outline: none;
  transition: all 0.3s ease;
}

.input::placeholder {
  color: rgba(22, 163, 74, 0.5);
  transition: all 0.3s ease;
}

.input-field:focus-within .input::placeholder {
  color: rgba(22, 163, 74, 0.3);
  transform: translateX(4px);
}

.input-suffix {
  padding: 0 16px;
  color: rgba(22, 163, 74, 0.5);
  transition: all 0.3s ease;
  cursor: pointer;
}

.input-suffix:hover {
  color: #16a34a;
  transform: scale(1.1);
}

.input-field:focus-within .input-suffix {
  color: #16a34a;
}

.error-message {
  position: absolute;
  top: calc(100% + 4px);
  left: 0;
  font-size: 12px;
  color: #ef4444;
  line-height: 1.4;
  white-space: normal;
  word-break: break-word;
  max-width: 100%;
}

/* 登录按钮 */
.login-button {
  width: 100%;
  height: 52px;
  margin-top: 8px;
  font-size: 16px;
  font-weight: 600;
  color: white;
  background: linear-gradient(135deg, #16a34a 0%, #15803d 100%);
  border: none;
  border-radius: 12px;
  cursor: pointer;
  transition: all 0.3s ease;
  box-shadow: 0 4px 16px rgba(22, 163, 74, 0.3);
  position: relative;
  overflow: hidden;
}

.login-button::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.login-button:hover::before {
  left: 100%;
}

.login-button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(22, 163, 74, 0.4);
}

.login-button:active:not(:disabled) {
  transform: translateY(0);
  box-shadow: 0 2px 8px rgba(22, 163, 74, 0.3);
}

.login-button:disabled {
  opacity: 0.7;
  cursor: not-allowed;
}

/* 加载动画 */
.loading-spinner {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
}

.spinner {
  width: 20px;
  height: 20px;
  border: 2px solid rgba(255, 255, 255, 0.3);
  border-top: 2px solid white;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

/* 登录底部 */
.login-footer {
  display: flex;
  justify-content: center;
  align-items: center;
  margin-top: 20px;
}

.register-link {
  font-size: 14px;
  font-weight: 600;
  color: #16a34a;
  text-decoration: none;
  transition: all 0.3s ease;
  position: relative;
  padding: 4px 0;
}

.register-link::after {
  content: '';
  position: absolute;
  bottom: 0;
  left: 0;
  width: 0;
  height: 2px;
  background: #16a34a;
  transition: width 0.3s ease;
}

.register-link:hover {
  color: #15803d;
  transform: translateY(-1px);
}

.register-link:hover::after {
  width: 100%;
}

/* 切换系统按钮 */
.switch-system-btn {
  width: 100%;
  height: 44px;
  margin-top: 20px;
  font-size: 14px;
  font-weight: 500;
  color: rgba(107, 114, 128, 0.8);
  background: rgba(243, 244, 246, 0.8);
  backdrop-filter: blur(10px);
  -webkit-backdrop-filter: blur(10px);
  border: 1px solid rgba(209, 213, 219, 0.5);
  border-radius: 10px;
  cursor: pointer;
  transition: all 0.3s ease;
  position: relative;
  overflow: hidden;
}

.switch-system-btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.3), transparent);
  transition: left 0.6s ease;
}

.switch-system-btn:hover::before {
  left: 100%;
}

.switch-system-btn:hover {
  color: rgba(55, 65, 81, 0.95);
  background: rgba(229, 231, 235, 0.9);
  border-color: rgba(156, 163, 175, 0.5);
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.05);
}

.switch-system-btn:active {
  transform: translateY(0);
  box-shadow: none;
}

/* 底部信息 */
.bottom-info {
  margin-top: auto;
  text-align: center;
  animation: fadeIn 1s ease-out 0.4s both;
}

@keyframes fadeIn {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

.copyright {
  font-size: 11px;
  color: rgba(22, 163, 74, 0.65);
  line-height: 1.6;
  margin-bottom: 2px;
}

.beian {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
  margin-top: 8px;
}

.beian-item {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
}

.beian-icon {
  width: 14px;
  height: 14px;
  object-fit: contain;
}

.beian-placeholder {
  display: inline-block;
  width: 14px;
  height: 14px;
}

.beian-link {
  font-size: 11px;
  color: rgba(22, 163, 74, 0.6);
  text-decoration: none;
  transition: color 0.2s ease;
}

.beian-link:hover {
  color: rgba(22, 163, 74, 0.85);
  text-decoration: underline;
}

/* 响应式设计 */
@media (max-width: 400px) {
  .login-container {
    padding: 48px 20px 32px;
  }
  
  .logo-wrapper {
    width: 80px;
    height: 80px;
  }
  
  .lark-logo {
    width: 48px;
    height: 48px;
  }
  
  .app-name {
    font-size: 24px;
  }
  
  .login-card {
    padding: 28px 20px;
  }
  
  .card-title {
    font-size: 18px;
  }
  
  .input {
    height: 48px;
  }
  
  .login-button {
    height: 48px;
  }
  
  .switch-system-btn {
    height: 42px;
    font-size: 13px;
  }
  
  .beian {
    gap: 2px;
  }
  
  .beian-icon,
  .beian-placeholder {
    width: 12px;
    height: 12px;
  }
  
  .copyright,
  .beian-link {
    font-size: 10px;
  }
}

/* 触摸反馈 */
@media (hover: none) and (pointer: coarse) {
  .login-button,
  .switch-system-btn {
    transition: none;
  }
  
  .login-button:active:not(:disabled) {
    transform: scale(0.98);
  }
  
  .switch-system-btn:active {
    transform: scale(0.98);
  }
  
  .register-link:active {
    opacity: 0.7;
  }
}

/* 桌面端提示 */
.desktop-notice {
  display: none;
}

@media (min-width: 769px) {
  .desktop-notice {
    display: flex;
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    z-index: 9999;
    background: #ffffff;
    align-items: center;
    justify-content: center;
  }

  .notice-content {
    text-align: center;
  }

  .notice-icon {
    color: #9ca3af;
    margin-bottom: 24px;
  }

  .notice-text {
    font-size: 18px;
    color: #6b7280;
    margin: 0;
    font-weight: 500;
  }
}
</style>
