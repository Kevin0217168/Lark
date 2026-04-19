<template>
  <div class="my-page">
    <!-- 背景装饰 -->
    <div class="bg-decoration">
      <div class="bg-ball bg-ball-1"></div>
      <div class="bg-ball bg-ball-2"></div>
      <div class="bg-ball bg-ball-3"></div>
      <div class="floating-element floating-1"></div>
      <div class="floating-element floating-2"></div>
      <div class="floating-element floating-3"></div>
      <div class="floating-element floating-4"></div>
    </div>

    <!-- 主内容区 -->
    <div class="main-content">
      <!-- 头像和用户信息 -->
      <div class="profile-header" :class="{ 'fade-in': isLoaded }">
        <div class="avatar-container">
          <div v-if="userInfo.avatar" class="avatar-circle">
            <img :src="userInfo.avatar" :alt="userInfo.nickname" />
          </div>
          <div v-else class="avatar-circle default-avatar">
            <span>{{ userInfo.nickname ? userInfo.nickname.charAt(0) : '?' }}</span>
          </div>
        </div>
        <h1 class="username">{{ userInfo.nickname || userInfo.username }}</h1>
        <span class="role-badge" :class="getRoleBadgeClass(userInfo.role)">
          {{ getRoleLabel(userInfo.role) }}
        </span>
      </div>

      <!-- 加载状态 -->
      <div v-if="loading" class="loading-section">
        <div class="spinner"></div>
        <span class="loading-text">加载中...</span>
      </div>

      <!-- 错误状态 -->
      <div v-else-if="error" class="error-section">
        <span class="error-icon">⚠️</span>
        <span class="error-text">{{ error }}</span>
        <button class="retry-btn" @click="fetchUserInfo">重新加载</button>
      </div>

      <!-- 个人信息区域 -->
      <div v-else class="info-section" :class="{ 'fade-in': isLoaded }">
        <h2 class="section-title">个人信息</h2>
        
        <div class="info-grid">
          <div class="info-item">
            <div class="info-icon">👤</div>
            <div class="info-content">
              <div class="info-label">用户名</div>
              <div class="info-value">{{ userInfo.username }}</div>
            </div>
          </div>
          
          <div class="info-item">
            <div class="info-icon">🏷️</div>
            <div class="info-content">
              <div class="info-label">昵称</div>
              <div class="info-value">{{ userInfo.nickname || '未设置' }}</div>
            </div>
          </div>
          
          <div class="info-item">
            <div class="info-icon">📧</div>
            <div class="info-content">
              <div class="info-label">邮箱</div>
              <div class="info-value">{{ userInfo.email || '未设置' }}</div>
            </div>
          </div>
        </div>
      </div>

      <!-- 操作按钮区域 -->
      <div v-if="!loading && !error" class="action-section" :class="{ 'fade-in': isLoaded }">
        <button v-if="showSwitchToAdmin" class="action-btn admin-btn" @click="goToAdmin">
          <span class="btn-icon">🏠</span>
          <span class="btn-text">鸟场管理系统</span>
        </button>

        <button class="action-btn edit-btn" @click="openEditDialog">
          <span class="btn-icon">✏️</span>
          <span class="btn-text">修改账号信息</span>
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

    <!-- 修改账号信息对话框 -->
    <div v-if="editDialogVisible" class="dialog-overlay" @click="editDialogVisible = false">
      <div class="dialog-content edit-dialog" @click.stop>
        <div class="dialog-title">修改账号信息</div>
        <div class="edit-form">
          <div class="form-item">
            <label class="form-label">用户名</label>
            <input v-model="editForm.username" type="text" class="form-input" placeholder="请输入用户名" />
          </div>
          <div class="form-item">
            <label class="form-label">昵称</label>
            <input v-model="editForm.nickname" type="text" class="form-input" placeholder="请输入昵称" />
          </div>
          <div class="form-item">
            <label class="form-label">邮箱</label>
            <input v-model="editForm.email" type="email" class="form-input" placeholder="请输入邮箱" />
          </div>
          <div class="form-item">
            <label class="form-label">头像URL</label>
            <input v-model="editForm.avatar" type="text" class="form-input" placeholder="请输入头像图片URL" />
          </div>
          <div class="form-item">
            <label class="form-label">
              <input v-model="editForm.changePassword" type="checkbox" class="form-checkbox" />
              修改密码
            </label>
          </div>
          <div v-if="editForm.changePassword" class="password-section">
            <div class="form-item">
              <label class="form-label">当前密码</label>
              <input v-model="editForm.currentPassword" type="password" class="form-input" placeholder="请输入当前密码" />
            </div>
            <div class="form-item">
              <label class="form-label">新密码</label>
              <input v-model="editForm.newPassword" type="password" class="form-input" placeholder="请输入新密码" />
            </div>
            <div class="form-item">
              <label class="form-label">确认新密码</label>
              <input v-model="editForm.confirmNewPassword" type="password" class="form-input" placeholder="请确认新密码" />
            </div>
          </div>
        </div>
        <div class="dialog-actions">
          <button class="dialog-btn cancel-btn" @click="editDialogVisible = false">取消</button>
          <button class="dialog-btn confirm-btn edit-confirm-btn" @click="saveChanges" :disabled="saving">
            {{ saving ? '保存中...' : '保存修改' }}
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
const editDialogVisible = ref(false);
const saving = ref(false);
const confirmDeleteDialogVisible = ref(false);
const deletingAccount = ref(false);
const verifyPasswordDialogVisible = ref(false);
const verifyingPassword = ref(false);
const showPassword = ref(false);
const isLoaded = ref(false);

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

const editForm = ref({
  username: '',
  nickname: '',
  email: '',
  avatar: '',
  changePassword: false,
  currentPassword: '',
  newPassword: '',
  confirmNewPassword: ''
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
    // 延迟显示动画
    setTimeout(() => {
      isLoaded.value = true;
    }, 100);
  }
};

// 退出登录
const handleLogout = async () => {
  try {
    await api.post('/api/logout');
  } catch (error) {
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

// 打开修改账号信息对话框
const openEditDialog = () => {
  editForm.value = {
    username: userInfo.value.username,
    nickname: userInfo.value.nickname,
    email: userInfo.value.email || '',
    avatar: userInfo.value.avatar || '',
    changePassword: false,
    currentPassword: '',
    newPassword: '',
    confirmNewPassword: ''
  };
  editDialogVisible.value = true;
};

// 验证当前密码
const verifyPassword = async (username: string, password: string): Promise<boolean> => {
  try {
    const params = new URLSearchParams();
    params.append('grant_type', 'password');
    params.append('username', username);
    params.append('password', password);
    params.append('scope', '');
    params.append('client_id', '');
    params.append('client_secret', '');

    const data = await api.post('/api/login', params.toString(), {
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
        'Accept': 'application/json',
      }
    });

    return data.access_token !== undefined;
  } catch (error) {
    return false;
  }
};

// 保存修改
const saveChanges = async () => {
  try {
    // 验证表单
    if (!editForm.value.username || editForm.value.username.length < 3 || editForm.value.username.length > 20) {
      ElMessage.warning('用户名长度在 3 到 20 个字符');
      return;
    }

    if (!editForm.value.nickname || editForm.value.nickname.length < 1 || editForm.value.nickname.length > 50) {
      ElMessage.warning('昵称长度在 1 到 50 个字符');
      return;
    }

    if (editForm.value.email && !/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(editForm.value.email)) {
      ElMessage.warning('请输入正确的邮箱地址');
      return;
    }

    if (editForm.value.changePassword) {
      if (!editForm.value.currentPassword) {
        ElMessage.warning('请输入当前密码');
        return;
      }
      if (!editForm.value.newPassword || editForm.value.newPassword.length < 8 || editForm.value.newPassword.length > 32) {
        ElMessage.warning('新密码长度在 8 到 32 个字符');
        return;
      }
      if (!/^(?=.*[a-zA-Z])(?=.*\d)[a-zA-Z\d]+$/.test(editForm.value.newPassword)) {
        ElMessage.warning('新密码必须同时包含字母和数字');
        return;
      }
      if (editForm.value.newPassword !== editForm.value.confirmNewPassword) {
        ElMessage.warning('两次输入的密码不一致');
        return;
      }
    }

    saving.value = true;

    // 如果用户选择修改密码，先验证当前密码
    if (editForm.value.changePassword) {
      const isPasswordValid = await verifyPassword(editForm.value.username, editForm.value.currentPassword);
      if (!isPasswordValid) {
        throw new Error('当前密码不正确');
      }
    }

    // 构建请求数据
    const requestData: Record<string, string> = {};

    if (editForm.value.username !== userInfo.value.username) {
      requestData["username"] = editForm.value.username;
    }

    if (editForm.value.nickname !== userInfo.value.nickname) {
      requestData["nickname"] = editForm.value.nickname;
    }

    if (editForm.value.email !== (userInfo.value.email || '')) {
      requestData["email"] = editForm.value.email || "";
    }

    if (editForm.value.avatar && editForm.value.avatar !== userInfo.value.avatar) {
      requestData["avatar"] = editForm.value.avatar;
    }

    if (editForm.value.changePassword) {
      requestData["password"] = editForm.value.newPassword;
    }

    // 发送请求
    const responseData = await api.put(`/api/users/${userInfo.value.id}`, requestData);

    if (responseData.code === 200) {
      ElMessage.success('修改账号信息成功');
      editDialogVisible.value = false;
      // 重新获取用户信息
      await fetchUserInfo();
      // 更新localStorage
      localStorage.setItem('username', editForm.value.username);
      localStorage.setItem('avatar', editForm.value.avatar || '');
      // 触发登录状态变化事件
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
    } else {
      if (responseData.detail && Array.isArray(responseData.detail) && responseData.detail.length > 0) {
        const errorMessages = responseData.detail.map((error: any) => error.msg).join(', ');
        throw new Error(errorMessages);
      } else {
        throw new Error(responseData.msg || '修改账号信息失败');
      }
    }
  } catch (err: any) {
    ElMessage.error(err.message || '修改账号信息失败');
  } finally {
    saving.value = false;
  }
};

onMounted(() => {
  fetchUserInfo();
});
</script>

<style scoped>
.my-page {
  min-height: 100vh;
  background: linear-gradient(135deg, #f0fdf4 0%, #dcfce7 100%);
  position: relative;
  overflow: hidden;
  padding: 20px 16px;
  padding-bottom: 100px;
  box-sizing: border-box;
}

/* 背景装饰 */
.bg-decoration {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  pointer-events: none;
  z-index: 0;
}

.bg-ball {
  position: absolute;
  border-radius: 50%;
  filter: blur(40px);
  opacity: 0.3;
}

.bg-ball-1 {
  width: 300px;
  height: 300px;
  background: #a3e635;
  top: -100px;
  right: -100px;
  animation: float 6s ease-in-out infinite;
}

.bg-ball-2 {
  width: 200px;
  height: 200px;
  background: #65a30d;
  bottom: -50px;
  left: -50px;
  animation: float 8s ease-in-out infinite reverse;
}

.bg-ball-3 {
  width: 150px;
  height: 150px;
  background: #84cc16;
  top: 50%;
  left: 20%;
  animation: float 10s ease-in-out infinite;
}

.floating-element {
  position: absolute;
  width: 4px;
  height: 4px;
  background: rgba(139, 173, 66, 0.6);
  border-radius: 50%;
  animation: floatUp 10s linear infinite;
}

.floating-1 {
  top: 20%;
  left: 10%;
  animation-delay: 0s;
}

.floating-2 {
  top: 60%;
  right: 15%;
  animation-delay: 2s;
}

.floating-3 {
  top: 40%;
  left: 80%;
  animation-delay: 4s;
}

.floating-4 {
  top: 80%;
  right: 25%;
  animation-delay: 6s;
}

@keyframes float {
  0%, 100% {
    transform: translateY(0) rotate(0deg);
  }
  50% {
    transform: translateY(-20px) rotate(180deg);
  }
}

@keyframes floatUp {
  0% {
    transform: translateY(100vh) scale(0);
    opacity: 1;
  }
  100% {
    transform: translateY(-100px) scale(1);
    opacity: 0;
  }
}

/* 主内容区 */
.main-content {
  position: relative;
  z-index: 1;
  max-width: 400px;
  margin: 0 auto;
  display: flex;
  flex-direction: column;
  gap: 32px;
}

/* 头像和用户信息 */
.profile-header {
  text-align: center;
  padding: 20px 0;
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out;
}

.avatar-container {
  margin-bottom: 20px;
}

.avatar-circle {
  width: 120px;
  height: 120px;
  border-radius: 50%;
  overflow: hidden;
  margin: 0 auto;
  box-shadow: 0 8px 32px rgba(139, 173, 66, 0.3);
  border: 4px solid rgba(255, 255, 255, 0.8);
  transition: all 0.3s ease;
}

.avatar-circle:hover {
  transform: scale(1.05);
  box-shadow: 0 12px 40px rgba(139, 173, 66, 0.4);
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
  font-size: 48px;
  font-weight: bold;
}

.username {
  margin: 0 0 12px 0;
  font-size: 28px;
  font-weight: 700;
  color: #166534;
  text-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.role-badge {
  display: inline-block;
  color: white;
  padding: 8px 20px;
  border-radius: 24px;
  font-size: 14px;
  font-weight: 600;
  letter-spacing: 0.5px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  transition: all 0.3s ease;
}

.role-badge:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.2);
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
.loading-section {
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  min-height: 300px;
  gap: 16px;
}

.spinner {
  width: 50px;
  height: 50px;
  border: 4px solid rgba(139, 173, 66, 0.2);
  border-top-color: #8BAD42;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

@keyframes spin {
  to { transform: rotate(360deg); }
}

.loading-text {
  color: #6b7280;
  font-size: 16px;
  font-weight: 500;
}

/* 错误状态 */
.error-section {
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  min-height: 300px;
  gap: 16px;
  text-align: center;
  padding: 0 20px;
}

.error-icon {
  font-size: 64px;
  display: block;
}

.error-text {
  display: block;
  color: #f56c6c;
  font-size: 16px;
  font-weight: 500;
  margin-bottom: 8px;
}

.retry-btn {
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
  color: white;
  border: none;
  padding: 12px 28px;
  border-radius: 16px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  box-shadow: 0 4px 12px rgba(139, 173, 66, 0.3);
}

.retry-btn:hover {
  transform: translateY(-3px);
  box-shadow: 0 6px 16px rgba(139, 173, 66, 0.4);
  background: linear-gradient(135deg, #6A9A35 0%, #8BAD42 100%);
}

.retry-btn:active {
  transform: translateY(0);
  box-shadow: 0 2px 8px rgba(139, 173, 66, 0.3);
}

/* 个人信息区域 */
.info-section {
  background: rgba(255, 255, 255, 0.7);
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
  border-radius: 24px;
  padding: 32px 24px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(255, 255, 255, 0.5);
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.2s;
}

.section-title {
  font-size: 20px;
  font-weight: 700;
  color: #166534;
  margin: 0 0 24px 0;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
}

.info-grid {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.info-item {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 16px;
  background: rgba(255, 255, 255, 0.5);
  border-radius: 16px;
  transition: all 0.3s ease;
}

.info-item:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.05);
  background: rgba(255, 255, 255, 0.8);
}

.info-icon {
  font-size: 24px;
  width: 48px;
  height: 48px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(139, 173, 66, 0.1);
  border-radius: 12px;
  flex-shrink: 0;
}

.info-content {
  flex: 1;
}

.info-label {
  font-size: 14px;
  color: #6b7280;
  font-weight: 500;
  margin-bottom: 4px;
}

.info-value {
  font-size: 16px;
  color: #166534;
  font-weight: 600;
}

/* 操作按钮区域 */
.action-section {
  display: flex;
  flex-direction: column;
  gap: 16px;
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.4s;
}

.action-btn {
  width: 100%;
  height: 56px;
  border-radius: 16px;
  border: none;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  position: relative;
  overflow: hidden;
}

.action-btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.action-btn:hover::before {
  left: 100%;
}

.action-btn:hover {
  transform: translateY(-3px);
  box-shadow: 0 6px 20px rgba(0, 0, 0, 0.15);
}

.action-btn:active {
  transform: translateY(0);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.logout-btn {
  background: linear-gradient(135deg, #8BAD42 0%, #A4C65F 100%);
  color: white;
  box-shadow: 0 4px 16px rgba(139, 173, 66, 0.3);
}

.logout-btn:hover {
  background: linear-gradient(135deg, #6A9A35 0%, #8BAD42 100%);
  box-shadow: 0 6px 20px rgba(139, 173, 66, 0.4);
}

.delete-btn {
  background: linear-gradient(135deg, #f87171 0%, #ef4444 100%);
  color: white;
  box-shadow: 0 4px 16px rgba(239, 68, 68, 0.3);
}

.delete-btn:hover {
  background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
  box-shadow: 0 6px 20px rgba(239, 68, 68, 0.4);
}

.admin-btn {
  background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
  color: white;
  box-shadow: 0 4px 16px rgba(59, 130, 246, 0.3);
}

.admin-btn:hover {
  background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
  box-shadow: 0 6px 20px rgba(59, 130, 246, 0.4);
}

.edit-btn {
  background: linear-gradient(135deg, #8BAD42 0%, #A4C65F 100%);
  color: white;
  box-shadow: 0 4px 16px rgba(139, 173, 66, 0.3);
}

.edit-btn:hover {
  background: linear-gradient(135deg, #6A9A35 0%, #8BAD42 100%);
  box-shadow: 0 6px 20px rgba(139, 173, 66, 0.4);
}

.btn-icon {
  font-size: 20px;
}

.btn-text {
  font-weight: 600;
}

/* 动画类 */
.fade-in {
  opacity: 1 !important;
  transform: translateY(0) !important;
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
  border-radius: 24px;
  padding: 32px 24px;
  width: 100%;
  max-width: 340px;
  animation: slideUp 0.3s ease;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.15);
  border: 1px solid rgba(255, 255, 255, 0.5);
}

@keyframes slideUp {
  from {
    opacity: 0;
    transform: translateY(20px) scale(0.95);
  }
  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}

.dialog-title {
  font-size: 20px;
  font-weight: 700;
  color: #166534;
  margin-bottom: 16px;
  text-align: center;
}

.dialog-message {
  font-size: 16px;
  color: #6b7280;
  line-height: 1.6;
  margin-bottom: 24px;
  text-align: center;
}

.dialog-actions {
  display: flex;
  gap: 16px;
}

.dialog-btn {
  flex: 1;
  height: 48px;
  border-radius: 12px;
  border: none;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  position: relative;
  overflow: hidden;
}

.dialog-btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.dialog-btn:hover::before {
  left: 100%;
}

.cancel-btn {
  background: #f5f7fa;
  color: #6b7280;
  border: 1px solid rgba(209, 213, 219, 0.5);
}

.cancel-btn:hover {
  background: #e5e7eb;
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
}

.confirm-btn {
  background: linear-gradient(135deg, #f87171 0%, #ef4444 100%);
  color: white;
  box-shadow: 0 4px 12px rgba(239, 68, 68, 0.3);
}

.confirm-btn:hover:not(:disabled) {
  background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(239, 68, 68, 0.4);
}

.confirm-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.dialog-btn:active:not(:disabled) {
  transform: translateY(0);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

/* 密码输入框样式 */
.password-input-wrapper {
  position: relative;
  margin-bottom: 24px;
}

.password-input {
  width: 100%;
  height: 52px;
  padding: 0 52px 0 20px;
  border: 2px solid rgba(209, 213, 219, 0.5);
  border-radius: 16px;
  font-size: 16px;
  color: #166534;
  background: rgba(255, 255, 255, 0.8);
  box-sizing: border-box;
  transition: all 0.3s ease;
}

.password-input:focus {
  outline: none;
  border-color: #8BAD42;
  background: white;
  box-shadow: 0 0 0 4px rgba(139, 173, 66, 0.1);
}

.password-input::placeholder {
  color: #9ca3af;
}

.toggle-password-btn {
  position: absolute;
  right: 16px;
  top: 50%;
  transform: translateY(-50%);
  background: none;
  border: none;
  font-size: 24px;
  cursor: pointer;
  padding: 4px;
  opacity: 0.7;
  transition: opacity 0.2s;
}

.toggle-password-btn:hover {
  opacity: 1;
}

/* 编辑对话框样式 */
.edit-dialog {
  max-height: 90vh;
  overflow-y: auto;
}

.edit-form {
  margin-bottom: 24px;
}

.form-item {
  margin-bottom: 16px;
}

.form-label {
  display: block;
  font-size: 14px;
  color: #6b7280;
  margin-bottom: 8px;
  font-weight: 500;
}

.form-input {
  width: 100%;
  height: 44px;
  padding: 0 16px;
  border: 2px solid rgba(209, 213, 219, 0.5);
  border-radius: 12px;
  font-size: 15px;
  color: #166534;
  background: rgba(255, 255, 255, 0.8);
  box-sizing: border-box;
  transition: all 0.3s ease;
}

.form-input:focus {
  outline: none;
  border-color: #8BAD42;
  background: white;
  box-shadow: 0 0 0 4px rgba(139, 173, 66, 0.1);
}

.form-input::placeholder {
  color: #9ca3af;
}

.form-checkbox {
  width: 18px;
  height: 18px;
  margin-right: 8px;
  vertical-align: middle;
  cursor: pointer;
}

.password-section {
  padding: 16px;
  background: rgba(139, 173, 66, 0.05);
  border-radius: 12px;
  margin-top: 12px;
}

.edit-confirm-btn {
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%) !important;
  box-shadow: 0 4px 12px rgba(139, 173, 66, 0.3) !important;
}

.edit-confirm-btn:hover:not(:disabled) {
  background: linear-gradient(135deg, #6A9A35 0%, #5A8A2D 100%) !important;
  box-shadow: 0 6px 16px rgba(139, 173, 66, 0.4) !important;
}

/* 响应式设计 */
@media (max-width: 768px) {
  .my-page {
    padding: 16px 12px;
    padding-bottom: 80px;
  }
  
  .main-content {
    gap: 24px;
  }
  
  .profile-header {
    padding: 16px 0;
  }
  
  .avatar-circle {
    width: 100px;
    height: 100px;
    border-width: 3px;
  }
  
  .default-avatar {
    font-size: 40px;
  }
  
  .username {
    font-size: 24px;
    margin-bottom: 10px;
  }
  
  .role-badge {
    padding: 6px 16px;
    font-size: 13px;
  }
  
  .info-section {
    padding: 24px 20px;
    border-radius: 20px;
  }
  
  .section-title {
    font-size: 18px;
    margin-bottom: 20px;
  }
  
  .info-item {
    padding: 12px;
    gap: 12px;
  }
  
  .info-icon {
    width: 40px;
    height: 40px;
    font-size: 20px;
  }
  
  .info-value {
    font-size: 14px;
  }
  
  .action-btn {
    height: 52px;
    font-size: 15px;
  }
  
  .dialog-content {
    padding: 24px 20px;
    border-radius: 20px;
  }
  
  .dialog-title {
    font-size: 18px;
  }
  
  .dialog-message {
    font-size: 14px;
  }
  
  .dialog-btn {
    height: 44px;
    font-size: 15px;
  }
  
  .password-input {
    height: 48px;
    padding: 0 48px 0 16px;
  }
  
  .toggle-password-btn {
    font-size: 20px;
  }
}

@media (max-width: 480px) {
  .avatar-circle {
    width: 80px;
    height: 80px;
  }
  
  .default-avatar {
    font-size: 32px;
  }
  
  .username {
    font-size: 20px;
  }
  
  .info-section {
    padding: 20px 16px;
  }
  
  .info-item {
    gap: 10px;
  }
  
  .info-icon {
    width: 36px;
    height: 36px;
    font-size: 18px;
  }
}
</style>
