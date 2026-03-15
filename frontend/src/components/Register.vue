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

      <el-button style="width: 100%; margin-bottom: 10px;" @click="handleRegister" type="primary" :loading="loading">
        注册
      </el-button>
      
      <!-- 测试注册按钮 - 仅用于前端测试，没有后端时使用 -->
      <el-button style="width: 100%;" @click="handleTestRegister" type="success" plain>
        测试注册（无后端）
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

const form = ref({
  username: "",
  password: "",
  confirmPassword: "",
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
  ]
};

// 正常注册方法 - 连接后端API
const handleRegister = async () => {
  try {
    await formRef.value.validate();
    
    loading.value = true;
    
    // 给后端发送注册请求
    const response = await fetch('http://localhost:3000/api/register', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        username: form.value.username,
        password: form.value.password,
      }),
    });
    
    if (response.ok) {
      const data = await response.json();
      
      if (data.success) {
        ElMessage.success("注册成功，请登录");
        
        // 跳转到登录页面
        router.push('/Login');
      } else {
        ElMessage.error(data.message || '注册失败，请检查用户名是否已存在');
      }
    } else {
      ElMessage.error('注册失败，请检查网络连接');
    }
  } catch (error) {
    console.error('注册错误:', error);
    ElMessage.error('注册失败，请稍后重试');
  } finally {
    loading.value = false;
  }
};

// ============================================
// 测试注册方法 - 仅用于前端测试，没有后端时使用
// 注意：此方法不连接后端，直接模拟注册成功
// 后端开发完成后，请删除此方法和对应的测试按钮
// ============================================
const handleTestRegister = async () => {
  try {
    await formRef.value.validate();
    
    loading.value = true;
    
    // 模拟注册成功，不连接后端
    ElMessage.success("测试注册成功，请登录");
    
    // 跳转到登录页面
    router.push('/Login');
  } catch (error) {
    console.error('注册错误:', error);
    ElMessage.error('注册失败，请稍后重试');
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
