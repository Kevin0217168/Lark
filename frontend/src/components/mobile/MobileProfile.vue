<template>
  <div class="mobile-profile">
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

    <div v-else-if="error" class="error-container">
      <el-icon><warning /></el-icon>
      <span>{{ error }}</span>
      <el-button type="primary" size="small" @click="fetchUserInfo">重新加载</el-button>
    </div>

    <div v-else class="info-section">
      <!-- 用户信息卡片 -->
      <div class="info-card">
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
        <el-button type="primary" @click="openEditDialog" class="action-btn">
          <el-icon><edit /></el-icon>
          修改账号信息
        </el-button>
        <el-button v-if="isAdmin" type="warning" @click="goToUserManage" class="action-btn">
          <el-icon><user /></el-icon>
          用户管理
        </el-button>
        <el-button type="danger" @click="handleLogout" class="action-btn">
          <el-icon><switch-button /></el-icon>
          退出登录
        </el-button>
        <el-button type="danger" plain @click="openDeleteAccountDialog" class="action-btn">
          <el-icon><delete /></el-icon>
          注销账号
        </el-button>
      </div>
    </div>

    <!-- 验证密码对话框 -->
    <el-dialog v-model="verifyPasswordDialogVisible" title="验证密码" width="90%">
      <el-form :model="verifyPasswordForm" ref="verifyPasswordFormRef" :rules="verifyPasswordRules">
        <el-form-item label="当前密码" prop="password">
          <el-input v-model="verifyPasswordForm.password" type="password" placeholder="请输入当前密码" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="verifyPasswordDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="verifyPasswordAndConfirmDelete" :loading="verifyingPassword">验证</el-button>
      </template>
    </el-dialog>

    <!-- 确认注销账号对话框 -->
    <el-dialog v-model="confirmDeleteDialogVisible" title="确认注销账号" width="90%">
      <p>确定要注销账号吗？此操作不可恢复，您的所有数据将被永久删除。</p>
      <template #footer>
        <el-button @click="confirmDeleteDialogVisible = false">取消</el-button>
        <el-button type="danger" @click="handleDeleteAccount" :loading="deletingAccount">确认注销</el-button>
      </template>
    </el-dialog>

    <!-- 修改账号信息对话框 -->
    <el-dialog v-model="editDialogVisible" title="修改账号信息" width="90%">
      <el-form :model="editForm" ref="editFormRef" :rules="editRules" label-width="100px">
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
        <el-button @click="editDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="saveChanges" :loading="saving">保存修改</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, computed } from "vue";
import { useRouter } from "vue-router";
import { ElMessage } from "element-plus";
import { Edit, SwitchButton, Delete, Loading, Warning, User } from '@element-plus/icons-vue';

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
    { pattern: /^(?=.*[a-zA-Z0-9])[a-zA-Z0-9]+$/, message: '密码必须包含至少一个数字或字母', trigger: 'blur' }
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

const fetchUserInfo = async () => {
  try {
    loading.value = true;
    error.value = '';

    const accessToken = localStorage.getItem('accessToken');
    if (!accessToken) {
      throw new Error('未登录，请先登录');
    }

    const response = await fetch('/api/users/me', {
      method: 'GET',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${accessToken}`
      },
      credentials: 'include'
    });

    if (response.status === 401) {
      const refreshed = await tryRefreshToken();
      if (refreshed) {
        return fetchUserInfo();
      } else {
        throw new Error('登录已过期，请重新登录');
      }
    }

    if (!response.ok) {
      throw new Error('获取用户信息失败');
    }

    const responseData = await response.json();
    if (responseData.code === 200 && responseData.data) {
      userInfo.value = responseData.data;
      localStorage.setItem('username', userInfo.value.username || '');
      localStorage.setItem('avatar', userInfo.value.avatar || '');
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

const tryRefreshToken = async (): Promise<boolean> => {
  try {
    const response = await fetch('/api/refresh', {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
      },
      credentials: 'include',
    });

    if (response.ok) {
      const data = await response.json();
      if (data.access_token) {
        localStorage.setItem('accessToken', data.access_token);
        localStorage.setItem('tokenType', data.token_type || 'bearer');
        return true;
      }
    }
    return false;
  } catch (error) {
    console.error('刷新token失败:', error);
    return false;
  }
};

const openDeleteAccountDialog = async () => {
  try {
    if (userInfo.value.role === 'root') {
      const accessToken = localStorage.getItem('accessToken');
      if (!accessToken) {
        ElMessage.error('未登录，请先登录');
        return;
      }

      const usersResponse = await fetch('/api/users', {
        method: 'GET',
        headers: {
          'Accept': 'application/json',
          'Authorization': `Bearer ${accessToken}`
        },
        credentials: 'include'
      });

      if (usersResponse.ok) {
        const usersData = await usersResponse.json();
        if (usersData.code === 200 && usersData.data) {
          const adminCount = usersData.data.filter((user: any) => user.role === 'root').length;
          
          if (adminCount <= 1) {
            ElMessage.error('系统中只有一个管理员账号，无法注销。请先创建其他管理员账号。');
            return;
          }
        }
      }
    }

    verifyPasswordForm.value.password = '';
    verifyPasswordDialogVisible.value = true;
  } catch (err) {
    ElMessage.error('检查管理员账号数量失败');
    console.error(err);
  }
};

const verifyPasswordAndConfirmDelete = async () => {
  try {
    await verifyPasswordFormRef.value.validate();
    verifyingPassword.value = true;

    const params = new URLSearchParams();
    params.append('grant_type', 'password');
    params.append('username', userInfo.value.username);
    params.append('password', verifyPasswordForm.value.password);
    params.append('scope', '');
    params.append('client_id', '');
    params.append('client_secret', '');

    const response = await fetch('/api/login', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
        'Accept': 'application/json',
      },
      body: params.toString(),
    });

    if (response.ok) {
      const data = await response.json();
      if (data.access_token) {
        verifyPasswordDialogVisible.value = false;
        confirmDeleteDialogVisible.value = true;
      } else {
        throw new Error('密码验证失败');
      }
    } else {
      throw new Error('密码不正确');
    }
  } catch (err) {
    ElMessage.error((err as Error).message || '密码验证失败');
  } finally {
    verifyingPassword.value = false;
  }
};

const handleDeleteAccount = async () => {
  try {
    deletingAccount.value = true;

    const meResponse = await fetch('/api/users/me', {
      method: 'GET',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${localStorage.getItem('accessToken') || ''}`
      },
      credentials: 'include'
    });

    if (!meResponse.ok) {
      throw new Error('获取账号信息失败');
    }

    const meResponseData = await meResponse.json();
    if (meResponseData.code !== 200 || !meResponseData.data) {
      throw new Error('获取账号信息失败');
    }

    const userId = meResponseData.data.id;

    const deleteResponse = await fetch(`/api/users/${userId}`, {
      method: 'DELETE',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${localStorage.getItem('accessToken') || ''}`
      },
      credentials: 'include'
    });

    if (!deleteResponse.ok) {
      throw new Error('注销账号失败');
    }

    const deleteResponseData = await deleteResponse.json();
    if (deleteResponseData.code === 200) {
      ElMessage.success('账号已注销');
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
            document.cookie = `${name}=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=.${window.location.hostname.split('.').slice(-2).join('.')}`;
          }
        }
      };
      
      clearCookies();
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
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

const handleLogout = async () => {
  try {
    const token = localStorage.getItem('accessToken');
    const response = await fetch('/api/logout', {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Authorization': token ? `Bearer ${token}` : ''
      }
    });
    
    if (response.ok) {
      const data = await response.json();
      console.log('退出登录成功:', data);
    } else {
      console.error('退出登录请求失败:', response.status);
    }
  } catch (error) {
    console.error('退出登录请求出错:', error);
  }
  
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
        document.cookie = `${name}=;expires=Thu, 01 Jan 1970 00:00:00 GMT;path=/;domain=.${window.location.hostname.split('.').slice(-2).join('.')}`;
      }
    }
  };

  clearCookies();
  window.dispatchEvent(new CustomEvent('loginStatusChanged'));
  ElMessage.success('已退出登录');
  router.push('/Login');
};

const openEditDialog = () => {
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

const verifyPassword = async (username: string, password: string): Promise<boolean> => {
  try {
    const params = new URLSearchParams();
    params.append('grant_type', 'password');
    params.append('username', username);
    params.append('password', password);
    params.append('scope', '');
    params.append('client_id', '');
    params.append('client_secret', '');
    
    const response = await fetch('/api/login', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/x-www-form-urlencoded',
        'Accept': 'application/json',
      },
      body: params.toString(),
    });

    if (response.ok) {
      const data = await response.json();
      return data.access_token !== undefined;
    }
    return false;
  } catch (error) {
    console.error('验证密码失败:', error);
    return false;
  }
};

const saveChanges = async () => {
  try {
    await editFormRef.value.validate((valid: boolean, invalidFields: any) => {
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
    
    saving.value = true;

    if (editForm.value.changePassword) {
      const isPasswordValid = await verifyPassword(editForm.value.username, editForm.value.currentPassword);
      if (!isPasswordValid) {
        throw new Error('当前密码不正确');
      }
    }

    const requestData: Record<string, string> = {};
    
    if (editForm.value.username !== userInfo.value.username) {
      requestData["username"] = editForm.value.username || "";
    }
    
    if (editForm.value.nickname !== userInfo.value.nickname) {
      requestData["nickname"] = editForm.value.nickname || "";
    }
    
    if (editForm.value.email !== userInfo.value.email) {
      requestData["email"] = editForm.value.email || "";
    }
    
    if (editForm.value.avatar && editForm.value.avatar !== userInfo.value.avatar) {
      requestData["avatar"] = editForm.value.avatar;
    }
    
    if (editForm.value.changePassword) {
      requestData["password"] = editForm.value.newPassword || "";
    }

    const response = await fetch(`/api/users/${userInfo.value.id}`, {
      method: 'PUT',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
        'Authorization': `Bearer ${localStorage.getItem('accessToken') || ''}`
      },
      body: JSON.stringify(requestData),
      credentials: 'include'
    });

    const responseData = await response.json();
    
    if (response.ok && responseData.code === 200) {
      ElMessage.success('修改账号信息成功');
      editDialogVisible.value = false;
      await fetchUserInfo();
      localStorage.setItem('username', editForm.value.username);
      localStorage.setItem('avatar', editForm.value.avatar || '');
      window.dispatchEvent(new CustomEvent('loginStatusChanged'));
    } else {
      if (response.status === 422 && responseData.detail && Array.isArray(responseData.detail) && responseData.detail.length > 0) {
        const errorMessages = responseData.detail.map((error: any) => error.msg).join(', ');
        throw new Error(errorMessages);
      } else {
        throw new Error(responseData.msg || '修改账号信息失败');
      }
    }
  } catch (err) {
    ElMessage.error((err as Error).message || '修改账号信息失败');
  } finally {
    saving.value = false;
  }
};

onMounted(() => {
  fetchUserInfo();
});
</script>

<style scoped>
.mobile-profile {
  min-height: 100vh;
  background: #f5f7fa;
  padding: 0;
}

/* 头像区域 */
.avatar-section {
  background: white;
  padding: 32px 16px;
  text-align: center;
  margin-bottom: 16px;
}

.avatar-circle {
  width: 80px;
  height: 80px;
  border-radius: 50%;
  overflow: hidden;
  margin: 0 auto 16px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
}

.avatar-circle .el-image {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.default-avatar {
  width: 100%;
  height: 100%;
  background: linear-gradient(135deg, #8bad42 0%, #6a9a35 100%);
  display: flex;
  justify-content: center;
  align-items: center;
  color: white;
  font-size: 32px;
  font-weight: bold;
}

.username {
  margin: 0 0 8px 0;
  font-size: 20px;
  font-weight: 600;
  color: #303133;
}

/* 加载和错误状态 */
.loading-container,
.error-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 40px 16px;
  color: #909399;
}

.loading-container .el-icon,
.error-container .el-icon {
  font-size: 32px;
  margin-bottom: 12px;
}

.error-container {
  color: #f56c6c;
}

/* 信息区域 */
.info-section {
  padding: 0 16px;
}

.info-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  margin-bottom: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.info-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 0;
  border-bottom: 1px solid #f0f0f0;
}

.info-item:last-child {
  border-bottom: none;
}

.info-item .label {
  font-size: 14px;
  color: #909399;
}

.info-item .value {
  font-size: 14px;
  font-weight: 500;
  color: #303133;
}

/* 操作按钮 */
.action-section {
  display: flex;
  flex-direction: column;
  gap: 12px;
  align-items: center;
}

.action-btn {
  width: 80% !important;
  height: 48px;
  font-size: 16px;
  border-radius: 12px;
  display: flex !important;
  align-items: center;
  justify-content: center;
  margin-left: 0 !important;
  margin-right: 0 !important;
}

.action-btn .el-icon {
  margin-right: 8px;
}
</style>
