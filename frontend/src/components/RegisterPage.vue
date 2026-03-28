<template>
  <div class="register-container" :class="{ 'mobile': isMobile }">
    <!-- 桌面端：使用 el-card 包装 -->
    <el-card v-if="!isMobile" class="register-card">
      <h3 class="title">云雀管理系统 - 注册</h3>

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

    <!-- 移动端：全屏设计 -->
    <div v-else class="mobile-register">
      <div class="register-header">
        <h1>鸟厂后台管理</h1>
        <p>创建账号，请填写注册信息</p>
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

        <!-- 底部备案信息 -->
        <div class="mobile-footer">
          <div class="copyright">© 2026</div>
          <div class="beian">
            <img src="https://www.beian.gov.cn/img/ghs.png" style="width: 15px; height: 15px;display: inline-block;margin-bottom: 4px;"> 
            <a class="beian-link" href="https://beian.mps.gov.cn/#/query/webSearch?code=41010502007493" rel="noreferrer" target="_blank">豫公网安备41010502007493号</a><br> 
            <a class="beian-link" href="https://beian.miit.gov.cn/" target="_blank">豫ICP备2026008104号</a><br>
            <img src="https://icp.gov.moe/images/ico64.png" style="width: 16px; height: 16px;display: inline-block;margin-bottom: 2px;">
            <a class="beian-link" target="_blank" href="https://icp.gov.moe/?keyword=20261324">萌ICP备20261324号</a>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from "vue";
import { useRouter } from "vue-router";
import { ElMessage } from "element-plus";
import { api } from '../utils/api';

const router = useRouter();
const formRef = ref();
const loading = ref(false);

// 检测是否为移动设备
const isMobile = ref(window.innerWidth < 768);

// 监听窗口大小变化
const handleWindowResize = () => {
  isMobile.value = window.innerWidth < 768;
};

onMounted(() => {
  window.addEventListener('resize', handleWindowResize);
});

onUnmounted(() => {
  window.removeEventListener('resize', handleWindowResize);
});

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
    { pattern: /^(?=.*[a-zA-Z])(?=.*\d)[a-zA-Z\d]+$/, message: '密码必须同时包含字母和数字', trigger: 'blur' }
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
    const data = await api.post('/api/users', {
      username: form.value.username,
      password: form.value.password,
      nickname: form.value.nickname,
      role: form.value.role,
      avatar: form.value.avatar,
    });
    
    // 注册成功
    ElMessage.success("注册成功，请登录");
    
    // 跳转到登录页面
    router.push('/Login');
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
.register-container {
  min-height: 100vh;
  display: flex;
  justify-content: center;
  position: relative;
  z-index: 1;
}

/* 桌面端样式 */
.register-card {
  z-index: 1;
  padding-bottom: 30px;
  width: 400px;
  margin-top: 5%;
  margin-bottom: 60px;
  border-radius: 15px;
  height: fit-content;
}

.register-card .title {
  text-align: center;
  margin-bottom: 20px;
}

.register-card :deep(.el-form-item) {
  margin-bottom: 36px;
}

.register-card :deep(.el-form-item__error) {
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

.register-card .login-link {
  text-align: center;
  margin-top: 20px;
  font-size: 14px;
  color: #606266;
}

.register-card .login-link a {
  color: #8bad42;
  text-decoration: none;
  margin-left: 5px;
}

.register-card .login-link a:hover {
  text-decoration: underline;
}

/* 移动端样式 */
.mobile-register {
  width: 100%;
  min-height: 100vh;
  background: linear-gradient(135deg, #8bad42 0%, #6a9c3d 100%);
  display: flex;
  flex-direction: column;
  padding: 0;
  overflow-x: hidden;
  box-sizing: border-box;
}

.mobile-register .register-header {
  display: flex;
  flex-direction: column;
  justify-content: flex-end;
  padding: 30px 24px 20px;
  text-align: center;
  color: white;
  min-height: 120px;
}

.mobile-register .register-header h1 {
  font-size: 28px;
  font-weight: 600;
  margin: 0 0 12px 0;
}

.mobile-register .register-header p {
  font-size: 16px;
  margin: 0;
  opacity: 0.9;
}

.mobile-register .register-form-container {
  flex: 1;
  background: white;
  border-radius: 24px 24px 0 0;
  padding: 32px 24px;
  overflow-y: auto;
}

.mobile-register .register-form {
  margin-bottom: 24px;
}

.mobile-register .register-form :deep(.el-form-item) {
  margin-bottom: 18px;
}

.mobile-register .register-form :deep(.el-input__wrapper) {
  border-radius: 12px;
  padding: 4px 12px;
}

.mobile-register .register-form :deep(.el-form-item__error) {
  padding-top: 4px;
}

.mobile-register .register-btn {
  width: 100%;
  height: 48px;
  font-size: 16px;
  border-radius: 12px;
  margin-top: 8px;
}

.mobile-register .login-link {
  text-align: center;
  font-size: 14px;
  color: #909399;
}

.mobile-register .login-link a {
  color: #8bad42;
  text-decoration: none;
  margin-left: 5px;
  font-weight: 500;
}

.mobile-register .login-link a:active {
  opacity: 0.8;
}

.mobile-register .mobile-footer {
  margin-top: 30px;
  text-align: center;
}

.mobile-register .mobile-footer .copyright {
  font-size: 12px;
  color: #909399;
  margin-bottom: 8px;
}

.mobile-register .mobile-footer .beian {
  font-size: 11px;
  color: #c0c4cc;
  line-height: 1.8;
}

.mobile-register .mobile-footer .beian-link {
  color: #c0c4cc;
  text-decoration: none;
}

.mobile-register .mobile-footer .beian-link:active {
  color: #8bad42;
}
</style>