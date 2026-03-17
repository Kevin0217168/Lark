<template>
  <el-card class="register-card">
    <h3 class="title">用户注册</h3>

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

      <!-- 角色默认设置为root，用户无需修改 -->
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
  role: "root", // 默认角色为root
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
    { min: 3, max: 20, message: '长度在 3 到 20 个字符', trigger: 'blur' }
  ],
  password: [
    { required: true, message: '请输入密码', trigger: 'blur' },
    { min: 6, max: 20, message: '长度在 6 到 20 个字符', trigger: 'blur' }
  ],
  confirmPassword: [
    { required: true, message: '请再次输入密码', trigger: 'blur' },
    { validator: validateConfirmPassword, trigger: 'blur' }
  ],
  nickname: [
    { required: true, message: '请输入昵称', trigger: 'blur' },
    { min: 1, max: 20, message: '长度在 1 到 20 个字符', trigger: 'blur' }
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
    await formRef.value.validate();
    
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
        // 验证错误
        const errorMsg = data.detail?.map((err: any) => err.msg).join('; ') || '注册信息验证失败';
        ElMessage.error(errorMsg);
      } else {
        // 其他错误
        ElMessage.error(data.detail || data.msg || '注册失败，请稍后重试');
      }
    }
  } catch (error) {
    console.error('注册错误:', error);
    ElMessage.error('注册失败，请检查网络连接');
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
  border-radius: 5%;
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
