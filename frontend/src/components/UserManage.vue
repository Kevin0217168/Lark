<template>
  <div class="user-manage-container" :class="{ 'mobile': isMobile }">
    <!-- 桌面端：使用 el-card 包装 -->
    <el-card v-if="!isMobile" class="user-manage-card">
      <div class="header-section desktop-header">
        <div class="header-left">
          <el-button type="primary" @click="openAddUserDialog">
            <el-icon><Plus /></el-icon>
            新增用户
          </el-button>
        </div>
        <h3 class="title">用户管理</h3>
        <div class="header-right"></div>
      </div>

      <div class="filter-section desktop-filter">
        <div class="filter-left">
          <el-select
            v-model="filterRole"
            placeholder="筛选角色"
            clearable
            style="width: 150px"
            @change="handleFilterChange"
          >
            <el-option label="全部角色" value="" />
            <el-option label="管理员" value="root" />
            <el-option label="普通用户" value="user" />
          </el-select>
          <el-button 
            type="danger" 
            :disabled="selectedUsers.length === 0"
            @click="handleBatchDelete"
          >
            <el-icon><Delete /></el-icon>
            批量删除 ({{ selectedUsers.length }})
          </el-button>
        </div>
        <div class="filter-right">
          <el-input
            v-model="searchUsername"
            placeholder="搜索用户名/昵称/邮箱"
            clearable
            style="width: 250px"
            @input="handleSearchChange"
          >
            <template #prefix>
              <el-icon><Search /></el-icon>
            </template>
          </el-input>
        </div>
      </div>

      <div v-if="loading" class="loading-container">
        <el-icon class="is-loading"><Loading /></el-icon>
        <span>加载中...</span>
      </div>

      <div v-else-if="error" class="error-container">
        <el-icon><Warning /></el-icon>
        <span>{{ error }}</span>
        <el-button type="primary" @click="fetchUsers">重新加载</el-button>
      </div>

      <div v-else>
        <el-table 
          :data="filteredUsers" 
          style="width: 100%" 
          v-loading="tableLoading"
          @selection-change="handleSelectionChange"
        >
          <el-table-column type="selection" width="55" align="center" />
          <el-table-column prop="id" label="ID" width="60" align="center" />
          <el-table-column prop="avatar" label="头像" width="80" align="center">
            <template #default="scope">
              <el-avatar v-if="scope.row.avatar" :src="scope.row.avatar" :size="40" />
              <el-avatar v-else :size="40">{{ scope.row.nickname ? scope.row.nickname.charAt(0) : '?' }}</el-avatar>
            </template>
          </el-table-column>
          <el-table-column prop="username" label="用户名" min-width="100" />
          <el-table-column prop="nickname" label="昵称" min-width="100" />
          <el-table-column prop="role" label="角色" width="100" align="center">
            <template #default="scope">
              <el-tag :type="scope.row.role === 'root' ? 'danger' : 'info'">
                {{ scope.row.role === 'root' ? '管理员' : '普通用户' }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="email" label="邮箱" min-width="180" show-overflow-tooltip>
            <template #default="scope">
              {{ scope.row.email || '-' }}
            </template>
          </el-table-column>
          <el-table-column label="操作" width="160" align="center" fixed="right">
            <template #default="scope">
              <el-button 
                type="primary" 
                size="small" 
                @click="handleEditUser(scope.row)"
              >
                编辑
              </el-button>
              <el-button 
                type="danger" 
                size="small" 
                @click="handleDeleteUser(scope.row)"
                :disabled="scope.row.id === currentUserId"
              >
                删除
              </el-button>
            </template>
          </el-table-column>
        </el-table>
      </div>
    </el-card>

    <!-- 移动端：不使用 el-card 包装 -->
    <div v-else class="mobile-user-manage">
      <!-- 头部 -->
      <div class="header-section mobile-header">
        <h3>用户管理</h3>
        <el-button type="primary" size="small" @click="openAddUserDialog">
          <el-icon><Plus /></el-icon>
          新增
        </el-button>
      </div>

      <!-- 筛选 -->
      <div class="filter-section mobile-filter">
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

      <!-- 备案信息 -->
      <MobileBeian />
    </div>

    <!-- 编辑用户对话框 -->
    <el-dialog
      v-model="editDialogVisible"
      title="编辑用户"
      :width="isMobile ? '90%' : '500px'"
    >
      <el-form
        :model="editForm"
        ref="editFormRef"
        :rules="editRules"
        label-width="100px"
      >
        <el-form-item label="用户名" prop="username">
          <el-input v-model="editForm.username" />
        </el-form-item>

        <el-form-item label="昵称" prop="nickname">
          <el-input v-model="editForm.nickname" />
        </el-form-item>

        <el-form-item label="角色" prop="role">
          <el-select v-model="editForm.role" placeholder="请选择角色" :disabled="editForm.id === currentUserId">
            <el-option label="管理员" value="root" />
            <el-option label="普通用户" value="user" />
          </el-select>
          <span v-if="editForm.id === currentUserId" class="role-hint">不能修改自己的角色</span>
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
        <span class="dialog-footer">
          <el-button @click="editDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="saveUserChanges" :loading="saving">保存</el-button>
        </span>
      </template>
    </el-dialog>

    <!-- 新增用户对话框 -->
    <el-dialog
      v-model="addDialogVisible"
      title="新增用户"
      :width="isMobile ? '90%' : '500px'"
    >
      <el-form
        :model="addForm"
        ref="addFormRef"
        :rules="addRules"
        label-width="100px"
      >
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
          <el-select v-model="addForm.role" placeholder="请选择角色">
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
import { ref, computed, onMounted } from 'vue';
import { ElMessage, ElMessageBox, ElIcon } from 'element-plus';
import { Plus, Loading, Warning, Search, Delete } from '@element-plus/icons-vue';
import { api } from '@/utils/api';
import MobileBeian from './mobile/MobileBeian.vue';

// 检测是否为移动设备
const isMobile = ref(window.innerWidth < 768);

// 监听窗口大小变化
const handleWindowResize = () => {
  isMobile.value = window.innerWidth < 768;
};

onMounted(() => {
  window.addEventListener('resize', handleWindowResize);
  fetchUsers();
  getCurrentUserId();
});

// 用户列表
const users = ref<any[]>([]);
const loading = ref(false);
const error = ref('');
const tableLoading = ref(false);

// 筛选条件
const filterRole = ref('');
const searchUsername = ref('');

// 选中的用户
const selectedUsers = ref<any[]>([]);

// 当前用户ID
const currentUserId = ref(0);

// 编辑用户相关
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

// 编辑用户验证规则
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
    
    const response = await api.get('/api/users');
    console.log('获取用户列表响应:', response);
    
    if (response.code === 200 && Array.isArray(response.data)) {
      users.value = response.data;
    } else {
      error.value = '获取用户列表失败';
    }
  } catch (err: any) {
    console.error('获取用户列表异常:', err);
    error.value = err.message || '获取用户列表失败';
  } finally {
    loading.value = false;
  }
};

// 筛选后的用户列表
const filteredUsers = computed(() => {
  return users.value.filter(user => {
    const roleMatch = !filterRole.value || user.role === filterRole.value;
    const searchMatch = !searchUsername.value || 
      user.username.toLowerCase().includes(searchUsername.value.toLowerCase()) ||
      (user.nickname && user.nickname.toLowerCase().includes(searchUsername.value.toLowerCase())) ||
      (user.email && user.email.toLowerCase().includes(searchUsername.value.toLowerCase()));
    return roleMatch && searchMatch;
  });
});

// 处理筛选变化
const handleFilterChange = () => {
  // 筛选变化时清空选中
  selectedUsers.value = [];
};

// 处理搜索变化
const handleSearchChange = () => {
  // 搜索变化时清空选中
  selectedUsers.value = [];
};

// 处理表格选择变化（桌面端）
const handleSelectionChange = (val: any[]) => {
  selectedUsers.value = val;
};

// 切换用户选择（移动端）
const toggleUserSelection = (user: any) => {
  const index = selectedUsers.value.findIndex(u => u.id === user.id);
  if (index !== -1) {
    selectedUsers.value.splice(index, 1);
  } else {
    selectedUsers.value.push(user);
  }
};

// 处理复选框变化（移动端）
const handleCheckboxChange = (user: any, val: boolean) => {
  const index = selectedUsers.value.findIndex(u => u.id === user.id);
  if (val && index === -1) {
    selectedUsers.value.push(user);
  } else if (!val && index !== -1) {
    selectedUsers.value.splice(index, 1);
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

// 处理新增用户
const handleAddUser = async () => {
  try {
    if (!addFormRef.value) return;
    
    const valid = await addFormRef.value.validate();
    if (!valid) return;
    
    adding.value = true;
    
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

    const response = await api.post('/api/users', requestData);
    console.log('新增用户响应:', response);
    
    if (response.code === 200) {
      ElMessage.success('新增用户成功');
      addDialogVisible.value = false;
      await fetchUsers();
    } else {
      if (response.detail && Array.isArray(response.detail) && response.detail.length > 0) {
        const errorMessages = response.detail
          .map((err: any) => {
            const msg = err.msg || '';
            return msg.replace(/^Value error,\s*/, '');
          })
          .filter((msg: string) => msg.length > 0)
          .join('\n');
        throw new Error(errorMessages || '创建用户失败');
      } else if (response.errors) {
        const errorMessages = Object.values(response.errors).flat().join('\n');
        throw new Error(errorMessages || '创建用户失败');
      } else {
        throw new Error(response.msg || '创建用户失败');
      }
    }
  } catch (err: any) {
    const errorMessage = err.message;
    
    if (errorMessage === '表单验证失败') {
      // 表单验证失败已经在validate回调中处理过
    } else if (errorMessage.includes('Failed to fetch') || errorMessage.includes('NetworkError')) {
      ElMessage.error('网络连接失败，请检查网络设置');
    } else {
      ElMessage.error(errorMessage || '新增用户失败');
    }
  } finally {
    adding.value = false;
  }
};

// 处理编辑用户
const handleEditUser = (user: any) => {
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
  originalUserData.value = { ...user };
  editDialogVisible.value = true;
};

// 保存用户修改
const saveUserChanges = async () => {
  try {
    if (!editFormRef.value) return;
    
    const valid = await editFormRef.value.validate();
    if (!valid) return;
    
    saving.value = true;
    
    // 构建请求数据，只发送有变化的字段
    const requestData: any = {};
    
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
    
    const response = await api.put(`/api/users/${editForm.value.id}`, requestData);
    console.log('更新用户响应:', response);
    
    if (response.code === 200) {
      ElMessage.success('用户信息更新成功');
      editDialogVisible.value = false;
      await fetchUsers();
    } else {
      if (response.detail && Array.isArray(response.detail) && response.detail.length > 0) {
        const errorMessages = response.detail
          .map((err: any) => {
            const msg = err.msg || '';
            return msg.replace(/^Value error,\s*/, '');
          })
          .filter((msg: string) => msg.length > 0)
          .join('\n');
        throw new Error(errorMessages || '更新用户失败');
      } else if (response.errors) {
        const errorMessages = Object.values(response.errors).flat().join('\n');
        throw new Error(errorMessages || '更新用户失败');
      } else {
        throw new Error(response.msg || '更新用户失败');
      }
    }
  } catch (err: any) {
    const errorMessage = err.message;
    
    if (errorMessage === '表单验证失败') {
      // 表单验证失败已经在validate回调中处理过
    } else if (errorMessage.includes('Failed to fetch') || errorMessage.includes('NetworkError')) {
      ElMessage.error('网络连接失败，请检查网络设置');
    } else {
      ElMessage.error(errorMessage || '用户信息更新失败');
    }
  } finally {
    saving.value = false;
  }
};

// 处理删除用户
const handleDeleteUser = async (user: any) => {
  try {
    await ElMessageBox.confirm(
      `确定要删除用户 ${user.username} 吗？此操作不可恢复。`,
      '确认删除',
      {
        confirmButtonText: '确认删除',
        cancelButtonText: '取消',
        type: 'warning',
      }
    );
    
    const response = await api.delete(`/api/users/${user.id}`);
    console.log('删除用户响应:', response);
    
    if (response.code === 200) {
      ElMessage.success('删除用户成功');
      await fetchUsers();
    } else {
      ElMessage.error('删除用户失败');
    }
  } catch (err: any) {
    if (err !== 'cancel') {
      ElMessage.error(err.message || '删除用户失败');
    }
  }
};

// 处理批量删除
const handleBatchDelete = async () => {
  try {
    if (selectedUsers.value.length === 0) {
      ElMessage.warning('请先选择要删除的用户');
      return;
    }
    
    const usernames = selectedUsers.value.map(user => user.username).join('\n');
    await ElMessageBox.confirm(
      `确定要删除以下用户吗？此操作不可恢复。\n${usernames}`,
      '确认批量删除',
      {
        confirmButtonText: '确认删除',
        cancelButtonText: '取消',
        type: 'warning',
      }
    );

    // 逐个删除用户
    let successCount = 0;
    let failCount = 0;

    for (const user of selectedUsers.value) {
      try {
        console.log('批量删除用户 - 发送请求:', {
          url: `/api/users/${user.id}`,
          method: 'DELETE',
          userId: user.id,
          username: user.username
        });

        const response = await api.delete(`/api/users/${user.id}`);
        console.log(`删除用户 ${user.username} (ID: ${user.id}) 响应:`, response);

        successCount++;
        console.log(`删除用户 ${user.username} (ID: ${user.id}) 成功`);
      } catch (err) {
        failCount++;
        console.error(`删除用户 ${user.username} (ID: ${user.id}) 异常:`, err);
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
</script>

<style lang="scss" scoped>
/* 桌面端样式 */
.user-manage-container:not(.mobile) {
  width: 90%;
  margin: 20px auto;
  position: relative;
  z-index: 1;
}

.user-manage-card {
  border-radius: 12px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  background: white;
  position: relative;
  z-index: 1;
}

.desktop-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  padding: 20px 0;
  border-bottom: 1px solid #f0f0f0;
}

.desktop-header .title {
  margin: 0;
  font-size: 20px;
  font-weight: 600;
  color: #303133;
}

.desktop-filter {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  padding: 15px;
  background: #f5f7fa;
  border-radius: 4px;
}

.desktop-filter .filter-left {
  display: flex;
  gap: 15px;
  align-items: center;
}

/* 移动端样式 */
.mobile-user-manage {
  min-height: calc(100vh - 120px);
  background: #f5f7fa;
  padding: 16px;
  box-sizing: border-box;
}

.mobile-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
  padding: 12px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.mobile-header h3 {
  margin: 0;
  font-size: 18px;
  font-weight: 600;
  color: #303133;
}

.mobile-filter {
  display: flex;
  flex-direction: column;
  gap: 12px;
  margin-bottom: 16px;
  padding: 16px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.filter-select, .filter-input {
  width: 100% !important;
}

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
  transition: transform 0.2s ease, box-shadow 0.2s ease;
}

.user-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.12);
}

.user-card.selected {
  border: 2px solid #409eff;
}

.user-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
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
  margin-left: 12px;
}

.user-details {
  margin-bottom: 12px;
  padding: 12px;
  background: #f5f7fa;
  border-radius: 8px;
}

.detail-row {
  display: flex;
  justify-content: space-between;
  padding: 8px 0;
  border-bottom: 1px solid #e4e7ed;
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
  justify-content: flex-end;
}

.role-hint {
  color: #f56c6c;
  font-size: 12px;
  margin-top: 8px;
}

/* 公共样式 */
.loading-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 60px 0;
  color: #909399;
}

.loading-container .is-loading {
  margin-bottom: 12px;
  font-size: 32px;
}

.error-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 60px 0;
  color: #f56c6c;
}

.error-container .el-icon {
  margin-bottom: 12px;
  font-size: 32px;
}

.empty-state {
  padding: 40px 0;
}

.dialog-footer {
  text-align: right;
}
</style>