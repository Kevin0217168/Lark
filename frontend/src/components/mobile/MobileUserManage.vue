<template>
  <div class="mobile-user-manage">
    <!-- 头部 -->
    <div class="header-section">
      <h3>用户管理</h3>
      <el-button type="primary" size="small" @click="openAddUserDialog">
        <el-icon><Plus /></el-icon>
        新增
      </el-button>
    </div>

    <!-- 筛选 -->
    <div class="filter-section">
      <el-select
        v-model="filterRole"
        placeholder="筛选角色"
        clearable
        class="filter-select"
        @change="handleFilterChange"
      >
        <el-option label="全部角色" value="" />
        <el-option label="管理员" value="root" />
        <el-option label="普通用户" value="user" />
      </el-select>
      <el-input
        v-model="searchUsername"
        placeholder="搜索用户名/昵称/邮箱"
        clearable
        class="filter-input"
        @input="handleSearchChange"
      >
        <template #prefix>
          <el-icon><Search /></el-icon>
        </template>
      </el-input>
      <el-button 
        type="danger" 
        size="small"
        :disabled="selectedUsers.length === 0"
        @click="handleBatchDelete"
      >
        <el-icon><Delete /></el-icon>
        批量删除 ({{ selectedUsers.length }})
      </el-button>
    </div>

    <!-- 加载状态 -->
    <div v-if="loading" class="loading-container">
      <el-icon class="is-loading"><Loading /></el-icon>
      <span>加载中...</span>
    </div>

    <!-- 错误状态 -->
    <div v-else-if="error" class="error-container">
      <el-icon><Warning /></el-icon>
      <span>{{ error }}</span>
      <el-button type="primary" size="small" @click="fetchUsers">重新加载</el-button>
    </div>

    <!-- 用户列表 -->
    <div v-else class="user-list">
      <div 
        v-for="user in filteredUsers" 
        :key="user.id" 
        class="user-card"
        :class="{ 'selected': selectedUsers.some(u => u.id === user.id) }"
        @click="toggleUserSelection(user)"
      >
        <div class="user-header">
          <div class="user-avatar">
            <el-avatar v-if="user.avatar" :src="user.avatar" :size="40" />
            <el-avatar v-else :size="40">{{ user.nickname ? user.nickname.charAt(0) : '?' }}</el-avatar>
          </div>
          <div class="user-info">
            <h4>{{ user.nickname || user.username }}</h4>
            <el-tag :type="user.role === 'root' ? 'danger' : 'info'" size="small">
              {{ user.role === 'root' ? '管理员' : '普通用户' }}
            </el-tag>
          </div>
          <div class="user-checkbox">
            <el-checkbox 
              :model-value="selectedUsers.some(u => u.id === user.id)"
              @click.stop
              @change="(val: boolean) => handleCheckboxChange(user, val)"
            />
          </div>
        </div>
        <div class="user-details">
          <div class="detail-row">
            <span class="label">ID</span>
            <span class="value">{{ user.id }}</span>
          </div>
          <div class="detail-row">
            <span class="label">用户名</span>
            <span class="value">{{ user.username }}</span>
          </div>
          <div class="detail-row">
            <span class="label">邮箱</span>
            <span class="value">{{ user.email || '-' }}</span>
          </div>
        </div>
        <div class="user-actions">
          <el-button 
            type="primary" 
            size="small" 
            @click.stop="handleEditUser(user)"
          >
            编辑
          </el-button>
          <el-button 
            type="danger" 
            size="small" 
            @click.stop="handleDeleteUser(user)"
            :disabled="user.id === currentUserId"
          >
            删除
          </el-button>
        </div>
      </div>
    </div>

    <!-- 空状态 -->
    <div v-if="filteredUsers.length === 0 && !loading && !error" class="empty-state">
      <el-empty description="暂无用户数据" />
    </div>

    <!-- 编辑用户对话框 -->
    <el-dialog v-model="editDialogVisible" title="编辑用户" width="90%">
      <el-form :model="editForm" ref="editFormRef" :rules="editRules" label-width="100px">
        <el-form-item label="用户名" prop="username">
          <el-input v-model="editForm.username" />
        </el-form-item>
        <el-form-item label="昵称" prop="nickname">
          <el-input v-model="editForm.nickname" />
        </el-form-item>
        <el-form-item label="角色" prop="role">
          <el-select v-model="editForm.role" placeholder="请选择角色" :disabled="editForm.id === currentUserId" style="width: 100%">
            <el-option label="管理员" value="root" />
            <el-option label="普通用户" value="user" />
          </el-select>
          <div v-if="editForm.id === currentUserId" class="role-hint">不能修改自己的角色</div>
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
        <el-form-item v-if="editForm.changePassword" label="新密码" prop="newPassword">
          <el-input v-model="editForm.newPassword" type="password" placeholder="请输入新密码" />
        </el-form-item>
        <el-form-item v-if="editForm.changePassword" label="确认密码" prop="confirmPassword">
          <el-input v-model="editForm.confirmPassword" type="password" placeholder="请确认新密码" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="editDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="saveUserChanges" :loading="saving">保存</el-button>
      </template>
    </el-dialog>

    <!-- 新增用户对话框 -->
    <el-dialog v-model="addDialogVisible" title="新增用户" width="90%">
      <el-form :model="addForm" ref="addFormRef" :rules="addRules" label-width="100px">
        <el-form-item label="用户名" prop="username">
          <el-input v-model="addForm.username" placeholder="请输入用户名" />
        </el-form-item>
        <el-form-item label="密码" prop="password">
          <el-input v-model="addForm.password" type="password" placeholder="请输入密码" show-password />
        </el-form-item>
        <el-form-item label="确认密码" prop="confirmPassword">
          <el-input v-model="addForm.confirmPassword" type="password" placeholder="请再次输入密码" show-password />
        </el-form-item>
        <el-form-item label="昵称" prop="nickname">
          <el-input v-model="addForm.nickname" placeholder="请输入昵称" />
        </el-form-item>
        <el-form-item label="角色" prop="role">
          <el-select v-model="addForm.role" placeholder="请选择角色" style="width: 100%">
            <el-option label="管理员" value="root" />
            <el-option label="普通用户" value="user" />
          </el-select>
        </el-form-item>
        <el-form-item label="邮箱" prop="email">
          <el-input v-model="addForm.email" placeholder="请输入邮箱（可选）" />
        </el-form-item>
        <el-form-item label="头像URL" prop="avatar">
          <el-input v-model="addForm.avatar" placeholder="请输入头像URL（可选）" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="addDialogVisible = false">取消</el-button>
        <el-button type="primary" @click="handleAddUser" :loading="adding">创建</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, computed } from "vue";
import { ElMessage, ElMessageBox } from "element-plus";
import { Loading, Warning, Plus, Search, Delete } from "@element-plus/icons-vue";

const API_BASE_URL = '';

const loading = ref(false);
const error = ref('');
const users = ref<any[]>([]);
const currentUserId = ref(0);

// 筛选和搜索
const filterRole = ref('');
const searchUsername = ref('');

// 多选相关
const selectedUsers = ref<any[]>([]);

// 过滤后的用户列表
const filteredUsers = computed(() => {
  let result = users.value;

  // 根据角色筛选
  if (filterRole.value) {
    result = result.filter(user => user.role === filterRole.value);
  }

  // 模糊搜索：支持用户名、昵称、邮箱
  if (searchUsername.value) {
    const searchTerm = searchUsername.value.toLowerCase();
    result = result.filter(user => {
      const username = (user.username || '').toLowerCase();
      const nickname = (user.nickname || '').toLowerCase();
      const email = (user.email || '').toLowerCase();
      return username.includes(searchTerm) || 
             nickname.includes(searchTerm) || 
             email.includes(searchTerm);
    });
  }

  return result;
});

const handleFilterChange = () => {
  // 筛选变化时，computed会自动更新
};

const handleSearchChange = () => {
  // 搜索变化时，computed会自动更新
};

// 处理复选框变化
const handleCheckboxChange = (user: any, checked: boolean) => {
  if (checked) {
    if (!selectedUsers.value.some(u => u.id === user.id)) {
      selectedUsers.value.push(user);
    }
  } else {
    selectedUsers.value = selectedUsers.value.filter(u => u.id !== user.id);
  }
};

// 点击卡片切换选择
const toggleUserSelection = (user: any) => {
  const index = selectedUsers.value.findIndex(u => u.id === user.id);
  if (index > -1) {
    selectedUsers.value.splice(index, 1);
  } else {
    selectedUsers.value.push(user);
  }
};

// 批量删除用户
const handleBatchDelete = async () => {
  if (selectedUsers.value.length === 0) {
    ElMessage.warning('请先选择要删除的用户');
    return;
  }

  // 检查是否包含当前用户
  const containsCurrentUser = selectedUsers.value.some(user => user.id === currentUserId.value);
  if (containsCurrentUser) {
    ElMessage.error('不能删除自己的账号');
    return;
  }

  const usernames = selectedUsers.value.map(user => user.username).join('、');
  
  try {
    await ElMessageBox.confirm(
      `确定要删除以下用户吗？此操作不可恢复。\n${usernames}`,
      '确认批量删除',
      {
        confirmButtonText: '确认删除',
        cancelButtonText: '取消',
        type: 'warning',
      }
    );

    const accessToken = localStorage.getItem('accessToken');
    if (!accessToken) {
      throw new Error('未登录，请先登录');
    }

    // 逐个删除用户
    let successCount = 0;
    let failCount = 0;

    for (const user of selectedUsers.value) {
      try {
        const currentToken = localStorage.getItem('accessToken');
        if (!currentToken) {
          throw new Error('登录已过期，请重新登录');
        }

        const response = await fetch(`${API_BASE_URL}/api/users/${user.id}`, {
          method: 'DELETE',
          headers: {
            'Accept': 'application/json',
            'Authorization': `Bearer ${currentToken}`
          },
          credentials: 'include'
        });

        if (response.ok) {
          successCount++;
        } else {
          failCount++;
        }
      } catch (err) {
        failCount++;
      }
    }
    
    if (failCount === 0) {
      ElMessage.success(`成功删除 ${successCount} 个用户`);
    } else {
      ElMessage.warning(`成功删除 ${successCount} 个用户，失败 ${failCount} 个`);
    }
    
    selectedUsers.value = [];
    await fetchUsers();
  } catch (err: any) {
    if (err !== 'cancel') {
      ElMessage.error(err.message || '批量删除失败');
    }
  }
};

const editDialogVisible = ref(false);
const saving = ref(false);
const editFormRef = ref();

const editForm = ref({
  id: 0,
  username: '',
  nickname: '',
  role: 'user',
  email: '',
  avatar: '',
  changePassword: false,
  newPassword: '',
  confirmPassword: ''
});

const originalUserData = ref<any>(null);

// 新增用户相关
const addDialogVisible = ref(false);
const adding = ref(false);
const addFormRef = ref();

const addForm = ref({
  username: '',
  password: '',
  confirmPassword: '',
  nickname: '',
  role: 'user',
  email: '',
  avatar: ''
});

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
  role: [
    { required: true, message: '请选择角色', trigger: 'change' }
  ],
  email: [
    { type: 'email', message: '请输入正确的邮箱地址', trigger: 'blur' }
  ],
  avatar: [
    { required: false, message: '请输入头像URL', trigger: 'blur' },
    {
      validator: (rule: any, value: string, callback: Function) => {
        if (!value || value === '') {
          callback();
          return;
        }
        if (value && !value.match(/^https?:\/\/.+/)) {
          callback(new Error('图片地址必须以 http:// 或 https:// 开头'));
          return;
        }
        callback();
      },
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
  confirmPassword: [
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

// 新增用户验证规则
const validateConfirmPassword = (rule: any, value: string, callback: Function) => {
  if (value !== addForm.value.password) {
    callback(new Error('两次输入的密码不一致'));
  } else {
    callback();
  }
};

const addRules = {
  username: [
    { required: true, message: '请输入用户名', trigger: 'blur' },
    { min: 3, max: 20, message: '用户名长度在 3 到 20 个字符', trigger: 'blur' },
    { pattern: /^[a-zA-Z0-9][a-zA-Z0-9_]*[a-zA-Z0-9]$/, message: '用户名只能包含字母、数字、下划线，且不能以下划线开头或结尾', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 8, max: 32, message: '密码长度在 8 到 32 个字符', trigger: 'blur' },
    { pattern: /^(?=.*[a-zA-Z0-9])[a-zA-Z0-9]+$/, message: '密码必须包含至少一个数字或字母', trigger: 'blur' }
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
  email: [
    { type: 'email', message: '请输入正确的邮箱地址', trigger: 'blur' }
  ],
  avatar: [
    { max: 255, message: '头像URL长度不能超过255个字符', trigger: 'blur' }
  ]
};

// 获取当前用户ID
const getCurrentUserId = () => {
  const userId = localStorage.getItem('userId');
  if (userId) {
    currentUserId.value = parseInt(userId);
  }
};

// 获取用户列表
const fetchUsers = async () => {
  try {
    loading.value = true;
    error.value = '';

    const accessToken = localStorage.getItem('accessToken');
    if (!accessToken) {
      throw new Error('未登录，请先登录');
    }

    const response = await fetch(`${API_BASE_URL}/api/users`, {
      method: 'GET',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${accessToken}`
      },
      credentials: 'include'
    });

    if (!response.ok) {
      throw new Error('获取用户列表失败');
    }

    const data = await response.json();
    if (data.code === 200 && data.data) {
      users.value = data.data;
    } else {
      throw new Error(data.msg || '获取用户列表失败');
    }
  } catch (err) {
    error.value = (err as Error).message || '获取用户列表失败';
  } finally {
    loading.value = false;
  }
};

// 编辑用户
const handleEditUser = (user: any) => {
  originalUserData.value = { ...user };
  
  editForm.value = {
    id: user.id,
    username: user.username,
    nickname: user.nickname,
    role: user.role,
    email: user.email || '',
    avatar: user.avatar || '',
    changePassword: false,
    newPassword: '',
    confirmPassword: ''
  };
  editDialogVisible.value = true;
};

// 保存用户修改
const saveUserChanges = async () => {
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

    const accessToken = localStorage.getItem('accessToken');
    if (!accessToken) {
      throw new Error('未登录，请先登录');
    }

    // 构建请求数据，只包含用户实际修改的字段
    const requestData: Record<string, any> = {};
    
    if (editForm.value.username !== originalUserData.value.username) {
      requestData["username"] = editForm.value.username || "";
    }
    
    if (editForm.value.nickname !== originalUserData.value.nickname) {
      requestData["nickname"] = editForm.value.nickname || "";
    }
    
    if (editForm.value.role !== originalUserData.value.role) {
      requestData["role"] = editForm.value.role || "";
    }
    
    if (editForm.value.email !== originalUserData.value.email) {
      requestData["email"] = editForm.value.email || "";
    }
    
    if (editForm.value.avatar && editForm.value.avatar !== originalUserData.value.avatar) {
      requestData["avatar"] = editForm.value.avatar;
    }
    
    if (editForm.value.changePassword) {
      requestData["password"] = editForm.value.newPassword || "";
    }

    // 如果没有任何修改，提示用户
    if (Object.keys(requestData).length === 0) {
      ElMessage.warning('没有任何修改');
      saving.value = false;
      return;
    }

    const response = await fetch(`${API_BASE_URL}/api/users/${editForm.value.id}`, {
      method: 'PUT',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
        'Authorization': `Bearer ${accessToken}`
      },
      body: JSON.stringify(requestData),
      credentials: 'include'
    });

    const responseData = await response.json();
    
    if (response.ok && responseData.code === 200) {
      ElMessage.success('用户更新成功');
      editDialogVisible.value = false;
      await fetchUsers();
    } else {
      if (response.status === 422 && responseData.detail && Array.isArray(responseData.detail) && responseData.detail.length > 0) {
        const errorMessages = responseData.detail
          .map((err: any) => {
            const msg = err.msg || '';
            return msg.replace(/^Value error,\s*/, '');
          })
          .filter((msg: string) => msg.length > 0)
          .join('\n');
        throw new Error(errorMessages || '更新用户失败');
      } else if (response.status === 422 && responseData.errors) {
        const errorMessages = Object.values(responseData.errors).flat().join('\n');
        throw new Error(errorMessages || '更新用户失败');
      } else {
        throw new Error(responseData.msg || '更新用户失败');
      }
    }
  } catch (err: any) {
    const errorMessage = err.message;
    
    if (errorMessage === '表单验证失败') {
      // 表单验证失败已经在validate回调中处理过
    } else if (errorMessage.includes('Failed to fetch') || errorMessage.includes('NetworkError')) {
      ElMessage.error('网络连接失败，请检查网络设置');
    } else {
      ElMessage.error(errorMessage || '更新用户失败');
    }
  } finally {
    saving.value = false;
  }
};

// 删除用户
const handleDeleteUser = async (user: any) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除用户 "${user.username}" 吗？此操作不可恢复。`,
      '确认删除',
      {
        confirmButtonText: '确认删除',
        cancelButtonText: '取消',
        type: 'warning',
      }
    );

    const accessToken = localStorage.getItem('accessToken');
    if (!accessToken) {
      throw new Error('未登录，请先登录');
    }

    const response = await fetch(`${API_BASE_URL}/api/users/${user.id}`, {
      method: 'DELETE',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${accessToken}`
      },
      credentials: 'include'
    });

    if (!response.ok) {
      throw new Error('删除用户失败');
    }

    const data = await response.json();
    if (data.code === 200) {
      ElMessage.success('用户删除成功');
      await fetchUsers();
    } else {
      throw new Error(data.msg || '删除用户失败');
    }
  } catch (err: any) {
    if (err !== 'cancel') {
      ElMessage.error(err.message || '删除用户失败');
    }
  }
};

// 打开新增用户对话框
const openAddUserDialog = () => {
  addForm.value = {
    username: '',
    password: '',
    confirmPassword: '',
    nickname: '',
    role: 'user',
    email: '',
    avatar: ''
  };
  addDialogVisible.value = true;
};

// 新增用户
const handleAddUser = async () => {
  try {
    await addFormRef.value.validate((valid: boolean, invalidFields: any) => {
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

    adding.value = true;

    const accessToken = localStorage.getItem('accessToken');
    if (!accessToken) {
      throw new Error('未登录，请先登录');
    }

    // 构建请求数据
    const requestData: any = {
      username: addForm.value.username,
      password: addForm.value.password,
      nickname: addForm.value.nickname,
      role: addForm.value.role
    };

    // 可选字段
    if (addForm.value.email) {
      requestData.email = addForm.value.email;
    }
    if (addForm.value.avatar) {
      requestData.avatar = addForm.value.avatar;
    }

    const response = await fetch(`${API_BASE_URL}/api/users`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
        'Authorization': `Bearer ${accessToken}`
      },
      body: JSON.stringify(requestData),
      credentials: 'include'
    });

    const responseData = await response.json();
    
    if (response.ok && responseData.code === 200) {
      ElMessage.success('用户创建成功');
      addDialogVisible.value = false;
      await fetchUsers();
    } else {
      if (response.status === 400) {
        throw new Error(responseData.detail || '用户名已存在');
      } else if (response.status === 422 && responseData.detail && Array.isArray(responseData.detail) && responseData.detail.length > 0) {
        const errorMessages = responseData.detail
          .map((err: any) => {
            const msg = err.msg || '';
            return msg.replace(/^Value error,\s*/, '');
          })
          .filter((msg: string) => msg.length > 0)
          .join('\n');
        throw new Error(errorMessages || '创建用户失败');
      } else if (response.status === 422 && responseData.errors) {
        const errorMessages = Object.values(responseData.errors).flat().join('\n');
        throw new Error(errorMessages || '创建用户失败');
      } else {
        throw new Error(responseData.msg || '创建用户失败');
      }
    }
  } catch (err: any) {
    const errorMessage = err.message;
    
    if (errorMessage === '表单验证失败') {
      // 表单验证失败已经在validate回调中处理过
    } else if (errorMessage.includes('Failed to fetch') || errorMessage.includes('NetworkError')) {
      ElMessage.error('网络连接失败，请检查网络设置');
    } else {
      ElMessage.error(errorMessage || '创建用户失败');
    }
  } finally {
    adding.value = false;
  }
};

// 组件挂载时获取用户列表
onMounted(() => {
  getCurrentUserId();
  fetchUsers();
});
</script>

<style scoped>
.mobile-user-manage {
  min-height: 100vh;
  background: #f5f7fa;
  padding: 0;
}

/* 头部 */
.header-section {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.header-section h3 {
  margin: 0;
  font-size: 18px;
  font-weight: 600;
  color: #303133;
}

/* 筛选 */
.filter-section {
  display: flex;
  flex-direction: column;
  gap: 12px;
  margin-bottom: 16px;
  padding: 16px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.filter-select,
.filter-input {
  width: 100% !important;
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

/* 用户列表 */
.user-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.user-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
  cursor: pointer;
  transition: all 0.3s;
}

.user-card.selected {
  border: 2px solid #8bad42;
  background: #f0f9eb;
}

.user-card:active {
  transform: scale(0.98);
}

.user-header {
  display: flex;
  align-items: center;
  margin-bottom: 12px;
}

.user-avatar {
  margin-right: 12px;
}

.user-info {
  flex: 1;
}

.user-info h4 {
  margin: 0 0 4px 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.user-checkbox {
  margin-left: auto;
}

.user-details {
  margin-bottom: 12px;
}

.detail-row {
  display: flex;
  justify-content: space-between;
  padding: 6px 0;
  border-bottom: 1px solid #f0f0f0;
}

.detail-row:last-child {
  border-bottom: none;
}

.detail-row .label {
  font-size: 14px;
  color: #909399;
}

.detail-row .value {
  font-size: 14px;
  font-weight: 500;
  color: #303133;
}

.user-actions {
  display: flex;
  gap: 8px;
  padding-top: 12px;
  border-top: 1px solid #f0f0f0;
}

/* 空状态 */
.empty-state {
  padding: 40px 0;
}

/* 角色提示 */
.role-hint {
  font-size: 12px;
  color: #909399;
  margin-top: 4px;
}
</style>