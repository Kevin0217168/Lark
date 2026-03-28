<template>
  <div class="profile-container" :class="{ 'mobile': isMobile }">
    <!-- 桌面端：使用 el-card 包装 -->
    <el-card v-if="!isMobile" class="profile-card">
      <h3 class="title">个人信息</h3>

      <!-- 头像显示 -->
      <div class="avatar-wrapper">
        <div v-if="userInfo.avatar" class="avatar-circle">
          <el-image :src="userInfo.avatar" :alt="userInfo.nickname" fit="cover" />
        </div>
        <div v-else class="avatar-circle default-avatar">
          <span>{{ userInfo.nickname ? userInfo.nickname.charAt(0) : '?' }}</span>
        </div>
      </div>

      <div v-if="loading" class="loading-container">
        <el-icon class="is-loading"><svg-icon /></el-icon>
        <span>加载中...</span>
      </div>

      <div v-else-if="error" class="error-container">
        <el-icon><warning /></el-icon>
        <span>{{ error }}</span>
        <el-button type="primary" @click="fetchUserInfo">重新加载</el-button>
      </div>

      <el-form v-else :model="userInfo" label-width="120px">
        <el-form-item label="用户名">
          <el-input v-model="userInfo.username" readonly />
        </el-form-item>

        <el-form-item label="昵称">
          <el-input v-model="userInfo.nickname" readonly />
        </el-form-item>

        <el-form-item label="账号类型">
          <el-input :model-value="userInfo.role === 'root' ? '管理员' : '普通用户'" readonly />
        </el-form-item>

        <el-form-item label="邮箱">
          <el-input v-model="userInfo.email" readonly placeholder="暂无邮箱" />
        </el-form-item>
      </el-form>

      <div class="action-container">
        <el-button type="primary" @click="openEditDialog">修改账号信息</el-button>
        <el-button type="danger" @click="handleLogout">退出登录</el-button>
        <el-button type="danger" @click="openDeleteAccountDialog">注销账号</el-button>
      </div>
    </el-card>

    <!-- 移动端：全屏设计 -->
    <div v-else class="mobile-profile">
      <!-- 头像显示 -->
      <div class="avatar-section">
        <div v-if="userInfo.avatar" class="avatar-circle">
          <el-image :src="userInfo.avatar" :alt="userInfo.nickname" fit="cover" />
        </div>
        <div v-else class="avatar-circle default-avatar">
          <span>{{ userInfo.nickname ? userInfo.nickname.charAt(0) : '?' }}</span>
        </div>
        <h3 class="username">{{ userInfo.nickname || userInfo.username }}</h3>
        <el-tag :type="userInfo.role === 'root' ? 'danger' : 'info'" size="small">
          {{ userInfo.role === 'root' ? '管理员' : '普通用户' }}
        </el-tag>
      </div>

      <div v-if="loading" class="loading-container">
        <el-icon class="is-loading"><loading /></el-icon>
        <span>加载中...</span>
      </div>

      <div v-else class="info-section">
        <!-- 错误信息 -->
        <div v-if="error" class="error-container">
          <el-icon><warning /></el-icon>
          <span>{{ error }}</span>
          <el-button type="primary" size="small" @click="fetchUserInfo">重新加载</el-button>
        </div>

        <!-- 用户信息卡片 -->
        <div v-if="!error" class="info-card">
          <div class="info-item">
            <span class="label">用户名</span>
            <span class="value">{{ userInfo.username }}</span>
          </div>
          <div class="info-item">
            <span class="label">昵称</span>
            <span class="value">{{ userInfo.nickname || '未设置' }}</span>
          </div>
          <div class="info-item">
            <span class="label">邮箱</span>
            <span class="value">{{ userInfo.email || '未设置' }}</span>
          </div>
        </div>

        <!-- 操作按钮 -->
        <div class="action-section">
          <el-button v-if="!error" type="primary" @click="openEditDialog" class="action-btn">
            <el-icon><edit /></el-icon>
            修改账号信息
          </el-button>
          <el-button v-if="!error && isAdmin" type="warning" @click="goToUserManage" class="action-btn">
            <el-icon><user /></el-icon>
            用户管理
          </el-button>
          <el-button v-if="!error" type="danger" plain @click="openDeleteAccountDialog" class="action-btn">
            <el-icon><delete /></el-icon>
            注销账号
          </el-button>
          <el-button type="danger" @click="handleLogout" class="action-btn">
            <el-icon><switch-button /></el-icon>
            退出登录
          </el-button>
        </div>
      </div>

      <!-- 备案信息 -->
      <MobileBeian v-if="isMobile" />
    </div>

    <!-- 验证密码对话框 -->
    <el-dialog
      v-model="verifyPasswordDialogVisible"
      title="验证密码"
      :width="isMobile ? '90%' : '400px'"
    >
      <el-form
        :model="verifyPasswordForm"
        ref="verifyPasswordFormRef"
        :rules="verifyPasswordRules"
        :label-width="isMobile ? '100px' : '100px'"
      >
        <el-form-item label="当前密码" prop="password">
          <el-input v-model="verifyPasswordForm.password" type="password" placeholder="请输入当前密码" />
        </el-form-item>
      </el-form>

      <template #footer>
        <span class="dialog-footer">
          <el-button @click="verifyPasswordDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="verifyPasswordAndConfirmDelete" :loading="verifyingPassword">验证</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 确认注销账号对话框 -->
    <el-dialog
      v-model="confirmDeleteDialogVisible"
      title="确认注销账号"
      :width="isMobile ? '90%' : '400px'"
    >
      <p>确定要注销账号吗？此操作不可恢复，您的所有数据将被永久删除。</p>

      <template #footer>
        <span class="dialog-footer">
          <el-button @click="confirmDeleteDialogVisible = false">取消</el-button>
          <el-button type="danger" @click="handleDeleteAccount" :loading="deletingAccount">确认注销</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 修改账号信息对话框 -->
    <el-dialog
      v-model="editDialogVisible"
      title="修改账号信息"
      :width="isMobile ? '90%' : '500px'"
    >
      <el-form
        :model="editForm"
        ref="editFormRef"
        :rules="editRules"
        :label-width="isMobile ? '100px' : '120px'"
      >
        <el-form-item label="用户名" prop="username">
          <el-input v-model="editForm.username" />
        </el-form-item>

        <el-form-item label="昵称" prop="nickname">
          <el-input v-model="editForm.nickname" />
        </el-form-item>

        <el-form-item label="邮箱" prop="email">
          <el-input v-model="editForm.email" placeholder="请输入邮箱" />
        </el-form-item>

        <el-form-item label="头像URL" prop="avatar">
          <el-input v-model="editForm.avatar" placeholder="请输入头像图片URL" />
        </el-form-item>

        <el-form-item>
          <el-checkbox v-model="editForm.changePassword">修改密码</el-checkbox>
        </el-form-item>

        <el-form-item v-if="editForm.changePassword" label="当前密码" prop="currentPassword">
          <el-input v-model="editForm.currentPassword" type="password" placeholder="请输入当前密码" />
        </el-form-item>

        <el-form-item v-if="editForm.changePassword" label="新密码" prop="newPassword">
          <el-input v-model="editForm.newPassword" type="password" placeholder="请输入新密码" />
        </el-form-item>

        <el-form-item v-if="editForm.changePassword" label="确认新密码" prop="confirmNewPassword">
          <el-input v-model="editForm.confirmNewPassword" type="password" placeholder="请确认新密码" />
        </el-form-item>
      </el-form>

      <template #footer>
        <span class="dialog-footer">
          <el-button @click="editDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="saveChanges" :loading="saving">保存修改</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from "vue";
import { useRouter } from "vue-router";
import { ElMessage } from "element-plus";
import { Edit, SwitchButton, Delete, Loading, Warning, User } from '@element-plus/icons-vue';
import MobileBeian from './mobile/MobileBeian.vue';
import { api } from '../utils/api';

const router = useRouter();
const loading = ref(false);
const error = ref('');
const editDialogVisible = ref(false);
const saving = ref(false);
const editFormRef = ref();
const verifyPasswordDialogVisible = ref(false);
const confirmDeleteDialogVisible = ref(false);
const verifyingPassword = ref(false);
const deletingAccount = ref(false);
const verifyPasswordFormRef = ref();

// 检测是否为移动设备
const isMobile = ref(window.innerWidth < 768);

// 监听窗口大小变化
const handleWindowResize = () => {
  isMobile.value = window.innerWidth < 768;
};

onMounted(() => {
  window.addEventListener('resize', handleWindowResize);
  fetchUserInfo();
});

onUnmounted(() => {
  window.removeEventListener('resize', handleWindowResize);
});

const editForm = ref({
  username: '',
  nickname: '',
  email: null as string | null,
  avatar: '',
  changePassword: false,
  currentPassword: '',
  newPassword: '',
  confirmNewPassword: ''
});

const verifyPasswordForm = ref({
  password: ''
});

const verifyPasswordRules = {
  password: [
    { required: true, message: '请输入当前密码', trigger: 'blur' },
    { min: 6, max: 20, message: '密码长度在 6 到 20 个字符', trigger: 'blur' }
  ]
};

const editRules = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, max: 20, message: '用户名长度在 3 到 20 个字符', trigger: 'blur' },
    { pattern: /^[a-zA-Z0-9][a-zA-Z0-9_]*[a-zA-Z0-9]$/, message: '用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾', trigger: 'blur' }
  ],
  nickname: [
    { required: true, message: '请输入昵称', trigger: 'blur' },
    { min: 1, max: 50, message: '昵称长度在 1 到 50 个字符', trigger: 'blur' }
  ],
  email: [
    { type: 'email', message: '请输入正确的邮箱地址', trigger: 'blur' }
  ],
  currentPassword: [
    {
      required: (rule: any, value: string, callback: Function) => {
        return editForm.value.changePassword;
      },
      message: '请输入当前密码',
      trigger: 'blur'
    }
  ],
  newPassword: [
    {
      required: (rule: any, value: string, callback: Function) => {
        return editForm.value.changePassword;
      },
      message: '请输入新密码',
      trigger: 'blur'
    },
    { min: 8, max: 32, message: '密码长度在 8 到 32 个字符', trigger: 'blur' },
    { pattern: /^(?=.*[a-zA-Z])(?=.*\d)[a-zA-Z\d]+$/, message: '密码必须同时包含字母和数字', trigger: 'blur' }
  ],
  confirmNewPassword: [
    {
      required: (rule: any, value: string, callback: Function) => {
        return editForm.value.changePassword;
      },
      message: '请确认新密码',
      trigger: 'blur'
    },
    {
      validator: (rule: any, value: string, callback: Function) => {
        if (editForm.value.changePassword && value !== editForm.value.newPassword) {
          callback(new Error('两次输入的密码不一致'));
        } else {
          callback();
        }
      },
      trigger: 'blur'
    }
  ]
};

const userInfo = ref({
  id: 0,
  username: '',
  nickname: '',
  role: '',
  email: null as string | null,
  banner: null as string | null,
  avatar: ''
});

const isAdmin = computed(() => userInfo.value.role === 'root');

const goToUserManage = () => {
  router.push('/UserManage');
};

// 获取用户信息
const fetchUserInfo = async () => {
  try {
    loading.value = true;
    error.value = '';

    const responseData = await api.get('/api/users/me');
    if (responseData.code === 200 && responseData.data) {
      userInfo.value = responseData.data;
      // 更新localStorage中的用户名和头像信息，确保Header中的头像同步更新
      localStorage.setItem('username', userInfo.value.username || '');
      localStorage.setItem('avatar', userInfo.value.avatar || '');
      // 触发登录状态变化事件，通知Header组件更新
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
    } else {
      throw new Error(responseData.msg || '获取用户信息失败');
    }
  } catch (err) {
    error.value = (err as Error).message || '获取用户信息失败';
  } finally {
    loading.value = false;
  }
};

// 尝试刷新token
const tryRefreshToken = async (): Promise<boolean> => {
  try {
    const data = await api.post('/api/refresh');
    if (data.access_token) {
      localStorage.setItem('accessToken', data.access_token);
      localStorage.setItem('tokenType', data.token_type || 'bearer');
      return true;
    }
    return false;
  } catch (error) {
    console.error('刷新token失败:', error);
    return false;
  }
};

// 打开注销账号对话框
const openDeleteAccountDialog = async () => {
  try {
    // 如果当前用户是管理员，先检查管理员账号数量
    if (userInfo.value.role === 'root') {
      const usersData = await api.get('/api/users');
      if (usersData.code === 200 && usersData.data) {
        // 统计管理员账号数量
        const adminCount = usersData.data.filter((user: any) => user.role === 'root').length;
        
        // 如果只有一个管理员账号，不允许注销
        if (adminCount <= 1) {
          ElMessage.error('系统中只有一个管理员账号，无法注销。请先创建其他管理员账号。');
          return;
        }
      }
    }

    // 检查通过，打开密码验证对话框
    verifyPasswordForm.value.password = '';
    verifyPasswordDialogVisible.value = true;
  } catch (err) {
    ElMessage.error('检查管理员账号数量失败');
    console.error(err);
  }
};

// 验证密码并打开确认对话框
const verifyPasswordAndConfirmDelete = async () => {
  try {
    await verifyPasswordFormRef.value.validate();
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

    // 先调用 /api/users/me 获取当前账号id
    const meResponseData = await api.get('/api/users/me');
    if (meResponseData.code !== 200 || !meResponseData.data) {
      throw new Error('获取账号信息失败');
    }

    const userId = meResponseData.data.id;

    // 再调用 /api/users/{id} 删除账号
    const deleteResponseData = await api.delete(`/api/users/${userId}`);
    if (deleteResponseData.code === 200) {
      ElMessage.success('账号已注销');
      // 清除登录状态和token信息
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
            document.cookie = `${name}=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=.${window.location.hostname.split('.').slice(-2).join('.')}`;
          }
        }
      };
      
      clearCookies();
      
      // 触发自定义事件，通知其他组件登录状态已变化
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
      
      // 重定向到登录页面
      router.push('/Login');
    } else {
      throw new Error(deleteResponseData.msg || '注销账号失败');
    }
  } catch (err) {
    ElMessage.error((err as Error).message || '注销账号失败');
  } finally {
    deletingAccount.value = false;
  }
};

// 退出登录
const handleLogout = async () => {
  try {
    const data = await api.post('/api/logout');
    console.log('退出登录成功:', data);
  } catch (error) {
    console.error('退出登录请求出错:', error);
  }
  
  // 清除登录状态和token信息
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
        document.cookie = `${name}=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=.${window.location.hostname.split('.').slice(-2).join('.')}`;
      }
    }
  };

  clearCookies();

  // 触发自定义事件，通知其他组件登录状态已变化
  window.dispatchEvent(new CustomEvent('loginStatusChanged'));

  // 显示退出成功提示
  ElMessage.success('已退出登录');

  // 重定向到登录页面
  router.push('/Login');
};

// 打开修改账号信息对话框
const openEditDialog = () => {
  // 填充表单数据
  editForm.value = {
    username: userInfo.value.username,
    nickname: userInfo.value.nickname,
    email: userInfo.value.email,
    avatar: userInfo.value.avatar || '',
    changePassword: false,
    currentPassword: '',
    newPassword: '',
    confirmNewPassword: ''
  };
  editDialogVisible.value = true;
};

// 验证当前密码 - 使用与Login.vue相同的登录逻辑
const verifyPassword = async (username: string, password: string): Promise<boolean> => {
  try {
    // 构建请求参数，符合OAuth2密码模式（与Login.vue相同的格式）
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
    console.error('验证密码失败:', error);
    return false;
  }
};

// 保存修改
const saveChanges = async () => {
  try {
    // 验证表单，如果失败会抛出错误
    await editFormRef.value.validate((valid: boolean, invalidFields: any) => {
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
    
    saving.value = true;

    // 如果用户选择修改密码，先验证当前密码
    if (editForm.value.changePassword) {
      const isPasswordValid = await verifyPassword(editForm.value.username, editForm.value.currentPassword);
      if (!isPasswordValid) {
        throw new Error('当前密码不正确');
      }
    }

    // 构建请求数据，只包含用户实际修改的字段
    const requestData: Record<string, string> = {};
    
    // 只包含用户名、昵称、邮箱、头像字段（这些字段用户可以修改）
    if (editForm.value.username !== userInfo.value.username) {
      requestData["username"] = editForm.value.username || "";
    }
    
    if (editForm.value.nickname !== userInfo.value.nickname) {
      requestData["nickname"] = editForm.value.nickname || "";
    }
    
    if (editForm.value.email !== userInfo.value.email) {
      requestData["email"] = editForm.value.email || "";
    }
    
    // 只有当头像URL有实际值且发生变化时才发送
    if (editForm.value.avatar && editForm.value.avatar !== userInfo.value.avatar) {
      requestData["avatar"] = editForm.value.avatar;
    }
    
    // 如果用户选择修改密码，才包含password字段
    if (editForm.value.changePassword) {
      requestData["password"] = editForm.value.newPassword || "";
    }

    // 发送请求
    const responseData = await api.put(`/api/users/${userInfo.value.id}`, requestData);
    
    if (responseData.code === 200) {
      ElMessage.success('修改账号信息成功');
      editDialogVisible.value = false;
      // 重新获取用户信息
      await fetchUserInfo();
      // 更新localStorage中的用户名和头像
      localStorage.setItem('username', editForm.value.username);
      localStorage.setItem('avatar', editForm.value.avatar || '');
      // 触发登录状态变化事件，通知Header组件更新
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
    } else {
      if (responseData.detail && Array.isArray(responseData.detail) && responseData.detail.length > 0) {
        const errorMessages = responseData.detail.map((error: any) => error.msg).join(', ');
        throw new Error(errorMessages);
      } else {
        throw new Error(responseData.msg || '修改账号信息失败');
      }
    }
  } catch (err) {
    const errorMessage = (err as Error).message;
    
    // 根据错误类型显示不同的错误信息
    if (errorMessage === '表单验证失败') {
      // 表单验证失败已经在validate回调中处理过，这里不需要重复显示
    } else if (errorMessage === '当前密码不正确') {
      ElMessage.error(errorMessage);
    } else if (errorMessage.includes('Failed to fetch') || errorMessage.includes('NetworkError')) {
      ElMessage.error('网络连接失败，请检查网络设置');
    } else {
      ElMessage.error(errorMessage || '修改账号信息失败');
    }
  } finally {
    saving.value = false;
  }
};
</script>

<style lang="scss" scoped>
.profile-container {
  min-height: 100vh;
  position: relative;
}

/* 桌面端样式 */
.profile-card {
  z-index: 1;
  padding: 20px;
  width: 600px;
  margin: 5% auto;
  border-radius: 15px;
}

.profile-card .title {
  text-align: center;
  margin-bottom: 20px;
  color: #303133;
}

.profile-card :deep(.el-form-item) {
  margin-bottom: 36px;
}

.profile-card :deep(.el-form-item__error) {
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

.profile-card .loading-container,
.profile-card .error-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 40px 0;
  gap: 16px;
}

.profile-card .avatar-wrapper {
  display: flex;
  justify-content: center;
  margin-bottom: 30px;
}

.profile-card .avatar-circle {
  width: 120px;
  height: 120px;
  border-radius: 50%;
  overflow: hidden;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
  
  img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }
}

.profile-card .default-avatar {
  background-color: #8bad42;
  display: flex;
  justify-content: center;
  align-items: center;
  color: white;
  font-size: 48px;
  font-weight: bold;
}

.profile-card .action-container {
  margin-top: 30px;
  display: flex;
  justify-content: center;
  gap: 20px;
}

/* 移动端样式 */
.mobile-profile {
  min-height: calc(100vh - 120px);
  background: #f5f7fa;
  padding: 0;
  display: flex;
  flex-direction: column;
}

/* 头像区域 */
.mobile-profile .avatar-section {
  background: white;
  padding: 32px 16px;
  text-align: center;
  margin-bottom: 16px;
}

.mobile-profile .avatar-circle {
  width: 80px;
  height: 80px;
  border-radius: 50%;
  overflow: hidden;
  margin: 0 auto 16px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.mobile-profile .avatar-circle .el-image {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.mobile-profile .default-avatar {
  background: linear-gradient(135deg, #8bad42 0%, #6a9a35 100%);
  display: flex;
  justify-content: center;
  align-items: center;
  color: white;
  font-size: 32px;
  font-weight: bold;
}

.mobile-profile .username {
  margin: 0 0 8px 0;
  font-size: 20px;
  font-weight: 600;
  color: #303133;
}

/* 加载和错误状态 */
.mobile-profile .loading-container,
.mobile-profile .error-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 40px 16px;
  color: #909399;
}

.mobile-profile .loading-container .el-icon,
.mobile-profile .error-container .el-icon {
  font-size: 32px;
  margin-bottom: 12px;
}

.mobile-profile .error-container {
  color: #f56c6c;
  margin-bottom: 16px;
}

/* 信息卡片 */
.mobile-profile .info-card {
  background: white;
  padding: 16px;
  margin-bottom: 16px;
  border-radius: 12px;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);
}

.mobile-profile .info-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 0;
  border-bottom: 1px solid #f0f0f0;
}

.mobile-profile .info-item:last-child {
  border-bottom: none;
}

.mobile-profile .info-item .label {
  font-size: 14px;
  color: #909399;
}

.mobile-profile .info-item .value {
  font-size: 14px;
  color: #303133;
  font-weight: 500;
  text-align: right;
  flex: 1;
  margin-left: 16px;
}

/* 操作按钮 */
.mobile-profile .action-section {
  padding: 0 16px 20px;
  display: flex;
  flex-direction: column;
  align-items: stretch;
  gap: 12px;
  box-sizing: border-box;
  width: 100%;
}

.mobile-profile .action-section .action-btn {
  width: 100%;
  max-width: 400px;
  margin: 0 auto;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  height: 48px;
  border-radius: 12px;
  box-sizing: border-box;
}

/* 对话框样式 */
.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}
</style>