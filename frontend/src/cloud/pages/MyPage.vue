<template>
  <div class="my-page">
    <div class="page-content">
      <!-- 头像区域 - 毛玻璃效果 -->
      <div class="profile-section">
        <div class="avatar-glass">
          <div class="avatar-container">
            <div v-if="userInfo.avatar" class="avatar-circle">
              <img :src="userInfo.avatar" :alt="userInfo.nickname" />
            </div>
            <div v-else class="avatar-circle default-avatar">
              <span>{{ userInfo.nickname ? userInfo.nickname.charAt(0) : '?' }}</span>
            </div>
          </div>
          <h2 class="username">{{ userInfo.nickname || userInfo.username }}</h2>
          <span class="role-badge" :class="getRoleBadgeClass(userInfo.role)">
            {{ getRoleLabel(userInfo.role) }}
          </span>
        </div>
      </div>

      <!-- 加载状态 -->
      <div v-if="loading" class="loading-section">
        <div class="loading-glass">
          <div class="spinner"></div>
          <span class="loading-text">加载中...</span>
        </div>
      </div>

      <!-- 错误状态 -->
      <div v-else-if="error" class="error-section">
        <div class="error-glass">
          <span class="error-icon">⚠️</span>
          <span class="error-text">{{ error }}</span>
          <button class="retry-btn" @click="fetchUserInfo">重新加载</button>
        </div>
      </div>

      <!-- 用户信息卡片 -->
      <div v-else class="info-sections">
        <!-- 信息卡片 - 毛玻璃效果 -->
        <div class="info-glass-card">
          <div class="info-title">个人信息</div>
          
          <div class="info-row">
            <span class="info-label">用户名</span>
            <span class="info-value">{{ userInfo.username }}</span>
          </div>
          
          <div class="info-row">
            <span class="info-label">昵称</span>
            <span class="info-value">{{ userInfo.nickname || '未设置' }}</span>
          </div>
          
          <div class="info-row">
            <span class="info-label">邮箱</span>
            <span class="info-value">{{ userInfo.email || '未设置' }}</span>
          </div>
        </div>

        <!-- 操作按钮区域 -->
        <div class="action-section">
          <button v-if="showSwitchToAdmin" class="action-btn admin-btn" @click="goToAdmin">
            <span class="btn-icon">🏠</span>
            <span class="btn-text">鸟场管理系统</span>
          </button>

          <button class="action-btn logout-btn" @click="handleLogout">
            <span class="btn-icon">🚪</span>
            <span class="btn-text">退出登录</span>
          </button>

          <button class="action-btn delete-btn" @click="openDeleteAccountDialog">
            <span class="btn-icon">🗑️</span>
            <span class="btn-text">注销账号</span>
          </button>
        </div>
      </div>
    </div>

    <!-- 密码验证对话框 -->
    <div v-if="verifyPasswordDialogVisible" class="dialog-overlay" @click="verifyPasswordDialogVisible = false">
      <div class="dialog-content" @click.stop>
        <div class="dialog-title">验证密码</div>
        <div class="dialog-message">
          请输入当前密码以验证身份
        </div>
        <div class="password-input-wrapper">
          <input 
            v-model="verifyPasswordForm.password" 
            :type="showPassword ? 'text' : 'password'" 
            class="password-input"
            placeholder="请输入当前密码"
            @keyup.enter="verifyPasswordAndConfirmDelete"
          />
          <button class="toggle-password-btn" @click="showPassword = !showPassword">
            {{ showPassword ? '🙈' : '👁️' }}
          </button>
        </div>
        <div class="dialog-actions">
          <button class="dialog-btn cancel-btn" @click="verifyPasswordDialogVisible = false">取消</button>
          <button class="dialog-btn confirm-btn" @click="verifyPasswordAndConfirmDelete" :disabled="verifyingPassword">
            {{ verifyingPassword ? '验证中...' : '验证' }}
          </button>
        </div>
      </div>
    </div>

    <!-- 确认注销对话框 -->
    <div v-if="confirmDeleteDialogVisible" class="dialog-overlay" @click="confirmDeleteDialogVisible = false">
      <div class="dialog-content" @click.stop>
        <div class="dialog-title">确认注销账号</div>
        <div class="dialog-message">
          确定要注销账号吗？此操作不可恢复，您的所有数据将被永久删除。
        </div>
        <div class="dialog-actions">
          <button class="dialog-btn cancel-btn" @click="confirmDeleteDialogVisible = false">取消</button>
          <button class="dialog-btn confirm-btn" @click="handleDeleteAccount" :disabled="deletingAccount">
            {{ deletingAccount ? '注销中...' : '确认注销' }}
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, computed } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import { api } from '../../utils/api';

const router = useRouter();
const loading = ref(false);
const error = ref('');
const confirmDeleteDialogVisible = ref(false);
const deletingAccount = ref(false);
const verifyPasswordDialogVisible = ref(false);
const verifyingPassword = ref(false);
const showPassword = ref(false);

const userInfo = ref({
  id: 0,
  username: '',
  nickname: '',
  role: '',
  email: null as string | null,
  banner: null as string | null,
  avatar: ''
});

// 是否显示切换到鸟场管理系统按钮（root 或 user 角色显示）
const showSwitchToAdmin = computed(() => {
  return userInfo.value.role === 'root' || userInfo.value.role === 'user';
});

// 跳转到鸟场管理系统
const goToAdmin = () => {
  router.push('/Profile');
};

const verifyPasswordForm = ref({
  password: ''
});

// 获取角色标签名称
const getRoleLabel = (role: string) => {
  switch (role) {
    case 'root':
      return '管理员';
    case 'user':
      return '普通用户';
    case 'clouduser':
      return '云养用户';
    default:
      return '普通用户';
  }
};

// 获取角色徽章样式类
const getRoleBadgeClass = (role: string) => {
  switch (role) {
    case 'root':
      return 'role-admin';
    case 'clouduser':
      return 'role-cloud';
    case 'user':
    default:
      return 'role-user';
  }
};

// 获取用户信息
const fetchUserInfo = async () => {
  try {
    loading.value = true;
    error.value = '';

    const responseData = await api.get('/api/users/me');
    if (responseData.code === 200 && responseData.data) {
      userInfo.value = responseData.data;
      localStorage.setItem('username', userInfo.value.username || '');
      localStorage.setItem('avatar', userInfo.value.avatar || '');
    } else {
      throw new Error(responseData.msg || '获取用户信息失败');
    }
  } catch (err) {
    error.value = (err as Error).message || '获取用户信息失败';
  } finally {
    loading.value = false;
  }
};

// 退出登录
const handleLogout = async () => {
  try {
    await api.post('/api/logout');
  } catch (error) {
    console.error('退出登录请求出错:', error);
  }
  
  // 清除登录状态
  localStorage.removeItem('isAuthenticated');
  localStorage.removeItem('username');
  localStorage.removeItem('accessToken');
  localStorage.removeItem('tokenType');
  localStorage.removeItem('avatar');
  localStorage.removeItem('role');
  localStorage.removeItem('userId');

  // 清除cookie
  const clearCookies = () => {
    const cookies = document.cookie.split(';');
    for (let cookie of cookies) {
      const eqPos = cookie.indexOf('=');
      const name = eqPos > -1 ? cookie.substring(0, eqPos).trim() : cookie.trim();
      if (name) {
        document.cookie = `${name}=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/`;
        document.cookie = `${name}=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=${window.location.hostname}`;
      }
    }
  };

  clearCookies();
  window.dispatchEvent(new CustomEvent('loginStatusChanged'));
  
  // 显示退出成功提示
  ElMessage.success('已退出登录');
  
  // 重定向到云养登录页面
  router.push('/cloud/login');
};

// 打开注销账号对话框
const openDeleteAccountDialog = () => {
  verifyPasswordForm.value.password = '';
  verifyPasswordDialogVisible.value = true;
};

// 验证密码并打开确认对话框
const verifyPasswordAndConfirmDelete = async () => {
  try {
    if (!verifyPasswordForm.value.password) {
      ElMessage.warning('请输入密码');
      return;
    }
    verifyingPassword.value = true;

    // 使用与Login.vue相同的登录逻辑验证密码
    const params = new URLSearchParams();
    params.append('grant_type', 'password');
    params.append('username', userInfo.value.username);
    params.append('password', verifyPasswordForm.value.password);
    params.append('scope', '');
    params.append('client_id', '');
    params.append('client_secret', '');

    const data = await api.post('/api/login', params.toString(), {
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
        'Accept': 'application/json',
      }
    });
    
    if (data.access_token) {
      // 密码验证成功，关闭验证对话框，打开确认对话框
      verifyPasswordDialogVisible.value = false;
      confirmDeleteDialogVisible.value = true;
    } else {
      throw new Error('密码验证失败');
    }
  } catch (err) {
    ElMessage.error((err as Error).message || '密码验证失败');
  } finally {
    verifyingPassword.value = false;
  }
};

// 处理注销账号
const handleDeleteAccount = async () => {
  try {
    deletingAccount.value = true;

    const meResponseData = await api.get('/api/users/me');
    if (meResponseData.code !== 200 || !meResponseData.data) {
      throw new Error('获取账号信息失败');
    }

    const userId = meResponseData.data.id;
    const deleteResponseData = await api.delete(`/api/users/${userId}`);
    
    if (deleteResponseData.code === 200) {
      ElMessage.success('账号已注销');
      
      // 清除登录状态
      localStorage.removeItem('isAuthenticated');
      localStorage.removeItem('username');
      localStorage.removeItem('accessToken');
      localStorage.removeItem('tokenType');
      localStorage.removeItem('avatar');
      localStorage.removeItem('role');
      localStorage.removeItem('userId');
      
      const clearCookies = () => {
        const cookies = document.cookie.split(';');
        for (let cookie of cookies) {
          const eqPos = cookie.indexOf('=');
          const name = eqPos > -1 ? cookie.substring(0, eqPos).trim() : cookie.trim();
          if (name) {
            document.cookie = `${name}=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/`;
            document.cookie = `${name}=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=${window.location.hostname}`;
          }
        }
      };
      
      clearCookies();
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
      
      confirmDeleteDialogVisible.value = false;
      router.push('/cloud/login');
    } else {
      throw new Error(deleteResponseData.msg || '注销账号失败');
    }
  } catch (err) {
    ElMessage.error((err as Error).message || '注销账号失败');
  } finally {
    deletingAccount.value = false;
  }
};

onMounted(() => {
  fetchUserInfo();
});
</script>

<style scoped>
.my-page {
  min-height: 100vh;
  background: linear-gradient(135deg, #f5f7fa 0%, #e8f5e9 100%);
  padding: 20px 16px;
  padding-bottom: 100px;
  box-sizing: border-box;
}

.page-content {
  max-width: 400px;
  margin: 0 auto;
}

/* 头像区域 */
.profile-section {
  margin-bottom: 20px;
}

.avatar-glass {
  background: rgba(255, 255, 255, 0.75);
  backdrop-filter: blur(20px) saturate(180%);
  -webkit-backdrop-filter: blur(20px) saturate(180%);
  border-radius: 24px;
  padding: 32px 20px;
  text-align: center;
  border: 1px solid rgba(255, 255, 255, 0.4);
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.08);
}

.avatar-container {
  margin-bottom: 16px;
}

.avatar-circle {
  width: 96px;
  height: 96px;
  border-radius: 50%;
  overflow: hidden;
  margin: 0 auto;
  box-shadow: 0 4px 16px rgba(139, 173, 66, 0.25);
}

.avatar-circle img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.default-avatar {
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
  display: flex;
  justify-content: center;
  align-items: center;
  color: white;
  font-size: 36px;
  font-weight: bold;
}

.username {
  margin: 0 0 8px 0;
  font-size: 22px;
  font-weight: 600;
  color: #1a1a1a;
}

.role-badge {
  display: inline-block;
  color: white;
  padding: 6px 16px;
  border-radius: 20px;
  font-size: 13px;
  font-weight: 500;
  letter-spacing: 0.3px;
}

/* 管理员 - 红色 */
.role-admin {
  background: linear-gradient(135deg, #f56c6c 0%, #e64a4a 100%);
}

/* 普通用户 - 灰色 */
.role-user {
  background: linear-gradient(135deg, #909399 0%, #a6a9ad 100%);
}

/* 云养用户 - 绿色 */
.role-cloud {
  background: linear-gradient(135deg, #8BAD42 0%, #A4C65F 100%);
}

/* 加载状态 */
.loading-section,
.error-section {
  margin-bottom: 20px;
}

.loading-glass,
.error-glass {
  background: rgba(255, 255, 255, 0.75);
  backdrop-filter: blur(20px) saturate(180%);
  -webkit-backdrop-filter: blur(20px) saturate(180%);
  border-radius: 20px;
  padding: 40px 20px;
  text-align: center;
  border: 1px solid rgba(255, 255, 255, 0.4);
}

.spinner {
  width: 40px;
  height: 40px;
  border: 3px solid rgba(139, 173, 66, 0.2);
  border-top-color: #8BAD42;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
  margin: 0 auto 12px;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

.loading-text {
  color: #666;
  font-size: 14px;
}

.error-icon {
  font-size: 40px;
  display: block;
  margin-bottom: 12px;
}

.error-text {
  display: block;
  color: #f56c6c;
  font-size: 14px;
  margin-bottom: 16px;
}

.retry-btn {
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
  color: white;
  border: none;
  padding: 10px 24px;
  border-radius: 12px;
  font-size: 14px;
  font-weight: 500;
  cursor: pointer;
  transition: transform 0.2s, box-shadow 0.2s;
}

.retry-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(139, 173, 66, 0.3);
}

.retry-btn:active {
  transform: translateY(0);
}

/* 信息卡片 */
.info-sections {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.info-glass-card {
  background: rgba(255, 255, 255, 0.75);
  backdrop-filter: blur(20px) saturate(180%);
  -webkit-backdrop-filter: blur(20px) saturate(180%);
  border-radius: 20px;
  padding: 20px;
  border: 1px solid rgba(255, 255, 255, 0.4);
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.05);
}

.info-title {
  font-size: 16px;
  font-weight: 600;
  color: #1a1a1a;
  margin-bottom: 16px;
  padding-bottom: 12px;
  border-bottom: 1px solid rgba(139, 173, 66, 0.15);
}

.info-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 0;
}

.info-row:not(:last-child) {
  border-bottom: 1px solid rgba(0, 0, 0, 0.05);
}

.info-label {
  font-size: 14px;
  color: #666;
}

.info-value {
  font-size: 14px;
  color: #1a1a1a;
  font-weight: 500;
}

/* 操作按钮 */
.action-section {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.action-btn {
  width: 100%;
  height: 52px;
  border-radius: 16px;
  border: none;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  font-size: 16px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s;
}

.action-btn:active {
  transform: scale(0.96);
}

.logout-btn {
  background: rgba(139, 173, 66, 0.1);
  color: #8BAD42;
  border: 1.5px solid rgba(139, 173, 66, 0.3);
}

.logout-btn:hover {
  background: rgba(139, 173, 66, 0.15);
}

.delete-btn {
  background: rgba(245, 108, 108, 0.1);
  color: #f56c6c;
  border: 1.5px solid rgba(245, 108, 108, 0.3);
}

.delete-btn:hover {
  background: rgba(245, 108, 108, 0.15);
}

.admin-btn {
  background: rgba(64, 158, 255, 0.1);
  color: #409eff;
  border: 1.5px solid rgba(64, 158, 255, 0.3);
}

.admin-btn:hover {
  background: rgba(64, 158, 255, 0.15);
}

.btn-icon {
  font-size: 18px;
}

.btn-text {
  font-weight: 500;
}

/* 对话框 */
.dialog-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
  padding: 20px;
}

.dialog-content {
  background: white;
  border-radius: 20px;
  padding: 24px;
  width: 100%;
  max-width: 340px;
  animation: slideUp 0.3s ease;
}

@keyframes slideUp {
  from {
    opacity: 0;
    transform: translateY(20px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

.dialog-title {
  font-size: 18px;
  font-weight: 600;
  color: #1a1a1a;
  margin-bottom: 12px;
}

.dialog-message {
  font-size: 14px;
  color: #666;
  line-height: 1.6;
  margin-bottom: 24px;
}

.dialog-actions {
  display: flex;
  gap: 12px;
}

.dialog-btn {
  flex: 1;
  height: 44px;
  border-radius: 12px;
  border: none;
  font-size: 15px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s;
}

.cancel-btn {
  background: #f5f7fa;
  color: #666;
}

.confirm-btn {
  background: linear-gradient(135deg, #f56c6c 0%, #e64a4a 100%);
  color: white;
}

.confirm-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.dialog-btn:active:not(:disabled) {
  transform: scale(0.96);
}

/* 密码输入框样式 */
.password-input-wrapper {
  position: relative;
  margin-bottom: 20px;
}

.password-input {
  width: 100%;
  height: 48px;
  padding: 0 48px 0 16px;
  border: 1.5px solid #e0e0e0;
  border-radius: 12px;
  font-size: 15px;
  color: #1a1a1a;
  background: #f9f9f9;
  box-sizing: border-box;
  transition: all 0.2s;
}

.password-input:focus {
  outline: none;
  border-color: #8BAD42;
  background: white;
}

.password-input::placeholder {
  color: #999;
}

.toggle-password-btn {
  position: absolute;
  right: 12px;
  top: 50%;
  transform: translateY(-50%);
  background: none;
  border: none;
  font-size: 20px;
  cursor: pointer;
  padding: 4px;
  opacity: 0.7;
  transition: opacity 0.2s;
}

.toggle-password-btn:hover {
  opacity: 1;
}
</style>
