<template>
  <el-card class="register-card">
    <h3 class="title">鸟厂后台管理 - 注册</h3>

    <el-form :model="form" ref="formRef" :rules="rules">
      <el-form-item label="用户名" prop="username">
        <el-input v-model="form.username" placeholder="请输入用户名"></el-input>
      </el-form-item>

      <el-form-item label="密码" prop="password">
        <el-input v-model="form.password" type="password" placeholder="请输入密码" show-password></el-input>
      </el-form-item>

      <el-form-item label="确认密码" prop="confirmPassword">
        <el-input v-model="form.confirmPassword" type="password" placeholder="请再次输入密码" show-password></el-input>
      </el-form-item>

      <el-form-item label="昵称" prop="nickname">
        <el-input v-model="form.nickname" placeholder="请输入昵称"></el-input>
      </el-form-item>

      <!-- 角色默认设置为user，用户无需修改 -->
      <el-form-item label="角色" prop="role" style="display: none;">
        <el-input v-model="form.role" readonly></el-input>
      </el-form-item>

      <el-form-item label="头像" prop="avatar">
        <el-input v-model="form.avatar" placeholder="请输入头像URL（可选）"></el-input>
      </el-form-item>

      <el-button style="width: 100%; margin-bottom: 10px;" @click="handleRegister" type="primary" :loading="loading">
        注册
      </el-button>
      
      <div class="login-link">
        <span>已有账号？</span>
        <router-link to="/Login">去登录</router-link>
      </div>
    </el-form>
  </el-card>
</template>

<script lang="ts" setup>
import { ref } from "vue";
import { useRouter } from "vue-router";
import { ElMessage } from "element-plus";

const router = useRouter();
const formRef = ref();
const loading = ref(false);

// API基础地址
// 使用相对路径，通过Vite代理解决CORS问题
const API_BASE_URL = '';

const form = ref({
  username: "",
  password: "",
  confirmPassword: "",
  nickname: "",
  role: "user", // 默认角色为user
  avatar: "",
});

// 验证两次密码是否一致
const validateConfirmPassword = (rule: any, value: string, callback: Function) => {
  if (value !== form.value.password) {
    callback(new Error('两次输入的密码不一致'));
  } else {
    callback();
  }
};

const rules = {
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
  avatar: [
    { max: 255, message: '头像URL长度不能超过255个字符', trigger: 'blur' }
  ]
};

// 正常注册方法 - 连接后端API
const handleRegister = async () => {
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
    
    // 给后端发送注册请求
    const response = await fetch(`${API_BASE_URL}/api/users`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      body: JSON.stringify({
        username: form.value.username,
        password: form.value.password,
        nickname: form.value.nickname,
        role: form.value.role,
        avatar: form.value.avatar,
      }),
    });
    
    const data = await response.json();
    
    if (response.ok) {
      // 注册成功
      ElMessage.success("注册成功，请登录");
      
      // 跳转到登录页面
      router.push('/Login');
    } else {
      // 处理错误响应
      if (response.status === 400) {
        // 用户已存在
        ElMessage.error(data.detail || '注册失败，用户名已存在');
      } else if (response.status === 422) {
        // 验证错误，处理后端返回的详细错误信息
        if (data.detail && Array.isArray(data.detail) && data.detail.length > 0) {
          // 处理 [{ loc: [...], msg: "...", type: "..." }] 格式的错误
          // 提取所有错误信息，移除 "Value error, " 前缀
          const errorMessages = data.detail
            .map((err: any) => {
              const msg = err.msg || '';
              // 移除 "Value error, " 前缀
              return msg.replace(/^Value error,\s*/, '');
            })
            .filter((msg: string) => msg.length > 0)
            .join('\n');
          ElMessage.error(errorMessages || '注册信息验证失败');
        } else if (data.errors) {
          const errorMessages = Object.values(data.errors).flat().join('\n');
          ElMessage.error(errorMessages || '注册信息验证失败');
        } else if (data.msg) {
          ElMessage.error(data.msg);
        } else {
          ElMessage.error('注册信息验证失败');
        }
      } else {
        // 其他错误
        ElMessage.error(data.detail || data.msg || '注册失败，请稍后重试');
      }
    }
  } catch (error) {
    console.error('注册错误:', error);
    const errorMessage = (error as Error).message;
    
    // 根据错误类型显示不同的错误信息
    if (errorMessage === '表单验证失败') {
      // 表单验证失败已经在validate回调中处理过，这里不需要重复显示
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
/* Register Card */
.register-card {
  z-index: 1;
  padding-bottom: 30px;
  width: 400px;
  margin-top: 5%;
  border-radius: 15px;
}

/* 增加表单项目之间的间距，确保错误信息完整显示 */
:deep(.el-form-item) {
  margin-bottom: 36px;
}

/* 确保表单验证提示信息完整显示 */
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

.login-link {
  text-align: center;
  margin-top: 20px;
  font-size: 14px;
  color: #606266;
}

.login-link a {
  color: #8bad42;
  text-decoration: none;
  margin-left: 5px;
}

.login-link a:hover {
  text-decoration: underline;
}
</style>
