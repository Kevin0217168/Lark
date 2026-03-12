<template>
  <el-card class="login-card">
    <h3 class="title">用户登录</h3>

    <el-form :model="form" ref="formRef">
      <el-form-item label="用户名">
        <el-input v-model="form.username"></el-input>
      </el-form-item>

      <el-form-item label="密码">
        <el-input v-model="form.password" type="password"></el-input>
      </el-form-item>

      <el-button style="width: 100%" @click="login" color="#8bad42" plain>
        登录
      </el-button>
    </el-form>
  </el-card>
</template>

<script lang="ts" setup>
import { ref } from "vue";
import * as ElementPlus from "element-plus";

const form = ref({
  username: "",
  password: "",
});

function login() {
  ElementPlus.ElMessage.success("登录成功");
  console.log(form.value);
}
</script>

<style lang="scss" scoped>
/* Login Card */
.login-card {
  z-index: 1;
  padding-bottom: 30px;
  width: 400px;
  margin-top: 5%;
  border-radius: 5%;
}
</style>
