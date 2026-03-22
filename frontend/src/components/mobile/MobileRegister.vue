<template>
  <div class="mobile-register">
    <div class="register-header">
      <h1>创建账号</h1>
      <p>请填写注册信息</p>
    </div>

    <div class="register-form-container">
      <el-form :model="form" ref="formRef" :rules="rules" class="register-form">
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
          />
        </el-form-item>

        <el-form-item prop="confirmPassword">
          <el-input 
            v-model="form.confirmPassword" 
            type="password" 
            placeholder="请再次输入密码"
            prefix-icon="Lock"
            size="large"
            show-password
          />
        </el-form-item>

        <el-form-item prop="nickname">
          <el-input 
            v-model="form.nickname" 
            placeholder="请输入昵称"
            prefix-icon="UserFilled"
            size="large"
          />
        </el-form-item>

        <el-form-item prop="avatar">
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
          class="register-btn"
          @click="handleRegister" 
          :loading="loading"
        >
          注册
        </el-button>
      </el-form>

      <div class="login-link">
        <span>已有账号？</span>
        <router-link to="/Login">去登录</router-link>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from "vue";
import { useRouter } from "vue-router";
import { ElMessage } from "element-plus";

const router = useRouter();
const formRef = ref();
const loading = ref(false);

const API_BASE_URL = '';

const form = ref({
  username: "",
  password: "",
  confirmPassword: "",
  nickname: "",
  role: "user",
  avatar: "",
});

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
  avatar: [
    { max: 255, message: '头像URL长度不能超过255个字符', trigger: 'blur' }
  ]
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
      ElMessage.success("注册成功，请登录");
      router.push('/Login');
    } else {
      if (response.status === 400) {
        ElMessage.error(data.detail || '注册失败，用户名已存在');
      } else if (response.status === 422) {
        if (data.detail && Array.isArray(data.detail) && data.detail.length > 0) {
          const errorMessages = data.detail
            .map((err: any) => {
              const msg = err.msg || '';
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
        ElMessage.error(data.detail || data.msg || '注册失败，请稍后重试');
      }
    }
  } catch (error) {
    console.error('注册错误:', error);
    const errorMessage = (error as Error).message;
    
    if (errorMessage === '表单验证失败') {
      // 表单验证失败已经在validate回调中处理过
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

<style scoped>
.mobile-register {
  min-height: 100vh;
  background: linear-gradient(135deg, #8bad42 0%, #6a9c3d 100%);
  display: flex;
  flex-direction: column;
  padding: 0;
}

.register-header {
  padding: 40px 24px 30px;
  text-align: center;
  color: white;
}

.register-header h1 {
  font-size: 28px;
  font-weight: 600;
  margin: 0 0 12px 0;
}

.register-header p {
  font-size: 16px;
  margin: 0;
  opacity: 0.9;
}

.register-form-container {
  flex: 1;
  background: white;
  border-radius: 24px 24px 0 0;
  padding: 32px 24px;
  overflow-y: auto;
}

.register-form {
  margin-bottom: 24px;
}

.register-form :deep(.el-form-item) {
  margin-bottom: 18px;
}

.register-form :deep(.el-input__wrapper) {
  border-radius: 12px;
  padding: 4px 12px;
}

.register-form :deep(.el-form-item__error) {
  padding-top: 4px;
}

.register-btn {
  width: 100%;
  height: 48px;
  font-size: 16px;
  border-radius: 12px;
  margin-top: 8px;
}

.login-link {
  text-align: center;
  font-size: 14px;
  color: #909399;
}

.login-link a {
  color: #8bad42;
  text-decoration: none;
  margin-left: 5px;
  font-weight: 500;
}

.login-link a:active {
  opacity: 0.8;
}
</style>