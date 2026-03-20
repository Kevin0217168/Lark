<template>
  <el-card class="user-manage-card">
    <div class="header-section">
      <div class="header-left">
        <el-button type="primary" @click="openAddUserDialog">
          <el-icon><Plus /></el-icon>
          新增用户
        </el-button>
      </div>
      <h3 class="title">用户管理</h3>
      <div class="header-right"></div>
    </div>

    <div class="filter-section">
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
          placeholder="输入用户名搜索"
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

    <!-- 编辑用户对话框 -->
    <el-dialog
      v-model="editDialogVisible"
      title="编辑用户"
      width="500px"
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
      width="500px"
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
        <span class="dialog-footer">
          <el-button @click="addDialogVisible = false">取消</el-button>
          <el-button type="primary" @click="handleAddUser" :loading="adding">创建</el-button>
        </span>
      </template>
    </el-dialog>
  </el-card>
</template>

<script lang="ts" setup>
import { ref, onMounted, computed } from "vue";
import { ElMessage, ElMessageBox } from "element-plus";
import { Loading, Warning, Plus, Search, Delete } from "@element-plus/icons-vue";

const API_BASE_URL = '';

const loading = ref(false);
const tableLoading = ref(false);
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

  // 根据用户名搜索
  if (searchUsername.value) {
    const searchTerm = searchUsername.value.toLowerCase();
    result = result.filter(user => 
      user.username.toLowerCase().includes(searchTerm)
    );
  }

  return result;
});

const handleFilterChange = () => {
  // 筛选变化时，computed会自动更新
};

const handleSearchChange = () => {
  // 搜索变化时，computed会自动更新
};

// 处理表格选择变化
const handleSelectionChange = (selection: any[]) => {
  selectedUsers.value = selection;
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
    console.log('批量删除 - accessToken检查:', accessToken ? '存在' : '不存在');
    
    if (!accessToken) {
      throw new Error('未登录，请先登录');
    }

    // 逐个删除用户
    let successCount = 0;
    let failCount = 0;

    for (const user of selectedUsers.value) {
      try {
        // 每次请求前重新获取token
        const currentToken = localStorage.getItem('accessToken');
        console.log('批量删除用户 - 发送请求:', {
          url: `${API_BASE_URL}/api/users/${user.id}`,
          method: 'DELETE',
          userId: user.id,
          username: user.username,
          hasToken: !!currentToken,
          tokenPrefix: currentToken ? currentToken.substring(0, 20) + '...' : 'null'
        });

        if (!currentToken) {
          throw new Error('登录已过期，请重新登录');
        }

        const requestHeaders = {
          'Accept': 'application/json',
          'Authorization': `Bearer ${currentToken}`
        };
        console.log('请求头:', requestHeaders);

        const requestUrl = `${API_BASE_URL}/api/users/${user.id}`;
        console.log('实际请求URL:', requestUrl);

        const response = await fetch(requestUrl, {
          method: 'DELETE',
          headers: requestHeaders,
          credentials: 'include'
        });

        console.log(`删除用户 ${user.username} (ID: ${user.id}) 响应状态:`, response.status);

        if (response.ok) {
          successCount++;
          console.log(`删除用户 ${user.username} (ID: ${user.id}) 成功`);
        } else {
          failCount++;
          console.error(`删除用户 ${user.username} (ID: ${user.id}) 失败:`, response.status);
        }
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
    console.log('获取用户列表 - accessToken检查:', accessToken ? '存在' : '不存在');
    if (!accessToken) {
      throw new Error('未登录，请先登录');
    }

    const getUrl = `${API_BASE_URL}/api/users`;
    console.log('获取用户列表 - 请求URL:', getUrl);

    const response = await fetch(getUrl, {
      method: 'GET',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${accessToken}`
      },
      credentials: 'include'
    });

    console.log('获取用户列表 - 响应状态:', response.status);

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
  // 保存原始用户数据
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
    
    // 只有当头像URL有实际值且发生变化时才发送
    if (editForm.value.avatar && editForm.value.avatar !== originalUserData.value.avatar) {
      requestData["avatar"] = editForm.value.avatar;
    }
    
    // 如果用户选择修改密码，才包含password字段（不需要验证当前密码）
    if (editForm.value.changePassword) {
      requestData["password"] = editForm.value.newPassword || "";
    }

    // 如果没有任何修改，提示用户
    if (Object.keys(requestData).length === 0) {
      ElMessage.warning('没有任何修改');
      saving.value = false;
      return;
    }

    // 显示发送到后端的数据格式
    console.log('用户管理 - 向后端发送的数据:', JSON.stringify(requestData, null, 2));
    console.log('用户管理 - 请求数据对象:', requestData);

    // 发送请求
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
      // 重新加载用户列表
      await fetchUsers();
    } else {
      // 处理422错误，显示详细错误信息
      if (response.status === 422 && responseData.detail && Array.isArray(responseData.detail) && responseData.detail.length > 0) {
        // 处理 [{ loc: [...], msg: "...", type: "..." }] 格式的错误
        const errorMessages = responseData.detail
          .map((err: any) => {
            const msg = err.msg || '';
            // 移除 "Value error, " 前缀
            return msg.replace(/^Value error,\s*/, '');
          })
          .filter((msg: string) => msg.length > 0)
          .join('\n');
        throw new Error(errorMessages || '更新用户失败');
      } else if (response.status === 422 && responseData.errors) {
        // 提取错误信息并显示
        const errorMessages = Object.values(responseData.errors).flat().join('\n');
        throw new Error(errorMessages || '更新用户失败');
      } else {
        throw new Error(responseData.msg || '更新用户失败');
      }
    }
  } catch (err: any) {
    const errorMessage = err.message;
    
    // 根据错误类型显示不同的错误信息
    if (errorMessage === '表单验证失败') {
      // 表单验证失败已经在validate回调中处理过，这里不需要重复显示
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
    const currentRole = localStorage.getItem('role');
    console.log('单个删除 - accessToken检查:', accessToken ? '存在' : '不存在');
    console.log('单个删除 - 当前用户角色:', currentRole);
    console.log('单个删除 - 目标用户角色:', user.role);
    if (!accessToken) {
      throw new Error('未登录，请先登录');
    }

    const deleteUrl = `${API_BASE_URL}/api/users/${user.id}`;
    console.log('单个删除 - 请求URL:', deleteUrl);
    console.log('单个删除 - 完整请求头:', {
      'Accept': 'application/json',
      'Authorization': `Bearer ${accessToken ? accessToken.substring(0, 30) + '...' : 'null'}`
    });

    const response = await fetch(deleteUrl, {
      method: 'DELETE',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${accessToken}`
      },
      credentials: 'include'
    });

    console.log('单个删除 - 响应状态:', response.status);

    if (!response.ok) {
      throw new Error('删除用户失败');
    }

    const data = await response.json();
    if (data.code === 200) {
      ElMessage.success('用户删除成功');
      // 重新加载用户列表
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

    // 发送请求
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
      // 重新加载用户列表
      await fetchUsers();
    } else {
      // 处理错误响应
      if (response.status === 400) {
        throw new Error(responseData.detail || '用户名已存在');
      } else if (response.status === 422 && responseData.detail && Array.isArray(responseData.detail) && responseData.detail.length > 0) {
        // 处理 [{ loc: [...], msg: "...", type: "..." }] 格式的错误
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

<style lang="scss" scoped>
.user-manage-card {
  z-index: 1;
  width: 75%;
  margin: 5% auto;
  border-radius: 15px;
  background-color: #fff;
  padding: 30px;
  min-height: 80vh;
}

.header-section {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.filter-section {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  padding: 15px 0;
  border-bottom: 1px solid #ebeef5;
}

.filter-left,
.filter-right {
  display: flex;
  align-items: center;
  gap: 10px;
}

.header-left,
.header-right {
  flex: 1;
}

.header-left {
  text-align: left;
}

.header-right {
  text-align: right;
}

.title {
  margin: 0;
  color: #303133;
  text-align: center;
  flex: 1;
}

.loading-container,
.error-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 40px 0;
  gap: 16px;
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
}

.role-hint {
  font-size: 12px;
  color: #909399;
  margin-left: 8px;
}
</style>
